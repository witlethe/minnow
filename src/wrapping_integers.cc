#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

namespace {
  uint64_t abs_diff(const uint64_t& a, const uint64_t& b) {
    if (a > b) {
      return a - b;
    }
    return b - a;
  }
} // namespace

/* convert absolute seqno to seqno */
Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32 { zero_point.raw_value_ + static_cast<uint32_t>( n ) };
}

/* convert seqno to cloest absolute seqno */
uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  auto z = zero_point.raw_value_;
  uint64_t offset = (raw_value_ - z) % (1ULL<<32);    // 我大概一辈子不会忘记你了，左移运算符优先级 :-)
  uint64_t anchor_base = (checkpoint & 0xFFFFFFFF00000000) | offset;
  uint64_t anchor_larger = anchor_base + (1ULL<<32);
  uint64_t anchor_smaller = anchor_base - (1ULL<<32);

  if (anchor_base > checkpoint) {
    return abs_diff(anchor_base, checkpoint) < abs_diff(anchor_smaller, checkpoint) ? anchor_base : anchor_smaller;
  }      
  return abs_diff(anchor_base, checkpoint) < abs_diff(anchor_larger, checkpoint) ? anchor_base : anchor_larger;
}
