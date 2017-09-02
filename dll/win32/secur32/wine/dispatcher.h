
#ifndef __WINE_DISPATCHER_H__
#define __WINE_DISPATCHER_H__

/* Functions from dispatcher.c used elsewhere in the code */
SECURITY_STATUS fork_helper(PNegoHelper *new_helper, const char *prog,
        char* const argv[]) DECLSPEC_HIDDEN;

SECURITY_STATUS run_helper(PNegoHelper helper, char *buffer,
        unsigned int max_buflen, int *buflen) DECLSPEC_HIDDEN;

void cleanup_helper(PNegoHelper helper) DECLSPEC_HIDDEN;

void check_version(PNegoHelper helper) DECLSPEC_HIDDEN;

#endif /* __WINE_DISPATCHER_H__ */
