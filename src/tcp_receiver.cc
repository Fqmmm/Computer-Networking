#include "tcp_receiver.hh"

#include <algorithm>
#include <utility>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST || reader().has_error() ) {
    reader().set_error();
    return;
  }

  // 只用第一个 SYN 建立序列号基准；收到 SYN 之前忽略普通报文
  if ( !isn_.has_value() ) {
    if ( !message.SYN ) {
      return;
    }
    isn_ = message.seqno;
  }

  // 已重组的字节数加上 SYN 占用的一个位置，作为绝对序列号参考点
  const uint64_t checkpoint = writer().bytes_pushed() + 1;
  const uint64_t absolute_seqno = message.seqno.unwrap( isn_.value(), checkpoint );
  if ( absolute_seqno == 0 && !message.SYN ) {
    return; // 此位置属于 SYN，没有对应的数据下标，也避免减一时下溢
  }

  // 带 SYN 时，payload 从该序列号的下一个位置开始；FIN 交由重组器处理
  const uint64_t stream_index = message.SYN ? absolute_seqno : absolute_seqno - 1;
  reassembler_.insert( stream_index, move( message.payload ), message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage message {};
  if ( isn_.has_value() ) {
    // 确认号指向下一个需要的序列号；SYN 和已完整接收的 FIN 各占一个位置
    const uint64_t absolute_ackno = writer().bytes_pushed() + 1 + writer().is_closed();
    message.ackno = Wrap32::wrap( absolute_ackno, isn_.value() );
  }

  // 接收窗口取字节流的剩余容量，并限制在 16 位窗口字段的范围内
  message.window_size = static_cast<uint16_t>( min<uint64_t>( writer().available_capacity(), UINT16_MAX ) );
  message.RST = reader().has_error();
  return message;
}
