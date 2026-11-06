// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../src/mpc-hc/SeekBarPrecision.h"
#include <cassert>
#include <iostream>

int main()
{
    constexpr int64_t second = 10000000;
    const int64_t duration = 1800 * second;
    const int64_t anchor = 600 * second + second / 2;
    assert(FineSeekPosition(0, duration, anchor, 0, 800) == anchor);
    assert(FineSeekPosition(0, duration, anchor, 1, 800) == anchor + second / 100);
    assert(FineSeekPosition(0, duration, anchor, -1, 800) == anchor - second / 100);
    assert(FineSeekPosition(0, duration, anchor, 100, 800) == anchor + second);
    assert(FineSeekPosition(0, duration, 0, -1, 800) == 0);
    assert(FineSeekPosition(0, duration, duration, 1, 800) == duration);
    assert(FineSeekPosition(0, second, second / 2, 1, 1000) == second / 2 + second / 1000);
    assert(FineSeekPosition(second, 2 * second, second, -1, 0) == second);
    std::cout << "PASS: fractional fine seeking, anchoring, direction, bounds and short clips\n";
}
