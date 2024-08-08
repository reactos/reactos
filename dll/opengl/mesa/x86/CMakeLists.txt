
list(APPEND SOURCE
    common_x86.c
    x86_xform.c
    3dnow.c
    sse.c
    rtasm/x86sse.c)

list(APPEND ASM_SOURCE
    common_x86_asm.S
    x86_xform2.S
    x86_xform3.S
    x86_xform4.S
    x86_cliptest.S
    mmx_blend.S
    3dnow_xform1.S
    3dnow_xform2.S
    3dnow_xform3.S
    3dnow_xform4.S
    3dnow_normal.S
    sse_xform1.S
    sse_xform2.S
    sse_xform3.S
    sse_xform4.S
    sse_normal.S
    read_rgba_span_x86.S)

add_asm_files(mesa_x86_asm ${ASM_SOURCE})
add_library(mesa_x86 ${SOURCE} ${mesa_x86_asm})
add_dependencies(mesa_x86 xdk)
target_link_libraries(mesa_x86 ${PSEH_LIB})
if(NOT MSVC)
    target_link_options(mesa_x86 PRIVATE "-Wno-format")
endif()
