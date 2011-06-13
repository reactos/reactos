
macro(add_compiler_flags)
    # Adds the compiler flag to both CMAKE_C_FLAGS and CMAKE_CXX_FLAGS
    foreach(flag ${ARGN})
        set(flags_list "${flags_list} ${flag}")
    endforeach()
    set(CMAKE_C_FLAGS ${flags_list})
    set(CMAKE_CXX_FLAGS ${flags_list})
endmacro()

macro(add_linkerflag MODULE _flag)
    set(NEW_LINKER_FLAGS ${_flag})
    get_target_property(LINKER_FLAGS ${MODULE} LINK_FLAGS)
    if(LINKER_FLAGS)
        set(NEW_LINKER_FLAGS "${LINKER_FLAGS} ${NEW_LINKER_FLAGS}")
    endif()
    set_target_properties(${MODULE} PROPERTIES LINK_FLAGS ${NEW_LINKER_FLAGS})
endmacro()

macro(set_unicode)
   add_definitions(-DUNICODE -D_UNICODE)
   set(IS_UNICODE 1)
endmacro()
