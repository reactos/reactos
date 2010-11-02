
add_definitions(
    -D__WINESRC__
    -D_DLL -D__USE_CRTIMP
    -D_WINE)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mciwave.dll mciwave.spec)

add_library(mciwave SHARED mciwave.c mciwave.def)

if(NOT MSVC)
    set_source_files_properties(mciwave.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(mciwave 0)

target_link_libraries(mciwave wine)

add_importlibs(mciwave winmm user32 msvcrt kernel32 ntdll)
add_dependencies(mciwave psdk buildno_header)
add_cab_target(mciwave 1)
