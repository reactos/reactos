include(${LWIP_DIR}/contrib/ports/CMakeCommon.cmake)

set (LWIP_INCLUDE_DIRS
    "${LWIP_DIR}/src/include"
    "${LWIP_DIR}/contrib/"
    "${LWIP_DIR}/contrib/ports/win32/include"
    "${LWIP_DIR}/contrib/examples/example_app"
)

include(${LWIP_DIR}/src/Filelists.cmake)
include(${LWIP_DIR}/contrib/Filelists.cmake)
include(${LWIP_DIR}/contrib/ports/win32/Filelists.cmake)

if(NOT EXISTS ${LWIP_DIR}/contrib/examples/example_app/lwipcfg.h)
  message(WARNING "${LWIP_DIR}/contrib/examples/example_app is missing lwipcfg.h
Copy ${LWIP_DIR}/contrib/examples/example_app/lwipcfg.h.example to ${LWIP_DIR}/contrib/examples/example_app/lwipcfg.h and edit appropriately")
endif()
add_executable(example_app ${LWIP_DIR}/contrib/examples/example_app/test.c default_netif.c)
target_include_directories(example_app PRIVATE ${LWIP_INCLUDE_DIRS})
target_compile_options(example_app PRIVATE ${LWIP_COMPILER_FLAGS})
target_compile_definitions(example_app PRIVATE ${LWIP_DEFINITIONS} ${LWIP_MBEDTLS_DEFINITIONS})
target_link_libraries(example_app ${LWIP_SANITIZER_LIBS} lwipallapps lwipcontribexamples lwipcontribapps lwipcontribaddons lwipcontribportwindows lwipcore lwipmbedtls)

add_executable(makefsdata ${lwipmakefsdata_SRCS})
target_compile_options(makefsdata PRIVATE ${LWIP_COMPILER_FLAGS})
target_include_directories(makefsdata PRIVATE ${LWIP_INCLUDE_DIRS})
target_link_libraries(makefsdata ${LWIP_SANITIZER_LIBS} lwipcore lwipcontribportwindows)
