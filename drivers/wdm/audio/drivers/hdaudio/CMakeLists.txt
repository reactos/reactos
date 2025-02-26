remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

list(APPEND SOURCE
    hdaudio.cpp
    common.cpp
    functiongroupnode.cpp
    miniport.cpp
    stream.cpp
    guid.cpp)

add_library(hdaudio MODULE
    ${SOURCE}
    hdaudio.rc)

set_module_type(hdaudio kernelmodedriver)
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(hdaudio PRIVATE -Wno-unused-variable -Wno-unused-but-set-variable -Wno-sign-compare -Wno-unknown-pragmas -Wno-switch -Wno-write-strings)
endif()
target_link_libraries(hdaudio ntoskrnl_vista libcntpr ${PSEH_LIB})
add_importlibs(hdaudio ntoskrnl hal portcls)
add_cd_file(TARGET hdaudio DESTINATION reactos/system32/drivers FOR all)
