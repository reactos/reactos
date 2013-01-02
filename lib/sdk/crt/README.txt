Big chunks of this CRT library are taken from Wine's msvcrt implementation,
you can find a list of synced files in README.WINE file.

Notes:
1. When syncing, omit MSVCRT_ prefix where possible, Wine has to keep this
because they are linking with *both* original crt, and ms crt implementation.
ReactOS has the only CRT, so no need to make distinct functions.
2. ReactOS compiles two versions of the CRT library, one for usermode
(called just "crt"), and one version for kernelmode usage (called "libcntpr").
In order to separate the code, you can use #ifdef _LIBCNT_ for libcntpr code.