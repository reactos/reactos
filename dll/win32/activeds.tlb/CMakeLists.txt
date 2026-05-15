
add_library(activeds.tlb MODULE)
set_module_type(activeds.tlb module)
set_target_properties(activeds.tlb PROPERTIES LINKER_LANGUAGE "C" SUFFIX "")

add_typelib(activeds_tlb.idl)
add_idl_reg_scripts(activeds.tlb regtypelib activeds_tlb.idl)

add_cd_file(TARGET activeds.tlb DESTINATION reactos/system32 FOR all)
set_wine_module(activeds.tlb)
