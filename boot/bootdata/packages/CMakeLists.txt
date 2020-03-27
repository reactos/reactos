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

# reactos.inf. We want this command to be always executed, in case someone added an optional file between two builds.
# So we pretend it generates another file although it will never do.
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/reactos_real.inf ${CMAKE_CURRENT_BINARY_DIR}/__some_non_existent_file
    COMMAND native-cabman -C ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff -L ${CMAKE_CURRENT_BINARY_DIR} -I -P ${REACTOS_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/reactos.inf ${CMAKE_CURRENT_BINARY_DIR}/reactos_real.inf
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff native-cabman
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/reactos.inf)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/reactos.cab
    COMMAND native-cabman -C ${CMAKE_CURRENT_BINARY_DIR}/reactos.dff -RC ${CMAKE_CURRENT_BINARY_DIR}/reactos.inf -N -P ${REACTOS_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/reactos_real.inf native-cabman $<TARGET_PROPERTY:reactos_cab,CAB_DEPENDENCIES>)

add_custom_target(reactos_cab DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/reactos.cab)

add_cd_file(
    TARGET reactos_cab
    FILE ${CMAKE_CURRENT_BINARY_DIR}/reactos.cab
    DESTINATION reactos
    NO_CAB FOR bootcd regtest)

add_cd_file(
    TARGET reactos_cab
    FILE ${CMAKE_CURRENT_BINARY_DIR}/reactos_real.inf
    NAME_ON_CD reactos.inf
    DESTINATION reactos
    NO_CAB FOR bootcd regtest)
