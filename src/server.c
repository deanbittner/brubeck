#ifdef MACBUILD
#include <sys/signal.h>
#else
#include <signal.h>
#endif
#include <poll.h>

#include "brubeck.h"

int gh_log_all_metrics = 0;
char *gh_log_all_regex = NULL;
int gh_active_backends = 0;

#if JANSSON_VERSION_HEX < 0x020500
#	error "libjansson-dev is too old (2.5+ required)"
#endif

struct brubeck_server *g_server = NULL; 
static void
update_flows(struct brubeck_server *server)
{
  int i;
  for (i = 0; i < server->active_samplers; ++i) {
    struct brubeck_sampler *sampler = server->samplers[i];
    sampler->current_flow = sampler->inflow;
    sampler->inflow = 0;
  }
}

#define UTF8_UPARROW "\xE2\x86\x91"
#define UTF8_DOWNARROW "\xE2\x86\x93"

static void
update_proctitle(struct brubeck_server *server)
{
  static const char *size_suffix[] = { "b", "kb", "mb", "gb", "tb", "pb", "eb" };
#define PUTS(...) pos += snprintf(buf + pos, sizeof(buf) - pos, __VA_ARGS__)
  char buf[2048];
  int i, j, pos = 0;

  PUTS("[%s] [ " UTF8_UPARROW, server->config_name);

  for (i = 0; i < server->active_backends; ++i)
    {
      struct brubeck_backend *backend = server->backends[i];
      if (backend->type == BRUBECK_BACKEND_CARBON)
        {
          struct brubeck_carbon *carbon = (struct brubeck_carbon *)backend;
          double sent = carbon->sent;

          for (j = 0; j < 7 && sent >= 1024.0; ++j)
            sent /= 1024.0;

          PUTS("%s #%d %.1f%s%s",
               (i > 0) ? "," : "",
               i + 1, sent, size_suffix[j],
               (carbon->out_sock >= 0) ? "" : " (dc)");
        }
      else if (backend->type == BRUBECK_BACKEND_RWI_CARBON)
        {
          struct brubeck_carbon *carbon = (struct brubeck_carbon *)backend;
          double sent = carbon->sent;

          for (j = 0; j < 7 && sent >= 1024.0; ++j)
            sent /= 1024.0;

          PUTS("%s #%d %.1f%s%s",
               (i > 0) ? "," : "",
               i + 1, sent, size_suffix[j],
               (carbon->out_sock >= 0) ? "" : " (dc)");
        }
    }

  PUTS(" ] [ " UTF8_DOWNARROW);

  for (i = 0; i < server->active_samplers; ++i)
    {
      struct brubeck_sampler *sampler = server->samplers[i];
      PUTS("%s :%d %d/s",
           (i > 0) ? "," : "",
           (int)ntohs(sampler->addr.sin_port),
           (int)sampler->current_flow);
    }

  PUTS(" ]");
  setproctitle("brubeck", buf);
}

static void
dump_metric(struct brubeck_metric *mt, void *out_file)
{
  static const char *METRIC_NAMES[] = {"g", "c", "C", "h", "ms", "internal"};
  fprintf((FILE *)out_file, "%s|%s\n", mt->key, METRIC_NAMES[mt->type]);
}

static void 
dump_all_metrics(int ig)
{
  FILE *dump = NULL;

  log_splunk("event=dump_metrics");

  if (g_server->dump_path)
    dump = fopen(g_server->dump_path, "w+");

  if (!dump) {
    log_splunk_errno("event=dump_failed");
    return;
  }

  brubeck_hashtable_foreach(g_server->metrics, &dump_metric, dump);
  fclose(dump);

  return;
}

static void load_backends(struct brubeck_server *server, json_t *backends)
{
  size_t idx;
  json_t *b;

  json_array_foreach(backends, idx, b)
    {
      const char *type = json_string_value(json_object_get(b, "type"));
      struct brubeck_backend *backend = NULL;

      if (type && !strcmp(type, "carbon"))
        {
          backend = brubeck_carbon_new(server, b);
          server->backends[server->active_backends++] = backend; 
        }
      else if (type && !strcmp(type, "rwi_carbon"))
        {
          backend = brubeck_rwi_carbon_new(server, b);
          server->backends[server->active_backends++] = backend; 
        }
      else if (type && !strcmp(type, "datadog"))
        {
          backend = brubeck_datadog_new(server, b);
          server->backends[server->active_backends++] = backend; 
        }
      else
        {
          log_splunk("backend=%s event=invalid_backend", type);
        }
    }
  gh_active_backends = server->active_backends;
}

static void load_samplers(struct brubeck_server *server, json_t *samplers)
{
  size_t idx;
  json_t *s;

  json_array_foreach(samplers, idx, s) {
    const char *type = json_string_value(json_object_get(s, "type"));

    if (type && !strcmp(type, "statsd")) {
      server->samplers[server->active_samplers++] = brubeck_statsd_new(server, s);
      /* } else if (type && !strcmp(type, "statsd-secure")) { */
      /* 	server->samplers[server->active_samplers++] = brubeck_statsd_secure_new(server, s); */
    }
    else if (type && !strcmp(type, "rwid")) {
      server->samplers[server->active_samplers++] = brubeck_rwid_new(server, s);
    } else {
      log_splunk("sampler=%s event=invalid_sampler", type);
    }
  }
}

static char *get_config_name(const char *full_path)
{
  const char *filename = strrchr(full_path, '/');
  char *config_name = strdup(filename ? filename + 1 : full_path);
  char *ext = strrchr(config_name, '.');

  if (ext)
    *ext = '\0';

  return config_name;
}

static void load_config(struct brubeck_server *server, const char *path)
{
  json_error_t error;

  /* required */
  int capacity;
  json_t *backends, *samplers;

  server->name = "brubeck";
  server->config_name = get_config_name(path);
  server->dump_path = NULL;
  server->config = json_load_file(path, 0, &error);
  if (!server->config) {
    die("failed to load config file, %s (%s:%d:%d)",
	error.text, error.source, error.line, error.column);
  }

  json_unpack_or_die(server->config,
		     "{s?:s, s:s, s:i, s:o, s:o, s?:i, s?:s}",
		     "server_name", &server->name,
		     "dumpfile", &server->dump_path,
		     "capacity", &capacity,
		     "backends", &backends,
		     "samplers", &samplers,
                     "log_all_metrics", &gh_log_all_metrics,
                     "log_all_regex", &gh_log_all_regex);

  gh_log_set_instance(server->name);

  server->metrics = brubeck_hashtable_new(1 << capacity);
  if (!server->metrics)
    die("failed to initialize hash table (size: %lu)", 1ul << capacity);

  load_backends(server, backends);
  load_samplers(server, samplers);
}

void brubeck_server_init(struct brubeck_server *server, const char *config)
{
  memset(server, 0x0, sizeof(struct brubeck_server));

  /* ignore SIGPIPE here so we don't crash when
   * backends get disconnected */
  signal(SIGPIPE, SIG_IGN);

  /* init the memory allocator */
  brubeck_slab_init(&server->slab);

  /* init the samplers and backends */
  load_config(server, config);

  /* Init the internal stats */
  brubeck_internal__init(server);
}

static void server_down(int ig)
{
  if (g_server)
    g_server->running = 0;

  return;
}

static void log_reopen (int ig)
{
  gh_log_reopen();

  return;
}

int brubeck_server_run(struct brubeck_server *server)
{
  int i = 0;
  int count = 0;

#ifdef MACBUILD
  int res = 0;
  res = timing_mach_init ();
  if (res != 0)
    {
      log_splunk("event=can't init timer");
      return 1;
    }
#endif

  server->running = 1;
  log_splunk("event=listening");

  signal (SIGHUP, log_reopen);
  signal (SIGUSR2, dump_all_metrics);
  g_server = server;
  signal (SIGINT, server_down);
  signal (SIGTERM, server_down);


  while (server->running)
    {
      usleep (1000000);
      count++;

      /* every 1 second */
      update_flows(server);
      update_proctitle(server);
    }

  for (i = 0; i < server->active_backends; ++i)
    pthread_cancel(server->backends[i]->thread);

  for (i = 0; i < server->active_samplers; ++i)
    {
      struct brubeck_sampler *sampler = server->samplers[i];
      if (sampler->shutdown)
	sampler->shutdown(sampler);
    }

  log_splunk("event=shutdown");
  return 0;
}

