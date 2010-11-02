
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(comcat SHARED comcat_main.c version.rc comcat.def)

if(NOT MSVC)
    set_source_files_properties(comcat.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(comcat 0)

target_link_libraries(comcat
    uuid
    wine)

add_importlibs(comcat
    ole32
    user32
    advapi32
    ntdll)

add_dependencies(comcat psdk buildno_header)
add_cab_target(comcat 1)
