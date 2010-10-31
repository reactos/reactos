
MACRO(_PCH_GET_COMPILE_FLAGS _target_name _out_compile_flags _header_filename)

    # Add the precompiled header to the build
    get_filename_component(FILE ${_header_filename} NAME)
    set(_gch_filename "${_target_name}_${FILE}.gch")
    list(APPEND ${_out_compile_flags} -c ${_header_filename} -o ${_gch_filename})

    # This gets us our includes
    get_directory_property(DIRINC INCLUDE_DIRECTORIES)
    foreach(item ${DIRINC})
        list(APPEND ${_out_compile_flags} -I${item})
    endforeach(item) 

    # This is a particular bit of undocumented/hacky magic I'm quite proud of
    get_directory_property(_compiler_flags DEFINITIONS)
    string(REPLACE "\ " "\t" _compiler_flags ${_compiler_flags})
    list(APPEND ${_out_compile_flags} ${_compiler_flags})

    # This gets any specific definitions that were added with set-target-property
    get_target_property(_target_defs ${_target_name} COMPILE_DEFINITIONS)
    if (_target_defs)
        foreach(item ${_target_defs})
            list(APPEND ${_out_compile_flags} -D${item})
        endforeach(item)
    endif()

ENDMACRO(_PCH_GET_COMPILE_FLAGS) 

MACRO(add_pch _target_name _header_filename _src_list)
    
    get_filename_component(FILE ${_header_filename} NAME)
    set(_gch_filename "${_target_name}_${FILE}.gch")
    list(APPEND ${_src_list} ${_gch_filename})
    _PCH_GET_COMPILE_FLAGS(${_target_name} _args ${_header_filename})
    file(REMOVE ${_gch_filename})
    add_custom_command(
        OUTPUT ${_gch_filename}
        COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_COMPILER_ARG1} ${_args}
        DEPENDS ${_header_filename})

ENDMACRO(add_pch _target_name _header_filename _src_list)

MACRO(spec2def _dllname _spec_file)
    get_filename_component(_file ${_spec_file} NAME_WE)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_file}.def
        COMMAND native-winebuild -o ${CMAKE_CURRENT_BINARY_DIR}/${_file}.def --def -E ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file} --filename ${_dllname}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_spec_file})
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${_file}.def
        PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)
ENDMACRO(spec2def _dllname _spec_file)

if (NOT MSVC)
MACRO(CreateBootSectorTarget _target_name _asm_file _object_file)

    get_filename_component(OBJECT_PATH ${_object_file} PATH)
    get_filename_component(OBJECT_NAME ${_object_file} NAME)
    file(MAKE_DIRECTORY ${OBJECT_PATH})
    get_directory_property(defines COMPILE_DEFINITIONS)
    get_directory_property(includes INCLUDE_DIRECTORIES)

    foreach(arg ${defines})
        set(result_defs ${result_defs} -D${arg})
    endforeach(arg ${defines})

    foreach(arg ${includes})
        set(result_incs -I${arg} ${result_incs})
    endforeach(arg ${includes})

    add_custom_command(
        OUTPUT ${_object_file}
        COMMAND nasm -o ${_object_file} ${result_incs} ${result_defs} -f bin ${_asm_file}
        DEPENDS native-winebuild)
    set_source_files_properties(${_object_file} PROPERTIES GENERATED TRUE)
    add_custom_target(${_target_name} ALL DEPENDS ${_object_file})
    add_minicd(${_object_file} loader ${OBJECT_NAME})
ENDMACRO(CreateBootSectorTarget _target_name _asm_file _object_file)
else()
MACRO(CreateBootSectorTarget _target_name _asm_file _object_file)
ENDMACRO()
endif()

MACRO(MACRO_IDL_COMPILE_OBJECT OBJECT SOURCE)
  GET_PROPERTY(FLAGS SOURCE ${SOURCE} PROPERTY COMPILE_FLAGS)
  GET_PROPERTY(DEFINES SOURCE ${SOURCE} PROPERTY COMPILE_DEFINITIONS)
  GET_PROPERTY(INCLUDE_DIRECTORIES DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
  FOREACH(DIR ${INCLUDE_DIRECTORIES})
    SET(FLAGS "${FLAGS} -I${DIR}")
  ENDFOREACH()

  SET(IDL_COMMAND ${CMAKE_IDL_COMPILE_OBJECT})
  STRING(REPLACE "<CMAKE_IDL_COMPILER>" "${CMAKE_IDL_COMPILER}" IDL_COMMAND "${IDL_COMMAND}")
  STRING(REPLACE <FLAGS> "${FLAGS}" IDL_COMMAND "${IDL_COMMAND}")
  STRING(REPLACE "<DEFINES>" "${DEFINES}" IDL_COMMAND "${IDL_COMMAND}")
  STRING(REPLACE "<OBJECT>" "${OBJECT}" IDL_COMMAND "${IDL_COMMAND}")
  STRING(REPLACE "<SOURCE>" "${SOURCE}" IDL_COMMAND "${IDL_COMMAND}")
  SEPARATE_ARGUMENTS(IDL_COMMAND)

  ADD_CUSTOM_COMMAND(
    OUTPUT ${OBJECT}
    COMMAND ${IDL_COMMAND}
    DEPENDS ${SOURCE}
    VERBATIM
  )
ENDMACRO()

MACRO(ADD_INTERFACE_DEFINITIONS TARGET)
  FOREACH(SOURCE ${ARGN})
    GET_FILENAME_COMPONENT(FILE ${SOURCE} NAME_WE)
    SET(OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${FILE}.h)
    MACRO_IDL_COMPILE_OBJECT(${OBJECT} ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE})
    LIST(APPEND OBJECTS ${OBJECT})
  ENDFOREACH()
  ADD_CUSTOM_TARGET(${TARGET} ALL DEPENDS ${OBJECTS})
ENDMACRO()

MACRO(add_minicd_target _targetname _dir) # optional parameter: _nameoncd
    if("${ARGN}" STREQUAL "")
        get_target_property(FILENAME ${_targetname} LOCATION)
    	get_filename_component(_nameoncd ${FILENAME} NAME)
    else()
    	set(_nameoncd ${ARGN})
    endif()
    
    file(APPEND ${REACTOS_BINARY_DIR}/boot/ros_minicd_target.txt "${_targetname}\t${_dir}\t${_nameoncd}\n")
ENDMACRO(add_minicd_target)

MACRO(add_minicd FILENAME _dir _nameoncd)
    file(APPEND ${REACTOS_BINARY_DIR}/boot/ros_minicd.txt "${FILENAME}\t${_dir}\t${_nameoncd}\n")
ENDMACRO(add_minicd)

macro(set_cpp)
  include_directories(BEFORE ${REACTOS_SOURCE_DIR}/lib/3rdparty/stlport/stlport)
  set(IS_CPP 1)
endmacro()

MACRO(add_livecd_target _targetname _dir )# optional parameter : _nameoncd
    if("${ARGN}" STREQUAL "")
        get_target_property(FILENAME ${_targetname} LOCATION)
    	get_filename_component(_nameoncd ${FILENAME} NAME)
    else()
    	set(_nameoncd ${ARGN})
    endif()
    
    file(APPEND ${REACTOS_BINARY_DIR}/boot/ros_livecd_target.txt "${_targetname}\t${_dir}\t${_nameoncd}\n")
ENDMACRO(add_livecd_target)

MACRO(add_livecd FILENAME _dir)# optional parameter : _nameoncd
    if("${ARGN}" STREQUAL "")
    	get_filename_component(_nameoncd ${FILENAME} NAME)
    else()
    	set(_nameoncd ${ARGN})
    endif()
    file(APPEND ${REACTOS_BINARY_DIR}/boot/ros_livecd.txt "${FILENAME}\t${_dir}\t${_nameoncd}\n")
ENDMACRO(add_livecd)

macro(cab_to_dir _dir_num _var_name)
#   1 = system32
#   2 = system32\drivers
#   3 = Fonts
#   4 =
#   5 = system32\drivers\etc
#   6 = inf
#   7 = bin
#   8 = media
    if(${_dir_num} STREQUAL "1")
        set(${_var_name} "reactos/system32")
    elseif(${_dir_num} STREQUAL "2")
        set(${_var_name} "reactos/system32/drivers")
    elseif(${_dir_num} STREQUAL "3")
        set(${_var_name} "reactos/fonts")
    elseif(${_dir_num} STREQUAL "4")
        set(${_var_name} "reactos")
    elseif(${_dir_num} STREQUAL "5")
        set(${_var_name} "reactos/system32/drivers/etc")
    elseif(${_dir_num} STREQUAL "6")
        set(${_var_name} "reactos/inf")
    elseif(${_dir_num} STREQUAL "7")
        set(${_var_name} "reactos/bin")
    elseif(${_dir_num} STREQUAL "8")
        set(${_var_name} "reactos/system32/drivers")
    else()
        message(FATAL_ERROR "Wrong directory ${_dir_num}")
    endif()
endmacro()

MACRO(add_cab_target _targetname _num )
    file(APPEND ${REACTOS_BINARY_DIR}/boot/ros_cab_target.txt "${_targetname}\t${_num}\n")
    cab_to_dir(${_num} _dir)
    add_livecd_target(${_targetname} ${_dir})
ENDMACRO(add_cab_target)

MACRO(add_cab FILENAME _num)
    file(APPEND ${REACTOS_BINARY_DIR}/boot/ros_cab.txt "${FILENAME}\t${_num}\n")
    cab_to_dir(${_num} _dir)
    add_livecd(${FILENAME} ${_dir})
ENDMACRO(add_cab)    

macro(custom_incdefs)
    if(NOT DEFINED result_incs) #rpc_defines
        get_directory_property(rpc_defines COMPILE_DEFINITIONS)
        get_directory_property(rpc_includes INCLUDE_DIRECTORIES)

        foreach(arg ${rpc_defines})
            set(result_defs ${result_defs} -D${arg})
        endforeach(arg ${defines})

        foreach(arg ${rpc_includes})
            set(result_incs -I${arg} ${result_incs})
        endforeach(arg ${includes})
    endif()
endmacro(custom_incdefs)

macro(rpcproxy TARGET)
    custom_incdefs()
        list(APPEND SOURCE ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_proxy.dlldata.c)

    foreach(_in_FILE ${ARGN})
        get_filename_component(FILE ${_in_FILE} NAME_WE)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_p.h ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_p.c
            COMMAND native-widl ${result_incs} ${result_defs} -m32 --win32 -h -H ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_p.h -p -P ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_p.c ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.idl
            DEPENDS native-widl)
        set_source_files_properties(
            ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_c.h ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_p.c
            PROPERTIES GENERATED TRUE)
        list(APPEND SOURCE ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_p.c)
        list(APPEND IDLS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.idl)
        list(APPEND PROXY_DEPENDS ${TARGET}_${FILE}_p)
        add_custom_target(${TARGET}_${FILE}_p 
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_p.c)
        #add_dependencies(${TARGET}_proxy ${TARGET}_${FILE}_p)
    endforeach(_in_FILE ${ARGN})
    
    	add_custom_command(
    	    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_proxy.dlldata.c
    	    COMMAND native-widl ${result_incs} ${result_defs}  -m32 --win32 --dlldata-only --dlldata=${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_proxy.dlldata.c ${IDLS}
    	    DEPENDS native-widl)
    	set_source_files_properties(
    	    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_proxy.dlldata.c
    	    PROPERTIES GENERATED TRUE)
        
        add_library(${TARGET}_proxy ${SOURCE})
        add_dependencies(${TARGET}_proxy psdk ${PROXY_DEPENDS})
endmacro(rpcproxy)

macro (MACRO_IDL_FILES)
    custom_incdefs()
    foreach(_in_FILE ${ARGN})
        get_filename_component(FILE ${_in_FILE} NAME_WE)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_s.h ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_s.c
            COMMAND ${IDL_COMPILER} ${result_incs} ${result_defs} ${IDL_FLAGS} ${IDL_HEADER_ARG} ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_s.h ${IDL_SERVER_ARG} ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_s.c ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.idl
            DEPENDS ${_in_file})
        set_source_files_properties(
            ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_s.h ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_s.c
            PROPERTIES GENERATED TRUE)
        add_library(${FILE}_server ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_s.c)
        add_dependencies(${FILE}_server psdk)
    
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_c.h ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_c.c
            COMMAND ${IDL_COMPILER} ${result_incs} ${result_defs} ${IDL_FLAGS} ${IDL_HEADER_ARG} ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_c.h ${IDL_CLIENT_ARG} ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_c.c ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.idl
            DEPENDS ${_in_file})
        set_source_files_properties(
            ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_c.h ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_c.c
            PROPERTIES GENERATED TRUE)
        add_library(${FILE}_client ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_c.c)
        add_dependencies(${FILE}_client psdk)
    endforeach(_in_FILE ${ARGN})

endmacro (MACRO_IDL_FILES)

macro(ADD_TYPELIB TARGET)
  custom_incdefs()
  foreach(SOURCE ${ARGN})
    get_filename_component(FILE ${SOURCE} NAME_WE)
    set(OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${FILE}.tlb)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${FILE}.tlb
                       COMMAND ${IDL_COMPILER} ${result_incs} ${IDL_FLAGS} ${IDL_TYPELIB_ARG} ${CMAKE_CURRENT_BINARY_DIR}/${FILE}.tlb ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}
                       DEPENDS ${SOURCE})
    list(APPEND OBJECTS ${OBJECT})
  endforeach()
  add_custom_target(${TARGET} ALL DEPENDS ${OBJECTS})
endmacro()
