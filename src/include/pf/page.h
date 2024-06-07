//
// Created by anthony on 2024/6/5.
//

#pragma once

#include "common/config.h"
#include "common/rwlatch.h"
#include "buffer/buffer_pool_manager.h"
#include <string.h>


namespace redbase {
    
class Page {

friend class BufferPoolManager;

private:
    /** Page data */
    char *data_{nullptr};
    RWLatch *rwlock_;

    /** How many txn use this page */
    int pin_count_{0};

    /** Page id */
    page_id_t page_id_{INVALID_PAGE_ID};
    
    bool is_dirty_{false};

    /*Init Page */
    inline void ResetMemory() {
        memset(data_, INIT_PAGE_VALUE, PAGE_SIZE);
    }

protected:
    static constexpr size_t INIT_PAGE_VALUE = 0;

public:
    Page() {
        data_ = new char[PAGE_SIZE];
        ResetMemory();
    }

    ~Page() {
        delete[] data_;
    }

    /* Get data */
    inline char *GetData() { return data_; }

    /* Get page id */
    inline page_id_t GetPageId() { return page_id_; }

    /* Get pin count */
    inline int GetPinCount() { return pin_count_; }

    /* Is Dirty */
    inline bool IsDirty() { return is_dirty_; }

    inline void RLatch() { rwlock_->RLock(); }

    inline void RUnlatch() { rwlock_->RUnlock(); }

    inline void WLatch() { rwlock_->WLock(); }

    inline void WUnlatch() { rwlock_->WUnlock(); }
};



    
} // namespace redbase


