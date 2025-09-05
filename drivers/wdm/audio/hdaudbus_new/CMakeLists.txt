
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

list(APPEND SOURCE
    buspdo.cpp
    fdo.cpp
    hdac_controller.cpp
    hdac_stream.cpp
    hdaudio.cpp
    sklhdaudbus.cpp)

add_library(hdaudbus MODULE ${SOURCE})
set_module_type(hdaudbus kernelmodedriver)
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(hdaudbus PRIVATE -Wno-unused-variable -Wno-unused-but-set-variable -Wno-sign-compare -Wno-unknown-pragmas -Wno-switch -Wno-write-strings)
endif()
target_link_libraries(hdaudbus ntoskrnl_vista libcntpr ${PSEH_LIB})
add_importlibs(hdaudbus ntoskrnl hal)
add_cd_file(TARGET hdaudbus DESTINATION reactos/system32/drivers FOR all)
add_driver_inf(hdaudbus sklhdaudbus.inf)