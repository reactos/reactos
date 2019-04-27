
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

# Wrapper functions for the important properties, using add_target_property
# where appropriate.
# Note that the functions for string properties take a single string
# argument while those for list properties can take a variable number of
# arguments, all of which will be added to the list
#
# Examples:
#  add_target_compile_flags(mymodule "-pedantic -O5")
#  add_target_link_flags(mymodule "-s --fatal-warnings")
#  add_target_compile_definitions(mymodule WIN32 _WIN32 INLINE=inline)
#  add_target_include_directories(mymodule include ../include)
function(add_target_compile_flags _module _flags)
    if(${ARGC} GREATER 2)
        message(FATAL_ERROR "Excess arguments to add_target_compile_flags! Module ${_module}, args ${ARGN}")
    endif()
    add_target_property(${_module} COMPILE_FLAGS ${_flags})
endfunction()

function(add_target_link_flags _module _flags)
    if(${ARGC} GREATER 2)
        message(FATAL_ERROR "Excess arguments to add_target_link_flags! Module ${_module}, args ${ARGN}")
    endif()
    add_target_property(${_module} LINK_FLAGS ${_flags})
endfunction()

function(add_target_compile_definitions _module)
    add_target_property(${_module} COMPILE_DEFINITIONS ${ARGN})
endfunction()

function(add_target_include_directories _module)
    add_target_property(${_module} INCLUDE_DIRECTORIES ${ARGN})
endfunction()

# replace_compiler_option
#  (taken from LLVM)
#  Replaces a compiler option or switch `_old' in `_var' by `_new'.
#  If `_old' is not in `_var', appends `_new' to `_var'.
#
# Example:
#  replace_compiler_option(CMAKE_CXX_FLAGS_RELEASE "-O3" "-O2")
macro(replace_compiler_option _var _old _new)
    # If the option already is on the variable, don't add it:
    if("${${_var}}" MATCHES "(^| )${_new}($| )")
        set(__n "")
    else()
        set(__n "${_new}")
    endif()
    if("${${_var}}" MATCHES "(^| )${_old}($| )")
        string(REGEX REPLACE "(^| )${_old}($| )" " ${__n} " ${_var} "${${_var}}")
    else()
        set(${_var} "${${_var}} ${__n}")
    endif()
endmacro(replace_compiler_option)

# add_compile_flags
# add_compile_flags_language
# replace_compile_flags
# replace_compile_flags_language
#  Add or replace compiler flags in the global scope for either all source
#  files or only those of the specified language.
#
# Examples:
#  add_compile_flags("-pedantic -O5")
#  add_compile_flags_language("-std=gnu99" "C")
#  replace_compile_flags("-O5" "-O3")
#  replace_compile_flags_language("-fno-exceptions" "-fexceptions" "CXX")
function(add_compile_flags _flags)
    if(${ARGC} GREATER 1)
        message(FATAL_ERROR "Excess arguments to add_compile_flags! Args ${ARGN}")
    endif()
    # Adds the compiler flag for all code files: C, C++, and assembly
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_flags}" PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_flags}" PARENT_SCOPE)
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${_flags}" PARENT_SCOPE)
endfunction()

function(add_compile_flags_language _flags _lang)
    if(NOT ${ARGC} EQUAL 2)
        message(FATAL_ERROR "Wrong arguments to add_compile_flags_language! Args ${ARGN}")
    endif()
    # Adds the compiler flag for the specified language only, e.g. CMAKE_C_FLAGS
    set(CMAKE_${_lang}_FLAGS "${CMAKE_${_lang}_FLAGS} ${_flags}" PARENT_SCOPE)
endfunction()

macro(replace_compile_flags _oldflags _newflags)
    if(NOT ${ARGC} EQUAL 2)
        message(FATAL_ERROR "Wrong arguments to replace_compile_flags! Args ${ARGN}")
    endif()
    replace_compiler_option(CMAKE_C_FLAGS ${_oldflags} ${_newflags})
    replace_compiler_option(CMAKE_CXX_FLAGS ${_oldflags} ${_newflags})
    replace_compiler_option(CMAKE_ASM_FLAGS ${_oldflags} ${_newflags})
endmacro()

macro(replace_compile_flags_language _oldflags _newflags _lang)
    if(NOT ${ARGC} EQUAL 3)
        message(FATAL_ERROR "Wrong arguments to replace_compile_flags_language! Args ${ARGN}")
    endif()
    replace_compiler_option(CMAKE_${_lang}_FLAGS ${_oldflags} ${_newflags})
endmacro()
