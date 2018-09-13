//  enum specifying routine that initialized search
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    INIT_base,
#ifdef NEVER
    INIT_tdef,
#endif
    INIT_sym,
    INIT_qual,
    INIT_right,
    INIT_RE,
    INIT_AErule
} INIT_t;


typedef enum HSYML_t {
    HSYML_lexical,
    HSYML_function,
    HSYML_class,
    HSYML_module,
    HSYML_global,
    HSYML_exe,
    HSYML_public
} HSYML_t;


#define FNCMP fnCmp
#ifdef NEVER
    #define TDCMP tdCmp
#endif
#define CSCMP csCmp

typedef struct {
    char    str[20];        // "operator delete" needs 17 bytes
} OPNAME;

OPNAME OpName[];

#ifdef __cplusplus
} // extern "C" {
#endif
