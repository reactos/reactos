
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/wine)
remove_definitions(-DWINVER=0x502 -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x502 -D_CRT_NON_CONFORMING_SWPRINTFS)
add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00 -DNTDDI_VERSION=0x0A000007)

# Comment this block out if you want to debug the heap issue with these tests on ROS x64. ROSTESTS-415
if(ARCH STREQUAL "amd64")
    add_definitions(-DSKIPBADHEAP_K32_WINETEST=1)
endif()

spec2def(dummy.dll dummy.spec)
add_library(kernel32_winetest_dummy MODULE dummy.c ${CMAKE_CURRENT_BINARY_DIR}/dummy.def)
set_target_properties(kernel32_winetest_dummy PROPERTIES OUTPUT_NAME "dummy")
set_module_type(kernel32_winetest_dummy win32dll)
add_importlibs(kernel32_winetest_dummy msvcrt kernel32)
add_rostests_file(TARGET kernel32_winetest_dummy)

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
    power.c
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

if (MSVC AND ARCH STREQUAL "amd64")
    list(APPEND SOURCE
    get_fpu_cw.asm)
endif()

add_executable(kernel32_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    resource.rc)

# CMake 3.9 and higher requires to specify this dependency manually
# see https://gitlab.kitware.com/cmake/cmake/issues/19933
set_property(SOURCE resource.rc PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dummy.dll ${CMAKE_CURRENT_BINARY_DIR}/dummy.dll)

# resource.rc: let rc.exe find the dlls in their subdirectory, i.e. Debug.
if (MSVC_IDE)
    target_include_directories(kernel32_winetest PRIVATE
        $<$<COMPILE_LANGUAGE:RC>:$<TARGET_FILE_DIR:kernel32_winetest_dummy>>)
endif()

if (MSVC)
    # error C4101: unreferenced local variable
    target_compile_options(kernel32_winetest PRIVATE /wd4101)

    # Approximates a __FILE__ macro with forward slashes instead of back slashes
    # and defines it as __FILE_FW_SLASH__.
    foreach(source_file ${SOURCE})
        get_filename_component(abs_source_file "${source_file}" ABSOLUTE)
        file(RELATIVE_PATH rel_path "${CMAKE_SOURCE_DIR}" "${abs_source_file}")
        string(REPLACE "\\" "/" rel_path_forward "${rel_path}")
        set_source_files_properties(${source_file} PROPERTIES
            COMPILE_DEFINITIONS "__FILE_FW_SLASH__=\"../${rel_path_forward}\""
        )
    endforeach()
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(kernel32_winetest PRIVATE
        -Wno-format
        -Wno-int-conversion
        -Wno-old-style-declaration
        -Wno-unused)
endif()

set_module_type(kernel32_winetest win32cui)
target_link_libraries(kernel32_winetest pseh oldnames)
add_importlibs(kernel32_winetest user32 advapi32 msvcrt kernel32 ntdll)
add_pch(kernel32_winetest precomp.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET kernel32_winetest)
target_compile_definitions(kernel32_winetest PRIVATE 
    WINETEST_USE_DBGSTR_LONGLONG
    __WINESRC__
    strncasecmp=_strnicmp
    strcasecmp=_stricmp)
