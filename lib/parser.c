#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "parser.h"
#include "base64.h"

#define DELIMS " \t\n"

command_t *parse_line(char *line, unsigned line_number)
{
	command_t *cmd = NULL;
	void *cmd_arg0, *cmd_arg1;
	char *word;
	char *saveptr;

	word = strtok_r(line, DELIMS, &saveptr);
	if (!word)
		return NULL;

	/* Ignore comments */
	if (word[0] == '#')
		return NULL;

	cmd = calloc(1, sizeof(*cmd));
	if (!cmd)
		ERR(EXIT_FAILURE, "ASSERT: calloc");

	if (!strcasecmp(word, "CONNECT")) {
		cmd->command_type = CT_CONNECT;
		/* We may have two arguments: host + port */
		cmd_arg0 = strtok_r(NULL, DELIMS, &saveptr);
		cmd->arg0 = strdup(cmd_arg0 ? cmd_arg0 : "localhost");
		if (!cmd->arg0)
			ERR(EXIT_FAILURE, "ASSERT: strdup");

		cmd->arg1 = malloc(sizeof(unsigned short));
		if (!cmd->arg1)
			ERR(EXIT_FAILURE, "ASSERT: malloc");
		cmd_arg1 = strtok_r(NULL, DELIMS, &saveptr);
		if (!cmd_arg1 || !sscanf(cmd_arg1, "%hu",
					 (unsigned short *)cmd->arg1))
			*(unsigned short *)cmd->arg1 = 9000;
	} else if (!strcasecmp(word, "DISCONNECT")) {
		cmd->command_type = CT_DISCONNECT;
		/* No arguments */
	} else if (!strcasecmp(word, "SLEEP")) {
		cmd->command_type = CT_SLEEP;

		/* We need an argument */
		cmd_arg0 = strtok_r(NULL, DELIMS, &saveptr);
		if (!cmd_arg0)
			ERR(EXIT_FAILURE,
			    "PARSING ERROR line %u, SLEEP needs an argument",
			    line_number);

		cmd->arg0 = malloc(sizeof(unsigned long));
		if (!cmd->arg0)
			ERR(EXIT_FAILURE, "ASSERT: malloc");

		if (!sscanf(cmd_arg0, "%lu", (long unsigned *)cmd->arg0))
			ERR(EXIT_FAILURE,
			    "PARSING ERROR line %u, wrong argument type for SLEEP, need an unsigned long",
			    line_number);
	} else if (!strcasecmp(word, "USLEEP")) {
		cmd->command_type = CT_USLEEP;

		/* We need an argument */
		cmd_arg0 = strtok_r(NULL, DELIMS, &saveptr);
		if (!cmd_arg0)
			ERR(EXIT_FAILURE,
			    "PARSING ERROR line %u, SLEEP needs an argument",
			    line_number);

		cmd->arg0 = malloc(sizeof(unsigned long));
		if (!cmd->arg0)
			ERR(EXIT_FAILURE, "ASSERT: malloc");

		if (!sscanf(cmd_arg0, "%lu", (long unsigned *)cmd->arg0))
			ERR(EXIT_FAILURE,
			    "PARSING ERROR line %u, wrong argument type for SLEEP, needs an unsigned long",
			    line_number);
	} else if (!strcasecmp(word, "SEND")) {
		cmd->command_type = CT_SEND;

		/* To send a string with spaces, tabs, newlines or
		 * non-printable characters use SEND_B64 */
		cmd_arg0 = strtok_r(NULL, DELIMS, &saveptr);
		if (!cmd_arg0)
			ERR(EXIT_FAILURE,
			    "PARSING ERROR line %u, SEND needs an argument",
			    line_number);

		cmd->arg0 = strdup(cmd_arg0);
		if (!cmd->arg0)
			ERR(EXIT_FAILURE, "ASSERT: strdup");
	} else if (!strcasecmp(word, "SEND_B64")) {
		cmd->command_type = CT_SEND_B64;

		/* We need an argument */
		cmd_arg0 = strtok_r(NULL, DELIMS, &saveptr);
		if (!cmd_arg0)
			ERR(EXIT_FAILURE,
			    "PARSING ERROR line %u, SEND_B64 needs an argument",
			    line_number);

		cmd->arg0 = strdup(cmd_arg0);
	} else if (!strcasecmp(word, "SEND_RND")) {
		cmd->command_type = CT_SEND_RND;

		/* We need an argument */
		cmd_arg0 = strtok_r(NULL, DELIMS, &saveptr);
		if (!cmd_arg0)
			ERR(EXIT_FAILURE,
			    "PARSING ERROR line %u, SEND_RND needs an argument",
			    line_number);

		cmd->arg0 = malloc(sizeof(unsigned long));
		if (!cmd->arg0)
			ERR(EXIT_FAILURE, "ASSERT: malloc");

		if (!sscanf(cmd_arg0, "%lu", (long unsigned *)cmd->arg0))
			ERR(EXIT_FAILURE,
			    "PARSING ERROR line %u, wrong argument type for SEND_RND, needs an unsigned long",
			    line_number);
	} else if (!strcasecmp(word, "RECV")) {
		cmd->command_type = CT_RECV;

		cmd_arg0 = strtok_r(NULL, DELIMS, &saveptr);
		if (!cmd_arg0)
			ERR(EXIT_FAILURE,
			    "PARSING ERROR line %u, RECV needs an argument",
			    line_number);

		cmd->arg0 = malloc(sizeof(unsigned long));
		if (!cmd->arg0)
			ERR(EXIT_FAILURE, "ASSERT: malloc");
		if (!sscanf(cmd_arg0, "%lu", (long unsigned *)cmd->arg0))
			ERR(EXIT_FAILURE,
			    "PARSING ERROR line %u, wrong argument type for RECV, needs an unsigned long",
			    line_number);

		cmd->arg1 = malloc(sizeof(unsigned long));
		if (!cmd->arg1)
			ERR(EXIT_FAILURE, "ASSERT: malloc");
		cmd_arg1 = strtok_r(NULL, DELIMS, &saveptr);
		if (cmd_arg1) {
			if (!sscanf(cmd_arg1, "%ld", (unsigned long *)cmd->arg1))
				ERR(EXIT_FAILURE,
				    "PARSING ERROR line %u, wrong argument type for RECV, needs an unsigned long",
				    line_number);
		} else
			*(unsigned long *)cmd->arg1 = 0;
	} else if (!strcasecmp(word, "LOOP")) {
		cmd->command_type = CT_LOOP;

		/* We may have an argument, the count */
		cmd->arg0 = malloc(sizeof(long));
		if (!cmd->arg0)
			ERR(EXIT_FAILURE, "ASSERT: malloc");

		cmd_arg0 = strtok_r(NULL, DELIMS, &saveptr);
		if (cmd_arg0) {
			if (!sscanf(cmd_arg0, "%ld", (long *)cmd->arg0))
				ERR(EXIT_FAILURE,
				    "PARSING ERROR line %u, wrong argument type for LOOP, needs a long",
				    line_number);
		} else
			*(long *)cmd->arg0 = -1; /* loop indefinitely */
	} else if (!strcasecmp(word, "ENDLOOP")) {
		cmd->command_type = CT_ENDLOOP;
		/* No arguments */
	} else {
		WARN("Ignoring unrecognized command %s", word);
		free(cmd);
		return NULL;
	}

	cmd->command = strdup(word);
	return cmd;
}

command_t *parse_file(const char *filename)
{
	command_t *root = NULL, *current_root = NULL, *last_cmd = NULL;
	char line[BUFSIZ];
	int line_number = 0;
	FILE *file = fopen(filename, "r");

	if (!file)
		return NULL;

	while (fgets(line, sizeof(line), file)) {
		command_t *cmd;
		line_number++;
		cmd = parse_line(line, line_number);
		if (!cmd)
			continue; /* Empty line or comment */

		if (!root)
			root = cmd;

		if (cmd->command_type == CT_ENDLOOP) {
			/* Silently ignore if no LOOP given before */
			if (current_root) {
				last_cmd = current_root;
				current_root = current_root->parent;
			}
			free_command1(cmd);
			continue;
		} else {
			cmd->parent = current_root;

			if (current_root) {
				if (!current_root->child)
					current_root->child = cmd;
				cmd->parent = current_root;
			}

			if (cmd->command_type == CT_LOOP)
				current_root = cmd;
			if (last_cmd && last_cmd->child != cmd)
				last_cmd->next = cmd;
			last_cmd = cmd;
		}
	}

	fclose(file);
	return root;
}

void free_command1(command_t *cmd)
{
	if (!cmd)
		return;
	free(cmd->arg0);
	free(cmd->arg1);
	free(cmd->command);
	free(cmd);
}

void free_command(command_t *cmd)
{
	if (!cmd)
		return;
	if (cmd->child)
		free_command(cmd->child);
	if (cmd->next)
		free_command(cmd->next);
	free_command1(cmd);
}

void print_command(command_t *cmd, int indent)
{
	if (!cmd) {
		printf("%*cNULL command\n", indent, ' ');
		return;
	}

	switch (cmd->command_type) {
	case CT_CONNECT:
		printf("%*cCONNECT HOST %s, PORT %hu\n", indent, ' ',
		       (char *)cmd->arg0, *(unsigned short *)cmd->arg1);
		break;

	case CT_DISCONNECT:
		printf("%*cDISCONNECT\n", indent, ' ');
		break;

	case CT_SEND:
	case CT_SEND_B64:
		printf("%*cSEND BUFFER %s\n", indent, ' ', (char *)cmd->arg0);
		break;

	case CT_SEND_RND:
		printf("%*cSEND RANDOM SIZE %lu\n", indent, ' ',
		       *(unsigned long *)cmd->arg0);
		break;

	case CT_RECV:
		printf("%*cRECV SIZE %lu, TIMEOUT %lu\n", indent, ' ',
			*(unsigned long *)cmd->arg0,
			*(unsigned long *)cmd->arg1);
		break;

	case CT_SLEEP:
		printf("%*cSLEEP %lu s\n", indent, ' ',
		       *(unsigned long *)cmd->arg0);
		break;

	case CT_USLEEP:
		printf("%*cUSLEEP %lu us\n", indent, ' ',
		       *(unsigned long *)cmd->arg0);
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

