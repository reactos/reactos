
if(NOT MSVC)
    # Use IMPORTED library to avoid re-archiving the dlltool output with 'ar crT',
    # which would create nested archives (thin archive containing non-thin archive).
    # This causes linker crashes with binutils 2.40+ (see binutils bug #31614).
    set(LIBRARY_PRIVATE_DIR ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/oldnames.dir)
    set(_oldnames_file ${LIBRARY_PRIVATE_DIR}/oldnames.a)
    add_custom_command(
        OUTPUT ${_oldnames_file}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${LIBRARY_PRIVATE_DIR}
        COMMAND ${CMAKE_DLLTOOL} --def ${CMAKE_CURRENT_SOURCE_DIR}/moldname-msvcrt.def --kill-at --output-lib=${_oldnames_file} -t oldnames
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/moldname-msvcrt.def)

    # Create a custom target to drive the library generation
    add_custom_target(oldnames_target DEPENDS ${_oldnames_file})

    # Create an IMPORTED library that references the dlltool output directly
    _add_library(oldnames STATIC IMPORTED GLOBAL)
    set_target_properties(oldnames PROPERTIES IMPORTED_LOCATION ${_oldnames_file})
    add_dependencies(oldnames oldnames_target)
else()
    add_asm_files(oldnames_asm oldnames-common.S oldnames-msvcrt.S)
    add_library(oldnames ${oldnames_asm})
    set_target_properties(oldnames PROPERTIES LINKER_LANGUAGE "C")
endif()

target_compile_definitions(oldnames INTERFACE
    _CRT_DECLARE_NONSTDC_NAMES=1 # This must be set to 1
    _CRT_NONSTDC_NO_DEPRECATE
)
