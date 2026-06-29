
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)

add_definitions(
    -D__WINESRC__
    -D_WINE
    -D_KERNELBASE_)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/wine)

add_subdirectory(wine)

spec2def(kernelbase_ros.dll kernelbase_ros.spec ADD_IMPORTLIB)

add_library(kernelbase_ros MODULE
    DllMain.c
    ${CMAKE_CURRENT_BINARY_DIR}/kernelbase_ros_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/kernelbase_ros.def)

set_module_type(kernelbase_ros win32dll ENTRYPOINT DllMain 12)
target_link_libraries(kernelbase_ros psapi_static kernelbase_static wine chkstk)
add_importlibs(kernelbase_ros msvcrt advapi32 advapi32_vista kernel32 kernel32_vista ntdll ntdll_vista)
add_dependencies(kernelbase_ros psdk errcodes)
add_cd_file(TARGET kernelbase_ros DESTINATION reactos/system32 FOR all)

# Fake import library, so consumers can link against it without needing to know about the actual implementation in kernelbase_ros.dll
add_library(libkernelbase INTERFACE)
target_link_libraries(libkernelbase INTERFACE libkernelbase_ros)
