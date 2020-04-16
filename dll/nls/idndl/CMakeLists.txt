
remove_definitions(-DWINVER=0x502)
add_definitions(-DWINVER=0x600)

spec2def(idndl.dll idndl.spec)

add_library(idndl MODULE
    idndl.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/idndl.def)

set_entrypoint(idndl 0)

if(NOT MSVC)
    target_compile_options(idndl PRIVATE -fno-exceptions -fno-rtti)
endif()

add_importlibs(idndl kernel32)
add_dependencies(idndl psdk)
add_cd_file(TARGET idndl DESTINATION reactos FOR all)
