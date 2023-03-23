#include "protocol.h"

void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);

  auto bs = Bitstream(packet->data);
  bs.write(E_CLIENT_TO_SERVER_JOIN);

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

void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   2 * sizeof(float),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  auto bs = Bitstream(packet->data);
  bs.write(E_CLIENT_TO_SERVER_STATE);
  bs.write(eid);
  bs.write(x);
  bs.write(y);

  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float size)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   3 * sizeof(float),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  auto bs = Bitstream(packet->data);
  bs.write(E_SERVER_TO_CLIENT_SNAPSHOT);
  bs.write(eid);
  bs.write(x);
  bs.write(y);
  bs.write(size);

  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  auto bs = Bitstream(packet->data);
  MessageType mt{};

  bs.read(mt);
  bs.read(ent);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  auto bs = Bitstream(packet->data);
  MessageType mt{};

  bs.read(mt);
  bs.read(eid);
}

void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y)
{
  auto bs = Bitstream(packet->data);
  MessageType mt{};

  bs.read(mt);
  bs.read(eid);
  bs.read(x);
  bs.read(y);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float& size)
{
  auto bs = Bitstream(packet->data);
  MessageType mt{};

  bs.read(mt);
  bs.read(eid);
  bs.read(x);
  bs.read(y);
  bs.read(size);
}

