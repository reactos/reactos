
list(APPEND SOURCE dummy.c)

if(ARCH MATCHES powerpc)
    list(APPEND SOURCE mmuutil.c)
endif()

add_library(ppcmmu ${SOURCE})
