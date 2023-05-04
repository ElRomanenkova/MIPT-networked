#pragma once

#include <cstring>
#include <iostream>

// https://stackoverflow.com/questions/4181951/how-to-check-whether-a-system-is-big-endian-or-little-endian
const int n = 1;
#define is_little_endian() ((*(char *)&n) == 1)

static uint16_t reverse_if_little_endian(uint16_t num) {
  if (!is_little_endian())
    return num;

  uint8_t c1, c2;
  c1 = num & 255;
  c2 = (num >> 8) & 255;
  return (c1 << 8) + c2;
}

static uint32_t reverse_if_little_endian(uint32_t num) {
  if (!is_little_endian())
    return num;

  uint8_t c1, c2, c3, c4;
  c1 = num & 255;
  c2 = (num >> 8) & 255;
  c3 = (num >> 16) & 255;
  c4 = (num >> 24) & 255;
  return ((uint32_t)c1 << 24) + ((uint32_t)c2 << 16) + ((uint32_t)c3 << 8) + c4;
}

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

  template<typename Type>
  void write_packed_uint(Type value);

  template<typename Type>
  void read_packed_uint(Type& value);

private:
  uint8_t* data_ptr_;
  uint32_t data_offset_;
};

///--------------------------------------------------

template<typename Type>
void Bitstream::write_packed_uint(Type value)
{
  if (value < 0x80) // = 2^7 = 128
  {
    const auto data = uint8_t{value};
    memcpy(data_offset_ + data_ptr_, reinterpret_cast<const uint8_t*>(&data), sizeof(uint8_t));
    data_offset_ += sizeof(uint8_t);
  }

  else if (value < 0x4'000) // = 2^14 = 16'384
  {
    const auto data = reverse_if_little_endian(uint16_t{(1 << 15) | value});
    memcpy(data_offset_ + data_ptr_, reinterpret_cast<const uint16_t*>(&data), sizeof(uint16_t));
    data_offset_ += sizeof(uint16_t);
  }

  else if (value < 0x40'000'000) // = 2^30 = 1'073'741'824
  {
    const auto data = reverse_if_little_endian(uint32_t{(3 << 30) | value});
    memcpy(data_offset_ + data_ptr_, reinterpret_cast<const uint32_t*>(&data), sizeof(uint32_t));
    data_offset_ += sizeof(uint32_t);
  }

  else
  {
    static_assert(true, "ASSERT: value is too big for writing");
  }
}

template<typename Type>
void Bitstream::read_packed_uint(Type& value) {
  uint8_t type = *(uint8_t*)(data_ptr_ + data_offset_);

  switch (type >> 6)
  {
    case 0:
    case 1:
      value = type;
      data_offset_ += sizeof(uint8_t);
      break;
    case 2:
      value = reverse_if_little_endian(*(uint16_t*)(data_ptr_ + data_offset_)) & 0x3FFF;
      data_offset_ += sizeof(uint16_t);
      break;
    default:
      value = reverse_if_little_endian(*(uint32_t*)(data_ptr_ + data_offset_)) & 0x3FFFFFFF;
      data_offset_ += sizeof(uint32_t);
      break;
  }
}