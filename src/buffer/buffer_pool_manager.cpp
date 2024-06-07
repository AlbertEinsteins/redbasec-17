#include "buffer/buffer_pool_manager.h"

#include "common/exception.h"
#include "common/macros.h"
#include "pf/page_guard.h"
#include "fmt/format.h"

namespace redbase {

BufferPoolManager::BufferPoolManager(size_t pool_size, PFManager *pf_manager, size_t replacer_k)
    : pool_size_(pool_size), disk_scheduler_(std::make_unique<DiskScheduler>(pf_manager)) {
  // we allocate a consecutive memory space for the buffer pool
  pages_ = new Page[pool_size_];
  replacer_ = std::make_unique<LRUKReplacer>(pool_size, replacer_k);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }

  std::cout << fmt::format("Create BPM (size={}, k={})", pool_size, replacer_k) << std::endl;
}

BufferPoolManager::~BufferPoolManager() { delete[] pages_; }

auto BufferPoolManager::NewPage(page_id_t *page_id) -> Page * {
  std::lock_guard<std::mutex> lk(latch_);

  if (!free_list_.empty()) {
    // Get The Physical Memory
    frame_id_t fid = free_list_.front();
    free_list_.pop_front();

    // reset memory and metadata
    *page_id = AllocatePage();
    this->ResetMetaInfo(&pages_[fid], *page_id);
    pages_[fid].pin_count_ = 1;
    page_table_.insert({*page_id, fid});
    replacer_->RecordAccess(fid);
    return &pages_[fid];
  }

  // check if it has the evictable frame

  frame_id_t fid;
  if (!replacer_->Evict(&fid)) {
    return nullptr;
  }

  if (pages_[fid].is_dirty_) {  // flush dirty page
    // Get the mapping page meta info
    const Page *evict_frame = &pages_[fid];
    WritePageData(evict_frame->data_, evict_frame->page_id_);
  }

  // allocate the new page_id, pin the frame
  *page_id = AllocatePage();
  this->ResetMetaInfo(&pages_[fid], *page_id);
  pages_[fid].pin_count_ = 1;

  page_table_.insert({*page_id, fid});
  replacer_->RecordAccess(fid);
  return &pages_[fid];
}

auto BufferPoolManager::FetchPage(page_id_t page_id) -> Page * {
  std::lock_guard<std::mutex> lk(latch_);
//  std::cout << "Fetch Page " << page_id << std::endl;

  if (page_table_.count(page_id) == 0) {
    return nullptr;
  }

  // check if in buffer now
  frame_id_t fid = -1;
  for (size_t i = 0; i < pool_size_; i++) {
    if (pages_[i].page_id_ == page_id) {
      fid = i;
      break;
    }
  }
  if (fid != -1) {
    pages_[fid].pin_count_++;
    if (pages_[fid].pin_count_ > 0) {
      replacer_->SetEvictable(fid, false);
    }
    return &pages_[fid];
  }

  // if not, find the replacement in the free_list
  if (!free_list_.empty()) {
    fid = free_list_.front();
    free_list_.pop_front();

    // set Meta Info And Read Data
    this->ResetMetaInfo(&pages_[fid], page_id);
    pages_[fid].pin_count_ = 1;
    ReadPageData(pages_[fid].data_, page_id);

    replacer_->RecordAccess(fid);
    return &pages_[fid];
  }

  frame_id_t evict_id;
  if (!replacer_->Evict(&evict_id)) {
    return nullptr;
  }

  if (pages_[evict_id].is_dirty_) {
    // flush
    const Page *page = &pages_[evict_id];
    WritePageData(page->data_, page->page_id_);
  }

  // reset meta info
  // read data
  this->ResetMetaInfo(&pages_[evict_id], page_id);
  pages_[evict_id].pin_count_ = 1;
  page_table_.insert({page_id, evict_id});
  ReadPageData(pages_[evict_id].data_, page_id);
  replacer_->RecordAccess(evict_id);

  return &pages_[evict_id];
}

auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) -> bool {
  std::lock_guard<std::mutex> lk(latch_);

  frame_id_t frame_id = -1;
  for (size_t i = 0; i < pool_size_; i++) {
    if (pages_[i].page_id_ == page_id) {
      frame_id = static_cast<int32_t>(i);
      break;
    }
  }

  if (frame_id == -1 || pages_[frame_id].pin_count_ == 0) {
    return false;
  }

  pages_[frame_id].pin_count_--;
  if (!pages_[frame_id].is_dirty_) {
    // if page is not dirty, then could set the dirty or non-dirty
    pages_[frame_id].is_dirty_ = is_dirty;
  }

  if (pages_[frame_id].pin_count_ == 0) {
    replacer_->SetEvictable(frame_id, true);
  }

  return true;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  std::lock_guard<std::mutex> lk(latch_);

  auto page_iter = page_table_.find(page_id);
  if (page_id == INVALID_PAGE_ID || page_iter == page_table_.end()) {
    return false;
  }

  frame_id_t fid = page_iter->second;
  if (pages_[fid].page_id_ != page_id) {
    return false;
  }

  WritePageData(pages_[fid].data_, page_id);
  pages_[fid].is_dirty_ = false;
  return true;
}

void BufferPoolManager::FlushAllPages() {
  std::lock_guard<std::mutex> lk(latch_);

  for (size_t i = 0; i < pool_size_; i++) {
    WritePageData(pages_[i].data_, pages_[i].page_id_);
  }
}

auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool {
  std::lock_guard<std::mutex> lk(latch_);

  frame_id_t frame_id = -1;
  for (size_t i = 0; i < pool_size_; i++) {
    if (pages_[i].page_id_ == page_id) {
      frame_id = static_cast<int32_t>(i);
      break;
    }
  }
  if (frame_id == -1) {
    return true;
  }

  if (pages_[frame_id].pin_count_ > 0) {
    return false;
  }

  page_table_.erase(page_id);
  replacer_->Remove(frame_id);
  free_list_.push_back(frame_id);

  this->ResetMetaInfo(&pages_[frame_id], INVALID_PAGE_ID);
  DeallocatePage(page_id);
  return true;
}

auto BufferPoolManager::AllocatePage() -> page_id_t { return next_page_id_++; }

auto BufferPoolManager::FetchPageBasic(page_id_t page_id) -> BasicPageGuard { return {this, FetchPage(page_id)}; }

auto BufferPoolManager::FetchPageRead(page_id_t page_id) -> ReadPageGuard {
  Page *page = FetchPage(page_id);
  if (page != nullptr) {
    page->RLatch();
    return {this, page};
  }
  return {this, nullptr};
}

auto BufferPoolManager::FetchPageWrite(page_id_t page_id) -> WritePageGuard {
  Page *page = FetchPage(page_id);
  if (page != nullptr) {
    page->WLatch();
    return {this, page};
  }
  return {this, nullptr};
}

auto BufferPoolManager::NewPageGuarded(page_id_t *page_id) -> BasicPageGuard { return {this, NewPage(page_id)}; }

void BufferPoolManager::ReadPageData(const char *data, page_id_t page_id) {
  auto promise = disk_scheduler_->CreatePromise();
  auto is_done_future = promise.get_future();

  disk_scheduler_->Schedule(
      {.is_write_ = false, .data_ = const_cast<char *>(data), .page_id_ = page_id, .callback_ = std::move(promise)});
  is_done_future.get();  // block until read
  std::cout << "finished read page " << page_id << std::endl;
}

void BufferPoolManager::WritePageData(const char *data, page_id_t page_id) {
  auto promise = disk_scheduler_->CreatePromise();
  auto is_done_future = promise.get_future();

  disk_scheduler_->Schedule(
      {.is_write_ = true, .data_ = const_cast<char *>(data), .page_id_ = page_id, .callback_ = std::move(promise)});

  is_done_future.get();  // block until write

  std::cout << "finished write page " << page_id << std::endl;
}

}  // namespace bustub
