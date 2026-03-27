#pragma once

#include <cstdint>
#include <list>
#include <string>
#include <string_view>

class Reader;
class Writer;

class ByteStream
{
public:
  explicit ByteStream( uint64_t capacity ); // 字节流构造器，接收字节流的最大容量

  // Helper functions (provided) to access the ByteStream's Reader and Writer interfaces
  Reader& reader(); // 读取端
  const Reader& reader() const;
  Writer& writer(); // 写入端
  const Writer& writer() const;

  // 调试接口
  void set_error() { error_ = true; };       // Signal that the stream suffered an error.
  bool has_error() const { return error_; }; // Has the stream had an error?

protected:
  // Please add any additional state to the ByteStream here, and not to the Writer and Reader interfaces.
  std::list<std::string> buf_; // 底层容器
  uint64_t written_bytes_;     // 总写入
  uint64_t read_bytes_;        // 总读取
  bool closed_;                // 流是否关闭
  uint64_t capacity_;          // 缓冲区容量
  uint64_t buffer_bytes_;      // 缓冲区现存字符
  uint64_t front_limit_;       // 底层容器首元字符数
  bool error_ {};              // 错误 flag
};

class Writer : public ByteStream
{
public:
  void push( std::string data ); // 将数据尽可能传输到字节流中，舍弃超出容量的部分
  void close();                  // 标记流已经来到结尾，无法再继续写入

  bool is_closed() const;              // 查询流是否已经被关闭
  uint64_t available_capacity() const; // 当前有多少字节可以被
  uint64_t bytes_pushed() const;       // 流中总写入字节数
};

class Reader : public ByteStream
{
public:
  std::string_view peek() const; // 尽可能观测接下来需要读取的字节
  void pop( uint64_t len );      // 从缓冲区中移除 len 个字节

  bool is_finished() const;        // 流是否已经结束（关闭 并且 完全读出）
  uint64_t bytes_buffered() const; // 当前缓冲的字节数（写入但还没有读出）
  uint64_t bytes_popped() const;   // 流的总读出字节数
};

/*
 * 辅助函数，用于 peek 并从流中 pop 出 max_len 个字节到 out 字符串中
 */
void read( Reader& reader, uint64_t max_len, std::string& out );
