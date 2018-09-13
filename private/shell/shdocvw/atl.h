
STDAPI_(void) AtlInit(HINSTANCE hinst);
STDAPI_(void) AtlTerm();

STDAPI        AtlGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv);
STDAPI_(LONG) AtlGetLockCount();
