#include <enet/enet.h>
#include <iostream>
#include <string.h>
#include <set>

void send_server_port(std::set<ENetPeer*>& clients, uint16_t server_port)
{
  std::string buffer = "#" + std::to_string(server_port);
  for (const auto& client : clients)
  {
    ENetPacket *packet = enet_packet_create(buffer.c_str(), strlen(buffer.c_str()) + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client, 0, packet);
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
  address.port = 10887;

  ENetHost *lobby = enet_host_create(&address, 32, 2, 0, 0);

  if (!lobby)
  {
    printf("Cannot create ENet lobby\n");
    return 1;
  }

  uint16_t server_port = 10889;

  std::set<ENetPeer*> clients;
  bool isGameStarted = false;

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(lobby, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);

        if (isGameStarted)
        {
          std::string buffer = "#" + std::to_string(server_port);
          ENetPacket *packet = enet_packet_create(buffer.c_str(), strlen(buffer.c_str()) + 1, ENET_PACKET_FLAG_RELIABLE);
          enet_peer_send(event.peer, 0, packet);
        }

        clients.insert(event.peer);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        printf("Packet received '%s'\n", event.packet->data);

        if (!isGameStarted && !strcmp((const char*)event.packet->data, "#Start"))
        {
          send_server_port(clients, server_port);
          isGameStarted = true;
        }

        enet_packet_destroy(event.packet);
        break;
      case ENET_EVENT_TYPE_DISCONNECT:
        printf("%x:%u disconnected\n", event.peer->address.host, event.peer->address.port);
        clients.erase(event.peer);
        break;
      default:
        break;
      };
    }
  }

  enet_host_destroy(lobby);

  atexit(enet_deinitialize);
  return 0;
}

