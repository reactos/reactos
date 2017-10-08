
# Simply use :
#  add_bison_files(foo.y)
#  and
#  add_flex_files(foo.l)
#  then add ${CMAKE_CURRENT_BINARY_DIR}/foo.tab.c
#  and ${CMAKE_CURRENT_BINARY_DIR}/foo.yy.c to the source list

function(add_bison_files)
    foreach(_file ${ARGN})
        get_filename_component(_name ${_file} NAME_WE)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_name}.tab.c ${CMAKE_CURRENT_BINARY_DIR}/${_name}.tab.h
            COMMAND bison -p ${_name}_ -o ${CMAKE_CURRENT_BINARY_DIR}/${_name}.tab.c --defines=${CMAKE_CURRENT_BINARY_DIR}/${_name}.tab.h ${CMAKE_CURRENT_SOURCE_DIR}/${_file}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_file})
    endforeach()
endfunction()

function(add_flex_files)
    foreach(_file ${ARGN})
        get_filename_component(_name ${_file} NAME_WE)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_name}.yy.c ${CMAKE_CURRENT_BINARY_DIR}/${_name}.yy.h
            COMMAND flex -o ${CMAKE_CURRENT_BINARY_DIR}/${_name}.yy.c --header-file=${CMAKE_CURRENT_BINARY_DIR}/${_name}.yy.h ${CMAKE_CURRENT_SOURCE_DIR}/${_file}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_file})
    endforeach()
endfunction()
