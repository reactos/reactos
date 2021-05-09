#reactos.dff

# reactos.dff is the concatenation of two files:
# - reactos.dff.in, which is a static one and can be altered to
#   add custom modules/files to reactos.cab
# - reactos.dff.dyn (dyn as in dynamic) which is generated at generation
#   time by our cmake scripts (from reactos.dff.cmake, which contains
#   generator expressions)
# If you want to slip-stream anything into the bootcd, then you want to alter reactos.dff.in

# Idea taken from there : http://www.cmake.org/pipermail/cmake/2010-July/038028.html
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/concat.cmake "
    file(READ \${SRC1} S1)
    file(READ \${SRC2} S2)
    file(WRITE \${DST} \"\${S1}\${S2}\")
")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff.cmake "")

# This generates reactos.dff.dyn by processing the generator expressions
file(GENERATE
     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff.$<CONFIG>.dyn
     INPUT ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff.cmake)

# This finalizes reactos.dff by concat-ing the two files: one generated and one static containing the optional file.
# please keep it this way as it permits to add files to reactos.dff.in without having to run cmake again
# and also avoids rebuilding reactos.cab in case nothing changes after a cmake run
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff
    COMMAND ${CMAKE_COMMAND} -D SRC1=${CMAKE_CURRENT_SOURCE_DIR}/reactos.dff.in
                             -D SRC2=${CMAKE_CURRENT_BINARY_DIR}/reactos.dff.$<CONFIG>.dyn
                             -D DST=${CMAKE_CURRENT_BINARY_DIR}/reactos.dff.maydiff
                             -P ${CMAKE_CURRENT_BINARY_DIR}/concat.cmake
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                             ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff.maydiff
                             ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/reactos.dff.in
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff.$<CONFIG>.dyn)

add_custom_command(
    COMMAND native-cabman -C ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff -L ${CMAKE_CURRENT_BINARY_DIR} -I -P ${REACTOS_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff native-cabman
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/reactos.inf)

add_custom_target(
    reactos_cab_inf DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/reactos.inf)

add_cd_file(
    TARGET reactos_cab_inf
    FILE ${CMAKE_CURRENT_BINARY_DIR}/reactos.inf
    DESTINATION reactos
    NO_CAB FOR bootcd regtest)
