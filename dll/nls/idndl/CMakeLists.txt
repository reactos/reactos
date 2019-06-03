
remove_definitions(-DWINVER=0x502)
add_definitions(-DWINVER=0x600)

add_library(idndl MODULE
    idndl.cpp)
spec2def(idndl idndl.dll idndl.spec)

set_entrypoint(idndl 0)

if(NOT MSVC)
    add_target_compile_flags(idndl "-fno-exceptions -fno-rtti")
endif()

add_importlibs(idndl kernel32)
add_dependencies(idndl psdk)
add_cd_file(TARGET idndl DESTINATION reactos FOR all)
