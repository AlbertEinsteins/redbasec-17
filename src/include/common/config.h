#pragma once

#include <cstdlib>

namespace redbase {

static constexpr int PAGE_SIZE = 1 << 12;       // 4K
static constexpr int INVALID_PAGE_ID = -1;
static constexpr int LRUK_REPLACER_K = 10;  // lookback window for lru-k replacer


using page_id_t = int32_t;
using frame_id_t = int32_t;

} // namespace redbase
