#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "utils.h"
#include "hash.h"
#include "os.h"

static unsigned short s_port = 9000;
static struct poll_struct *listen_s;

void server_cleanup(void)
{
	h_destroy_hash();
	remove_fd(listen_s);

	/* Print the stats */
	print_poll_stats();
}

int main(int argc, char *argv[])
{
	int clientsock;

	if (argc > 1) {
		unsigned short port;
		if (sscanf(argv[1], "%hu", &port))
			s_port = port;
	}

	/*setup_environment();*/
	h_init_hash();

	if (atexit(server_cleanup))
		ERR_SYS(-1, "atexit");

	init_signals();

	clientsock = listen_tcp(s_port, INADDR_ANY);
	listen_s = monitor_fd(clientsock, TYPE_CLIENT_LISTEN, EV_IN);

	/* Main loop */
	server_loop();

	return 0;
}
