#pragma once

#include <cstdlib>

namespace redbase {

static constexpr int PAGE_SIZE = 1 << 12;       // 4K

using page_id_t = int32_t;

} // namespace redbase
