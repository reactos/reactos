
#include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys)

add_library(ntlmlib
    avl.c
    calculations.c
    ciphers.c
    precomp.h
    util.c)
#add_dependencies(lsalib xdk)
add_pch(ntlmlib precomp.h SOURCE)

#add_library(ntlsalib lsa.c)
#target_compile_definitions(ntlsalib PRIVATE _NTSYSTEM_)
#add_dependencies(ntlsalib xdk)
