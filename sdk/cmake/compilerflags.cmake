
# remove_target_compile_options
#  Remove one option from the target COMPILE_OPTIONS property,
#  previously added through add_compile_options
function(remove_target_compile_option _module _option)
    get_target_property(_options ${_module} COMPILE_OPTIONS)
    list(REMOVE_ITEM _options ${_option})
    set_target_properties(${_module} PROPERTIES COMPILE_OPTIONS "${_options}")
endfunction()
