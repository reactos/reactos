
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
remove_definitions(-DWINVER=0x502 -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x502)
add_definitions(-DWINVER=0x600 -D_WIN32_WINNT=0x601)

add_definitions(
    -Dstrcasecmp=_stricmp
)

list(APPEND SOURCE
    actctx.c
    atom.c
    change.c
    codepage.c
    comm.c
    console.c
    debugger.c
    directory.c
    drive.c
    environ.c
    fiber.c
    file.c
    format_msg.c
    generated.c
    heap.c
    loader.c
    locale.c
    mailslot.c
    module.c
    path.c
    pipe.c
    process.c
    profile.c
    resource.c
    sync.c
    thread.c
    time.c
    timer.c
    toolhelp.c
    version.c
    virtual.c
    volume.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(kernel32_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    resource.rc)

if(MSVC)
    # Disable warning C4477 (printf format warnings)
    remove_target_compile_option(kernel32_winetest "/we4477")

    if(ARCH STREQUAL "amd64")
        # error C4101: 'is_wow64': unreferenced local variable
        remove_target_compile_option(kernel32_winetest "/we4101")
        target_compile_options(kernel32_winetest PRIVATE /wd4101)
    endif()
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(kernel32_winetest PRIVATE "-Wno-format")
endif()

set_module_type(kernel32_winetest win32cui)
target_link_libraries(kernel32_winetest pseh)
add_importlibs(kernel32_winetest user32 advapi32 msvcrt kernel32 ntdll)
add_pch(kernel32_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET kernel32_winetest)
