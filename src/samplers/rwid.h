#ifndef __BRUBECK_RWID_H__
#define __BRUBECK_RWID_H__

struct brubeck_rwid_msg {
  char *key;              /* The key of the message, NULL terminated */
  uint16_t key_len;       /* length of the key */
  uint16_t type;          /* type of the messaged, as a brubeck_mt_t */
  value_t value;          /* floating point value of the message */
  value_t sample_freq;    /* floating poit sample freq (1.0 / sample_rate) */
  uint8_t modifiers;      /* modifiers, as a brubeck_metric_mod_t */
  uint32_t timestamp;     /* seconds since epoch, our addition */
};

struct brubeck_rwid {
  struct brubeck_sampler sampler;
  pthread_t *workers;
  unsigned int worker_count;
  unsigned int mmsg_count;
};

void brubeck_rwid_packet_parse(struct brubeck_server *server, char *buffer, char *end);
int brubeck_rwid_msg_parse(struct brubeck_rwid_msg *msg, char *buffer, char *end);
struct brubeck_sampler *brubeck_rwid_new(struct brubeck_server *server, json_t *settings);

#endif
