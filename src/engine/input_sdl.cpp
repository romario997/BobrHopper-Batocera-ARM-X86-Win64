// SDL devices for Input (PC keyboard, R36S game controller / raw joystick).
#include "input.h"

#include <SDL.h>

#include "log.h"

namespace cr {

static const int kStickDeadzone = int(0.4 * 32767); // same as OpenSWOS on the R36S

static uint16_t keyAction(SDL_Keycode k)
{
    switch (k) {
    case SDLK_UP: case SDLK_w: return ActUp;
    case SDLK_DOWN: case SDLK_s: return ActDown;
    case SDLK_LEFT: case SDLK_a: return ActLeft;
    case SDLK_RIGHT: case SDLK_d: return ActRight;
    case SDLK_SPACE: case SDLK_RETURN: case SDLK_z: return ActA;
    case SDLK_x: case SDLK_BACKSPACE: return ActB;
#ifdef _WIN32
    // Windows: Esc is also B - a PC player expects it to go back in every menu. The pause menu counts B's release only
    // after a press it saw itself (Screens::bSeen_), so the Esc that opened it does not close it again.
    case SDLK_ESCAPE: return ActStart | ActB;
    case SDLK_p: return ActStart;
#else
    case SDLK_ESCAPE: case SDLK_p: return ActStart;
#endif
    case SDLK_TAB: return ActSelect;
    case SDLK_q: return ActL;
    case SDLK_e: return ActR;
    default: return 0;
    }
}

// O23: the keyboard as TWO devices, so two players can share it - arrows with space and enter for one, WSAD with
// the left shift for the other. keys_ is left exactly as it was, so nothing changes for a single player; these only
// fill the per-device masks the settings screen hands out.
static uint16_t arrowsAction(SDL_Keycode k)
{
    switch (k) {
    case SDLK_UP: return ActUp;
    case SDLK_DOWN: return ActDown;
    case SDLK_LEFT: return ActLeft;
    case SDLK_RIGHT: return ActRight;
    case SDLK_SPACE: case SDLK_RETURN: return ActA;
    case SDLK_BACKSPACE: return ActB;
    default: return 0;
    }
}

static uint16_t wasdAction(SDL_Keycode k)
{
    switch (k) {
    case SDLK_w: return ActUp;
    case SDLK_s: return ActDown;
    case SDLK_a: return ActLeft;
    case SDLK_d: return ActRight;
    case SDLK_LSHIFT: case SDLK_LCTRL: return ActA;
    default: return 0;
    }
}

static uint16_t buttonAction(int b)
{
    switch (b) {
    case SDL_CONTROLLER_BUTTON_DPAD_UP: return ActUp;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return ActDown;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return ActLeft;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return ActRight;
    case SDL_CONTROLLER_BUTTON_A: return ActA;
    case SDL_CONTROLLER_BUTTON_B: return ActB;
    case SDL_CONTROLLER_BUTTON_START: return ActStart;
    case SDL_CONTROLLER_BUTTON_BACK: return ActSelect;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return ActL;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return ActR;
    default: return 0;
    }
}

void Input::init()
{
    SDL_GameControllerEventState(SDL_ENABLE);
    SDL_JoystickEventState(SDL_ENABLE);
    // PortMaster's get_controls on ArkOS provides the pad mappings as a file (SDL_GAMECONTROLLERCONFIG_FILE, e.g.
    // /tmp/gamecontrollerdb.txt, seen in the OpenSWOS log on the R36S); older system SDL2 does not read that
    // variable itself, so load it here
    if (const char *file = SDL_getenv("SDL_GAMECONTROLLERCONFIG_FILE")) {
        if (*file) {
            int added = SDL_GameControllerAddMappingsFromFile(file);
            logf("input: %d controller mappings from %s", added, file);
        }
    }
    int n = SDL_NumJoysticks();
    logf("input: %d joystick(s)", n);
    for (int i = 0; i < n; i++) openController(i);
}

// O23: the slot a pad event belongs to, by SDL's instance id. -1 for a pad beyond the slots, which only device 0 reads.
int Input::padSlot(int32_t which) const
{
    for (int i = 0; i < kMaxPads; i++)
        if (padIds_[i] == which) return i;
    return -1;
}

int Input::padCount() const
{
    int n = 0;
    for (int i = 0; i < kMaxPads; i++) n += padIds_[i] >= 0 ? 1 : 0;
    return n;
}

bool Input::padConnected(int slot) const { return slot >= 0 && slot < kMaxPads && padIds_[slot] >= 0; }

std::string Input::padName(int slot) const
{
    if (!padConnected(slot)) return std::string();
    if (SDL_GameController *gc = SDL_GameControllerFromInstanceID(padIds_[slot])) {
        const char *n = SDL_GameControllerName(gc);
        return n ? n : "";
    }
    if (SDL_Joystick *js = SDL_JoystickFromInstanceID(padIds_[slot])) {
        const char *n = SDL_JoystickName(js);
        return n ? n : "";
    }
    return std::string();
}

// the first free slot, in the order pads were opened - so the first two pads are devices 3 and 4, as they always were
void Input::assignSlot(int32_t instanceId)
{
    for (int i = 0; i < kMaxPads; i++) {
        if (padIds_[i] >= 0) continue;
        padIds_[i] = instanceId;
        padBtn_[i] = padStick_[i] = padHat_[i] = padRaw_[i] = 0;
        axisState_[i][0] = axisState_[i][1] = 0;
        break;
    }
    padGeneration_++;
}

void Input::openController(int index)
{
    // hot-plug (and the ADDED events SDL queues at start-up for pads init() already opened): never twice. Asked of
    // this Input's own handles, not of SDL - a pad somebody else opened (test_two_pads) is still ours to open.
    const SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(index);
    for (void *gc : controllers_)
        if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(static_cast<SDL_GameController *>(gc))) == id) return;
    for (void *js : joysticks_)
        if (SDL_JoystickInstanceID(static_cast<SDL_Joystick *>(js)) == id) return;
    if (SDL_IsGameController(index)) {
        SDL_GameController *gc = SDL_GameControllerOpen(index);
        if (gc) {
            controllers_.push_back(gc);
            assignSlot(SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gc)));
            char *mapping = SDL_GameControllerMapping(gc);
            logf("input: controller %d '%s' mapping: %s", index, SDL_GameControllerName(gc), mapping ? mapping : "-");
            SDL_free(mapping);
            return;
        }
    }
    SDL_Joystick *js = SDL_JoystickOpen(index);
    if (js) {
        joysticks_.push_back(js);
        assignSlot(SDL_JoystickInstanceID(js));
        logf("input: raw joystick %d '%s' buttons=%d axes=%d hats=%d (no controller mapping)", index,
             SDL_JoystickName(js), SDL_JoystickNumButtons(js), SDL_JoystickNumAxes(js), SDL_JoystickNumHats(js));
    }
}

// a pad was unplugged: close it, free its slot, and forget every button it held (they would stay pressed for good)
void Input::closePad(int32_t instanceId)
{
    bool found = false;
    for (size_t i = 0; i < controllers_.size(); i++) {
        SDL_GameController *gc = static_cast<SDL_GameController *>(controllers_[i]);
        if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gc)) != instanceId) continue;
        logf("input: controller '%s' removed", SDL_GameControllerName(gc));
        SDL_GameControllerClose(gc);
        controllers_.erase(controllers_.begin() + long(i));
        found = true;
        break;
    }
    for (size_t i = 0; !found && i < joysticks_.size(); i++) {
        SDL_Joystick *js = static_cast<SDL_Joystick *>(joysticks_[i]);
        if (SDL_JoystickInstanceID(js) != instanceId) continue;
        logf("input: raw joystick '%s' removed", SDL_JoystickName(js));
        SDL_JoystickClose(js);
        joysticks_.erase(joysticks_.begin() + long(i));
        found = true;
    }
    if (!found) return;
    const int slot = padSlot(instanceId);
    if (slot >= 0) {
        padIds_[slot] = -1;
        padBtn_[slot] = padStick_[slot] = padHat_[slot] = padRaw_[slot] = 0;
        axisState_[slot][0] = axisState_[slot][1] = 0;
    }
    pad_ = stick_ = hat_ = rawButtons_ = 0;
    padGeneration_++;
}

void Input::shutdown()
{
    flushRecording();
    for (void *gc : controllers_) SDL_GameControllerClose(static_cast<SDL_GameController *>(gc));
    for (void *js : joysticks_) SDL_JoystickClose(static_cast<SDL_Joystick *>(js));
    controllers_.clear();
    joysticks_.clear();
    for (int i = 0; i < kMaxPads; i++) padIds_[i] = -1;
}

void Input::handleEvents(const std::vector<SDL_Event> &events)
{
    for (const SDL_Event &ev : events) {
        switch (ev.type) {
        case SDL_KEYDOWN:
            if (!ev.key.repeat) {
                keys_ |= keyAction(ev.key.keysym.sym);
                keysArrows_ |= arrowsAction(ev.key.keysym.sym);
                keysWasd_ |= wasdAction(ev.key.keysym.sym);
            }
            break;
        case SDL_KEYUP:
            keys_ &= uint16_t(~keyAction(ev.key.keysym.sym));
            keysArrows_ &= uint16_t(~arrowsAction(ev.key.keysym.sym));
            keysWasd_ &= uint16_t(~wasdAction(ev.key.keysym.sym));
            break;
        case SDL_CONTROLLERBUTTONDOWN: {
            pad_ |= buttonAction(ev.cbutton.button);
            const int slot = padSlot(ev.cbutton.which);
            if (slot >= 0) padBtn_[slot] |= buttonAction(ev.cbutton.button);
            if (logRawButtons)
                logf("input: pad %d button %s down", slot, SDL_GameControllerGetStringForButton(SDL_GameControllerButton(ev.cbutton.button)));
            break;
        }
        case SDL_CONTROLLERBUTTONUP: {
            pad_ &= uint16_t(~buttonAction(ev.cbutton.button));
            const int slot = padSlot(ev.cbutton.which);
            if (slot >= 0) padBtn_[slot] &= uint16_t(~buttonAction(ev.cbutton.button));
            break;
        }
        case SDL_CONTROLLERAXISMOTION: {
            int v = ev.caxis.value;
            const int slot = padSlot(ev.caxis.which);
            if (ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX || ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                uint8_t &state = axisState_[slot >= 0 ? slot : 2][ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX ? 0 : 1];
                if (state != 2) {
                    const bool centred = v >= -kStickDeadzone && v <= kStickDeadzone;
                    if (centred || state == 0)
                        logf("input: pad %d stick %s = %d%s", slot, ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX ? "x" : "y", v,
                             centred ? " - centred, in use" : " - off centre, ignored until it centres");
                    state = centred ? 2 : 1;
                    if (!centred) break;
                }
            }
            if (ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
                stick_ &= uint16_t(~(ActLeft | ActRight));
                if (slot >= 0) padStick_[slot] &= uint16_t(~(ActLeft | ActRight));
                if (v < -kStickDeadzone) {
                    stick_ |= ActLeft;
                    if (slot >= 0) padStick_[slot] |= ActLeft;
                }
                if (v > kStickDeadzone) {
                    stick_ |= ActRight;
                    if (slot >= 0) padStick_[slot] |= ActRight;
                }
            } else if (ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                stick_ &= uint16_t(~(ActUp | ActDown));
                if (slot >= 0) padStick_[slot] &= uint16_t(~(ActUp | ActDown));
                if (v < -kStickDeadzone) {
                    stick_ |= ActUp;
                    if (slot >= 0) padStick_[slot] |= ActUp;
                }
                if (v > kStickDeadzone) {
                    stick_ |= ActDown;
                    if (slot >= 0) padStick_[slot] |= ActDown;
                }
            }
            break;
        }
        case SDL_JOYHATMOTION: {
            if (!controllers_.empty()) break;
            hat_ = 0;
            if (ev.jhat.value & SDL_HAT_UP) hat_ |= ActUp;
            if (ev.jhat.value & SDL_HAT_DOWN) hat_ |= ActDown;
            if (ev.jhat.value & SDL_HAT_LEFT) hat_ |= ActLeft;
            if (ev.jhat.value & SDL_HAT_RIGHT) hat_ |= ActRight;
            const int slot = padSlot(ev.jhat.which);
            if (slot >= 0) padHat_[slot] = hat_;
            break;
        }
        case SDL_JOYBUTTONDOWN: {
            if (logRawButtons && controllers_.empty()) logf("input: raw joystick button %d down", ev.jbutton.button);
            // without a mapping: treat the first buttons like A, B, ... so the game is still usable
            if (controllers_.empty() && ev.jbutton.button < 8) {
                static const uint16_t fallback[8] = {ActA, ActB, ActA, ActB, ActL, ActR, ActSelect, ActStart};
                rawButtons_ |= fallback[ev.jbutton.button];
                const int slot = padSlot(ev.jbutton.which);
                if (slot >= 0) padRaw_[slot] |= fallback[ev.jbutton.button];
            }
            break;
        }
        case SDL_JOYBUTTONUP: {
            if (controllers_.empty() && ev.jbutton.button < 8) {
                static const uint16_t fallback[8] = {ActA, ActB, ActA, ActB, ActL, ActR, ActSelect, ActStart};
                rawButtons_ &= uint16_t(~fallback[ev.jbutton.button]);
                const int slot = padSlot(ev.jbutton.which);
                if (slot >= 0) padRaw_[slot] &= uint16_t(~fallback[ev.jbutton.button]);
            }
            break;
        }
        case SDL_CONTROLLERDEVICEADDED:
        case SDL_JOYDEVICEADDED: // a pad without a mapping only sends this one; openController never opens twice
            openController(ev.type == SDL_JOYDEVICEADDED ? ev.jdevice.which : ev.cdevice.which);
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
        case SDL_JOYDEVICEREMOVED: // both arrive for a controller; the second finds nothing left to close
            closePad(ev.type == SDL_JOYDEVICEREMOVED ? ev.jdevice.which : ev.cdevice.which);
            break;
        default:
            break;
        }
    }
    // O23: the devices the settings screen offers here, in its own order: 1 arrows, 2 WSAD, 3 and 4 the pads.
    // A single player reads device 0, which is all of them together, so this changes nothing for one player.
    setDevice(1, keysArrows_);
    setDevice(2, keysWasd_);
    for (int slot = 0; slot < kMaxPads; slot++)
        setDevice(3 + slot, uint16_t(padBtn_[slot] | padStick_[slot] | padHat_[slot] | padRaw_[slot]));
}

} // namespace cr
