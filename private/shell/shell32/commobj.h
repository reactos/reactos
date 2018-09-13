//
// common object helper stuff (not to be confused with OLE common object modle)
//
//

#define IToCommonUnknown(p) (CCommonUnknown *)((LPBYTE)OFFSETOF(p) - ((CommonKnownHelper *)OFFSETOF(p))->nOffset)

typedef struct
{
    IUnknown unk;
    int cRef;
} CCommonUnknown;

typedef struct
{
    IUnknown unk;
    int nOffset;
} CommonKnownHelper;

STDMETHODIMP Common_QueryInterface(void * punk, REFIID riid, void **ppvObj);
STDMETHODIMP_(ULONG) Common_AddRef(void * punk);
STDMETHODIMP_(ULONG) Common_Release(void * punk);

#define DEFKNOWNCLASS(_interface) \
typedef struct                    \
{                                 \
    I##_interface unk;            \
    int           nOffset;        \
} CKnown##_interface              \

//
//  By using following CKnownXX classes we can initialize Vtables
// without casting them.
//
DEFKNOWNCLASS(ShellFolder);
DEFKNOWNCLASS(ContextMenu);
DEFKNOWNCLASS(ShellView);
DEFKNOWNCLASS(ShellExtInit);
DEFKNOWNCLASS(ShellPropSheetExt);
DEFKNOWNCLASS(ShellBrowser);
DEFKNOWNCLASS(DropTarget);

