#ifndef __BRUBECK_RWI_CARBON_H__
#define __BRUBECK_RWI_CARBON_H__

struct brubeck_rwi_carbon {
	struct brubeck_backend backend;

	int out_sock;
	struct sockaddr_in out_sockaddr;
	size_t sent;
};

struct brubeck_backend *brubeck_rwi_carbon_new(
	struct brubeck_server *server, json_t *settings);

#endif
