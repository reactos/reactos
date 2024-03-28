
add_library(pciahci MODULE pciahci.c pciahci.rc)
set_module_type(pciahci kernelmodedriver)
if(MSVC)
    target_link_options(pciahci PRIVATE /SECTION:.rsrc,!D)
else()
    add_custom_command(
        TARGET pciahci POST_BUILD
        COMMAND native-pefixup --section:.rsrc,!D $<TARGET_FILE:pciahci>
        VERBATIM)
endif()
add_importlibs(pciahci pciidex ntoskrnl)
add_cd_file(TARGET pciahci DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(pciahci_reg.inf)
