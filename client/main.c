#include <stdio.h>
#include <stdlib.h>

#include "base64.h"
#include "utils.h"
#include "parser.h"

void print_command(command_t *cmd, int indent)
{
	if (!cmd) {
		printf("%*cNULL command\n", indent, ' ');
		return;
	}

	switch (cmd->command_type) {
	case CT_CONNECT:
		printf("%*cCONNECT HOST %s, PORT %hu\n", indent, ' ', (char *)cmd->arg0, *(unsigned short *)cmd->arg1);
		break;

	case CT_DISCONNECT:
		printf("%*cDISCONNECT\n", indent, ' ');
		break;

	case CT_SEND:
	case CT_SEND_B64:
		printf("%*cSEND BUFFER %s\n", indent, ' ', (char *)cmd->arg0);
		break;

	case CT_SEND_RND:
		printf("%*cSEND RANDOM SIZE %lu\n", indent, ' ', *(unsigned long *)cmd->arg0);
		break;

	case CT_RECV:
		printf("%*cRECV SIZE %lu, TIMEOUT %lu\n", indent, ' ',
			*(unsigned long *)cmd->arg0, *(unsigned long *)cmd->arg1);
		break;

	case CT_SLEEP:
		printf("%*cSLEEP %lu s\n", indent, ' ', *(unsigned long *)cmd->arg0);
		break;

	case CT_USLEEP:
		printf("%*cUSLEEP %lu us\n", indent, ' ', *(unsigned long *)cmd->arg0);
		break;

	case CT_ENDLOOP:
		printf("%*cENDLOOP\n", indent, ' ');
		break;

	case CT_LOOP:
		printf("%*cLOOP %ld\n", indent, ' ', *(long *)cmd->arg0);
		if (cmd->child)
			print_command(cmd->child, indent + 4);
		break;

	default:
		break;
	}

	if (cmd->next)
		print_command(cmd->next, indent);
}

int main(int argc, char *argv[])
{
#if 0
	int i;
	char test[][30] = {
		"	  CONNECT clevertaxi.com",
		"SLEEP 1",
		"USLEEP 100",
		"LOOP 100",
		"ENDLOOP",
		"SEND_RND 1",
		"SEND lalalalala",
		"SEND_B64 bGFzZGFzZGFzZHNhZAo=",
	};
	for (i = 0; i < sizeof(test) / sizeof(test[0]); ++i) {
		command_t *cmd = parse_line(test[i], i);
		print_command(cmd, 0);
		if (cmd)
			free_command1(cmd);
	}
#endif
	char *fname = "../tests/inputs/test_0001.txt";
	command_t *cmd = parse_file(fname);
	print_command(cmd, 0);
	free_command(cmd);

	return 0;
}
