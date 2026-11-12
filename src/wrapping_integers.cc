#include "wrapping_integers.hh"

#include <limits>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + static_cast<uint32_t>( n );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  constexpr uint64_t modulus = uint64_t { 1 } << 32;
  const uint32_t offset = raw_value_ - zero_point.raw_value_;

  // 先构造与 checkpoint 位于同一圈（每圈 2^32）的绝对序列号
  const uint64_t candidate = ( checkpoint / modulus ) * modulus + offset;

  // 相邻圈的候选值可能更近；加减一圈前需检查 uint64_t 的上下界
  if ( candidate > checkpoint && candidate - checkpoint > modulus / 2 && candidate >= modulus ) {
    return candidate - modulus;
  }
  if ( candidate < checkpoint && checkpoint - candidate > modulus / 2
       && candidate <= numeric_limits<uint64_t>::max() - modulus ) {
    return candidate + modulus;
  }
  return candidate;
}
