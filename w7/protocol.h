#pragma once
#include <enet/enet.h>
#include <cstdint>
#include "entity.h"
#include "bitstream.h"

#include <vector>
#include <deque>
#include <map>

struct Input
{
  uint16_t id;
  float thr;
  float steer;
};

struct InputHistory
{
  uint16_t reference_id = 0;
  std::deque<Input> inputHistory;
};

static std::map<uint16_t, InputHistory> serverInputHistory;

enum MessageType : uint8_t
{
  E_CLIENT_TO_SERVER_JOIN = 0,
  E_SERVER_TO_CLIENT_NEW_ENTITY,
  E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY,
  E_CLIENT_TO_SERVER_INPUT,
  E_SERVER_TO_CLIENT_INPUT_ACK,
  E_SERVER_TO_CLIENT_SNAPSHOT,
  E_CLIENT_TO_SERVER_SNAPSHOT_ACK
};

void send_join(ENetPeer *peer);
void send_new_entity(ENetPeer *peer, const Entity &ent);
void send_set_controlled_entity(ENetPeer *peer, uint16_t eid);
void send_entity_input(ENetPeer *peer, uint16_t eid, float thr, float steer, uint8_t header, uint16_t cur_id, uint16_t ref_id);
void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float ori);
void send_input_ack(ENetPeer* peer, uint16_t ref_id);

MessageType get_packet_type(ENetPacket *packet);

void deserialize_new_entity(ENetPacket *packet, Entity &ent);
void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid);
void deserialize_entity_input(ENetPacket *packet, uint16_t &eid, float &thr, float &steer, uint16_t& cur_id);
void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &ori);
void deserialize_input_ack(ENetPacket* packet, uint16_t& ref_id);

