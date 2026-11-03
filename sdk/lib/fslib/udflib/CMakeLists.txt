include_directories(${CMAKE_CURRENT_SOURCE_DIR}/compat ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${REACTOS_SOURCE_DIR}/drivers/filesystems/udfs/udf_info)
add_library(udflib udflib.c udfchk.c core/udf_create.c core/udf_write.c core/udf_osta.c
    ${REACTOS_SOURCE_DIR}/drivers/filesystems/udfs/udf_info/recovery_core.c)
add_dependencies(udflib xdk)
if(NOT MSVC)
    set_source_files_properties(core/udf_create.c PROPERTIES COMPILE_FLAGS "-Wno-address-of-packed-member")
endif()
