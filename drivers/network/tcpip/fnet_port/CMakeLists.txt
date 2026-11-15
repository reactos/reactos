
set(FNET_DIR ${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/fnet/fnet_stack)

# Only what TCP over a FNET_NETIF_TYPE_OTHER interface needs, see fnet_user_config.h
list(APPEND FNET_SOURCE
    ${FNET_DIR}/port/cpu/fnet_cpu.c
    ${FNET_DIR}/service/fnet_service.c
    ${FNET_DIR}/stack/fnet_checksum.c
    ${FNET_DIR}/stack/fnet_error.c
    ${FNET_DIR}/stack/fnet_ip.c
    ${FNET_DIR}/stack/fnet_ip4.c
    ${FNET_DIR}/stack/fnet_isr.c
    ${FNET_DIR}/stack/fnet_mempool.c
    ${FNET_DIR}/stack/fnet_netbuf.c
    ${FNET_DIR}/stack/fnet_netif.c
    ${FNET_DIR}/stack/fnet_prot.c
    ${FNET_DIR}/stack/fnet_socket.c
    ${FNET_DIR}/stack/fnet_stack.c
    ${FNET_DIR}/stack/fnet_stdlib.c
    ${FNET_DIR}/stack/fnet_tcp.c
    ${FNET_DIR}/stack/fnet_timer.c)

add_library(fnetcore EXCLUDE_FROM_ALL ${FNET_SOURCE})
target_include_directories(fnetcore
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${FNET_DIR})
add_dependencies(fnetcore xdk)
if(MSVC)
    # FNET declares structs inside casts (fnet_netbuf.c)
    target_compile_options(fnetcore PRIVATE /wd4115)
endif()

add_library(fnetport fnet_ros_port.c)
target_link_libraries(fnetport fnetcore)
add_dependencies(fnetport bugcodes xdk)
