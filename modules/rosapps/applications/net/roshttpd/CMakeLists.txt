
include_directories(BEFORE include)

list(APPEND SOURCE
    config.cpp
    error.cpp
    http.cpp
    httpd.cpp
    roshttpd.cpp
    common/list.cpp
    common/socket.cpp
    common/thread.cpp)

add_executable(roshttpd ${SOURCE} common/roshttpd.rc)
target_link_libraries(roshttpd cppstl)
set_target_cpp_properties(roshttpd WITH_EXCEPTIONS)
target_compile_definitions(roshttpd PRIVATE __USE_W32_SOCKETS)
set_module_type(roshttpd win32cui)
add_importlibs(roshttpd user32 ws2_32 msvcrt kernel32)
add_cd_file(TARGET roshttpd DESTINATION reactos/system32 FOR all)
