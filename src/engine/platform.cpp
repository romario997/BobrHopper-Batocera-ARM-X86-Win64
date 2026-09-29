#include "platform.h"

#include "gl_api.h"
#include "log.h"

namespace cr {

// O19: ask for 24 bits of depth before settling for 16. The shadows are flattened geometry laid just above the
// floor, so they live or die by depth precision: over the camera's 60-unit range a 16-bit buffer resolves about
// 0.0009 of a unit, and on the R36S (Mali-G31) whole triangles of a shadow dropped out as the camera moved,
// while a still camera looked fine. The renderer also applies a polygon offset now, but the deeper buffer is
// what removes the cause rather than papering over it.
static SDL_GLContext createContext(SDL_Window *win, bool es, int forceDepth)
{
    const int wanted[2] = {forceDepth ? forceDepth : 24, forceDepth ? forceDepth : 16};
    for (int attempt = 0; attempt < 2; attempt++) {
        const int depth = wanted[attempt];
        SDL_GL_ResetAttributes();
        if (es) {
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        } else {
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        }
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depth);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
        SDL_GLContext c = SDL_GL_CreateContext(win);
        if (!c) continue;
        // A driver may hand back the nearest config rather than refusing, and "24 bits of depth, no stencil" is a
        // real answer. The shadow pass needs the stencil to stop overlapping shadows darkening a floor twice, so
        // a config without one is worse than a shallower depth buffer: drop it and try the next depth.
        int gotStencil = 0;
        SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &gotStencil);
        if (gotStencil >= 8 || attempt == 1) return c; // the second attempt is the last: take what there is
        SDL_GL_DeleteContext(c);
    }
    return nullptr;
}

bool Platform::init(const PlatformConfig &cfg)
{
    headless_ = cfg.headless;
    hidden_ = cfg.hidden;
    t0_ = SDL_GetPerformanceCounter();
    if (headless_) {
        if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0) {
            logf("FATAL SDL_Init(headless): %s", SDL_GetError());
            return false;
        }
        w_ = cfg.width;
        h_ = cfg.height;
        logf("platform: headless %dx%d", w_, h_);
        return true;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0) {
        logf("FATAL SDL_Init: %s", SDL_GetError());
        return false;
    }
    logf("platform: video driver %s", SDL_GetCurrentVideoDriver());
    SDL_DisplayMode mode;
    if (SDL_GetCurrentDisplayMode(0, &mode) == 0) logf("platform: display %dx%d @%dHz", mode.w, mode.h, mode.refresh_rate);

    Uint32 flags = SDL_WINDOW_OPENGL;
    if (cfg.fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    if (cfg.hidden) flags |= SDL_WINDOW_HIDDEN;
    if (cfg.resizable && !cfg.hidden) flags |= SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    if (cfg.maximized && !cfg.hidden) flags |= SDL_WINDOW_MAXIMIZED;
    int ww = cfg.hidden ? 64 : cfg.width, wh = cfg.hidden ? 64 : cfg.height;
    fullscreenToggle_ = cfg.fullscreenToggle && !cfg.hidden;
    winW_ = cfg.width;
    winH_ = cfg.height;

    for (int attempt = 0; attempt < 2 && !ctx_; attempt++) {
        bool es = attempt == 0;
        win_ = SDL_CreateWindow(cfg.title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ww, wh, flags);
        if (!win_) {
            logf("platform: SDL_CreateWindow failed: %s", SDL_GetError());
            continue;
        }
        ctx_ = createContext(win_, es, cfg.depthBits);
        if (!ctx_) {
            logf("platform: %s context failed: %s", es ? "GLES2" : "GL2.1", SDL_GetError());
            SDL_DestroyWindow(win_);
            win_ = nullptr;
        } else {
            gl::setDesktop(!es);
        }
    }
    if (!ctx_) {
        logf("FATAL no GL context");
        return false;
    }
    const char *missing = nullptr;
    if (!gl::load(&missing)) {
        logf("FATAL GL function missing: %s", missing);
        return false;
    }
    SDL_GL_SetSwapInterval(cfg.vsync ? 1 : 0);
    if (cfg.resizable && !cfg.hidden && cfg.minWidth > 0 && cfg.minHeight > 0)
        SDL_SetWindowMinimumSize(win_, cfg.minWidth, cfg.minHeight);
    // Batocera PC runs the game fullscreen on X11, where the desktop's pointer would sit in the middle of the picture
    // (the Windows window keeps it: the mouse and the touch screen work the menus there)
    if (cfg.fullscreen && !cfg.fullscreenToggle) SDL_ShowCursor(SDL_DISABLE);
    SDL_GL_GetDrawableSize(win_, &w_, &h_);
    if (cfg.hidden) {
        w_ = cfg.width;
        h_ = cfg.height;
    }
    // what the driver actually handed over, not what was asked for: the shadows depend on it, and without this
    // line a report of flickering shadows leaves nothing to reason from (O19)
    int gotDepth = 0, gotStencil = 0;
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &gotDepth);
    SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &gotStencil);
    depthBits_ = gotDepth;
    stencilBits_ = gotStencil;
    logf("platform: GL %s | %s | %s | desktop=%d size=%dx%d depth=%d stencil=%d", glGetString(GL_VENDOR),
         glGetString(GL_RENDERER), glGetString(GL_VERSION), gl::isDesktop() ? 1 : 0, w_, h_, gotDepth, gotStencil);
    return true;
}

void Platform::shutdown()
{
    if (ctx_) SDL_GL_DeleteContext(ctx_);
    if (win_) SDL_DestroyWindow(win_);
    ctx_ = nullptr;
    win_ = nullptr;
    SDL_Quit();
}

void Platform::updateDrawableSize()
{
    if (!win_ || hidden_) return;
    int w = 0, h = 0;
    SDL_GL_GetDrawableSize(win_, &w, &h);
    if (w <= 0 || h <= 0) return; // minimized
    if (w != w_ || h != h_) {
        logf("platform: drawable %dx%d -> %dx%d%s", w_, h_, w, h, fullscreen() ? " (fullscreen)" : "");
        w_ = w;
        h_ = h;
        resized_ = true;
    }
    if (!fullscreen() && !maximized()) SDL_GetWindowSize(win_, &winW_, &winH_);
}

void Platform::requestSize(int w, int h)
{
    if (w <= 0 || h <= 0) return;
    if (hidden_ || !win_) {
        if (w != w_ || h != h_) {
            logf("platform: hidden size %dx%d -> %dx%d", w_, h_, w, h);
            w_ = w;
            h_ = h;
            resized_ = true;
        }
        return;
    }
    if (fullscreen()) SDL_SetWindowFullscreen(win_, 0);
    SDL_RestoreWindow(win_);
    SDL_SetWindowSize(win_, w, h);
    updateDrawableSize();
}

bool Platform::fullscreen() const
{
    return win_ && (SDL_GetWindowFlags(win_) & SDL_WINDOW_FULLSCREEN) != 0;
}

bool Platform::maximized() const
{
    return win_ && (SDL_GetWindowFlags(win_) & SDL_WINDOW_MAXIMIZED) != 0;
}

void Platform::toggleFullscreen()
{
    if (!win_ || hidden_) return;
    const bool fs = !fullscreen();
    if (SDL_SetWindowFullscreen(win_, fs ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) != 0) {
        logf("platform: fullscreen %d failed: %s", fs ? 1 : 0, SDL_GetError());
        return;
    }
    logf("platform: %s", fs ? "fullscreen" : "windowed");
    updateDrawableSize();
}

bool Platform::pump()
{
    events_.clear();
    bool keep = true;
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) keep = false;
        if (ev.type == SDL_WINDOWEVENT) {
            switch (ev.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_RESIZED:
            case SDL_WINDOWEVENT_MAXIMIZED:
            case SDL_WINDOWEVENT_RESTORED:
            case SDL_WINDOWEVENT_SHOWN: updateDrawableSize(); break;
            default: break;
            }
        }
        // Alt+Enter / F11 (desktop): the window's own business - the game never sees these keys, so Enter here does
        // not also confirm a menu
        if (fullscreenToggle_ && ev.type == SDL_KEYDOWN &&
            (((ev.key.keysym.sym == SDLK_RETURN || ev.key.keysym.sym == SDLK_KP_ENTER) && (ev.key.keysym.mod & KMOD_ALT)) ||
             ev.key.keysym.sym == SDLK_F11)) {
            if (!ev.key.repeat) toggleFullscreen();
            continue;
        }
        events_.push_back(ev);
    }
    return keep;
}

void Platform::swap()
{
    if (win_) SDL_GL_SwapWindow(win_);
}

double Platform::now() const
{
    return double(SDL_GetPerformanceCounter() - t0_) / double(SDL_GetPerformanceFrequency());
}

} // namespace cr
