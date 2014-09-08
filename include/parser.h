#ifndef PARSER_H_
#define PARSER_H_

enum command_type {
	CT_NONE = 0,
	CT_CONNECT,
	CT_DISCONNECT,
	CT_SEND,
	CT_SEND_B64,
	CT_SEND_RND,
	CT_RECV,
	CT_LOOP,
	CT_ENDLOOP,
	CT_SLEEP,
	CT_USLEEP,
	CT_USLEEP_RND,
};

/* Tree to store commands */
typedef struct command {
	enum command_type command_type;
	char *command;
	void *arg0;
	void *arg1;

	struct command *parent;
	struct command *next;
	struct command *child;
} command_t;

command_t *parse_line(char *line, unsigned line_number);
command_t *parse_file(const char *filename);

void free_command1(command_t *command);
void free_command(command_t *command);

/* For debugging purposed */
void print_command(command_t *cmd, int indent);

#endif /* PARSER_H_ */
