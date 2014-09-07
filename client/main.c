#include <stdio.h>
#include <stdlib.h>

#include "base64.h"
#include "utils.h"
#include "parser.h"
#include "client.h"

int main(int argc, char *argv[])
{
	char *fname;
	command_t *cmd;
	client_state_t state;

	if (argc < 2)
		exit(EXIT_FAILURE);

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
