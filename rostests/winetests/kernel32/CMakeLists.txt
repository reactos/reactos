
add_definitions(
    -D__ROS_LONG64__)

remove_definitions(-DWINVER=0x502 -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x502)

list(APPEND SOURCE
    actctx.c
    alloc.c
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
    #generated.c
    heap.c
    interlck.c
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
    volume.c
    dosdev.c
    testlist.c)

add_executable(kernel32_winetest
    ${SOURCE}
    resource.rc)

target_link_libraries(kernel32_winetest wine)
set_module_type(kernel32_winetest win32cui)
add_importlibs(kernel32_winetest user32 advapi32 msvcrt kernel32 ntdll)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #add_target_compile_flags(kernel32_winetest "-Wno-format")
    #allow_warnings(kernel32_winetest)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-format -Wno-error")
endif()

add_cd_file(TARGET kernel32_winetest DESTINATION reactos/bin FOR all)
