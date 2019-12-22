
include_directories(
    include)
#    ${REACTOS_SOURCE_DIR}/sdk/include/wdf/kmdf/1.9)

add_definitions(-D__WDF_MAJOR_VERSION=1 -D__WDF_MINOR_VERSION=9 -D__WDF_BUILD_NUMBER=7600 -DWDF_DEVICE_NO_WDMSEC_H)

list(APPEND SOURCE
    wdf01000.cpp
    librarycommon/fxlibrarycommon.cpp
    object/globals.cpp
)

add_library(wdf01000 MODULE ${SOURCE})
set_module_type(wdf01000 kernelmodedriver)
add_importlibs(wdf01000 ntoskrnl hal wdfldr)
add_cd_file(TARGET wdf01000 DESTINATION reactos/system32/drivers FOR all)