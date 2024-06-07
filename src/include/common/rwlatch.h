#pragma once

#include <mutex>
#include <shared_mutex>

namespace redbase {

class RWLatch
{
private:
    /* data */
    std::shared_mutex mutex_;

public:
    /** Reader Lock */
    void RLock() { mutex_.lock_shared(); }
    /** Writer Lock */
    void WLock() { mutex_.lock(); }
    /** Reader Unlock */
    void RUnlock() { mutex_.unlock_shared(); }
    /** Writer Unlock */
    void WUnlock() { mutex_.unlock(); }
};




} // namespace redbase
