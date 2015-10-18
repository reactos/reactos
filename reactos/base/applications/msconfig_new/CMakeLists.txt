
PROJECT(msconfig_new)

# Currently, C++ files are the exception in this project. Therefore we
# manually set up only what's needed for C++ support. If the project
# is converted to C++, then we'll need to use the general C++ set-up.
set_cpp(WITH_RUNTIME)
set(IS_CPP 0) # Disable C++ for PCH

include_directories(
    .
    comctl32ex
    ${REACTOS_SOURCE_DIR}/include/c++)

list(APPEND C_SOURCE
    comctl32ex/listviewfuncs.c
    # toolspage.c
    # srvpage.c
    # systempage.c
    # startuppage.c
    # freeldrpage.c
    generalpage.c
    msconfig.c
    stringutils.c
    utils.c
    precomp.h)

list(APPEND CPP_SOURCE
    toolspage.cpp
    xmldomparser.cpp)

add_rc_deps(msconfig.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/msconfig.ico)
add_executable(msconfig_new ${C_SOURCE} ${CPP_SOURCE} msconfig.rc)

# Retrieve the COMPILE_FLAGS property for the .cpp files only
get_property(cpp_file1_flag SOURCE toolspage.cpp PROPERTY COMPILE_FLAGS)
get_property(cpp_file2_flag SOURCE xmldomparser.cpp PROPERTY COMPILE_FLAGS)
# Define PCH usage (see 'add_pch' macro code for more information)
add_pch(msconfig_new precomp.h C_SOURCE)
# Remove PCH usage for the .cpp files only (set the original COMPILE_FLAGS property)
set_property(SOURCE toolspage.cpp PROPERTY COMPILE_FLAGS ${cpp_file1_flag})
set_property(SOURCE xmldomparser.cpp PROPERTY COMPILE_FLAGS ${cpp_file2_flag})

set_module_type(msconfig_new win32gui UNICODE)
target_link_libraries(msconfig_new comsupp)
add_importlibs(msconfig_new user32 advapi32 version comctl32 ole32 oleaut32 msxml3 shell32 shlwapi msvcrt kernel32)
add_cd_file(TARGET msconfig_new DESTINATION reactos/system32 FOR all)
