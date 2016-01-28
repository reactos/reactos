
include_directories(usersrv)

list(APPEND USERSRV_SOURCE
    usersrv/harderror.c
    usersrv/init.c
    usersrv/register.c
    usersrv/shutdown.c
    # usersrv/usersrv.rc
    usersrv/usersrv.h)

add_library(usersrv ${USERSRV_SOURCE})
add_dependencies(usersrv xdk)
add_pch(usersrv usersrv/usersrv.h USERSRV_SOURCE)
#add_object_library(usersrv ${USERSRV_SOURCE})
list(APPEND USERSRV_IMPORT_LIBS basesrv)
set_module_type(usersrv module UNICODE)
