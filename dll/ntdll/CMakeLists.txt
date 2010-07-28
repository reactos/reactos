
ADD_CUSTOM_COMMAND(
  OUTPUT ${REACTOS_BINARY_DIR}/dll/ntdll/ntsys.a
  COMMAND ${MINGW_PREFIX}gcc ${CMAKE_C_FLAGS} -xc -E ${REACTOS_BINARY_DIR}/dll/ntdll/def/ntsys.pspec > ${REACTOS_BINARY_DIR}/dll/ntdll/def/ntsys_ntsys.spec
  COMMAND native-winebuild -o ${REACTOS_BINARY_DIR}/dll/ntdll/def/ntsys_ntsys.stubs.c --pedll ${REACTOS_BINARY_DIR}/dll/ntdll/def/ntsys_ntsys.spec --filename ntdll.dll
  COMMAND ${MINGW_PREFIX}gcc -o ${REACTOS_BINARY_DIR}/dll/ntdll/def/ntsys_ntsys.stubs.o ${CMAKE_C_FLAGS} -c ${REACTOS_BINARY_DIR}/dll/ntdll/def/ntsys_ntsys.stubs.c
  COMMAND native-winebuild -o ${REACTOS_BINARY_DIR}/dll/ntdll/def/ntsys_ntsys.auto.def --def -E ${REACTOS_BINARY_DIR}/dll/ntdll/def/ntsys_ntsys.spec --filename ntdll.dll
  COMMAND ${MINGW_PREFIX}dlltool --def ${REACTOS_BINARY_DIR}/dll/ntdll/def/ntsys_ntsys.auto.def --kill-at --output-lib ${REACTOS_BINARY_DIR}/dll/ntdll/ntsys.a
  COMMAND ${MINGW_PREFIX}ar -rc ${REACTOS_BINARY_DIR}/dll/ntdll/ntsys.a ${REACTOS_BINARY_DIR}/dll/ntdll/def/ntsys_ntsys.stubs.o
  DEPENDS native-winebuild
)

SET_SOURCE_FILES_PROPERTIES(${REACTOS_BINARY_DIR}/dll/ntdll/ntsys.a PROPERTIES GENERATED TRUE)

ADD_CUSTOM_TARGET(ntsys ALL DEPENDS ${REACTOS_BINARY_DIR}/dll/ntdll/ntsys.a)
add_dependencies(ntsys ntsys_pspec)
