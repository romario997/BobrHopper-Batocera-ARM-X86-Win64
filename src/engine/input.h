// Digital action input from keyboard (PC) and SDL game controllers / raw joysticks (R36S).
// Sampled once per fixed logic step into a bitmask, which is also what --record / --replay store,
// so a recorded session replays bit-exactly with the seeded RNG.
// Devices are platform code: input_sdl.cpp (init, shutdown, handleEvents); input.cpp is shared.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

union SDL_Event;

namespace cr {

enum Action : uint16_t {
    ActUp = 1 << 0,
    ActDown = 1 << 1,
    ActLeft = 1 << 2,
    ActRight = 1 << 3,
    ActA = 1 << 4,      // confirm / hop forward
    ActB = 1 << 5,      // back
    ActStart = 1 << 6,  // pause
    ActSelect = 1 << 7, // with Start: quit
    ActL = 1 << 8,
    ActR = 1 << 9,
};

// O23 (two players): the same buttons, kept per input device as well as all together. Device 0 is EVERYTHING - the
// mask a single player has always played with, every keyboard and pad at once - and 1..4 are the platform's own
// devices in the order the settings screen lists them (arrows, WSAD, joystick port 1, joystick port 2). Nothing else
// about Input changes, so a one-player game reads device 0 and behaves exactly as before.
// Windows (a desktop with any number of pads plugged in and out): four pad slots, so devices 3..6; the consoles still
// list only the first two (their settings hand out devices 1..4), and a slot is kept by its pad until it is unplugged.
static const int kMaxPads = 4;
static const int kInputDevices = 3 + kMaxPads;

class Input {
public:
    void init();
    void shutdown();

    // Feed this frame's SDL events (from Platform::events()).
    void handleEvents(const std::vector<SDL_Event> &events);

    // Latch the live state for one logic step; with a replay loaded, the recorded state is used.
    void step();

    uint16_t held() const { return cur_; }
    bool down(Action a) const { return (cur_ & a) != 0; }
    bool pressed(Action a) const { return (cur_ & a) && !(prev_ & a); }
    bool released(Action a) const { return !(cur_ & a) && (prev_ & a); }

    // state from a bot (--smoke), ORed into the live devices; recorded like real input
    void setSynthetic(uint16_t mask) { synthetic_ = mask; }

    // O23: platform code fills devices 1.. before step(); device 0 is always the whole live state
    void setDevice(int device, uint16_t mask)
    {
        if (device > 0 && device < kInputDevices) devSet_[device] = mask;
    }
    // what the platform code put on a device for the coming step (before step() latches it)
    uint16_t deviceLive(int device) const { return device > 0 && device < kInputDevices ? devSet_[device] : 0; }
    uint16_t deviceHeld(int device) const { return device > 0 && device < kInputDevices ? devCur_[device] : cur_; }
    bool deviceDown(int device, Action a) const { return (deviceHeld(device) & a) != 0; }
    bool devicePressed(int device, Action a) const
    {
        if (device <= 0 || device >= kInputDevices) return pressed(a);
        return (devCur_[device] & a) && !(devPrev_[device] & a);
    }
    bool deviceReleased(int device, Action a) const
    {
        if (device <= 0 || device >= kInputDevices) return released(a);
        return !(devCur_[device] & a) && (devPrev_[device] & a);
    }

    bool startRecording(const std::string &path);
    bool loadReplay(const std::string &path);
    bool replayFinished() const { return replaying_ && replayPos_ >= replay_.size(); }
    void flushRecording();

    // log every raw button once with its name, for the device report
    bool logRawButtons = true;

    // O23: how many pads were opened (0, 1 or 2). The app uses it to pick sensible defaults: a console with
    // two pads should hand one to each player without anybody visiting the settings screen first.
    int padCount() const;
    // Pad slot `slot` (Input device 3 + slot): is a pad there, and SDL's name for it ("" when none). The slots are
    // stable: a pad keeps its slot until it is unplugged, and a new one takes the first free slot (hot-plug).
    bool padConnected(int slot) const;
    std::string padName(int slot) const;
    // changes whenever a pad is plugged in or out, so the app can refresh the names on the settings screen
    unsigned padsChanged() const { return padGeneration_; }

private:
    void openController(int index);
    uint16_t live() const;

    // O23 (SDL): the two keyboard halves as separate devices - arrows and WSAD map to the same actions in keys_,
    // so two players on one keyboard need them counted apart
    uint16_t keysArrows_ = 0, keysWasd_ = 0;
    uint16_t keys_ = 0, pad_ = 0, stick_ = 0, hat_ = 0, rawButtons_ = 0, synthetic_ = 0;
    uint16_t cur_ = 0, prev_ = 0;
    uint16_t devSet_[kInputDevices] = {};
    uint16_t devCur_[kInputDevices] = {};
    uint16_t devPrev_[kInputDevices] = {};
    std::vector<void *> controllers_; // SDL_GameController *
    std::vector<void *> joysticks_;   // SDL_Joystick *
    // O23: which pad an event came from. SDL identifies a pad by its instance id, and the id is NOT the open
    // order, so the two are kept side by side here: padIds_[slot] is the instance id of pad `slot`.
    // -1 = a free slot. A pad beyond kMaxPads is still opened and still reaches device 0, but has no slot.
    int32_t padIds_[kMaxPads] = {-1, -1, -1, -1};
    uint16_t padBtn_[kMaxPads] = {}, padStick_[kMaxPads] = {}, padHat_[kMaxPads] = {}, padRaw_[kMaxPads] = {};
    unsigned padGeneration_ = 0;
    void closePad(int32_t instanceId);
    void assignSlot(int32_t instanceId);
    // Batocera v40 on the RG35XX H: a left stick that never reports its centre held "up" for good, so the d-pad's
    // up never changed the combined state and never hopped. An axis counts only once it has been seen inside the
    // deadzone: [slot 0..kMaxPads-1, other][x, y] = 0 not seen yet, 1 seen off-centre (logged), 2 centred - trusted.
    uint8_t axisState_[kMaxPads + 1][2] = {};
    int padSlot(int32_t which) const;

    std::vector<uint16_t> replay_;
    size_t replayPos_ = 0;
    bool replaying_ = false;
    std::vector<uint16_t> recording_;
    std::string recordPath_;
};

} // namespace cr
