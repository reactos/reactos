#include <freeldr.h>
#include "of.h"

typedef struct _ofw_method_call {
    const char *call_method;
    int nargs;
    int nrets;
    const char *method_name;
    int handle;
    int args_rets[8];
} ofw_method_call;

extern int (*ofw_call_addr)(void *argstruct);

int ofw_callmethod_ret(const char *method, int handle, int nargs, int *args, int ret)
{
    ofw_method_call callframe = { 0 };
    callframe.call_method = "call-method";
    callframe.nargs = nargs + 2;
    callframe.nrets = ret+1;
    callframe.method_name = method;
    callframe.handle = handle;
    memcpy(callframe.args_rets, args, sizeof(int)*nargs);
    ofw_call_addr(&callframe);
    return callframe.args_rets[nargs+ret];
}
