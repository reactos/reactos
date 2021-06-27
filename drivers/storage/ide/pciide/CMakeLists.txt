
add_library(pciide MODULE pciide.c pciide.rc)
set_module_type(pciide kernelmodedriver)
if(MSVC)
    target_link_options(pciide PRIVATE /SECTION:.rsrc,!D)
else()
    add_custom_command(
        TARGET pciide POST_BUILD
        COMMAND native-pefixup --section:.rsrc,!D $<TARGET_FILE:pciide>
        VERBATIM)
endif()
add_importlibs(pciide pciidex ntoskrnl)
add_cd_file(TARGET pciide DESTINATION reactos/system32/drivers NO_CAB FOR all)
