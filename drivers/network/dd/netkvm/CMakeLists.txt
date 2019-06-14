
include_directories(BEFORE Common)

add_definitions(
   -DNDIS50_MINIPORT
   -DNDIS_MINIPORT_DRIVER
   -DNDIS_LEGACY_MINIPORT
   -DNDIS51_MINIPORT)

list(APPEND SOURCE
    Common/ParaNdis-Common.c
    Common/ParaNdis-Oid.c
    Common/ParaNdis-VirtIO.c
    wxp/ParaNdis5-Driver.c
    wxp/ParaNdis5-Impl.c
    wxp/ParaNdis5-Oid.c)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/VirtIO)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/VirtIO/Tools)
add_library(netkvm MODULE ${SOURCE} wxp/parandis.rc)
set_module_type(netkvm kernelmodedriver)
add_importlibs(netkvm ndis ntoskrnl hal)
add_cd_file(TARGET netkvm DESTINATION reactos/system32/drivers FOR all)
if(NOT MSVC)
  add_target_compile_flags(netkvm "-Wno-unused-but-set-variable")
  add_target_compile_flags(netkvm "-Wno-unused-function")
  add_target_compile_flags(netkvm "-Wno-unknown-pragmas")
  add_target_compile_flags(netkvm "-Wno-unused-variable")
endif()
