
include_directories(../inc)

add_definitions(
    -DCLASS_GLOBAL_BREAK_ON_LOST_IRPS=0
    -DCLASS_GLOBAL_SECONDS_TO_WAIT_FOR_SYNCHRONOUS_SRB=100
    -DCLASS_GLOBAL_USE_DELAYED_RETRY=1
    -DCLASS_GLOBAL_BUFFERED_DEBUG_PRINT=0
    -DCLASS_GLOBAL_BUFFERED_DEBUG_PRINT_BUFFER_SIZE=512
    -DCLASS_GLOBAL_BUFFERED_DEBUG_PRINT_BUFFERS=512
)

if(NOT MSVC)
    add_definitions(-mrtd -fno-builtin -w)
endif()

list(APPEND SOURCE
    autorun.c
    class.c
    classwmi.c
    clntirp.c
    create.c
    data.c
    debug.c
    dictlib.c
    lock.c
    obsolete.c
    power.c
    retry.c
    utils.c
    xferpkt.c
    class.rc
    ${CMAKE_CURRENT_BINARY_DIR}/class.def
)

spec2def(classpnp.sys class.spec)

add_library(classpnp SHARED ${SOURCE})
target_link_libraries(classpnp ${PSEH_LIB} libcntpr)

set_module_type(classpnp kernelmodedriver)

add_importlibs(classpnp hal ntoskrnl)

add_minicd_target(classpnp reactos classpnp.sys)
add_livecd_target(classpnp reactos/system32/drivers)