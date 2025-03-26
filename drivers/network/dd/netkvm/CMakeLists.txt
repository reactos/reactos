
include_directories(BEFORE Common
                    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/virtio)

add_definitions(
   -DNDIS_MINIPORT_DRIVER
   -DNDIS51_MINIPORT=1)

list(APPEND SOURCE
    Common/ParaNdis-Common.c
    Common/ParaNdis-Oid.c
    Common/ParaNdis-VirtIO.c
    Common/ParaNdis-Debug.c
    Common/sw-offload.c
    wxp/ParaNdis5-Driver.c
    wxp/ParaNdis5-Impl.c
    wxp/ParaNdis5-Oid.c)

add_library(netkvm MODULE ${SOURCE} wxp/parandis.rc)
target_link_libraries(netkvm virtio)
set_module_type(netkvm kernelmodedriver)
add_importlibs(netkvm ndis ntoskrnl hal)
add_cd_file(TARGET netkvm DESTINATION reactos/system32/drivers FOR all)
add_driver_inf(netkvm netkvm.inf)

if(NOT MSVC)
    target_compile_options(netkvm PRIVATE
        -Wno-unused-function
        -Wno-pointer-sign
        -Wno-pointer-to-int-cast
        -Wno-int-to-pointer-cast
        -Wno-attributes)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(netkvm PRIVATE -Wno-old-style-declaration -Wno-unused-but-set-variable)
endif()
