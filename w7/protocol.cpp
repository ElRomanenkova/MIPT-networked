#include "protocol.h"
#include "quantisation.h"
#include <cstring> // memcpy
#include <iostream>

void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
  *packet->data = E_CLIENT_TO_SERVER_JOIN;

  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(Entity),
                                                   ENET_PACKET_FLAG_RELIABLE);
  auto bs = Bitstream(packet->data);
  bs.write(E_SERVER_TO_CLIENT_NEW_ENTITY);
  bs.write(ent);

  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);
  auto bs = Bitstream(packet->data);
  bs.write(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
  bs.write(eid);

  enet_peer_send(peer, 0, packet);
}

void send_entity_input(ENetPeer *peer, uint16_t eid, float thr, float ori, uint8_t header, uint16_t cur_id, uint16_t ref_id)
{
  size_t packet_size = sizeof(uint8_t) + sizeof(uint16_t) * 3 + sizeof(uint8_t);
  if (header == 0x80) // 0b10000000
    packet_size += sizeof(uint8_t);

  ENetPacket *packet = enet_packet_create(nullptr, packet_size,
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  auto bs = Bitstream(packet->data);
  bs.write(E_CLIENT_TO_SERVER_INPUT);
  bs.write(eid);
  bs.write(cur_id);
  bs.write(ref_id);
  bs.write(header);

  if (header == 0x80) { // 0b10000000
    float4bitsQuantized thrPacked(thr, -1.f, 1.f);
    float4bitsQuantized oriPacked(ori, -1.f, 1.f);
    uint8_t thrSteerPacked = (thrPacked.packedVal << 4) | oriPacked.packedVal;
    bs.write(thrSteerPacked);
  }

  enet_peer_send(peer, 1, packet);
}

typedef PackedFloat2<uint32_t, 11, 10> PositionQuantized;

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float ori)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   sizeof(uint16_t) +
                                                   sizeof(uint16_t) +
                                                   sizeof(uint8_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  auto bs = Bitstream(packet->data);
  bs.write(E_SERVER_TO_CLIENT_SNAPSHOT);
  bs.write(eid);
  PositionQuantized posQuantized{{x, y}, {-16.f, -8.f}, {16.f, 8.f}};
  auto oriPacked = pack_float<uint8_t>(ori, -pi, pi, 8);

  bs.write(posQuantized.packedVal);
  bs.write(oriPacked);

  enet_peer_send(peer, 1, packet);
}

void send_input_ack(ENetPeer* peer, uint16_t ref_id)
{
  ENetPacket* packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
                                          ENET_PACKET_FLAG_UNSEQUENCED);
  auto bs = Bitstream(packet->data);
  bs.write(E_SERVER_TO_CLIENT_INPUT_ACK);
  bs.write(ref_id);
  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  auto bs = Bitstream(packet->data);
  MessageType type{};
  bs.read(type);
  bs.read(ent);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  auto bs = Bitstream(packet->data);
  MessageType type{};
  bs.read(type);
  bs.read(eid);
}

void deserialize_entity_input(ENetPacket *packet, uint16_t &eid, float &thr, float &steer, uint16_t& cur_id)
{
  auto bs = Bitstream(packet->data);
  MessageType type{};
  uint16_t ref_id;
  uint8_t header;
  bs.read(type);
  bs.read(eid);
  bs.read(cur_id);
  bs.read(ref_id);
  bs.read(header);

  if (header == 0x80) { // 0b10000000
    int8_t thrSteerPacked = 0;
    bs.read(thrSteerPacked);

    static auto neutralPackedValue = pack_float<uint8_t>(0.f, -1.f, 1.f, 4);
    float4bitsQuantized thrPacked(thrSteerPacked >> 4);
    float4bitsQuantized steerPacked(thrSteerPacked & 0x0f);
    thr = thrPacked.packedVal == neutralPackedValue ? 0.f : thrPacked.unpack(-1.f, 1.f);
    steer = steerPacked.packedVal == neutralPackedValue ? 0.f : steerPacked.unpack(-1.f, 1.f);

  }
  else {
    auto cur_input = *serverInputHistory[eid].inputHistory.end();
    steer = cur_input.steer;
    thr = cur_input.thr;
  }

  serverInputHistory[eid].inputHistory.push_back({cur_id, thr, steer});

  if (serverInputHistory[eid].reference_id < ref_id)
  {
    std::erase_if(serverInputHistory[eid].inputHistory, [ref_id](auto& localInput) {
      return localInput.id < ref_id;
    });
    serverInputHistory[eid].reference_id = ref_id;
  }
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &ori)
{
  auto bs = Bitstream(packet->data);
  MessageType type{};
  bs.read(type);
  bs.read(eid);

  uint32_t posPacked = 0;
  bs.read(posPacked);
  uint8_t oriPacked = 0;
  bs.read(oriPacked);

  PositionQuantized posQuantized{posPacked};
  PositionQuantized::float2 pos = posQuantized.unpack({-16.f, -8.f}, {16.f, 8.f});
  x = pos.x;
  y = pos.y;

  ori = unpack_float<uint8_t>(oriPacked, -pi, pi, 8);
}

void deserialize_input_ack(ENetPacket* packet, uint16_t& ref_id)
{
  auto bs = Bitstream(packet->data);
  MessageType type{};
  bs.read(type);
  bs.read(ref_id);
}
