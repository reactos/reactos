
#pragma once

#include <io.h>

#define HAVE_PROCESS_H 1

int
_getopt_internal(
    int argc,
    char *const *argv,
    const char *optstring,
    const struct option *longopts,
    int *longind,
    int long_only);
