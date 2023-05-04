// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <functional>
#include "raylib.h"
#include <enet/enet.h>
#include <math.h>
#include <assert.h>

#include <vector>
#include "entity.h"
#include "protocol.h"
#include "quantisation.h"


static std::vector<Entity> entities;
static uint16_t my_entity = invalid_entity;

void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);
  // TODO: Direct adressing, of course!
  for (const Entity &e : entities)
    if (e.eid == newEntity.eid)
      return; // don't need to do anything, we already have entity
  entities.push_back(newEntity);
}

void on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_entity);
}

void on_snapshot(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f; float ori = 0.f;
  deserialize_snapshot(packet, eid, x, y, ori);
  // TODO: Direct adressing, of course!
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.x = x;
      e.y = y;
      e.ori = ori;
    }
}

bool is_quantized_successfully(float x, float y, float lo, float hi, int num_bits)
{
  int range = (1 << num_bits) - 1;
  return abs(x - y) < (hi - lo) / static_cast<float>(range);
}

void testing()
{
  PackedFloat<uint8_t, 4> packedFloat1(0.1f, -1.f, 1.f);
  auto unpackedFloat1 = packedFloat1.unpack(-1.f, 1.f);
  assert(is_quantized_successfully(unpackedFloat1, 0.1f, -1.f, 1.f, 4));

  PackedFloat2<uint16_t, 8, 8> packedFloat2({4.f, -5.f}, {-10.f, -10.f}, {10.f, 10.f});
  auto unpackedFloat2 = packedFloat2.unpack({-10.f, -10.f}, {10.f, 10.f});
  assert(is_quantized_successfully(unpackedFloat2.x, 4.f, -10.f, 10.f, 8));
  assert(is_quantized_successfully(unpackedFloat2.y, -5.f, -10.f, 10.f, 8));

  PackedFloat3<uint32_t, 11, 10, 11> packedFloat3({0.1f, -0.2f, 0.3f}, {-1.f, -1.f, -1.f}, {1.f, 1.f, 1.f});
  auto unpackedFloat3 = packedFloat3.unpack({-1.f, -1.f, -1.f}, {1.f, 1.f, 1.f});
  assert(is_quantized_successfully(unpackedFloat3.x, 0.1f, -1.f, 1.f, 11));
  assert(is_quantized_successfully(unpackedFloat3.y, -0.2f, -1.f, 1.f, 10));
  assert(is_quantized_successfully(unpackedFloat3.z, 0.3f, -1.f, 1.f, 11));

  printf("Part1: Test passed successfully!\n");

  auto* mem = (uint8_t *)malloc(sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t));
  auto b1 = Bitstream(mem);
  auto b2 = Bitstream(mem);
  uint32_t val = 0;

  b1.write_packed_uint(111u);
  b1.write_packed_uint(2222u);
  b1.write_packed_uint(333444555u);

  b2.read_packed_uint(val);
  assert(val == 111);
  b2.read_packed_uint(val);
  assert(val == 2222);
  b2.read_packed_uint(val);
  assert(val == 333444555);

  printf("Part2: Test passed successfully!\n");
}

int main(int argc, const char **argv)
{
  testing();

  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = 10131;

  ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
  if (!serverPeer)
  {
    printf("Cannot connect to server");
    return 1;
  }

  int width = 600;
  int height = 600;

  InitWindow(width, height, "w5 networked MIPT");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  Camera2D camera = { {0, 0}, {0, 0}, 0.f, 1.f };
  camera.target = Vector2{ 0.f, 0.f };
  camera.offset = Vector2{ width * 0.5f, height * 0.5f };
  camera.rotation = 0.f;
  camera.zoom = 10.f;


  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

  bool connected = false;
  while (!WindowShouldClose())
  {
    float dt = GetFrameTime();
    ENetEvent event;
    while (enet_host_service(client, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        send_join(serverPeer);
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
        case E_SERVER_TO_CLIENT_NEW_ENTITY:
          on_new_entity_packet(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
          on_set_controlled_entity(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SNAPSHOT:
          on_snapshot(event.packet);
          break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    if (my_entity != invalid_entity)
    {
      bool left = IsKeyDown(KEY_LEFT);
      bool right = IsKeyDown(KEY_RIGHT);
      bool up = IsKeyDown(KEY_UP);
      bool down = IsKeyDown(KEY_DOWN);
      // TODO: Direct adressing, of course!
      for (Entity &e : entities)
        if (e.eid == my_entity)
        {
          // Update
          float thr = (up ? 1.f : 0.f) + (down ? -1.f : 0.f);
          float steer = (left ? -1.f : 0.f) + (right ? 1.f : 0.f);

          // Send
          send_entity_input(serverPeer, my_entity, thr, steer);
        }
    }

    BeginDrawing();
      ClearBackground(GRAY);
      BeginMode2D(camera);
        DrawRectangleLines(-16, -8, 32, 16, GetColor(0xff00ffff));
        for (const Entity &e : entities)
        {
          const Rectangle rect = {e.x, e.y, 3.f, 1.f};
          DrawRectanglePro(rect, {0.f, 0.5f}, e.ori * 180.f / PI, GetColor(e.color));
        }

      EndMode2D();
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
