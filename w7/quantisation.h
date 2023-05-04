#pragma once
#include "mathUtils.h"
#include <limits>

template<typename T>
T pack_float(float v, float lo, float hi, int num_bits)
{
  T range = (1 << num_bits) - 1;  // std::numeric_limits<T>::max();
  return range * ((clamp(v, lo, hi) - lo) / (hi - lo));
}

template<typename T>
float unpack_float(T c, float lo, float hi, int num_bits)
{
  T range = (1 << num_bits) - 1;  // std::numeric_limits<T>::max();
  return float(c) / range * (hi - lo) + lo;
}

///--------------------------------------------------

template<typename T, int num_bits>
struct PackedFloat
{
  static_assert(num_bits <= sizeof(T) * 8, "ASSERT: type is too small");

  T packedVal;

  PackedFloat(float v, float lo, float hi)
  {
    pack(v, lo, hi);
  }

  PackedFloat(T compressed_val) : packedVal(compressed_val) {}

  void pack(float v, float lo, float hi)
  {
    packedVal = pack_float<T>(v, lo, hi, num_bits);
  }

  float unpack(float lo, float hi)
  {
    return unpack_float<T>(packedVal, lo, hi, num_bits);
  }
};

typedef PackedFloat<uint8_t, 4> float4bitsQuantized;

///--------------------------------------------------

template <typename T, int num_bits_x, int num_bits_y>
struct PackedFloat2
{
  static_assert(num_bits_x + num_bits_y <= sizeof(T) * 8, "ASSERT: type is too small");

  T packedVal;

  struct float2
  {
    float x;
    float y;
  };

  PackedFloat2(float2 v, float2 lo, float2 hi)
  {
    pack(v, lo, hi);
  }

  PackedFloat2(T compressed_val) : packedVal(compressed_val) {}

  void pack(float2 v, float2 lo, float2 hi)
  {
    const auto packed_x = pack_float<T>(v.x, lo.x, hi.x, num_bits_x);
    const auto packed_y = pack_float<T>(v.y, lo.y, hi.y, num_bits_y);

    packedVal = packed_x << num_bits_y | packed_y;
  }

  float2 unpack(float2 lo, float2 hi)
  {
    float2 ret;
    ret.x = unpack_float<T>(packedVal >> num_bits_y, lo.x, hi.x, num_bits_x);
    ret.y = unpack_float<T>(packedVal & ((1 << num_bits_y) - 1), lo.y, hi.y, num_bits_y);
    return ret;
  }
};

///--------------------------------------------------

template <typename T, int num_bits_x, int num_bits_y, int num_bits_z>
struct PackedFloat3
{
  static_assert(num_bits_x + num_bits_y + num_bits_z <= sizeof(T) * 8, "ASSERT: type is too small");

  T packedVal;

  struct float3
  {
    float x;
    float y;
    float z;
  };

  PackedFloat3(float3 v, float3 lo, float3 hi)
  {
    pack(v, lo, hi);
  }

  PackedFloat3(T compressed_val) : packedVal(compressed_val) {}

  void pack(float3 v, float3 lo, float3 hi)
  {
    const auto packedX = pack_float<T>(v.x, lo.x, hi.x, num_bits_x);
    const auto packedY = pack_float<T>(v.y, lo.y, hi.y, num_bits_y);
    const auto packedZ = pack_float<T>(v.z, lo.z, hi.z, num_bits_z);

    packedVal = (packedX << (num_bits_y + num_bits_z)) | (packedY << num_bits_z) | packedZ;
  }

  float3 unpack(float3 lo, float3 hi)
  {
    float3 ret;
    ret.x = unpack_float<T>(packedVal >> (num_bits_y + num_bits_z), lo.x, hi.x, num_bits_x);
    ret.y = unpack_float<T>((packedVal >> num_bits_z) & ((1 << num_bits_y) - 1), lo.y, hi.y, num_bits_y);
    ret.z = unpack_float<T>(packedVal & ((1 << num_bits_z) - 1), lo.z, hi.z, num_bits_z);
    return ret;
  }
};
