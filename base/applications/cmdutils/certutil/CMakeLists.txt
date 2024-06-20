
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

list(APPEND SOURCE
    asn.cpp
    certutil.cpp
    hashfile.cpp
    precomp.h)

add_executable(certutil ${SOURCE})
set_module_type(certutil win32cui UNICODE)
target_link_libraries(certutil conutils ${PSEH_LIB})
add_importlibs(certutil crypt32 advapi32 msvcrt kernel32)
add_pch(certutil precomp.h SOURCE)
add_cd_file(TARGET certutil DESTINATION reactos/system32 FOR all)
