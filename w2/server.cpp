#include <enet/enet.h>
#include <iostream>
#include <cstring>
#include <random>
#include <unordered_map>

struct Client {
  Client()
  {
    std::random_device dev;
    std::mt19937 gen(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, 1000);
    int client_id = dist(gen);
    name = "Client-" + std::to_string(client_id);
    id = client_id;
  }

  std::string name;  //generated with id
  int id;  //random number [0, 1000]
};

using Clients = std::unordered_map<ENetPeer*, Client>;

void send_info_on_init(const Client& new_client, ENetPeer* new_client_peer, const Clients& clients)
{
  std::string buffer = "Welcome new user: " + new_client.name + ", with ID: " + std::to_string(new_client.id);
  std::string new_buffer = "Here are all connected users:\n";

  for (const auto& client : clients)
  {
    ENetPacket *packet = enet_packet_create(buffer.c_str(), strlen(buffer.c_str()) + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client.first, 0, packet);

    new_buffer += client.second.name + "\n";
  }

  if (clients.empty())
  {
    new_buffer = "You are the first user!";
  }

  ENetPacket *new_packet = enet_packet_create(new_buffer.c_str(), strlen(new_buffer.c_str()) + 1, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(new_client_peer, 0, new_packet);
}

void send_server_sys_time(const Clients& clients, uint32_t sys_time)
{
  std::string buffer = "Server time: ";
  for (const auto& client : clients)
  {
    buffer += std::to_string(sys_time + client.second.id);
    ENetPacket *packet = enet_packet_create(buffer.c_str(), strlen(buffer.c_str()) + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client.first, 0, packet);
  }
}

void send_client_ping(const Clients& clients)
{
  for (const auto& client_to : clients)
  {
    for (const auto& client_from : clients)
    {
      if (client_to.second.name != client_from.second.name)
      {
        std::string buffer = client_from.second.name + " is alive!";
        ENetPacket *packet = enet_packet_create(buffer.c_str(), strlen(buffer.c_str()) + 1, ENET_PACKET_FLAG_UNSEQUENCED);
        enet_peer_send(client_to.first, 1, packet);
      }
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
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10889;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  Clients clients;

  uint32_t timeStart = enet_time_get();
  uint32_t lastPingTime = timeStart;
  uint32_t lastSysTime = timeStart;

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(server, &event, 10) > 0)
    {
      switch (event.type)
      {
        case ENET_EVENT_TYPE_CONNECT:
        {
          printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
          Client client = Client();
          send_info_on_init(client, event.peer, clients);
          clients[event.peer] = client;
          break;
        }
        case ENET_EVENT_TYPE_RECEIVE:
          printf("Packet received '%s' from '%u'\n", event.packet->data, event.peer->address.port);
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

    uint32_t curTime = enet_time_get();
    if (!clients.empty())
    {
      if (curTime - lastPingTime > 10000)
      {
        lastPingTime = curTime;
        send_client_ping(clients);
      }
      if (curTime - lastSysTime > 10000)
      {
        lastSysTime = curTime;
        send_server_sys_time(clients, curTime);
      }
    }
  }
  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}
