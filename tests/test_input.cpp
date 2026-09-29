// O23: Input's per-device masks. One player reads device 0 (every device at once, the way the game has always
// worked); two players read a device each, and a device must be ONLY ITSELF.
//
// This test exists because of a bug the user found by playing: the whole-input mask was being folded into every
// device, so with two players the arrows drove both of them. Nothing automated caught it, because in an unattended
// run the bot IS the whole-input mask and the two looked the same.
//   test_input.exe
#include <cstdio>

#include "engine/input.h"
#include "ui/controls.h"
#include "ui/screens.h"

using namespace cr;

static int failures = 0;

static void check(bool ok, const char *what)
{
    if (!ok) {
        std::printf("  FAIL %s\n", what);
        failures++;
    }
}

int main()
{
    // The Amiga hands Input one mask per device plus the OR of all of them as the synthetic (whole-input) mask,
    // which is what the menus read. src/amiga/game_amiga.cpp does exactly this every step.
    {
        Input in;
        in.setDevice(1, ActUp);            // player one presses up on the arrows
        in.setDevice(2, 0);                // player two's keys are untouched
        in.setSynthetic(ActUp);            // the whole keyboard, as the menus see it
        in.step();

        check(in.pressed(ActUp), "device 0 sees the press (the menus read this one)");
        check(in.devicePressed(1, ActUp), "the device the key belongs to sees the press");
        check(!in.deviceDown(2, ActUp), "THE OTHER PLAYER'S DEVICE MUST NOT SEE IT");
        check(!in.devicePressed(2, ActUp), "and must not report it as a press");
    }

    // Releases are per device too, and a hop is committed on the release.
    {
        Input in;
        in.setDevice(1, ActUp);
        in.setSynthetic(ActUp);
        in.step();
        in.setDevice(1, 0);
        in.setSynthetic(0);
        in.step();
        check(in.deviceReleased(1, ActUp), "the release reaches the right device");
        check(!in.deviceReleased(2, ActUp), "and not the other one");
    }

    // Device 0 means "every device at once" - what a single player has always played with.
    {
        Input in;
        in.setSynthetic(ActLeft);
        in.step();
        check(in.devicePressed(0, ActLeft), "device 0 is the whole input state");
    }

    // playerDevice: one player reads everything; two players read what the settings gave them.
    {
        UserSettings s;
        s.players = 1;
        check(playerDevice(s, 0) == 0, "one player reads device 0");
        check(playerDevice(s, 1) == 0, "and so does a second player that does not exist");
        s.players = 2;
        s.control[0] = 0; // ARROWS is the first entry of the platform's list
        s.control[1] = 1; // WSAD is the second
        check(playerDevice(s, 0) == 1, "player one reads its own device");
        check(playerDevice(s, 1) == 2, "player two reads a different one");
        check(playerDevice(s, 0) != playerDevice(s, 1), "and the two are never the same device");
    }

    // Windows: the arrows always drive player one, on top of the device player one chose - but never player two's.
    {
        UserSettings s;
        s.players = 2;
        s.control[0] = 2; // player one on the first pad (device 3)
        s.control[1] = 1; // player two on WSAD (device 2)
        Input in;
        in.setDevice(1, ActUp); // an arrow key
        in.setDevice(3, 0);
        arrowsAlsoForPlayerOne(in, s);
        in.step();
        check(in.devicePressed(3, ActUp), "the arrows reach player one on a pad");
        check(!in.deviceDown(2, ActUp), "and not player two");
        s.control[1] = 0; // player two takes the arrows: they are theirs alone
        Input in2;
        in2.setDevice(1, ActUp);
        arrowsAlsoForPlayerOne(in2, s);
        in2.step();
        check(!in2.deviceDown(3, ActUp), "arrows chosen by player two do not reach player one");
        s.players = 1;
        Input in3;
        in3.setDevice(1, ActLeft);
        arrowsAlsoForPlayerOne(in3, s);
        in3.step();
        check(!in3.deviceDown(3, ActLeft), "one player: nothing is copied (device 0 reads everything anyway)");
    }

    if (failures) {
        std::printf("test_input: %d FAILED\n", failures);
        return 1;
    }
    std::printf("test_input: OK\n");
    return 0;
}
