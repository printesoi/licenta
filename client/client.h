#ifndef CLIENT_H_
#define CLIENT_H_

typedef struct client_state {
	int sockfd;
	char *host;
	unsigned short port;
} client_state_t;

void init_client_state(client_state_t *state);
void connect_client(client_state_t *state, const char *host,
		    unsigned short port);
void disconnect_client(client_state_t *state);

void execute_command(client_state_t *state, command_t *cmd);

#endif /* CLIENT_H_ */
