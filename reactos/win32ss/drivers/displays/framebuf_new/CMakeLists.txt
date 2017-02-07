
spec2def(framebuf_new.dll framebuf_new.spec)

list(APPEND SOURCE
    debug.c
    enable.c
    palette.c
    pointer.c
    screen.c
    ${CMAKE_CURRENT_BINARY_DIR}/framebuf_new.def)

add_library(framebuf_new SHARED
    ${SOURCE}
    framebuf_new.rc)

if(ARCH MATCHES i386)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    if(MSVC)
        #add_target_compile_flags(framebuf_new "/Gz")
        set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "/Gz")
    else()
        #add_target_compile_flags(framebuf_new "-mrtd -fno-builtin")
        set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-mrtd -fno-builtin")
    endif()
endif()

set_entrypoint(framebuf_new DrvEnableDriver 12)
set_subsystem(framebuf_new native)
set_image_base(framebuf_new 0x00010000)

target_link_libraries(framebuf_new libcntpr)

add_importlibs(framebuf_new win32k)
add_pch(framebuf_new driver.h)
add_dependencies(framebuf_new psdk bugcodes)
