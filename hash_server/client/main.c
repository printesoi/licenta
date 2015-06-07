#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "base64.h"
#include "utils.h"
#include "parser.h"
#include "client.h"

int main(int argc, char *argv[])
{
	char *fname;
	command_t *cmd;
	client_state_t state;
	struct timeval tv;

	if (argc < 2) {
		exit(EXIT_FAILURE);
	}

	if (gettimeofday(&tv, NULL) < 0)
		ERR_SYS(-1, "gettimeofday");

	srandom(tv.tv_sec ^ tv.tv_usec ^ getpid());

	fname = argv[1];
	cmd = parse_file(fname);
	if (cmd) {
		init_client_state(&state);
		execute_command(&state, cmd);
		cleanup_client_state(&state);
	}

	free_command(cmd);
	return 0;
}
