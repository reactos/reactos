
add_definitions(
    -DWIN32
    -D_WINDOWS
    -D_MBCS
    -DJPEG_DLL
    -Dmain=mainptr)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/libjpeg
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib)

spec2def(libjpeg.dll libjpeg.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    jaricom.c
    jcapimin.c
    jcapistd.c
    jcarith.c
    jccoefct.c
    jccolor.c
    jcdctmgr.c
    jchuff.c
    jcinit.c
    jcmainct.c
    jcmarker.c
    jcmaster.c
    jcomapi.c
    jcparam.c
    jcprepct.c
    jcsample.c
    jctrans.c
    jdapimin.c
    jdapistd.c
    jdarith.c
    jdatadst.c
    jdatasrc.c
    jdcoefct.c
    jdcolor.c
    jddctmgr.c
    jdhuff.c
    jdinput.c
    jdmainct.c
    jdmarker.c
    jdmaster.c
    jdmerge.c
    jdpostct.c
    jdsample.c
    jdtrans.c
    jerror.c
    jfdctflt.c
    jfdctfst.c
    jfdctint.c
    jidctflt.c
    jidctfst.c
    jidctint.c
    jquant1.c
    jquant2.c
    jutils.c
    jmemmgr.c
    jmemnobs.c
    precomp.h)

add_library(libjpeg MODULE ${SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/libjpeg.def)
set_module_type(libjpeg win32dll)
add_importlibs(libjpeg msvcrt kernel32)
add_pch(libjpeg precomp.h SOURCE)
add_cd_file(TARGET libjpeg DESTINATION reactos/system32 FOR all)
