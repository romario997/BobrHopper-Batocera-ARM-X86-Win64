// Windows (mouse and touch screen): the original's GestureView (upstream src/components/GestureView*.tsx) for one
// finger or the left mouse button. Pressing starts the hop's squat (beginMoveWithDirection), letting go performs it:
// a swipe hops that way, anything else is a tap and hops forward (the original's onTap -> SWIPE_UP).
//
// The original's thresholds, in its layout points: a swipe needs a velocity over 0.3 points per millisecond along its
// axis and may stray less than 80 points across it (react-native-swipe-gestures). Here the unit is the overlay's
// logical pixel (480 of them from top to bottom - about what a phone in landscape has in points), the velocity is
// measured over the last 100 ms before the finger lifts, like the pan responder's, and a swipe must also cover 12
// logical pixels, so the few pixels a mouse click wobbles never count as one.
#pragma once

#include <vector>

#include "game/game.h"

namespace cr {

class GestureTracker {
public:
    static constexpr double kVelocity = 300.0;   // logical pixels per second (0.3 per ms)
    static constexpr double kOffAxis = 80.0;     // logical pixels across the swipe
    static constexpr double kMinDistance = 12.0; // logical pixels along it
    static constexpr double kWindow = 0.1;       // seconds of motion the velocity is taken over

    void down(double x, double y, double t);
    void move(double x, double y, double t);
    // the finger lifted: true = a swipe (dir says where), false = a tap
    bool up(double x, double y, double t, Swipe &dir);
    bool active() const { return active_; }
    void cancel() { active_ = false; }

private:
    struct Sample {
        double x, y, t;
    };
    bool active_ = false;
    double x0_ = 0, y0_ = 0;
    std::vector<Sample> samples_;
};

} // namespace cr
