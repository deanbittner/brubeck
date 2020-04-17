#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include "brubeck.h"

static bool rwi_carbon_is_connected(void *backend)
{
  struct brubeck_rwi_carbon *self = (struct brubeck_rwi_carbon *)backend;
  return (self->out_sock >= 0);
}

static int rwi_carbon_connect(void *backend)
{
  struct brubeck_rwi_carbon *self = (struct brubeck_rwi_carbon *)backend;

  if (rwi_carbon_is_connected(self))
    return 0;

  self->out_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (self->out_sock >= 0) {
    int rc = connect(self->out_sock,
                     (struct sockaddr *)&self->out_sockaddr,
                     sizeof(self->out_sockaddr));
		
    if (rc == 0) {
      log_splunk("backend=rwi_carbon event=connected");
      sock_enlarge_out(self->out_sock);
      return 0;
    }

    close(self->out_sock);
    self->out_sock = -1;
  }

  log_splunk_errno("backend=rwi_carbon event=failed_to_connect");
  return -1;
}

static void rwi_carbon_disconnect(struct brubeck_rwi_carbon *self)
{
  log_splunk_errno("backend=rwi_carbon event=disconnected");

  close(self->out_sock);
  self->out_sock = -1;
}

static void plaintext_each(
                           const char *key,
                           value_t value,
                           void *backend,
                           int timestamp)
{
  struct brubeck_rwi_carbon *rwi_carbon = (struct brubeck_rwi_carbon *)backend;
  char buffer[1024];
  char *ptr = buffer;
  size_t key_len = strlen(key);
  ssize_t wr;
  char *pc = NULL;

  if (!rwi_carbon_is_connected(rwi_carbon))
    return;

  pc = strchr (key, '|');
  if (pc != NULL)
    key_len = pc - key;

  memcpy(ptr, key, key_len);
  ptr += key_len;
  *ptr++ = ' ';

  ptr += brubeck_ftoa(ptr, value);
  *ptr++ = ' ';

  /* set this to the timestamp */
  if (timestamp > 0)
    {
      ptr += brubeck_itoa(ptr, timestamp);
    }
  else
    {
      /* this is set to zero.  rwi_carbon doc says -1 is time now at server */
      ptr += brubeck_itoa(ptr, rwi_carbon->backend.tick_time);
      //	ptr += brubeck_itoa(ptr, -1);
    }
  *ptr++ = '\n';

  wr = write_in_full(rwi_carbon->out_sock, buffer, ptr - buffer);
  if (wr < 0)
    {
      rwi_carbon_disconnect(rwi_carbon);
      return;
    }

  rwi_carbon->sent += wr;
}

struct brubeck_backend *
brubeck_rwi_carbon_new(struct brubeck_server *server, json_t *settings)
{
  struct brubeck_rwi_carbon *rwi_carbon = xcalloc(1, sizeof(struct brubeck_rwi_carbon));
  char *address;
  int port, frequency;

  json_unpack_or_die(settings,
                     "{s:s, s:i, s:i, s?:i}",
                     "address", &address,
                     "port", &port,
                     "frequency", &frequency,
                     "expire", &(rwi_carbon->backend.expire));

  rwi_carbon->backend.type = BRUBECK_BACKEND_RWI_CARBON;
  rwi_carbon->backend.connect = &rwi_carbon_connect;
  rwi_carbon->backend.is_connected = &rwi_carbon_is_connected;

  rwi_carbon->backend.sample = &plaintext_each;
  rwi_carbon->backend.flush = NULL;

  rwi_carbon->backend.sample_freq = frequency;
  rwi_carbon->backend.server = server;
  rwi_carbon->out_sock = -1;
  url_to_inaddr2(&rwi_carbon->out_sockaddr, address, port);

  brubeck_backend_run_threaded((struct brubeck_backend *)rwi_carbon);
  log_splunk("backend=rwi_carbon event=started");

  return (struct brubeck_backend *)rwi_carbon;
}
