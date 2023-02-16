#pragma once

struct addrinfo;

enum MessageType : uint32_t
{
  INIT,
  KEEPALIVE,
  DATA
};

int create_dgram_socket(const char *address, const char *port, addrinfo *res_addr);
