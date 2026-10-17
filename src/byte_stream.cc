#include "byte_stream.hh"

#include <algorithm>
#include <utility>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return closed_;
}

void Writer::push( string data )
{
  if ( closed_ ) {
    return;
  }

  const uint64_t len = min( static_cast<uint64_t>( data.size() ), available_capacity() );
  if ( len == 0 ) {
    return;
  }

  data.resize( len );
  buffer_.push_back( move( data ) );
  bytes_pushed_ += len;
}

void Writer::close()
{
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - ( bytes_pushed_ - bytes_popped_ );
}

uint64_t Writer::bytes_pushed() const
{
  return bytes_pushed_;
}

bool Reader::is_finished() const
{
  return closed_ and bytes_buffered() == 0;
}

uint64_t Reader::bytes_popped() const
{
  return bytes_popped_;
}

string_view Reader::peek() const
{
  if ( buffer_.empty() ) {
    return {};
  }

  return string_view { buffer_.front() }.substr( front_offset_ );
}

void Reader::pop( uint64_t len )
{
  len = min( len, bytes_buffered() );
  while ( len > 0 ) {
    const uint64_t popped = min( len, static_cast<uint64_t>( buffer_.front().size() ) - front_offset_ );
    front_offset_ += popped;
    bytes_popped_ += popped;
    len -= popped;

    if ( front_offset_ == buffer_.front().size() ) {
      buffer_.pop_front();
      front_offset_ = 0;
    }
  }
}

uint64_t Reader::bytes_buffered() const
{
  return bytes_pushed_ - bytes_popped_;
}
