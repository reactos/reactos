
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(idndl ${CMAKE_CURRENT_SOURCE_DIR}/idndl.spec ${CMAKE_CURRENT_BINARY_DIR}/idndl.def)

add_library(idndl SHARED idndl.cpp)

set_entrypoint(idndl 0)

target_link_libraries(idndl ${CMAKE_CURRENT_BINARY_DIR}/idndl.def)

if(NOT MSVC)
    set_target_properties(idndl PROPERTIES COMPILE_FLAGS "-fno-exceptions -fno-rtti")
endif(NOT MSVC)

add_importlibs(idndl kernel32)

add_dependencies(idndl idndl_def psdk buildno_header)

add_livecd_target(idndl reactos)
