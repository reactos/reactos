#reactos.dff

# reactos.dff is the concatenation of two files:
# - reactos.dff.in, which is a static one and can be altered to
#   add custom modules/files to reactos.cab
# - reactos.dff.dyn (dyn as in dynamic) which is generated at generation
#   time by our cmake scripts.
# If you want to slip-stream anything into the bootcd, then you want to alter reactos.dff.in

# Idea taken from there : http://www.cmake.org/pipermail/cmake/2010-July/038028.html
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/concat.cmake "
    file(READ \${SRC1} S1)
    file(READ \${SRC2} S2)
    file(WRITE \${DST} \"\${S1}\${S2}\")
")

# This generates reactos.dff.dyn by processing the generator expressions
file(GENERATE
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff.$<CONFIG>.dyn
    CONTENT $<GENEX_EVAL:$<JOIN:$<TARGET_PROPERTY:reactos_cab,FILE_LIST>,\n>>\n)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff
    # Concatenate the pre-formatted file and the generated one.
    COMMAND ${CMAKE_COMMAND} -D SRC1=${CMAKE_CURRENT_SOURCE_DIR}/reactos.dff.in
                            -D SRC2=${CMAKE_CURRENT_BINARY_DIR}/reactos.dff.$<CONFIG>.dyn
                            -D DST=${CMAKE_CURRENT_BINARY_DIR}/reactos.dff
                            -P ${CMAKE_CURRENT_BINARY_DIR}/concat.cmake
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/reactos.dff.in ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff.$<CONFIG>.dyn)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/reactos.cab ${CMAKE_CURRENT_BINARY_DIR}/reactos.inf
    COMMAND native-cabman -C ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff -L ${CMAKE_CURRENT_BINARY_DIR} -I -P ${REACTOS_SOURCE_DIR}
    COMMAND native-cabman -C ${REACTOS_BINARY_DIR}/boot/bootdata/packages/reactos.dff -RC ${CMAKE_CURRENT_BINARY_DIR}/reactos.inf -N -P ${REACTOS_SOURCE_DIR}
    DEPENDS native-cabman
        ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff
        $<GENEX_EVAL:$<TARGET_PROPERTY:reactos_cab,CAB_DEPENDENCIES>>)

add_custom_target(reactos_cab DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/reactos.cab ${CMAKE_CURRENT_BINARY_DIR}/reactos.inf)

add_cd_file(
    TARGET reactos_cab
    FILE "${CMAKE_CURRENT_BINARY_DIR}/reactos.inf;${CMAKE_CURRENT_BINARY_DIR}/reactos.cab"
    DESTINATION reactos
    NO_CAB FOR bootcd regtest)
