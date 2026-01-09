
if(NOT MSVC)
    # Use the same trick as with the other import libs. See gcc.cmake --> generate_import_lib function
    set(LIBRARY_PRIVATE_DIR ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/oldnames.dir)
    add_custom_command(
        OUTPUT ${LIBRARY_PRIVATE_DIR}/oldnames.a
        # ar just puts stuff into the archive, without looking twice. Just delete the lib, we're going to rebuild it anyway
        COMMAND ${CMAKE_COMMAND} -E rm -f ${LIBRARY_PRIVATE_DIR}/oldnames.a
        COMMAND ${CMAKE_DLLTOOL} --def ${CMAKE_CURRENT_SOURCE_DIR}/moldname-msvcrt.def --kill-at --output-lib=oldnames.a -t oldnames
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/moldname-msvcrt.def
        WORKING_DIRECTORY ${LIBRARY_PRIVATE_DIR})

        # Create a custom target for the import library generation
        add_custom_target(oldnames_target DEPENDS ${LIBRARY_PRIVATE_DIR}/oldnames.a)

        # Create an IMPORTED library that references the dlltool output
        _add_library(oldnames STATIC IMPORTED GLOBAL)
        set_target_properties(oldnames PROPERTIES IMPORTED_LOCATION ${LIBRARY_PRIVATE_DIR}/oldnames.a)
        add_dependencies(oldnames oldnames_target)
        set_target_properties(oldnames PROPERTIES LINKER_LANGUAGE "C")
else()
    add_asm_files(oldnames_asm oldnames-common.S oldnames-msvcrt.S)
    add_library(oldnames ${oldnames_asm})
    set_target_properties(oldnames PROPERTIES LINKER_LANGUAGE "C")
endif()

target_compile_definitions(oldnames INTERFACE
    _CRT_DECLARE_NONSTDC_NAMES=1 # This must be set to 1
    _CRT_NONSTDC_NO_DEPRECATE
)
