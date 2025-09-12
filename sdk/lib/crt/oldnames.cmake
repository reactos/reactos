
if(NOT MSVC)
    # Use the same trick as with the other import libs. See gcc.cmake --> generate_import_lib function
    set(LIBRARY_PRIVATE_DIR ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/oldnames.dir)
    # Run ranlib to add index after dlltool for both amd64 and i386
    if(ARCH STREQUAL "amd64" OR ARCH STREQUAL "i386")
        add_custom_command(
            OUTPUT ${LIBRARY_PRIVATE_DIR}/oldnames.a
            # ar just puts stuff into the archive, without looking twice. Just delete the lib, we're going to rebuild it anyway
            COMMAND ${CMAKE_COMMAND} -E rm -f $<TARGET_FILE:oldnames>
            COMMAND ${CMAKE_DLLTOOL} --def ${CMAKE_CURRENT_SOURCE_DIR}/moldname-msvcrt.def --kill-at --output-lib=oldnames.a -t oldnames
            COMMAND ${CMAKE_RANLIB} oldnames.a
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/moldname-msvcrt.def
            WORKING_DIRECTORY ${LIBRARY_PRIVATE_DIR})
    else()
        add_custom_command(
            OUTPUT ${LIBRARY_PRIVATE_DIR}/oldnames.a
            # ar just puts stuff into the archive, without looking twice. Just delete the lib, we're going to rebuild it anyway
            COMMAND ${CMAKE_COMMAND} -E rm -f $<TARGET_FILE:oldnames>
            COMMAND ${CMAKE_DLLTOOL} --def ${CMAKE_CURRENT_SOURCE_DIR}/moldname-msvcrt.def --kill-at --output-lib=oldnames.a -t oldnames
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/moldname-msvcrt.def
            WORKING_DIRECTORY ${LIBRARY_PRIVATE_DIR})
    endif()
    set_source_files_properties(
        ${LIBRARY_PRIVATE_DIR}/oldnames.a
        PROPERTIES
        EXTERNAL_OBJECT TRUE)

    _add_library(oldnames STATIC EXCLUDE_FROM_ALL ${LIBRARY_PRIVATE_DIR}/oldnames.a)
    set_target_properties(oldnames PROPERTIES LINKER_LANGUAGE "C")
    # Ensure the final library has an index for both amd64 and i386
    # The EXTERNAL_OBJECT property causes AR to re-archive and lose the index
    if(ARCH STREQUAL "amd64" OR ARCH STREQUAL "i386")
        add_custom_command(TARGET oldnames POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${LIBRARY_PRIVATE_DIR}/oldnames.a $<TARGET_FILE:oldnames>
            COMMAND ${CMAKE_RANLIB} $<TARGET_FILE:oldnames>
            COMMENT "Overwriting oldnames with proper library index (amd64/i386 workaround)")
    endif()
else()
    add_asm_files(oldnames_asm oldnames-common.S oldnames-msvcrt.S)
    add_library(oldnames ${oldnames_asm})
    set_target_properties(oldnames PROPERTIES LINKER_LANGUAGE "C")
endif()

target_compile_definitions(oldnames INTERFACE
    _CRT_DECLARE_NONSTDC_NAMES=1 # This must be set to 1
    _CRT_NONSTDC_NO_DEPRECATE
)
