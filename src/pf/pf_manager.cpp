
#include "pf/pf_manager.h"
#include "common/exception.h"
#include "common/logger.h"
#include "common/util/file.h"
#include "fmt/core.h"

namespace redbase {

PF_Manager::PF_Manager(const std::string& db_file) : db_filename_(db_file) {
    std::scoped_lock scoped_io_lock(db_io_latch_);

    db_io_.open(db_filename_, std::ios::binary |std::ios::in |std::ios::out );
    if (!db_io_.is_open()) {
        // need create
        db_io_.clear();
        db_io_.open(db_filename_, std::ios::binary |std::ios::trunc |std::ios::in |std::ios::out);
        if (!db_io_.is_open()) {
            throw Exception(fmt::format("db file {} can not open", db_filename_));
        }
    }
}

void PF_Manager::Shutdown() {
    {
        std::scoped_lock scoped_io_lock(db_io_latch_);
        db_io_.close();
    }
}

void PF_Manager::ReadPage(page_id_t page_id, char *data) {
    std::scoped_lock scoped_io_lock(db_io_latch_);
    size_t offset = page_id * PAGE_SIZE;

    std::cout << GetSelfFileSize() << std::endl;
    if (offset >= GetSelfFileSize()) {
      LOG_DEBUG("I/O err reading pass the end of file");
      return ;
    }

    db_io_.seekp(offset);
    db_io_.read(data, PAGE_SIZE);

    if (db_io_.bad()) {
        LOG_DEBUG("I/O error while reading data");
        return ;
    }

    size_t read_size = db_io_.gcount();
    if (read_size < PAGE_SIZE) {
        LOG_DEBUG("The size of read data less than a page size");
        db_io_.clear();
        memset(data + read_size, 0, PAGE_SIZE - read_size);
    }
}

void PF_Manager::WritePage(page_id_t page_id, const char *data) {
    std::scoped_lock scoped_io_lock(db_io_latch_);

    std::cout << db_io_.tellp() << " " << db_io_.tellg() << std::endl;
    size_t offset = page_id * PAGE_SIZE;
    db_io_.seekp(offset);
    db_io_.write(data, PAGE_SIZE);

    std::cout << db_io_.tellp() << " " << db_io_.tellg() << std::endl;

    if (db_io_.bad()) {
        LOG_DEBUG("I/O error while writing data");
        return ;
    }
    
    // flush
    db_io_.flush();
}


auto PF_Manager::GetSelfFileSize() -> size_t {
    return redbase::GetFileSize(db_filename_);
}

}