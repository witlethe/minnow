#include "byte_stream.hh"
// #include "debug.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : buf_()
  , written_bytes_( 0 )
  , read_bytes_( 0 )
  , closed_( false )
  , capacity_( capacity )
  , buffer_bytes_( 0 )
  , front_limit_( 0 )
{}

// Push data to stream, but only as much as available capacity allows.
/* push data to the byte stream, with error handling */
void Writer::push( string data )
{
  if ( closed_ || has_error() ) {
    return;
  }
  uint64_t aval_cap = available_capacity();
  if ( aval_cap > capacity_ ) {
    set_error();
    return;
  }
  if ( data.size() > aval_cap ) {
    data.resize( aval_cap );
  }
  if ( data.empty() ) {
    return;
  }

  uint64_t bytes_to_write = data.size();
  buf_.push_back( std::move( data ) );
  buffer_bytes_ += bytes_to_write;
  written_bytes_ += bytes_to_write;
}

// Signal that the stream has reached its ending. Nothing more will be written.
/* set close flag */
void Writer::close()
{
  closed_ = true;
}

// Has the stream been closed?
/* return the state of the close flag */
bool Writer::is_closed() const
{
  return closed_;
}

// How many bytes can be pushed to the stream right now?
/* available capacity now, no error handling */
uint64_t Writer::available_capacity() const
{
  uint64_t ret_val = capacity_ - buffer_bytes_;
  return ret_val;
}

// Total number of bytes cumulatively pushed to the stream
/* return pushed bytes from the start time */
uint64_t Writer::bytes_pushed() const
{
  return written_bytes_;
}

// Peek at the next bytes in the buffer -- ideally as many as possible.
// It's not required to return a string_view of the *whole* buffer, but
// if the peeked string_view is only one byte at a time, it will probably force
// the caller to do a lot of extra work.
string_view Reader::peek() const
{
  if ( buf_.empty() ) {
    return {};
  }
  std::string_view ret_view = buf_.front();
  return ret_view.substr( front_limit_ );
}

// Remove `len` bytes from the buffer.
void Reader::pop( uint64_t len )
{
  size_t n = std::min( len, buffer_bytes_ );
  buffer_bytes_ -= n;
  read_bytes_ += n;

  // 通过底层容器实现弹出 n 个字节的操作
  while ( n > 0 && !buf_.empty() ) {
    size_t cur_chunk_size = buf_.front().size() - front_limit_;
    if ( n < cur_chunk_size ) {
      front_limit_ += n;
      break;
    }

    n -= cur_chunk_size;
    buf_.pop_front();
    front_limit_ = 0;
  }
}

// Is the stream finished (closed and fully popped)?
bool Reader::is_finished() const
{
  return closed_ && buf_.empty();
}

// Number of bytes currently buffered (pushed and not popped)
uint64_t Reader::bytes_buffered() const
{
  return buffer_bytes_;
}

// Total number of bytes cumulatively popped from stream
uint64_t Reader::bytes_popped() const
{
  return read_bytes_;
}
