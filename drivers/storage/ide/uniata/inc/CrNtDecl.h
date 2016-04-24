
#ifdef CROSSNT_DECL
#undef CROSSNT_DECL
#undef CROSSNT_DECL_EX
#endif //CROSSNT_DECL

/***************************/
#ifdef CROSSNT_DECL_API

#define CROSSNT_DECL(type, dspec, name, args, callargs) \
typedef type (dspec *ptr##name) args;        \
extern "C" ptr##name CrNt##name; \
type dspec CrNt##name##_impl args;

#define CROSSNT_DECL_EX(mod, type, dspec, name, args, callargs) \
typedef type (dspec *ptr##name) args;        \
extern "C" ptr##name CrNt##name; \
type dspec CrNt##name##_impl args;

#endif //CROSSNT_DECL_API

/***************************/
#ifdef CROSSNT_DECL_STUB

#define CROSSNT_DECL(type, dspec, name, args, callargs) \
extern "C" ptr##name CrNt##name = NULL;          \

#define CROSSNT_DECL_EX(mod, type, dspec, name, args, callargs) \
extern "C" ptr##name CrNt##name = NULL;          \

#endif //CROSSNT_DECL_STUB

/***************************/
#ifdef CROSSNT_INIT_STUB

#define CROSSNT_DECL(type, dspec, name, args, callargs) \
    KdPrint(("Init " #name " cur %x\n", CrNt##name)); \
    if(!CrNt##name) {                 \
        CrNt##name = (ptr##name)CrNtGetProcAddress(g_hNtosKrnl, #name); \
        KdPrint(("  GetProcAddr(NTOSKRNL.EXE," #name ") = %x\n", CrNt##name)); \
        if(!CrNt##name) {             \
            CrNt##name = CrNt##name##_impl; \
        }                              \
        KdPrint(("  final %\n", CrNt##name)); \
    }

#define CROSSNT_DECL_EX(mod, type, dspec, name, args, callargs) \
    KdPrint(("Init " mod "," #name " cur %x\n", CrNt##name)); \
    if(!CrNt##name) {                 \
        CrNt##name = (ptr##name)CrNtGetProcAddress(CrNtGetModuleBase(mod), #name); \
        KdPrint(("  GetProcAddr(" mod "," #name ") = %x\n", CrNt##name)); \
        if(!CrNt##name) {             \
            CrNt##name = CrNt##name##_impl; \
        }                              \
        KdPrint(("  final %x\n", CrNt##name)); \
    }

#endif //CROSSNT_INIT_STUB
