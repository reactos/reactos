
add_library(lautus.msstyles SHARED lautus.rc)
set_module_type(lautus.msstyles module)
set_target_properties(lautus.msstyles PROPERTIES SUFFIX "")
add_cd_file(TARGET lautus.msstyles DESTINATION reactos/Resources/Themes/Lautus FOR all)
