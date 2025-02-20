
# Hack until we support globally defining _DEBUG
if(DBG)
    #add_compile_definitions(_DEBUG)
endif()

# Generate threaddll.dll
spec2def(threaddll.dll threaddll.spec)
add_library(threaddll SHARED
    threaddll.c
    threaddll.def
)
set_entrypoint(threaddll DllMain 12)
if(MSVC)
    target_link_libraries(threaddll runtmchk)
endif()
add_importlibs(threaddll ucrtbase kernel32)

# Hacks to make it work with MSVC IDE / MSBUILD
if(MSVC_IDE)
    if(DBG)
        set(THREADDLL_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/Debug)
    else()
        set(THREADDLL_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/Release)
    endif()
    message(STATUS "threaddll.dll output directory: ${THREADDLL_OUTPUT_DIR}")

    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /I${THREADDLL_OUTPUT_DIR}")
endif()

# Hack to replace the old CRT include directory with the UCRT include directory
get_property(INCLUDE_DIRS DIRECTORY . PROPERTY INCLUDE_DIRECTORIES)
list(REMOVE_ITEM INCLUDE_DIRS "${REACTOS_SOURCE_DIR}/sdk/include/crt")
set_property(DIRECTORY . PROPERTY INCLUDE_DIRECTORIES ${INCLUDE_DIRS})
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/ucrt)

add_definitions(
    -DUSE_WINE_TODOS
    -D_USE_MATH_DEFINES
    -DWINETEST_USE_DBGSTR_LONGLONG
    -D_finddata64_t=__finddata64_t
)

list(APPEND SOURCE
    cpp.c
    environ.c
    file.c
    misc.c
    printf.c
    scanf.c
    string.c
    thread.c
    testlist.c
    ucrtbase_winetest.rc
)

set_source_files_properties(ucrtbase_winetest.rc OBJECT_DEPENDS threaddll)

add_executable(ucrtbase_winetest ${SOURCE})
if(NOT MSVC)
    #set_target_properties(ucrtbase_winetest PROPERTIES LINK_FLAGS "-Wl,--major-os-version,6")
    target_compile_options(ucrtbase_winetest PRIVATE -Wno-format)
endif()
set_module_type(ucrtbase_winetest win32cui)

target_link_libraries(ucrtbase_winetest wine oldnames ${PSEH_LIB} chkstk)
if(NOT MSVC)
    # Add this manually here, so it can link to ucrtbase.dll and vcstartup
    target_link_libraries(ucrtbase_winetest -lgcc)
endif()

add_importlibs(ucrtbase_winetest ucrtbase kernel32 ntdll)
add_dependencies(ucrtbase_winetest threaddll)
add_rostests_file(TARGET ucrtbase_winetest)
