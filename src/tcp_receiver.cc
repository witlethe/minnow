#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( const TCPSenderMessage& message )
{
  if ( message.RST ) {
    is_closed_ = true;
    reassembler_.reader().set_error();
    return;
  }
  if (!is_ISN_valid) {
    if (!message.SYN) {
      return;
    }
    zero_point_ = message.seqno;
    is_ISN_valid = true;
  }
  
  uint64_t checkpoint = reassembler_.get_first_unassembled();
  uint64_t abs_seqno = message.seqno.unwrap(zero_point_, checkpoint);
  uint64_t first_index = message.SYN ? 0 : abs_seqno - 1;

  reassembler_.insert( first_index, message.payload, message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage msg;
  msg.window_size = std::min( reassembler_.writer().available_capacity(), static_cast<uint64_t>( UINT16_MAX ) );

  if ( is_ISN_valid ) {
    uint64_t ack_val = reassembler_.get_first_unassembled() + 1;
    if ( reassembler_.writer().is_closed() ) {
      ack_val += 1;
    }
    msg.ackno = Wrap32::wrap( ack_val, zero_point_ );
  }

  if ( reassembler_.reader().has_error() ) {
    msg.RST = true;
  }
  return msg;
}
