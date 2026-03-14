#pragma once

#include "byte_stream.hh"
#include <map>
#include <string>

class Reassembler
{
public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler( ByteStream&& output ) : output_( std::move( output ) ) {}

  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: 子字符串的第一个字节(表明其在这个字节流中的索引)
   *   `data`: 子字符串本体
   *   `is_last_substring`: 指明自己是原始字节流中的最后一个子字符串
   *   `output`: 对于写入器的引用
   *
   * 重组器的任务是 重组被编号的子字符串 (可能无序或者冗余) 为原始正确的字节流. 一旦重组器
   * 读取到了字节流中的下一个字符, 它会将其写入 output_.
   *
   * 如果重组器识别了适合字节流可用容量的 子字符串
   * 但是还不能写入 (因为原始字节流中更早子字符串还没有收到), 它应该先 存储在内部直到空隙被填满
   *
   * 重组器应该丢弃超过 ByteStream 可用容量 大小的任何子字符串
   * (i.e., 即使之前的空隙被填充完成了也不能写入的子字符串).
   *
   * 重组器应该在 最后一个子字符串被写入时 关闭字节流
   */
  void insert( uint64_t first_index, const std::string& data, bool is_last_substring );

  // 重组器内部存储的字节数
  // 这个函数仅用作测试; 不要添加额外的状态来支持它.
  uint64_t count_bytes_pending() const;

  // 访问输出流读取器
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // 访问输出流写入器 const-only 不可以从外部进行写入
  const Writer& writer() const { return output_.writer(); }

private:
  ByteStream output_;
  std::map<uint64_t, std::string> inner_cache_ {};
  uint64_t first_unassembled_ { 0 };
  uint64_t first_unacceptable_ { 0 };
  uint64_t eof_index_ { 0 };
  bool has_last_ { false };

  void condition_close()
  {
    if ( has_last_ && first_unassembled_ == eof_index_ ) {
      output_.writer().close();
    }
  }
};
