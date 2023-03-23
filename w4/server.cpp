#include <enet/enet.h>
#include <iostream>
//#include "entity.h"
#include "protocol.h"
#include <cstdlib>
#include <vector>
#include <map>
#include <raymath.h>


static std::vector<Entity> entities;
static std::map<uint16_t, Vector2> targets;
static std::map<uint16_t, ENetPeer*> controlledMap;

const uint16_t NUM_AI_ENTITIES = 10;
const uint16_t FPS = 60;

std::random_device rd;
std::mt19937 gen(rd());

Vector2 gen_rand_position(int wight, int height) {
  std::uniform_real_distribution<float> dist_w(-static_cast<float>(wight), static_cast<float>(wight));
  std::uniform_real_distribution<float> dist_h(-static_cast<float>(height), static_cast<float>(height));
  return {dist_w(gen), dist_h(gen)};
}

Color gen_rand_color() {
  std::uniform_int_distribution<unsigned char> dist(0, 255);
  return {255, dist(gen), dist(gen), 255};
}

float gen_random_size() {
  std::uniform_real_distribution<float> dist(4, 10);
  return dist(gen);
}

void gen_ai_entities() {
  for (uint16_t i = 0; i < NUM_AI_ENTITIES; i++) {
    Color color = gen_rand_color();
    Vector2 pos = gen_rand_position(350, 350);

    entities.push_back({color, pos.x, pos.y, gen_random_size(), Entity::Type::AI_TYPE, i});

    targets[i] = gen_rand_position(350, 350);
  }
}

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  // send all entities
  for (const Entity &ent : entities)
    send_new_entity(peer, ent);

  // find max eid
  uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].eid;
  for (const Entity &e : entities)
    maxEid = std::max(maxEid, e.eid);

  uint16_t newEid = maxEid + 1;
  Color color = gen_rand_color();
  auto pos = gen_rand_position(350, 350);
  Entity ent = {color, pos.x, pos.y, 10.f, Entity::Type::PLAYER, newEid};
  entities.push_back(ent);

  controlledMap[newEid] = peer;

  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
}

void on_state(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f;
  deserialize_entity_state(packet, eid, x, y);
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.x = x;
      e.y = y;
    }
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10132;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  gen_ai_entities();

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(server, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
          case E_CLIENT_TO_SERVER_JOIN:
            on_join(event.packet, event.peer, server);
            break;
          case E_CLIENT_TO_SERVER_STATE:
            on_state(event.packet);
            break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }

    for (Entity& e : entities)
    {
      for (Entity& e_inner : entities)
      {
        if (e.eid == e_inner.eid)
          continue;

        auto type = e.type;
        auto inner_type = e.type;

        bool collision;
        float is_circle_size1;
        float is_circle_size2;

        if (type == Entity::Type::AI_TYPE && inner_type == Entity::Type::AI_TYPE)
        {
          collision = CheckCollisionCircles({e.x, e.y}, e.size / 2.f, {e_inner.x, e_inner.y}, e.size / 2.f);
          is_circle_size1 = e.size / 2.f;
          is_circle_size2 = e_inner.size / 2.f;
        }
        else if (type == Entity::Type::AI_TYPE && inner_type == Entity::Type::PLAYER)
        {
          collision = CheckCollisionCircleRec({e.x, e.y}, e.size / 2.f, {e_inner.x, e_inner.y, e_inner.size, e_inner.size});
          is_circle_size1 = e.size / 2.f;
          is_circle_size2 = e_inner.size;
        }
        else if (type == Entity::Type::PLAYER && inner_type == Entity::Type::AI_TYPE)
        {
          collision = CheckCollisionCircleRec({e_inner.x, e_inner.y}, e_inner.size / 2.f, {e.x, e.y, e.size, e.size});
          is_circle_size1 = e.size;
          is_circle_size2 = e_inner.size / 2.f;
        }
        else
        {
          collision = CheckCollisionRecs({e_inner.x, e_inner.y, e_inner.size, e_inner.size}, {e.x, e.y, e.size, e.size});
          is_circle_size1 = e.size;
          is_circle_size2 = e_inner.size;
        }

        if (!collision)
          continue;

        auto pos = gen_rand_position(350, 350);

        if (is_circle_size1 > is_circle_size2)
        {
          e.size = fmin(e.size + e_inner.size / 2.f, 40.f);
          e_inner.size = fmax(e_inner.size / 2.f, 4.f);
          e_inner.x = pos.x;
          e_inner.y = pos.y;
        }
        if (is_circle_size2 > is_circle_size1)
        {
          e_inner.size = fmin(e_inner.size + e.size / 2.f, 40.f);
          e.size = fmax(e.size / 2.f, 4.f);
          e.x = pos.x;
          e.y = pos.y;
        }

        if (controlledMap.contains(e_inner.eid))
          send_snapshot(controlledMap[e_inner.eid], e_inner.eid, e_inner.x, e_inner.y, e_inner.size);

        if (controlledMap.contains(e.eid))
          send_snapshot(controlledMap[e.eid], e.eid, e.x, e.y, e.size);
      }

      if (e.type == Entity::Type::AI_TYPE)
      {
        if (Vector2Distance(targets[e.eid], {e.x, e.y}) < 1.f)
          targets[e.eid] = gen_rand_position(350, 350);

        Vector2 dir = Vector2Normalize(Vector2Subtract(targets[e.eid], {e.x, e.y}));

        e.x += dir.x * 1 / FPS * 100.f;
        e.y += dir.y * 1 / FPS * 100.f;
      }

      for (size_t i = 0; i < server->peerCount; ++i)
      {
        ENetPeer *peer = &server->peers[i];
        if (!controlledMap.contains(e.eid) || controlledMap[e.eid] != peer)
          send_snapshot(peer, e.eid, e.x, e.y, e.size);
      }
    }
//    usleep(20000);
    usleep(static_cast<useconds_t>(1.f / FPS * 1000000.f));
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}


