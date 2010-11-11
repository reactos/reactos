
add_library(genincdata SHARED genincdata.c)

set_entrypoint(genincdata DummyEntry)

if(MSVC)
set(OPT_MS "-ms")
else()
set(OPT_MS "")
endif()

if(ARCH MATCHES i386)
set(_filename ks386)
endif()
if(ARCH MATCHES amd64)
set(_filename ksamd64)
endif()

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_filename}.inc
    COMMAND native-geninc ${CMAKE_CURRENT_BINARY_DIR}/genincdata.dll  ${CMAKE_CURRENT_BINARY_DIR}/${_filename}.inc ${OPT_MS}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/genincdata.dll
)

add_custom_target(
    ${_filename}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_filename}.inc
)
