
#define typeof(X_) __typeof_ ## X_

struct IMILUnknown1;

#define WINAPI __stdcall
#define HRESULT int
#define IMILUnknown1 struct IMILUnknown1

typedef void (WINAPI typeof(IMILUnknown1Impl_unknown1))(IMILUnknown1 *iface, void *arg);
typedef HRESULT (WINAPI typeof(IMILUnknown1Impl_unknown3))(IMILUnknown1 *iface, void *arg);
typedef HRESULT (WINAPI typeof(IMILUnknown1Impl_unknown8))(IMILUnknown1 *iface);

#undef WINAPI
#undef HRESULT
#undef IMILUnknown1
