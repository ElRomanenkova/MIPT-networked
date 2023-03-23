#pragma once

#include <cstring>

class Bitstream {
public:
  explicit Bitstream(uint8_t* data_ptr)
    : data_ptr_(data_ptr),
      data_offset_(0) {
  }

  template <typename Type>
  void write(const Type& data) {
    memcpy(data_offset_ + data_ptr_, reinterpret_cast<const uint8_t*>(&data), sizeof(Type));
    data_offset_ += sizeof(Type);
  }

  template <typename Type>
  void read(Type& data) {
    memcpy(reinterpret_cast<uint8_t*>(&data), data_offset_ + data_ptr_, sizeof(Type));
    data_offset_ += sizeof(Type);
  }

private:
  uint8_t* data_ptr_;
  uint32_t data_offset_;
};