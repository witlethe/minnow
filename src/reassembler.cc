#include "reassembler.hh"
#include "debug.hh"
#include <iterator>

using namespace std;

// 将一个待重组的子字符串插入到 ByteStream 中
void Reassembler::insert( uint64_t first_index, const string& data, bool is_last_substring )
{

  auto& w = output_.writer();
  first_unacceptable_ = first_unassembled_ + w.available_capacity();  // get current status of writer

  uint64_t end_index = first_index + data.size();
  uint64_t actual_start = std::max(first_index, first_unassembled_);
  uint64_t actual_end = std::min(first_index + data.size(), first_unacceptable_);

  if ( is_last_substring ) {    // set flags for conditional closing
    has_last_ = true;
    eof_index_ = end_index;
  }

  if (actual_start >= actual_end) {
    condition_close();
    return;
  }

  auto it = inner_cache_.lower_bound( actual_start );    // get insert location

  // exist element lower than current substring's left bound
  if ( it != inner_cache_.begin() ) {
    auto prev_it = std::prev( it );
    uint64_t prev_end = prev_it->first + prev_it->second.size(); // get the right bound of the prev element
    actual_start = std::max( actual_start, prev_end );  // * strategy: use former substring's end
  }

  if ( actual_start >= actual_end ) {   // aka, covered by former string
    condition_close();
    return;
  }

  std::string_view final_data( data );
  final_data.remove_prefix( actual_start - first_index );
  final_data.remove_suffix(end_index - actual_end);

  while ( it != inner_cache_.end() && it->first < actual_end ) {  // coleascing all covered or ovelaped substring stored in map
    uint64_t next_end = it->first + it->second.size();
    if ( next_end <= actual_end ) { // covered
      inner_cache_.erase( it++ );
    } else {  // overlaped
      uint64_t overlap = actual_end - it->first;
      final_data.remove_suffix( overlap );
      break;
    }
  }

  if (final_data.empty()) {   // nothing changed
    return;
  }
  inner_cache_[actual_start] = std::string( final_data );

  // try push substrings into writer
  auto e = inner_cache_.begin();
  while ( e != inner_cache_.end() && e->first == first_unassembled_ ) {
    first_unassembled_ += e->second.size();
    w.push( std::move( e->second ) );
    e = inner_cache_.erase( e );
  }

  condition_close();
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t pending_bytes = 0;
  for ( const auto& it : inner_cache_ ) {
    pending_bytes += it.second.size();
  }
  return pending_bytes;
}
