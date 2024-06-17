
include_directories(BEFORE Common virtio)

add_definitions(
   -DNDIS_MINIPORT_DRIVER
   -DNDIS51_MINIPORT=1)

list(APPEND SOURCE
    Common/ParaNdis-Common.c
    Common/ParaNdis-Oid.c
    Common/ParaNdis-VirtIO.c
    Common/ParaNdis-Debug.c
    Common/sw-offload.c
    virtio/VirtIOPCICommon.c
    virtio/VirtIOPCILegacy.c
    virtio/VirtIOPCIModern.c
    virtio/VirtIORing.c
    virtio/VirtIORing-Packed.c
    wxp/ParaNdis5-Driver.c
    wxp/ParaNdis5-Impl.c
    wxp/ParaNdis5-Oid.c)

add_library(netkvm MODULE ${SOURCE} wxp/parandis.rc)
set_module_type(netkvm kernelmodedriver)
add_importlibs(netkvm ndis ntoskrnl hal)
add_cd_file(TARGET netkvm DESTINATION reactos/system32/drivers FOR all)
add_driver_inf(netkvm netkvm.inf)

if(NOT MSVC)
    add_compile_flags("-Wno-unused-function")
    add_compile_flags("-Wno-old-style-declaration")
    add_compile_flags("-Wno-unknown-pragmas")
    add_compile_flags("-Wno-unused-but-set-variable")
    add_compile_flags("-Wno-pointer-sign")
    add_compile_flags("-Wno-pointer-to-int-cast")
    add_compile_flags("-Wno-int-to-pointer-cast")
    add_compile_flags("-Wno-attributes")
endif()
