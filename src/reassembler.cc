#include "reassembler.hh"
#include "debug.hh"

using namespace std;

inline void Reassembler::condition_close()
{
  if ( has_last_ && first_unassembled_ == eof_index_ ) {
    output_.writer().close();
  }
}

// 将一个待重组的子字符串插入到 ByteStream 中
void Reassembler::insert( uint64_t first_index, const string& data, bool is_last_substring )
{
  auto& w = output_.writer();
  uint64_t start_index = first_index;
  uint64_t end_index = first_index + data.size();

  first_unacceptable_ = first_unassembled_ + w.available_capacity();

  if ( is_last_substring ) {
    has_last_ = true;
    eof_index_ = end_index;
  }

  // left bound of substring is greater than first_unacceptable_
  if ( start_index >= first_unacceptable_) {
    condition_close();
    return;
  }

  // right bound of substring is less than first_unassembled_
  if ( end_index <= first_unassembled_ ) {
    condition_close();
    return;
  }

  uint64_t actual_start = std::max( first_index, first_unassembled_ );
  auto it = inner_cache_.lower_bound( start_index );

  // exist element lower than current substring's left bound
  if ( it != inner_cache_.begin() ) {
    auto prev_it = std::prev( it );   // prev method for bidirectional_iterator or higher
    uint64_t prev_end = prev_it->first + prev_it->second.size();    // get the right bound of the prev element
    actual_start = std::max( actual_start, prev_end );    // refresh the leftend of current substring if possible
  }
  // current substring has been tailed to 0
  if ( actual_start >= end_index ) {
    condition_close();
    return;
  }

  std::string_view final_data = std::string_view( data ).substr( actual_start - first_index );
  // cut bytes exceed first_unacceptable_
  if ( actual_start + final_data.size() > first_unacceptable_ ) {
    final_data.remove_suffix( actual_start + final_data.size() - first_unacceptable_ );
  }
  end_index = actual_start + final_data.size();

  // exist element greater than current substring's left bound
  while ( it != inner_cache_.end() && it->first < end_index ) { // following key's left bound is lower then current substring's right bound
    uint64_t next_end = it->first + it->second.size();
    if ( next_end <= end_index ) { // covered
      inner_cache_.erase( it++ );  // delete the following element and get next element, loop
    } else {
      uint64_t overlap = end_index - it->first;
      final_data.remove_suffix( overlap );
      break;
    }
  }

  if ( !final_data.empty() ) {
    inner_cache_[actual_start] = std::string( final_data );
  }

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
