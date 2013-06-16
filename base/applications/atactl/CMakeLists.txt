set_cpp()
add_definitions(-DUSER_MODE)
include_directories(${REACTOS_SOURCE_DIR}/drivers/storage/ide/uniata)
add_executable(atactl atactl.cpp atactl.rc)
set_module_type(atactl win32cui)
add_importlibs(atactl advapi32 msvcrt kernel32 ntdll)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #allow_warnings(atactl)
    set_source_files_properties(atactl.cpp PROPERTIES COMPILE_FLAGS "-Wno-error")
    target_link_libraries(atactl gcc)
endif()

add_cd_file(TARGET atactl DESTINATION reactos/system32 FOR all)
