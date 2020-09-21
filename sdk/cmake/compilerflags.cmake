
# add_target_property
#  Adds one or more values to the specified property of the specified target.
#  Note that there are properties which require (semicolon-separated) lists,
#  while others require space-separated strings. The function has a list of
#  properties of the former variety and handles the values accordingly
function(add_target_property _module _propname)
    list(APPEND _list_properties COMPILE_DEFINITIONS INCLUDE_DIRECTORIES LINK_DEPENDS)
    set(_newvalue "")
    get_target_property(_oldvalue ${_module} ${_propname})
    if(_oldvalue)
        set(_newvalue ${_oldvalue})
    endif()
    list(FIND _list_properties ${_propname} _list_index)
    if(NOT _list_index EQUAL -1)
        # list property
        list(APPEND _newvalue ${ARGN})
    else()
        # string property
        foreach(_flag ${ARGN})
            set(_newvalue "${_newvalue} ${_flag}")
        endforeach()
    endif()
    set_property(TARGET ${_module} PROPERTY ${_propname} ${_newvalue})
endfunction()

# remove_target_compile_options
#  Remove one option from the target COMPILE_OPTIONS property,
#  previously added through add_compile_options
function(remove_target_compile_option _module _option)
    get_target_property(_options ${_module} COMPILE_OPTIONS)
    list(REMOVE_ITEM _options ${_option})
    set_target_properties(${_module} PROPERTIES COMPILE_OPTIONS "${_options}")
endfunction()

# Wrapper functions for the important properties, using add_target_property
# where appropriate.
# Note that the functions for string properties take a single string
# argument while those for list properties can take a variable number of
# arguments, all of which will be added to the list
#
# Examples:
#  add_target_link_flags(mymodule "-s --fatal-warnings")

function(add_target_link_flags _module _flags)
    if(${ARGC} GREATER 2)
        message(FATAL_ERROR "Excess arguments to add_target_link_flags! Module ${_module}, args ${ARGN}")
    endif()
    add_target_property(${_module} LINK_FLAGS ${_flags})
endfunction()
