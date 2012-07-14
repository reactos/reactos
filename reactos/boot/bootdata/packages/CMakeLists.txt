#reactos.dff

# reactos.dff is the concatenation of two files :
#   - reactos.dff.in, which is a static one and can be altered to
# add custom modules/files to reactos.cab
#   - reactos.dff.dyn (dyn as dynamic) which is generated at configure time by our cmake scripts
# If you want to slip-stream anything into the bootcd, then you want to alter reactos.dff.in

# Idea taken from there : http://www.cmake.org/pipermail/cmake/2010-July/038028.html
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/concat.cmake "
    file(READ \${SRC1} S1)
    file(READ \${SRC2} S2)
    file(WRITE \${DST} \"\${S1}\${S2}\")
")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff.dyn "")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff
    COMMAND ${CMAKE_COMMAND} -D SRC1=${CMAKE_CURRENT_SOURCE_DIR}/reactos.dff.in
                             -D SRC2=${CMAKE_CURRENT_BINARY_DIR}/reactos.dff.dyn
                             -D DST=${CMAKE_CURRENT_BINARY_DIR}/reactos.dff
                             -P ${CMAKE_CURRENT_BINARY_DIR}/concat.cmake
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/reactos.dff.in
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff.dyn
)

# And now we build reactos.cab

# First we create reactos.inf
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/reactos.inf
    COMMAND native-cabman -C ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff -L ${CMAKE_CURRENT_BINARY_DIR} -I -P ${REACTOS_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff native-cabman)

# Then we create the actual cab file
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/reactos.cab
    COMMAND native-cabman -C ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff -RC ${CMAKE_CURRENT_BINARY_DIR}/reactos.inf -L ${CMAKE_CURRENT_BINARY_DIR} -N -P ${REACTOS_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/reactos.inf native-cabman)
# Finally, we create the reactos_cab target
add_custom_target(reactos_cab DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/reactos.cab)

add_cd_file(TARGET reactos_cab FILE ${CMAKE_CURRENT_BINARY_DIR}/reactos.cab ${CMAKE_CURRENT_BINARY_DIR}/reactos.inf DESTINATION reactos NO_CAB FOR bootcd regtest)
