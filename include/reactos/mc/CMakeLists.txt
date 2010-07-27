   # bugcodes
   ADD_CUSTOM_COMMAND(
     OUTPUT ${REACTOS_BINARY_DIR}/include/reactos/bugcodes.h
     COMMAND native-wmc -i -H ${REACTOS_BINARY_DIR}/include/reactos/bugcodes.h -o ${REACTOS_BINARY_DIR}/include/reactos/mc/bugcodes.rc ${REACTOS_SOURCE_DIR}/include/reactos/mc/bugcodes.mc
     DEPENDS native-wmc
   )

   SET_SOURCE_FILES_PROPERTIES(
     ${REACTOS_BINARY_DIR}/include/reactos/bugcodes.h
     PROPERTIES
     GENERATED TRUE
   )

   ADD_CUSTOM_TARGET(bugcodes ALL DEPENDS ${REACTOS_BINARY_DIR}/include/reactos/bugcodes.h)

   # errcodes
   ADD_CUSTOM_COMMAND(
     OUTPUT ${REACTOS_BINARY_DIR}/include/reactos/errcodes.h
     COMMAND native-wmc -i -H ${REACTOS_BINARY_DIR}/include/reactos/errcodes.h -o ${REACTOS_BINARY_DIR}/include/reactos/mc/errcodes.rc ${REACTOS_SOURCE_DIR}/include/reactos/mc/errcodes.mc
     DEPENDS native-wmc
   )

   SET_SOURCE_FILES_PROPERTIES(
     ${REACTOS_BINARY_DIR}/include/reactos/errcodes.h
     PROPERTIES
     GENERATED TRUE
   )

   ADD_CUSTOM_TARGET(errcodes ALL DEPENDS ${REACTOS_BINARY_DIR}/include/reactos/errcodes.h)

   # netevent
   ADD_CUSTOM_COMMAND(
     OUTPUT ${REACTOS_BINARY_DIR}/include/reactos/netevent.h
     COMMAND native-wmc -i -H ${REACTOS_BINARY_DIR}/include/reactos/netevent.h -o ${REACTOS_BINARY_DIR}/include/reactos/mc/netevent.rc ${REACTOS_SOURCE_DIR}/include/reactos/mc/netevent.mc
     DEPENDS native-wmc
   )

   SET_SOURCE_FILES_PROPERTIES(
     ${REACTOS_BINARY_DIR}/include/reactos/netevent.h
     PROPERTIES
     GENERATED TRUE
   )

   ADD_CUSTOM_TARGET(netevent ALL DEPENDS ${REACTOS_BINARY_DIR}/include/reactos/netevent.h)

   # ntstatus
   ADD_CUSTOM_COMMAND(
     OUTPUT ${REACTOS_BINARY_DIR}/include/reactos/ntstatus.h
     COMMAND native-wmc -i -H ${REACTOS_BINARY_DIR}/include/reactos/ntstatus.h -o ${REACTOS_BINARY_DIR}/include/reactos/mc/ntstatus.rc ${REACTOS_SOURCE_DIR}/include/reactos/mc/ntstatus.mc
     DEPENDS native-wmc
   )

   SET_SOURCE_FILES_PROPERTIES(
     ${REACTOS_BINARY_DIR}/include/reactos/ntstatus.h
     PROPERTIES
     GENERATED TRUE
   )

   ADD_CUSTOM_TARGET(ntstatus ALL DEPENDS ${REACTOS_BINARY_DIR}/include/reactos/ntstatus.h)

   # pciclass
   ADD_CUSTOM_COMMAND(
     OUTPUT ${REACTOS_BINARY_DIR}/include/reactos/pciclass.h
     COMMAND native-wmc -i -H ${REACTOS_BINARY_DIR}/include/reactos/pciclass.h -o ${REACTOS_BINARY_DIR}/include/reactos/mc/pciclass.rc ${REACTOS_SOURCE_DIR}/include/reactos/mc/pciclass.mc
     DEPENDS native-wmc
   )

   SET_SOURCE_FILES_PROPERTIES(
     ${REACTOS_BINARY_DIR}/include/reactos/pciclass.h
     PROPERTIES
     GENERATED TRUE
   )

   ADD_CUSTOM_TARGET(pciclass ALL DEPENDS ${REACTOS_BINARY_DIR}/include/reactos/pciclass.h)