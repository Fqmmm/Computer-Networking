#include "reassembler.hh"

#include <algorithm>
#include <utility>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  auto& output = output_.writer();
  if ( output.is_closed() ) {
    return;
  }

  if ( is_last_substring ) {
    end_index_ = first_index + data.size();
  }

  // 计算本次可以接收的范围
  const uint64_t next_index = output.bytes_pushed();
  uint64_t begin = max( first_index, next_index );
  const uint64_t end = min( first_index + data.size(), next_index + output.available_capacity() );

  // 按索引顺序检查缓存，只保存尚未收到的部分
  for ( const auto& [index, substring] : pending_ ) {
    if ( begin >= end || index >= end ) {
      break;
    }
    const uint64_t substring_end = index + substring.size();
    if ( substring_end <= begin ) {
      continue;
    }
    if ( begin < index ) {
      // 新片段位于当前片段之前；map 插入不会影响当前的遍历。
      pending_.emplace( begin, data.substr( begin - first_index, index - begin ) );
      pending_bytes_ += index - begin;
    }
    begin = max( begin, substring_end );
  }
  if ( begin < end ) {
    pending_.emplace( begin, data.substr( begin - first_index, end - begin ) );
    pending_bytes_ += end - begin;
  }

  // 按起始索引直接查找并输出连续片段，遇到缺口就停止
  while ( pending_.contains( output.bytes_pushed() ) ) {
    const uint64_t index = output.bytes_pushed();
    string substring = move( pending_.at( index ) );
    pending_bytes_ -= substring.size();
    pending_.erase( index );
    output.push( move( substring ) );
  }

  if ( end_index_.has_value() && output.bytes_pushed() == end_index_.value() ) {
    output.close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return pending_bytes_;
}
