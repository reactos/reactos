
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(idndl.dll idndl.spec)

add_library(idndl SHARED
    idndl.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/idndl.def)

set_entrypoint(idndl 0)

if(NOT MSVC)
    set_target_properties(idndl PROPERTIES COMPILE_FLAGS "-fno-exceptions -fno-rtti")
endif(NOT MSVC)

add_importlibs(idndl kernel32)

add_dependencies(idndl psdk)

add_livecd_target(idndl reactos)
