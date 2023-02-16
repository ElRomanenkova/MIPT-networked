#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include "socket_tools.h"

int main(int argc, const char **argv)
{
  const char *port = "2023";

  int sfd = create_dgram_socket(nullptr, port, nullptr);

  if (sfd == -1)
    return 1;

  const char *c_port = "2000";
  addrinfo clientAddrInfo;
  int c_sfd = -1;

  printf("Listening!\n");

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
      {
        auto message_type = buffer[0];

        switch (message_type)
        {
          case '0':  //INIT
          {
            printf("Welcome new user: %s\n", buffer + 1);
            c_sfd = create_dgram_socket("localhost", c_port, &clientAddrInfo);

            if (c_sfd == -1)
              return 1;
            break;
          }

          case '1':  //KEEPALIVE
            printf("KEEPALIVE: %s\n", buffer + 1);
            break;

          case '2':  //DATA
          {
            printf("%s\n", buffer + 1);

            if (c_sfd > 0)
            {
              std::string response = "Your message was received!";
              ssize_t res = sendto(c_sfd, response.c_str(), response.size(), 0, clientAddrInfo.ai_addr, clientAddrInfo.ai_addrlen);
              if (res == -1)
                std::cout << strerror(errno) << std::endl;
            }

            break;
          }

          default:
            break;
        }
      }
    }
  }
  return 0;
}
