#include "ui/gesture.h"

#include <algorithm>
#include <cmath>

namespace cr {

void GestureTracker::down(double x, double y, double t)
{
    active_ = true;
    x0_ = x;
    y0_ = y;
    samples_.clear();
    samples_.push_back({x, y, t});
}

void GestureTracker::move(double x, double y, double t)
{
    if (!active_) return;
    samples_.push_back({x, y, t});
    // keep what the velocity needs, and a little more
    while (samples_.size() > 2 && t - samples_[1].t > kWindow) samples_.erase(samples_.begin());
}

bool GestureTracker::up(double x, double y, double t, Swipe &dir)
{
    if (!active_) return false;
    active_ = false;
    samples_.push_back({x, y, t});
    // the velocity over the last kWindow seconds (from the newest sample at least that old, else the first)
    Sample from = samples_.front();
    for (const Sample &s : samples_)
        if (t - s.t >= kWindow) from = s;
    const double dt = std::max(t - from.t, 1.0 / 120.0);
    const double vx = (x - from.x) / dt, vy = (y - from.y) / dt;
    const double dx = x - x0_, dy = y - y0_;
    // _getSwipeDirection: horizontal first, then vertical
    if (std::fabs(vx) > kVelocity && std::fabs(dy) < kOffAxis && std::fabs(dx) >= kMinDistance) {
        dir = dx > 0 ? Swipe::Right : Swipe::Left;
        return true;
    }
    if (std::fabs(vy) > kVelocity && std::fabs(dx) < kOffAxis && std::fabs(dy) >= kMinDistance) {
        dir = dy > 0 ? Swipe::Down : Swipe::Up;
        return true;
    }
    return false;
}

} // namespace cr
