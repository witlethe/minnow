#include "reassembler.hh"
#include "debug.hh"
#include <cstdint>
#include <iterator>
#include <utility>

using namespace std;

// 将一个待重组的子字符串插入到 ByteStream 中
void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  auto w = output_.writer();
  // uint64_t cur_capacity = w.available_capacity();
  // first_unacceptable_ = first_unassembled_ + cur_capacity;

  // // 查过上届或者超过下界
  // if (
  //   ((first_index + data.size()) < first_unassembled_)
  //   ||
  //   ((first_index + data.size()) > first_unacceptable_)
  // ) {
  //   return;
  // }


  uint64_t start_index = first_index;
  uint64_t end_index = first_index + data.size();

  auto it = inner_cache_.lower_bound(start_index);

  if (it != inner_cache_.begin()) {
    auto prev_it = prev(it);
    uint64_t prev_end = prev_it->first + prev_it->second.size();
    if (prev_end > start_index) {
      if (prev_end >= end_index) {
        return;
      }
      uint64_t offset = prev_end - start_index;
      data = data.substr(offset);
      start_index = prev_end;
    }
  }

  while (it != inner_cache_.end() && it->first < end_index) {
    uint64_t next_end = it->first + it->second.size();
    if (next_end <= end_index) {
      inner_cache_.erase(it++);
    } else {
      uint64_t overlap = end_index - it->first;
      data = data.substr(0, data.size() - overlap);
      end_index = it->first;
      break;
    }
  }
  
  if (!data.empty()) {
    inner_cache_[start_index] = std::move(data);
  }


  if (is_last_substring) {
    w.close();
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t pending_bytes = 0;
  for (const auto& it : inner_cache_) {
    pending_bytes += it.second.size();
  }
  return pending_bytes;
}
