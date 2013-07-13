
include_directories(usersrv)

list(APPEND USERSRV_SOURCE
    usersrv/harderror.c
    usersrv/init.c
    usersrv/register.c
    usersrv/shutdown.c
    # usersrv/usersrv.rc
    )

add_library(usersrv ${USERSRV_SOURCE})
#add_object_library(usersrv ${USERSRV_SOURCE})

set_module_type(usersrv module UNICODE)
