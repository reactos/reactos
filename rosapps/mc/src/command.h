#ifndef __COMMAND_H
#define __COMMAND_H

typedef struct {
    WInput input;
    callback_fn old_callback;
} WCommand;

WCommand *command_new (int y, int x, int len);

/* Return the Input * from a WCommand */
#define input_w(x) (&(x->input))

extern WCommand *cmdline;

void do_cd_command (char *cmd);

#endif /* __COMMAND_H */
