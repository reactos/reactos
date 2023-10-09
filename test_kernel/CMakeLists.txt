PROJECT(NTOS)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Enable this again. CORE-17637
    add_compile_options(-Wunused-result)
endif()

spec2def(testkernel.exe testkernel.spec ADD_IMPORTLIB)

# Embed RTC libs
if (STACK_PROTECTOR)
    target_sources(testkernel PRIVATE $<TARGET_OBJECTS:gcc_ssp_nt>)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Clang optimises strcmp calls to memcmp.
    target_sources(testkernel PRIVATE $<TARGET_OBJECTS:memcmp>)
endif()

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_executable(testkernel
    main.c
    testkernel.rc
    ${CMAKE_CURRENT_BINARY_DIR}/testkernel.def)
set_module_type(testkernel kernel)



target_link_libraries(testkernel cportlib csq cmlib ntlsalib ${ROSSYM_LIB} libcntpr wdmguid ioevent)
add_dependencies(testkernel psdk asm)