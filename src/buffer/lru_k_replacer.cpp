#include "buffer/lru_k_replacer.h"
#include <fmt/format.h>
#include "common/exception.h"

namespace redbase {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : k_(k), maximum_frame_(num_frames) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  std::unordered_map<frame_id_t, size_t> distance_map;
  std::vector<frame_id_t> inf_fids;

  std::lock_guard<std::mutex> lk(this->latch_);

  for (const auto &entry : this->node_store_) {
    if (entry.second.IsEvictable()) {
      frame_id_t fid = entry.first;
      size_t distance = entry.second.GetKDistance(current_timestamp_);

      if (distance == LRUKNode::MAX_TIMESTAMP) {
        inf_fids.push_back(fid);
      } else {
        distance_map.insert({fid, distance});
      }
    }
  }

  // check if replacer only has one inf
  if (inf_fids.size() == 1) {
    frame_id_t temp = inf_fids.front();
    *frame_id = temp;
  } else if (inf_fids.empty()) {
    // check if the replacer has the evictable frame
    if (distance_map.empty()) {
      return false;
    }

    // compute the max_distance
    frame_id_t evict_fid = -1;
    size_t max_dis = 0;
    for (const auto &entry : distance_map) {
      if (entry.second > max_dis) {
        max_dis = entry.second;
        evict_fid = entry.first;
      }
    }
    *frame_id = evict_fid;
  } else {
    // exist more inf frame
    int recent_fid = -1;
    size_t far_from_recent_timestamp = LRUKNode::MAX_TIMESTAMP - 1;

    for (auto fid : inf_fids) {
      size_t frame_recent_acc_time = this->node_store_.at(fid).GetRecentAccessTimestamp();
      if (frame_recent_acc_time < far_from_recent_timestamp) {
        far_from_recent_timestamp = frame_recent_acc_time;
        recent_fid = fid;
      }
    }
    *frame_id = recent_fid;
  }

  this->node_store_.erase(*frame_id);
  this->replacer_size_--;
  return true;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id, [[maybe_unused]] AccessType access_type) {
  std::lock_guard<std::mutex> lk(this->latch_);

  if (static_cast<size_t>(frame_id) >= this->maximum_frame_) {
    throw Exception(fmt::format("the frame_id {%d} is greater than lru size {%d}", frame_id, this->maximum_frame_));
  }

  auto node = this->node_store_.find(frame_id);
  if (node == this->node_store_.end()) {  // add a new node with history dsz
    LRUKNode new_node(this->k_, frame_id, false);
    this->node_store_.insert({frame_id, new_node});

    node = this->node_store_.find(frame_id);
  }
  node->second.AddHistory(this->current_timestamp_);
  this->current_timestamp_++;
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  std::lock_guard<std::mutex> lk(this->latch_);

  if (static_cast<size_t>(frame_id) >= this->maximum_frame_) {
    throw Exception(fmt::format("the frame_id {} is greater than lru size {}", frame_id, this->replacer_size_));
  }

  auto node_iter = this->node_store_.find(frame_id);
  if (node_iter == this->node_store_.end()) {
    throw Exception(fmt::format("the frame_id {} is not exist", frame_id));
  }
  LRUKNode &node = node_iter->second;

  if ((!node.IsEvictable() && !set_evictable) ||
      (node.IsEvictable() && set_evictable)) {  // do not need to change status
    return;
  }

  if (node.IsEvictable()) {
    this->replacer_size_ -= 1;
  } else {
    this->replacer_size_ += 1;
  }
  node.SetEvictable(set_evictable);
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  std::lock_guard<std::mutex> lk(this->latch_);

  auto node_iter = this->node_store_.find(frame_id);
  if (node_iter == this->node_store_.end()) {
    return;
  }

  if (!node_iter->second.IsEvictable()) {
    throw Exception(fmt::format("the frame_id {%d} is non-evictable", frame_id));
  }
  this->node_store_.erase(frame_id);
  this->replacer_size_ -= 1;
}

auto LRUKReplacer::Size() -> size_t { return this->replacer_size_; }

}  // namespace bustub
