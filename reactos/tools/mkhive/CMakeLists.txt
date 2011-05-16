
add_definitions(
    -DMKHIVE_HOST
    -DWINE_UNICODE_API= )

include_directories(
    ${REACTOS_SOURCE_DIR}/lib/newinflib
    ${REACTOS_SOURCE_DIR}/lib/cmlib
    ${REACTOS_SOURCE_DIR}/lib/rtl)

list(APPEND SOURCE
    binhive.c
    cmi.c
    mkhive.c
    reginf.c
    registry.c
    rtl.c)

add_executable(mkhive ${SOURCE})
target_link_libraries(mkhive unicode cmlibhost inflibhost)
