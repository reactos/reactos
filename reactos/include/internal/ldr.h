
NTSTATUS LdrLoadDriver(PUNICODE_STRING Filename);
NTSTATUS LdrLoadInitialProcess(VOID);
VOID LdrLoadAutoConfigDrivers(VOID);
VOID LdrInitModuleManagement(VOID);
NTSTATUS LdrProcessDriver(PVOID ModuleLoadBase);


