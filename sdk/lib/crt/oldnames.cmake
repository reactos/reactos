
if(NOT MSVC)
    # Use the same trick as with the other import libs. See gcc.cmake --> generate_import_lib function
    set(LIBRARY_PRIVATE_DIR ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/oldnames.dir)

    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND USE_GNU_DLLTOOL)
        add_custom_command(
            OUTPUT ${LIBRARY_PRIVATE_DIR}/oldnames.a
            # ar just puts stuff into the archive, without looking twice. Just delete the lib, we're going to rebuild it anyway
            COMMAND ${CMAKE_COMMAND} -E rm -f $<TARGET_FILE:oldnames>
            COMMAND ${CMAKE_DLLTOOL} -d ${CMAKE_CURRENT_SOURCE_DIR}/moldname-msvcrt.def -k -l oldnames.a -m ${ARCH}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/moldname-msvcrt.def
            WORKING_DIRECTORY ${LIBRARY_PRIVATE_DIR})
        set_source_files_properties(
            ${LIBRARY_PRIVATE_DIR}/oldnames.a
            PROPERTIES
            EXTERNAL_OBJECT TRUE)
    else()
        add_custom_command(
            OUTPUT ${LIBRARY_PRIVATE_DIR}/oldnames.a
            # ar just puts stuff into the archive, without looking twice. Just delete the lib, we're going to rebuild >
            COMMAND ${CMAKE_COMMAND} -E rm -f $<TARGET_FILE:oldnames>
            COMMAND ${CMAKE_DLLTOOL} --def ${CMAKE_CURRENT_SOURCE_DIR}/moldname-msvcrt.def --kill-at --output-lib=oldnames.a -t oldnames
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/moldname-msvcrt.def
            WORKING_DIRECTORY ${LIBRARY_PRIVATE_DIR})
        set_source_files_properties(
            ${LIBRARY_PRIVATE_DIR}/oldnames.a
            PROPERTIES
            EXTERNAL_OBJECT TRUE)
    endif()

    _add_library(oldnames STATIC EXCLUDE_FROM_ALL ${LIBRARY_PRIVATE_DIR}/oldnames.a)
    set_target_properties(oldnames PROPERTIES LINKER_LANGUAGE "C")
else()
    add_asm_files(oldnames_asm oldnames-msvcrt.S)
    add_library(oldnames ${oldnames_asm})
    set_target_properties(oldnames PROPERTIES LINKER_LANGUAGE "C")
endif()
