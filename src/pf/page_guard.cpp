#include "pf/page_guard.h"
#include "buffer/buffer_pool_manager.h"

#include <iostream>

namespace redbase {

BasicPageGuard::BasicPageGuard(BasicPageGuard &&that) noexcept {
  // drop if has
  *this = std::move(that);
}

void BasicPageGuard::Drop() {
  if (this->page_ != nullptr) {
    bpm_->UnpinPage(page_->GetPageId(), is_dirty_);
//    std::cout << fmt::format("Unpin Page {}, After that Pin Count {}", page_->GetPageId(), page_->GetPinCount())
//              << std::endl;
    ResetData(this);
  }
}

auto BasicPageGuard::operator=(BasicPageGuard &&that) noexcept -> BasicPageGuard & {
  if (this->page_ != nullptr) {
    this->Drop();
  }

  bpm_ = that.bpm_;
  page_ = that.page_;
  is_dirty_ = that.is_dirty_;

  ResetData(&that);
  return *this;
}

BasicPageGuard::~BasicPageGuard() { Drop(); }

auto BasicPageGuard::UpgradeRead() -> ReadPageGuard {
  std::cout << "Basic Guard Call Upgrade read in page " << page_->GetPageId() << std::endl;

  ReadPageGuard r = ReadPageGuard(bpm_, page_);
  if (page_ != nullptr) {
    page_->RLatch();
    ResetData(this);
  }
  return r;
}

auto BasicPageGuard::UpgradeWrite() -> WritePageGuard {
  std::cout << "Basic Guard Call Upgrade Write in page " << page_->GetPageId() << std::endl;
  WritePageGuard w = WritePageGuard(bpm_, page_);
  if (page_ != nullptr) {
    page_->WLatch();
    ResetData(this);
  }
  return w;
};  // NOLINT

ReadPageGuard::ReadPageGuard(ReadPageGuard &&that) noexcept { *this = std::move(that); }

auto ReadPageGuard::operator=(ReadPageGuard &&that) noexcept -> ReadPageGuard & {
  // release shared lock
  if (that.guard_.page_ != nullptr) {
    that.guard_.page_->RUnlatch();
  }
  if (guard_.page_ != nullptr && guard_.page_ != that.guard_.page_) {  // in case, that == this, release once
    guard_.page_->RUnlatch();
  }

  // move date, then lock self
  guard_ = std::move(that.guard_);
  if (guard_.page_ != nullptr) {
    guard_.page_->RLatch();
  }

  return *this;
}

void ReadPageGuard::Drop() {
  if (guard_.page_ != nullptr) {
    guard_.page_->RUnlatch();
    guard_.Drop();
  }
}

ReadPageGuard::~ReadPageGuard() {
  if (guard_.page_ != nullptr) {
    guard_.page_->WUnlatch();
    guard_.Drop();
  }
}  // NOLINT

WritePageGuard::WritePageGuard(WritePageGuard &&that) noexcept { *this = std::move(that); }

auto WritePageGuard::operator=(WritePageGuard &&that) noexcept -> WritePageGuard & {
  // release lock
  if (that.guard_.page_ != nullptr) {
    that.guard_.page_->WUnlatch();
  }
  if (guard_.page_ != nullptr) {
    guard_.page_->WUnlatch();
  }

  guard_ = std::move(that.guard_);
  if (guard_.page_ != nullptr) {
    guard_.page_->WLatch();
  }

  return *this;
}

void WritePageGuard::Drop() {
  if (guard_.page_ != nullptr) {
    guard_.page_->WUnlatch();
    guard_.Drop();
  }
}

WritePageGuard::~WritePageGuard() { Drop(); }  // NOLINT

}  // namespace bustub
