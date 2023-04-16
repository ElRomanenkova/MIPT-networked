#pragma once

#include <cstring>

const uint32_t TPS = 128; //ticks per second (on clients)
const float DT = 1.0f / TPS; // TPS=128 -> DT=0.0078 sec

const uint32_t SERVER_USLEEP = 100'000; //mks
const uint32_t OFFSET = SERVER_USLEEP * 0.0011; //ms

struct Snapshot {
  uint32_t time;

  float x;
  float y;
  float ori;
};

struct TickSnapshot {
  uint32_t tick;

  float x;
  float y;
  float ori;
};

struct TickInput {
  uint32_t tick;

  float thr;
  float steer;
};

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