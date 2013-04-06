
add_subdirectory(cmicontrol)
add_subdirectory(cpl)

set_cpp()

# for WaveRT support
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

list(APPEND SOURCE
    adapter.cpp
    common.cpp
    mintopo.cpp
    minwave.cpp
)

add_library(cmipci SHARED
    ${SOURCE}
    cmipci.rc
)

target_link_libraries(cmipci
    stdunk
    libcntpr
)

set_module_type(cmipci wdmdriver UNICODE ENTRYPOINT 0)

add_importlibs(cmipci
    portcls
    hal
    ntoskrnl)

# FIXME: http://www.cmake.org/Bug/view.php?id=12998
if(MSVC)
    #add_target_compile_flags(portcls "/GR-")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "/GR-")
else()
    #add_target_compile_flags(portcls "-fno-exceptions -fno-rtti")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-fno-exceptions -fno-rtti -Wno-write-strings -Wno-switch -Wno-error")
endif()

add_cd_file(TARGET cmipci DESTINATION reactos/system32/drivers FOR all)
if(ARCH STREQUAL "i386")
    add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/CM8738-x32-WaveRT.inf" DESTINATION reactos/inf FOR all)
else()
    add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/CM8738-x64-WaveRT.inf" DESTINATION reactos/inf FOR all)
endif()
