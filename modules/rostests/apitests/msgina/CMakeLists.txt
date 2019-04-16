
add_definitions(
    -D_ATL_NO_EXCEPTIONS)

set_cpp(WITH_RUNTIME)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)

add_executable(msgina_apitest
    ShellDimScreen.cpp
    testlist.c)

target_link_libraries(msgina_apitest wine uuid)
set_module_type(msgina_apitest win32cui)
add_importlibs(msgina_apitest msvcrt user32 kernel32)
add_rostests_file(TARGET msgina_apitest)
