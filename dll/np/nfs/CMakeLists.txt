spec2def(nfs41_np.dll nfs41_np.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    dllmain.c
    nfs41_np.c
    nfs41_np.h
    options.c
    options.h
    precomp.h)

include_directories(
    ${REACTOS_SOURCE_DIR}/drivers/filesystems/nfs)

add_library(nfs41_np MODULE
    ${SOURCE}
    nfsnp.rc
    ${CMAKE_CURRENT_BINARY_DIR}/nfs41_np.def)

set_module_type(nfs41_np win32dll UNICODE)
target_link_libraries(nfs41_np ${PSEH_LIB})
add_importlibs(nfs41_np msvcrt kernel32)
add_pch(nfs41_np precomp.h SOURCE)
add_cd_file(TARGET nfs41_np DESTINATION reactos/system32 FOR all)
