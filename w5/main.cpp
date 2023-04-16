// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <functional>
#include "raylib.h"
#include <enet/enet.h>
#include <math.h>

#include <vector>
#include <map>
#include <deque>
#include "entity.h"
#include "protocol.h"


static std::vector<Entity> entities;
static std::map<uint16_t, std::deque<Snapshot>> snapshots;
static uint16_t my_entity = invalid_entity;

static std::vector<TickSnapshot> snapshotsHistory;
static std::vector<TickInput> inputsHistory;

void interpolate_entity(Entity& entity, uint32_t cur_time)
{
  auto snap_time = snapshots[entity.eid][1].time;

  while (cur_time > snap_time and snapshots[entity.eid].size() > 2) {
    snapshots[entity.eid].pop_front();
    snap_time = snapshots[entity.eid][1].time;
  }

  Snapshot& cur_snap = snapshots[entity.eid][1];
  Snapshot& prev_snap = snapshots[entity.eid][0];

  float t = static_cast<float>(cur_time - prev_snap.time) / static_cast<float>(cur_snap.time - prev_snap.time);

  entity.x = prev_snap.x + t * (cur_snap.x - prev_snap.x);
  entity.y = prev_snap.y + t * (cur_snap.y - prev_snap.y);
  entity.ori = prev_snap.ori + t * (cur_snap.ori - prev_snap.ori);
}

void resimulate_entity(TickSnapshot snap, Entity& entity)
{
  entity.x = snap.x;
  entity.y = snap.y;
  entity.ori = snap.ori;

  for (const auto& input : inputsHistory) {
    entity.thr = input.thr;
    entity.steer = input.steer;
    simulate_entity(entity, DT);
  }
}

void clear_history(uint32_t tick)
{
  std::erase_if(snapshotsHistory, [tick](auto &snapshot){
    return snapshot.tick < tick;
  });
  std::erase_if(inputsHistory, [tick](auto &input){
    return input.tick < tick;
  });
}

void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);
  // TODO: Direct adressing, of course!
  for (const Entity &e : entities)
    if (e.eid == newEntity.eid)
      return; // don't need to do anything, we already have entity
  entities.push_back(newEntity);

  if (newEntity.eid != my_entity) {
    snapshots[newEntity.eid].push_back({enet_time_get(), newEntity.x, newEntity.y, newEntity.ori});
  }
}

void on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_entity);
}

void on_snapshot(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  uint32_t tick = 0;
  float x = 0.f; float y = 0.f; float ori = 0.f;
  deserialize_snapshot(packet, eid, x, y, ori, tick);

  if (eid != my_entity) {
    auto t = enet_time_get() + OFFSET;
    snapshots[eid].push_back({t, x, y, ori});
  }
  else {
    if (snapshotsHistory.empty() or inputsHistory.empty())
      return;

    clear_history(tick - 1);
    auto last_snap = snapshotsHistory.front();

    if (last_snap.x != x or last_snap.y != y or last_snap.ori != ori) {
      TickSnapshot snap = {tick, x, y, ori};
      resimulate_entity(snap, entities[my_entity]);
    }
  }
}

int main(int argc, const char **argv)
{
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
  uint32_t prev_time = enet_time_get();

  while (!WindowShouldClose())
  {

    ENetEvent event;
    uint32_t cur_time = enet_time_get();

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

          e.thr = thr;
          e.steer = steer;

          auto dt_count = static_cast<uint32_t>(static_cast<float>(cur_time - prev_time) / (DT * 1000));
          prev_time += cur_time - prev_time;

          for (uint32_t t = 0; t < dt_count; t++) {
            simulate_entity(e, DT);
            e.last_tick++;
            snapshotsHistory.push_back({e.last_tick, e.x, e.y, e.ori});
            inputsHistory.push_back({e.last_tick, e.thr, e.steer});
          }

          // Send
          send_entity_input(serverPeer, e.eid, e.thr, e.steer);
        }
        else
        {
          interpolate_entity(e, cur_time);
        }
    }

    BeginDrawing();
      ClearBackground(WHITE);
      BeginMode2D(camera);
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
