// SDL window + GL context. On PC automated runs use a hidden window and render into an FBO,
// so tests never pop up a window or take keyboard focus from the user's desktop.
#pragma once

#include <SDL.h>

#include <vector>

namespace cr {

struct PlatformConfig {
    // O19 (diagnostics): force the depth buffer the context asks for, so the device's 16-bit buffer - where the
    // shadows flickered - can be reproduced on a PC that would otherwise hand out 24 bits. 0 = ask for the best.
    int depthBits = 0;
    int width = 640;
    int height = 480;
    bool fullscreen = false; // device: fullscreen desktop mode at native resolution
    bool hidden = false;     // PC automation: invisible window, draw into a RenderTarget
    bool headless = false;   // no video at all (logic tests, qemu)
    bool vsync = true;
    const char *title = "BobrHopper";
    // Windows desktop: a window the user can resize (min size below) and maximize, and Alt+Enter / F11 switching it
    // between that window and the whole screen (SDL_WINDOW_FULLSCREEN_DESKTOP). The consoles leave all of it off.
    bool resizable = false;
    int minWidth = 0, minHeight = 0;
    bool maximized = false;        // open maximized (the last session ended that way)
    bool fullscreenToggle = false; // Alt+Enter / F11
};

class Platform {
public:
    bool init(const PlatformConfig &cfg);
    void shutdown();

    // Moves pending SDL events into events(); returns false when the app should quit.
    bool pump();
    const std::vector<SDL_Event> &events() const { return events_; }

    void swap();
    double now() const;

    int width() const { return w_; }
    int height() const { return h_; }
    // O19: what the driver actually granted. The shadow pass masks itself with the stencil, and a context without
    // one throws the shadows away instead of merely double-darkening them, so the renderer has to know.
    int stencilBits() const { return stencilBits_; }
    int depthBits() const { return depthBits_; }
    // The drawable changed size since the last call (the window was resized, maximized, went fullscreen, or a hidden
    // run was asked for another size): the app recomputes everything it derived from width() and height().
    bool takeResized()
    {
        const bool r = resized_;
        resized_ = false;
        return r;
    }
    // a real window: SDL_SetWindowSize (the resize then arrives as the usual window event); a hidden run: the size of
    // the picture it renders (tests of the resize path)
    void requestSize(int w, int h);
    void toggleFullscreen();
    bool fullscreen() const;
    bool maximized() const;
    // the window's size when it last was an ordinary window (neither fullscreen nor maximized), for the config
    int windowedWidth() const { return winW_; }
    int windowedHeight() const { return winH_; }
    bool headless() const { return headless_; }
    bool hidden() const { return hidden_; }
    SDL_Window *window() const { return win_; }

private:
    SDL_Window *win_ = nullptr;
    SDL_GLContext ctx_ = nullptr;
    int w_ = 0, h_ = 0;
    int depthBits_ = 0, stencilBits_ = 0; // what the driver granted, not what was asked for (O19)
    bool headless_ = false, hidden_ = false;
    bool resized_ = false, fullscreenToggle_ = false;
    int winW_ = 0, winH_ = 0;
    void updateDrawableSize();
    std::vector<SDL_Event> events_;
    Uint64 t0_ = 0;
};

} // namespace cr
