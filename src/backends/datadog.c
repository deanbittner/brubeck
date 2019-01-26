
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

#include "brubeck.h"
#include "backends/datadog.h"

static bool
datadog_is_connected(void *backend)
{
  struct brubeck_datadog *datadog = (struct brubeck_datadog *)backend;
  return (datadog->out != NULL);
}

static int
datadog_connect(void *backend)
{
  struct brubeck_datadog *datadog = (struct brubeck_datadog *)backend;

  /* filter the key */
  if (datadog->regex_good != 1)
    return -1;

  if (datadog->out != NULL)
    return 0;

  datadog->out = udp_s_alloc(datadog->address, datadog->port, UDP_CAST_UNI);
  if (datadog->out == NULL)
    {
      log_splunk_errno("backend=datadog event=failed_to_connect");
      return -1;
    }
  log_splunk("backend=datadog event=connected");

  return 0;
}

static void
datadog_disconnect(struct brubeck_datadog *datadog)
{
  log_splunk_errno("backend=datadog event=disconnected");

  if (datadog->out)
    udp_s_free (datadog->out);
  datadog->out = NULL;
}

static void
datadog_plaintext_each(const char *key, value_t value, void *backend)
{
  struct brubeck_datadog *datadog =
    (struct brubeck_datadog *)backend;
  char wbuf[1024];
  unsigned len = 0;
  char *rstr = NULL;

  /* filter the key */
  if (datadog->regex_good != 1)
    return;

  // simple
  // metric.name:value|type
  if (!datadog_is_connected(datadog))
    return;

  /*
    Datadog format w extensions, from datadog doc

    metric.name:value|type|@sample_rate|#tag1:value,tag2

    metric.name - 
         a string with no colons, bars, or @ characters. See the metric naming policy.

    value - 
         an integer or float.

    type - 
         c for counter, g for gauge, ms for timer, h for histogram, s for set.

    sample rate (optional) - 
         a float between 0 and 1, inclusive. Only works with counter,
         histogram, and timer metrics. Default is 1 (i.e. sample 100%
         of the time).

    tags (optional) - 
         a comma separated list of tags. Use colons for key/value
         tags, i.e. env:prod. The key device is reserved; Datadog
         drops a user-added tag like device:foobar.
  */

  if (regexec(&(datadog->regex_c), key, 0, NULL, 0) != 0)
    { return; }

  sprintf (wbuf,"%s:%.6f|g\n",key,value);
  len = strlen(wbuf);

  /* need a socket here */
  rstr = udp_s_send (datadog->out, wbuf, len);
  if (rstr != NULL)
    {
      /* log it */
      log_splunk("backend=datadog bad_send=%s", rstr);
      free (rstr);
      datadog_disconnect(datadog);
      return;
    }

  datadog->sent += len;
}

static void
_free_datadog (struct brubeck_datadog *datadog)
{
  if (datadog == NULL ) return;

  if (datadog->regex_good)
    regfree (&(datadog->regex_c));
  datadog->regex_good = 0;

  if (datadog->out)
    udp_s_free (datadog->out);
  datadog->out = NULL;

  free (datadog);
}

struct brubeck_backend *
brubeck_datadog_new(struct brubeck_server *server, json_t *settings, int shard_n)
{
  struct brubeck_datadog *datadog = xcalloc(1, sizeof(struct brubeck_datadog));
  static char *a_l = "127.0.0.1";

  datadog->port = 8125;
  datadog->frequency = 10;
  datadog->address = a_l;

  json_unpack_or_die(settings,
		     "{s?:s, s?:i, s?:i, s?:s, s?:i}",
		     "address", &(datadog->address),
		     "port", &(datadog->port),
		     "frequency", &(datadog->frequency),
		     "filter", &(datadog->regex_s),
		     "expire", &(datadog->backend.expire)
		     );

  //  log_splunk ("%s:%d@%d->%s\n", datadog->address, datadog->port, datadog->frequency, datadog->regex_s);

  /* convert the filters to compiled regex */
  if (datadog->regex_s[0] == '\0')
    {
      log_splunk("backend=datadog no regex");
      _free_datadog (datadog);
      return NULL;
    }

  if ((regcomp(&(datadog->regex_c), datadog->regex_s, REG_EXTENDED) != 0))
    {
      log_splunk("backend=datadog badregex=%s",
		 datadog->regex_s);
      _free_datadog (datadog);
      return NULL;
    }
  datadog->regex_good = 1;

  datadog->backend.type = BRUBECK_BACKEND_DATADOG;
  datadog->backend.shard_n = shard_n;
  datadog->backend.connect = &datadog_connect;
  datadog->backend.is_connected = &datadog_is_connected;

  datadog->backend.sample = &datadog_plaintext_each;
  datadog->backend.flush = NULL;

  datadog->backend.sample_freq = datadog->frequency;
  datadog->backend.server = server;

  /* it will connect things and disconnect things */
  brubeck_backend_run_threaded((struct brubeck_backend *)datadog);
  log_splunk("backend=datadog event=started");

  return (struct brubeck_backend *)datadog;
}
