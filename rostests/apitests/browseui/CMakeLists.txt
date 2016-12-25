
set_cpp(WITH_RTTI WITH_RUNTIME WITH_EXCEPTIONS)
if(NOT MSVC)
    # HACK: this should be enabled globally!
    add_compile_flags_language("-std=c++11" "CXX")
endif()

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/atl 
    includes)

list(APPEND SOURCE
    ACListISF.cpp
    SHExplorerParseCmdLine.c
    testlist.c)

add_executable(browseui_apitest ${SOURCE})
target_link_libraries(browseui_apitest uuid wine)
set_module_type(browseui_apitest win32cui)
add_importlibs(browseui_apitest shell32 ole32 shlwapi msvcrt kernel32 ntdll)
add_cd_file(TARGET browseui_apitest DESTINATION reactos/bin FOR all)
