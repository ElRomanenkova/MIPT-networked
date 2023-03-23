#pragma once
#include <cstdint>
#include <random>
#include <raylib.h>

constexpr uint16_t invalid_entity = -1;
struct Entity
{
  enum Type {
    DEFAULT,
    AI_TYPE,
    PLAYER
  };

  Color color = {0, 0, 0, 0};
  float x = 0.f;
  float y = 0.f;
  float size = 0.f;
  Type type = DEFAULT;
  uint16_t eid = invalid_entity;
};

