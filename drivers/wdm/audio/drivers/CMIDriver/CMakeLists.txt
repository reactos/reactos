
add_subdirectory(cmicontrol)
add_subdirectory(cpl)


# For (NT6+) WaveRT support.
# See WAVERT define in debug.hpp.
# remove_definitions(-D_WIN32_WINNT=0x502)
# add_definitions(-D_WIN32_WINNT=0x600)

list(APPEND SOURCE
    adapter.cpp
    common.cpp
    mintopo.cpp
    minwave.cpp)

list(APPEND PCH_SKIP_SOURCE
    guid.cpp)

add_library(cmipci MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    cmipci.rc)

target_link_libraries(cmipci stdunk libcntpr uuid)
set_module_type(cmipci wdmdriver UNICODE)
add_importlibs(cmipci portcls hal ntoskrnl)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(cmipci PRIVATE -Wno-write-strings -Wno-switch)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(cmipci PRIVATE -Wno-enum-constexpr-conversion)
endif()

add_pch(cmipci precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET cmipci DESTINATION reactos/system32/drivers FOR all)

if(ARCH STREQUAL "i386")
    add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/CM8738-x32.inf" DESTINATION reactos/inf FOR all)
else()
    add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/CM8738-x64.inf" DESTINATION reactos/inf FOR all)
endif()
