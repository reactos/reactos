
ADD_CUSTOM_COMMAND(
  OUTPUT ${REACTOS_BINARY_DIR}/lib/debugsup/debugsup_ntdll.a
  COMMAND native-winebuild -o ${REACTOS_BINARY_DIR}/lib/debugsup/debugsup-ntos_debugsup_ntdll.stubs.c --pedll ${REACTOS_SOURCE_DIR}/lib/debugsup/debugsup-ntos.spec --filename ntdll.dll
  COMMAND ${MINGW_PREFIX}gcc -o ${REACTOS_BINARY_DIR}/lib/debugsup/debugsup-ntos_debugsup_ntdll.stubs.o ${CMAKE_C_FLAGS} -c ${REACTOS_BINARY_DIR}/lib/debugsup/debugsup-ntos_debugsup_ntdll.stubs.c
  COMMAND native-winebuild -o ${REACTOS_BINARY_DIR}/lib/debugsup/debugsup-ntos_debugsup_ntdll.auto.def --def -E ${REACTOS_SOURCE_DIR}/lib/debugsup/debugsup-ntos.spec --filename ntdll.dll
  COMMAND ${MINGW_PREFIX}dlltool --def ${REACTOS_BINARY_DIR}/lib/debugsup/debugsup-ntos_debugsup_ntdll.auto.def --kill-at --output-lib ${REACTOS_BINARY_DIR}/lib/debugsup/debugsup_ntdll.a
  COMMAND ${MINGW_PREFIX}ar -rc ${REACTOS_BINARY_DIR}/lib/debugsup/debugsup_ntdll.a ${REACTOS_BINARY_DIR}/lib/debugsup/debugsup-ntos_debugsup_ntdll.stubs.o
  DEPENDS native-winebuild
)

SET_SOURCE_FILES_PROPERTIES(${REACTOS_BINARY_DIR}/lib/debugsup/debugsup_ntdll.a PROPERTIES GENERATED TRUE)

ADD_CUSTOM_TARGET(debugsup_ntdll ALL DEPENDS ${REACTOS_BINARY_DIR}/lib/debugsup/debugsup_ntdll.a)

