
add_definitions(
    -DUNICODE -D_UNICODE)
if(DBG)
    add_compile_definitions(NDEBUG)
endif()

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/sound
                    ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/resampler)

list(APPEND SOURCE
    capabilities.c
    devicelist.c
    deviceinstance.c
    functiontable.c
    mmewrap.c
    reentrancy.c
    utility.c
    kernel.c
    thread.c
    auxiliary/auxMessage.c
    midi/midMessage.c
    midi/modMessage.c
    mixer/mxdMessage.c
    wave/widMessage.c
    wave/wodMessage.c
    wave/format.c
    wave/header.c
    wave/resample.c
    wave/streaming.c
    precomp.h)

add_library(mmebuddy ${SOURCE})
target_link_libraries(mmebuddy resampler)
add_pch(mmebuddy precomp.h SOURCE)
add_dependencies(mmebuddy psdk)
