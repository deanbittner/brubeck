#ifndef __BRUBECK_DATADOG_H__
#define __BRUBECK_DATADOG_H__

#include "net/udp.h"
#include <regex.h>

struct brubeck_datadog {

  struct brubeck_backend backend;

  UdpSender_t *out;
  char host[512];
  int port;
  char regex_s[8192];
  regex_t regex_c;
  int regex_good;
  
  size_t sent;
};

struct brubeck_backend *
brubeck_datadog_new(struct brubeck_server *server,
		    json_t *settings, int shard_n);

#endif
