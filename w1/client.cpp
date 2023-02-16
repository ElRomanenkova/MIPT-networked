#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <chrono>
#include <random>
#include <thread>
#include "socket_tools.h"

int send_keepalive_messages(int sfd, addrinfo addrInfo, int init_num)
{
  while (true)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(20000));
    // little reference to Rick and Morty
    std::string input = std::to_string(MessageType::KEEPALIVE) + std::to_string(init_num) + " wanna be alive, I am alive! Alive, I tell you...";
    ssize_t res = sendto(sfd, input.c_str(), input.size(), 0, addrInfo.ai_addr, addrInfo.ai_addrlen);
    if (res == -1)
      std::cout << strerror(errno) << std::endl;
  }
}

int listen_messages(int sfd)
{
  while (true)
  {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sfd, &readSet);

    timeval timeout = { 0, 100000 }; // 100 ms
    select(sfd + 1, &readSet, nullptr, nullptr, &timeout);


    if (FD_ISSET(sfd, &readSet))
    {
      constexpr size_t buf_size = 1000;
      static char buffer[buf_size];
      memset(buffer, 0, buf_size);

      ssize_t numBytes = recvfrom(sfd, buffer, buf_size - 1, 0, nullptr, nullptr);
      if (numBytes > 0)
        printf("--%s\n", buffer); // assume that buffer is a string
    }
  }
}

int send_messages(int sfd, addrinfo addrInfo)
{
  while (true)
  {
    std::string input;
    printf(">");
    std::getline(std::cin, input);
    input = std::to_string(MessageType::DATA) + input;
    ssize_t res = sendto(sfd, input.c_str(), input.size(), 0, addrInfo.ai_addr, addrInfo.ai_addrlen);
    if (res == -1)
      std::cout << strerror(errno) << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}


int main(int argc, const char **argv)
{
  const char *port = "2023";

  addrinfo resAddrInfo;

  int sfd = create_dgram_socket("localhost", port, &resAddrInfo);
  if (sfd == -1)
  {
    printf("Cannot create a socket\n");
    return 1;
  }

  const char *r_port = "2000";

  int r_sfd = create_dgram_socket(nullptr, r_port, nullptr);
  if (r_sfd == -1)
  {
    printf("Cannot create a socket\n");
    return 1;
  }

  std::random_device dev;
  std::mt19937 gen(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, 1000);
  int init_number = dist(gen);

  printf("My init number: %d\n", init_number);
  std::string hello_message = std::to_string(MessageType::INIT) + "Hello, server! I'm your client with init number: " + std::to_string(init_number);
  ssize_t res = sendto(sfd, hello_message.c_str(), hello_message.size(), 0, resAddrInfo.ai_addr, resAddrInfo.ai_addrlen);

  if (res == -1)
    std::cout << strerror(errno) << std::endl;

  std::thread keepalive_thread(send_keepalive_messages, sfd, resAddrInfo, init_number);
  std::thread listening_thread(listen_messages, r_sfd);
  std::thread sending_thread(send_messages, sfd, resAddrInfo);

  keepalive_thread.join();
  listening_thread.join();
  sending_thread.join();

  return 0;
}
