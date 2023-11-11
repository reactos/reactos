
list(APPEND SOURCE
    chkreg.c
    hivinit.c
    hivchk.c
    hivio.c
    hivmem.c
    rtl.c)

add_host_tool(chkreg ${SOURCE})
target_include_directories(chkreg PRIVATE ${REACTOS_SOURCE_DIR}/sdk/lib/rtl)
target_compile_definitions(chkreg PRIVATE CHKREG_HOST)
if(NOT MSVC)
    target_compile_options(chkreg PRIVATE "-fshort-wchar")
endif()

target_link_libraries(chkreg PRIVATE host_includes unicode cmlibhost)
