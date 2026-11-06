// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cstdint>

// Ctrl-drag uses at most 10 ms per pixel, without making short clips coarser.
inline int64_t FineSeekPosition(int64_t start, int64_t stop, int64_t anchor, int pixels, int width)
{
    const int64_t step = std::min<int64_t>(100000, (stop - start) / std::max(1, width));
    return std::clamp(anchor + int64_t(pixels) * step, start, stop);
}
