
list(APPEND SOURCE
    binhive.c
    cmi.c
    mkhive.c
    reginf.c
    registry.c
    rtl.c)

add_host_tool(mkhive ${SOURCE})
target_include_directories(mkhive PRIVATE ${REACTOS_SOURCE_DIR}/sdk/lib/rtl)
target_compile_definitions(mkhive PRIVATE MKHIVE_HOST)
if(NOT MSVC)
    target_compile_options(mkhive PRIVATE "-fshort-wchar")
endif()

target_link_libraries(mkhive PRIVATE host_includes unicode cmlibhost inflibhost)
