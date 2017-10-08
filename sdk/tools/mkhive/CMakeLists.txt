
add_definitions(-DMKHIVE_HOST)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/inflib
    ${REACTOS_SOURCE_DIR}/sdk/lib/cmlib
    ${REACTOS_SOURCE_DIR}/sdk/lib/rtl)

list(APPEND SOURCE
    binhive.c
    cmi.c
    mkhive.c
    reginf.c
    registry.c
    rtl.c)

add_host_tool(mkhive ${SOURCE})

if(NOT MSVC)
    add_target_compile_flags(mkhive "-fshort-wchar")
endif()

target_link_libraries(mkhive unicode cmlibhost inflibhost)
