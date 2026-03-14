#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::condition_close( bool lst, uint64_t eof_idx )
{
  if ( lst && first_unassembled_ == eof_idx ) {
    output_.writer().close();
  }
}

// 将一个待重组的子字符串插入到 ByteStream 中
void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // auto w = output_.writer();
  first_unacceptable_ = first_unassembled_ + output_.writer().available_capacity();

  uint64_t start_index = first_index;             // substring left end(closed)
  uint64_t end_index = first_index + data.size(); // substring right end(closed)

  if ( is_last_substring ) {
    has_last_ = true;
    eof_index_ = end_index;
  }

  // situation 1 : right end over first_unacceptable_, aka excced current byte_stream capacity
  if ( end_index > first_unacceptable_ ) {
    data = data.substr( 0, first_unacceptable_ - first_index );
    end_index = first_unacceptable_;
  }

  // situation 2 : right end under first_unassembled_, aka byte covered by substring has been written to the
  // byte_stream
  if ( end_index <= first_unassembled_ ) {
    condition_close( has_last_, eof_index_ );
    return;
  }

  // tail the substring before it's insert into the map

  uint64_t actual_start = std::max( first_index, first_unassembled_ );
  auto it = inner_cache_.lower_bound( start_index );

  // situation 3 & 5
  if ( it != inner_cache_.begin() ) {
    auto prev_it = std::prev( it );
    uint64_t prev_end = prev_it->first + prev_it->second.size();
    actual_start = std::max( actual_start, prev_end );
  }
  if ( actual_start >= end_index ) {
    condition_close( has_last_, eof_index_ );
    return;
  }

  std::string_view final_data = std::string_view( data ).substr( actual_start - first_index );

  // Truncate final_data if it exceeds capacity
  if ( actual_start + final_data.size() > first_unacceptable_ ) {
    final_data.remove_suffix( actual_start + final_data.size() - first_unacceptable_ );
  }

  // Update end_index to actual end
  end_index = actual_start + final_data.size();

  // situation 4 : latter overlap, need to colacing recursively
  while ( it != inner_cache_.end() && it->first < end_index
          && it->first >= actual_start ) { // has following key and overlap
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
    output_.writer().push( std::move( e->second ) );
    e = inner_cache_.erase( e );
  }

  condition_close( has_last_, eof_index_ );
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
