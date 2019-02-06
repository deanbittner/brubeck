#ifndef __BRUBECK_DATADOG_H__
#define __BRUBECK_DATADOG_H__

#include "net/udp.h"
#include <regex.h>

struct brubeck_datadog {

  struct brubeck_backend backend;

  UdpSender_t *out;
  char *address;
  int port;
  int frequency;
  char *regex_s;
  regex_t regex_c;
  int regex_good;
  
  size_t sent;
};

struct brubeck_backend *
brubeck_datadog_new(struct brubeck_server *server,
		    json_t *settings);

#endif
