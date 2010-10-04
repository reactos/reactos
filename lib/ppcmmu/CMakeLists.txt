
list(APPEND SOURCE dummy.c)

if(ARCH MATCHES powerpc)
list(APPEND SOURCE
    mmuutil.c)
endif(ARCH MATCHES powerpc)

add_library(ppcmmu ${SOURCE})
