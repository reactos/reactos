
# add_target_property
#  Adds one or more values to the specified property of the specified target.
#  Note that there are properties which require (semicolon-separated) lists,
#  while others require space-separated strings. The function has a list of
#  properties of the former variety and handles the values accordingly
function(add_target_property _module _propname)
    list(APPEND _list_properties COMPILE_DEFINITIONS INCLUDE_DIRECTORIES)
    set(_newvalue "")
    get_target_property(_oldvalue ${_module} ${_propname})
    if (_oldvalue)
        set(_newvalue ${_oldvalue})
    endif()
    list(FIND _list_properties ${_propname} _list_index)
    if (NOT _list_index EQUAL -1)
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

#
# For backwards compatibility. To be removed soon.
#
function(add_compiler_flags)
    set(flags_list "")
    # Adds the compiler flag to both CMAKE_C_FLAGS and CMAKE_CXX_FLAGS
    foreach(flag ${ARGN})
        set(flags_list "${flags_list} ${flag}")
    endforeach()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flags_list}" PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flags_list}" PARENT_SCOPE)
endfunction()

function(add_linkerflag MODULE _flag)
    if (${ARGC} GREATER 2)
        message(STATUS "Excess arguments to add_linkerflag! Module ${MODULE}, args ${ARGN}")
    endif()
    set(NEW_LINKER_FLAGS ${_flag})
    get_target_property(LINKER_FLAGS ${MODULE} LINK_FLAGS)
    if(LINKER_FLAGS)
        set(NEW_LINKER_FLAGS "${LINKER_FLAGS} ${NEW_LINKER_FLAGS}")
    endif()
    set_target_properties(${MODULE} PROPERTIES LINK_FLAGS ${NEW_LINKER_FLAGS})
endfunction()

# New versions, using add_target_property where appropriate.
# Note that the functions for string properties take a single string
# argument while those for list properties can take a variable number of
# arguments, all of which will be added to the list
#
# Examples:
#  add_compile_flags("-pedantic -O5")
#  add_target_link_flags(mymodule "-s --fatal-warnings")
#  add_target_compile_flags(mymodule "-pedantic -O5")
#  add_target_compile_definitions(mymodule WIN32 _WIN32)
#  add_target_include_directories(mymodule include ../include)
function(add_compile_flags _flags)
    if (${ARGC} GREATER 1)
        message(STATUS "Excess arguments to add_compile_flags! Args ${ARGN}")
    endif()
    # Adds the compiler flag to both CMAKE_C_FLAGS and CMAKE_CXX_FLAGS
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_flags}" PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_flags}" PARENT_SCOPE)
endfunction()

function(add_target_compile_flags _module _flags)
    if (${ARGC} GREATER 2)
        message(STATUS "Excess arguments to add_target_compile_flags! Module ${_module}, args ${ARGN}")
    endif()
    add_target_property(${_module} COMPILE_FLAGS ${_flags})
endfunction()

function(add_target_link_flags _module _flags)
    if (${ARGC} GREATER 2)
        message(STATUS "Excess arguments to add_target_link_flags! Module ${_module}, args ${ARGN}")
    endif()
    add_target_property(${_module} LINK_FLAGS ${_flags})
endfunction()

function(add_target_compile_definitions _module)
    add_target_property(${_module} COMPILE_DEFINITIONS ${ARGN})
endfunction()

function(add_target_include_directories _module)
    add_target_property(${_module} INCLUDE_DIRECTORIES ${ARGN})
endfunction()

macro(set_unicode)
   add_definitions(-DUNICODE -D_UNICODE)
   set(IS_UNICODE 1)
endmacro()

function(add_compiler_flags_target __module)
    get_target_property(__flags ${__module} COMPILE_FLAGS)
    if(NOT __flags)
        set(__flags "")
    endif()
    foreach(flag ${ARGN})
        set(__flags "${__flags} ${flag}")
    endforeach()
    set_target_properties(${__module} PROPERTIES COMPILE_FLAGS ${__flags})
endfunction()
