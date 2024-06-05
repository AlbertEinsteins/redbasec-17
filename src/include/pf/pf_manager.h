#pragma once

#include "common/config.h"
#include <string>
#include <mutex>
#include <fstream>

namespace redbase {

class PF_Manager {
public:
    /* Create a DB file */
    explicit PF_Manager(const std::string& db_file);

    PF_Manager() = default;

    /* For sub class to implement */
    virtual ~PF_Manager() = default;

    /* close file resources */
    void Shutdown();

    /* Read a page data by a page_number from db_file */
    virtual void ReadPage(page_id_t page_id, char *data);

    /* Write a page data use a page_number */
    virtual void WritePage(page_id_t page_id, const char *data);
    

protected:
    auto GetSelfFileSize() -> size_t;

private:
    std::fstream db_io_;
    std::string db_filename_;
    std::mutex db_io_latch_;
};


}