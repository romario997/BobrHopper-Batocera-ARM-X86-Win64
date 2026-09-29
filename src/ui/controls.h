// O23 (two players): the one place that turns a step's input into hops.
//
// All three ports used to write this loop themselves - five directions, press begins the hop and release performs it.
// With two players there is more to get right (which device belongs to which player, and that a single player must
// keep reading EVERY device), so it lives here and the Amiga, the R36S and the SF2000 all call the same function.
#pragma once

#include "engine/input.h"

namespace cr {

class Game;
struct UserSettings;

// Which Input device a player reads. With one player it is 0 - all devices at once, the way it has always been - so
// nothing about a single-player game changes. With two it is the device that player chose in the settings.
int playerDevice(const UserSettings &settings, int player);

// Windows (the author's rule for the PC): the arrow keys - with Enter and Space - always drive player one, whatever
// device player one picked, unless player two plays on the arrows. Call after Input::handleEvents, before step().
// One player reads every device anyway (device 0), and so do the menus, so this only matters with two.
void arrowsAlsoForPlayerOne(Input &input, const UserSettings &settings);

// Windows: a player whose device can no longer be chosen (their pad was unplugged: available[control] is false) goes
// back to the keyboard - player one to the arrows, player two to WSAD, or the other half when the other player
// already has that one. Nobody is ever left on a pad that is not there. true = a setting changed (save it).
// The lists index: 0 ARROWS, 1 WSAD, 2.. the pads (Screens::controlNames).
bool fallBackFromMissingPads(UserSettings &settings, const bool *available, int count);

// Feeds this step's presses and releases to the game, for as many players as it has. Call it only while the game is
// being played and no menu has taken the input.
void applyPlayerInput(const Input &input, const UserSettings &settings, Game &game);

} // namespace cr
