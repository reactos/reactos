
add_library(legide MODULE legide.c legide.rc)
set_module_type(legide kernelmodedriver)

if(MSVC)
    # Required for NT5.x pciidex.sys
    target_link_options(legide PRIVATE /SECTION:.rsrc,!D)
else()
    add_custom_command(
        TARGET legide POST_BUILD
        COMMAND native-pefixup --section:.rsrc,!D $<TARGET_FILE:legide>
        VERBATIM)
endif()

add_importlibs(legide pciidex ntoskrnl hal)
add_cd_file(TARGET legide DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(legide_reg.inf)
