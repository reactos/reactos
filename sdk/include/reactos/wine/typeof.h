#define typeof(X_) __typeof_ ## X_

#ifdef _WIN64
#define __typeof_intptr long long
#define __typeof_longptr long long
#else
#define __typeof_intptr int
#define __typeof_longptr long
#endif

#ifdef __cplusplus
#define __typeof_size size_t
#define __typeof_wchar wchar_t
#else
#define __typeof_size __typeof_intptr
#define __typeof_wchar unsigned short
#endif

struct IUnknown;
struct _tagIMEINFO;
struct tagSTYLEBUFW;
struct tagCANDIDATELIST;
struct tagIMEMENUITEMINFOW;
struct _xsltStylesheet;
struct jpeg_decompress_struct;
struct _iobuf;
struct _xsltTransformContext;
struct _xmlOutputBuffer;
struct _xmlXPathParserContext;
struct _xmlDict;
enum xsltLoadType;

typedef struct IWineD3D * (__stdcall typeof(WineDirect3DCreate))(unsigned int, struct IUnknown *);
typedef struct IWineD3DClipper * (__stdcall typeof(WineDirect3DCreateClipper))(struct IUnknown *);
typedef int (__stdcall typeof(ImeInquire))(struct _tagIMEINFO *, __typeof_wchar *, const __typeof_wchar *);
typedef int (__stdcall typeof(ImeConfigure))(void *, void *, unsigned int, void *);
typedef int (__stdcall typeof(ImeDestroy))(unsigned int);
typedef __typeof_longptr (__stdcall typeof(ImeEscape))(void *, unsigned int, void *);
typedef int (__stdcall typeof(ImeSelect))(void *, int);
typedef int (__stdcall typeof(ImeSetActiveContext))(void *, int);
typedef unsigned int (__stdcall typeof(ImeToAsciiEx))(unsigned int, unsigned int, unsigned char *, unsigned int *, unsigned int, void *);
typedef int (__stdcall typeof(NotifyIME))(void *, unsigned int, unsigned int, unsigned int);
typedef int (__stdcall typeof(ImeRegisterWord))(const __typeof_wchar *, unsigned int, const __typeof_wchar *);
typedef int (__stdcall typeof(ImeUnregisterWord))(const __typeof_wchar *, unsigned int, const __typeof_wchar *);
typedef unsigned int (__stdcall typeof(ImeGetRegisterWordStyle))(unsigned int, struct tagSTYLEBUFW *);
typedef unsigned int (__stdcall typeof(ImeEnumRegisterWord))(int (__stdcall *)(const __typeof_wchar *, unsigned int, const __typeof_wchar *, void *), const __typeof_wchar *, unsigned int, const __typeof_wchar *, void *);
typedef int (__stdcall typeof(ImeSetCompositionString))(void *, unsigned int, const void *, unsigned int, const void *, unsigned int);
typedef unsigned int (__stdcall typeof(ImeConversionList))(void *, const __typeof_wchar *, struct tagCANDIDATELIST *, unsigned int, unsigned int);
typedef int (__stdcall typeof(ImeProcessKey))(void *, unsigned int, __typeof_longptr, unsigned char *);
typedef unsigned int (__stdcall typeof(ImeGetImeMenuItems))(void *, unsigned int, unsigned int, struct tagIMEMENUITEMINFOW *, struct tagIMEMENUITEMINFOW *, unsigned int);
typedef struct _xmlDoc * (__cdecl typeof(xsltApplyStylesheet))(struct _xsltStylesheet *, struct _xmlDoc *, const char **);
typedef struct _xmlDoc * (__cdecl typeof(xsltApplyStylesheetUser))(struct _xsltStylesheet *, struct _xmlDoc *, const char **, const char *, struct _iobuf *, struct _xsltTransformContext *);
typedef struct _xsltTransformContext * (__cdecl typeof(xsltNewTransformContext))(struct _xsltStylesheet *, struct _xmlDoc *);
typedef void (__cdecl typeof(xsltFreeTransformContext))(struct _xsltTransformContext *);
typedef int (__cdecl typeof(xsltQuoteUserParams))(struct _xsltTransformContext *, const char **);
typedef int (__cdecl typeof(xsltSaveResultTo))(struct _xmlOutputBuffer *, struct _xmlDoc *, struct _xsltStylesheet *);
typedef struct _xsltStylesheet * (__cdecl typeof(xsltNextImport))(struct _xsltStylesheet *);
typedef void (__cdecl typeof(xsltCleanupGlobals))(void);
typedef void (__cdecl typeof(xsltFreeStylesheet))(struct _xsltStylesheet *);
typedef struct _xsltStylesheet * (__cdecl typeof(xsltParseStylesheetDoc))(struct _xmlDoc *);
typedef void (__cdecl typeof(xsltFunctionNodeSet))(struct _xmlXPathParserContext *, int);
typedef void (__cdecl typeof(xmlXPathFunction))(struct _xmlXPathParserContext *, int);
typedef int (__cdecl typeof(xsltRegisterExtModuleFunction))(const unsigned char *, const unsigned char *, typeof(xmlXPathFunction));
typedef struct _xmlDoc * (__cdecl typeof(xsltDocLoaderFunc))(const unsigned char *, struct _xmlDict *, int, void *, enum xsltLoadType);
typedef void (__cdecl typeof(xsltSetLoaderFunc))(typeof(xsltDocLoaderFunc));
typedef struct jpeg_error_mgr * (__cdecl typeof(jpeg_std_error))(struct jpeg_error_mgr *);
typedef void (__cdecl typeof(jpeg_CreateDecompress))(struct jpeg_decompress_struct *, int, __typeof_size);
typedef int (__cdecl typeof(jpeg_read_header))(struct jpeg_decompress_struct *, int);
typedef int (__cdecl typeof(jpeg_start_decompress))(struct jpeg_decompress_struct *);
typedef unsigned int (__cdecl typeof(jpeg_read_scanlines))(struct jpeg_decompress_struct *, unsigned char **, unsigned int);
typedef int (__cdecl typeof(jpeg_finish_decompress))(struct jpeg_decompress_struct *);
typedef void (__cdecl typeof(jpeg_destroy_decompress))(struct jpeg_decompress_struct *);

#undef __typeof_intptr
#undef __typeof_longptr
#undef __typeof_wchar
#undef __typeof_size

/* EOF */

