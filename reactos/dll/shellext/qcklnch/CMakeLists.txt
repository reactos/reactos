
project(SHELL)

set_cpp(WITH_RUNTIME)

if(NOT MSVC)
    # HACK: this should be enabled globally!
    add_compile_flags_language("-std=c++11" "CXX")
endif()

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/atl
    ${REACTOS_SOURCE_DIR})

spec2def(qcklnch.dll qcklnch.spec)

add_library(qcklnch SHARED    
    qcklnch.cpp    
    ${CMAKE_CURRENT_BINARY_DIR}/qcklnch.def)

set_module_type(qcklnch win32dll UNICODE)
target_link_libraries(qcklnch uuid wine atlnew)

add_importlibs(qcklnch
    advapi32
    winmm
    ole32
    oleaut32
    shlwapi
    shell32
    comctl32
    msvcrt
    gdi32
    user32
    kernel32
    ntdll)

add_cd_file(TARGET qcklnch DESTINATION reactos/system32 FOR all)
