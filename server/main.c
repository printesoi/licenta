#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

#include "utils.h"
#include "hash.h"
#include "os.h"

unsigned short s_port = 9000;

int main(int argc, char *argv[])
{
	int clientsock;

	if (argc > 1) {
		unsigned short port;
		if (sscanf(argv[1], "%hu", &port))
			s_port = port;
	}

	setup_environment();

	clientsock = listen_tcp(s_port, INADDR_ANY);
	monitor_fd(clientsock, TYPE_CLIENT_LISTEN, EV_IN);

	/* Main loop */
	server_loop();

	return 0;
}
