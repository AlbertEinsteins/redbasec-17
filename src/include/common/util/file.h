#pragma once

#include <sys/stat.h>
#include <stdio.h>
#include <string>

namespace redbase
{

/**
 * Return the size of a filename
 * return -1 if failed or not a common file
*/
auto GetFileSize(const std::string& filepath) -> int {
    struct stat st;
    int ret = stat(filepath.c_str(), &st);
    if (ret == -1 || !S_ISREG(st.st_mode)) {
        return -1;
    }

    return st.st_size;
}

} // namespace redbase
