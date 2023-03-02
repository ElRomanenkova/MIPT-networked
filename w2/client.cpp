#include <enet/enet.h>
#include <iostream>
#include <string.h>
#include <thread>
#include <random>

void processing_async_input(bool* isStart, ENetPeer* lobby)
{
  printf("Press 's' if you want to enter the server: ");

  while (lobby)
  {
    std::string input;
    std::getline(std::cin, input);

    if (input == "s")
    {
      printf("Connecting to server...\n");
      *isStart = true;
      break;
    }
  }
}

void send_server_sys_time(ENetPeer* server, uint32_t sys_time)
{
  std::random_device dev;
  std::mt19937 gen(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, 1000);
  int client_rand = dist(gen);

  std::string buffer = "My time: ";

  buffer += std::to_string(sys_time + client_rand);
  ENetPacket *packet = enet_packet_create(buffer.c_str(), strlen(buffer.c_str()) + 1, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(server, 0, packet);
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 2, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress lobby_address;
  enet_address_set_host(&lobby_address, "localhost");
  lobby_address.port = 10887;

  ENetPeer *lobbyPeer = enet_host_connect(client, &lobby_address, 2, 0);
  if (!lobbyPeer)
  {
    printf("Cannot connect to lobby");
    return 1;
  }

  ENetPeer *serverPeer = nullptr;

  bool isStart = false;
  bool isServerConnected = false;
  std::thread starting_session(processing_async_input, &isStart, lobbyPeer);

  uint32_t timeStart = enet_time_get();
  uint32_t lastFragmentedSendTime = timeStart;
  bool connected = false;

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(client, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        if (event.packet->data[0] == '#')
        {
          auto data = reinterpret_cast<char *>(event.packet->data);
          uint16_t server_port = std::atoi(data + 1);

          ENetAddress server_address;
          enet_address_set_host(&server_address, "localhost");
          server_address.port = server_port;

          serverPeer = enet_host_connect(client, &server_address, 2, 0);
          if (!serverPeer)
          {
            printf("Cannot connect to lobby");
            return 1;
          }

          isServerConnected = true;
          printf("Connected to server successfully!\n");
        }
        else
          printf("> '%s'\n", event.packet->data);

        enet_packet_destroy(event.packet);
        break;

      default:
        break;
      };
    }

    if (connected)
    {
      if (isStart)
      {
        std::string buffer = "#Start";
        ENetPacket *packet = enet_packet_create(buffer.c_str(), strlen(buffer.c_str()) + 1, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(lobbyPeer, 0, packet);
        isStart = false;
      }

      if (isServerConnected)
      {
        uint32_t curTime = enet_time_get();
        if (curTime - lastFragmentedSendTime > 10000)
        {
          lastFragmentedSendTime = curTime;
          send_server_sys_time(serverPeer, curTime);
        }
      }
    }
  }

  starting_session.join();

  atexit(enet_deinitialize);
  return 0;
}
