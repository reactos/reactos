#define SZ_SERVER_CLSID TEXT("{3050f4d8-98B5-11CF-BB82-00AA00BDCE0B}")
#define SZ_APPLICATION_NAME TEXT("HTML Application Host")
#define SZ_APPLICATION_VERSION TEXT("1.0")
#define SZ_APPLICATION_WNDCLASS SZ_APPLICATION_NAME TEXT(" Window Class")
#define SZ_APPLICATION_HIDDENWNDCLASS SZ_APPLICATION_NAME TEXT(" Hidden Window Class")

#define SZ_APPLICATION_BEHAVIORCSS TEXT("HTA\\:APPLICATION {behavior:url(#default#APPLICATION)}")
#define SZ_APPLICATION_BEHAVIORNAMESPACE TEXT("HTA")

// Registry key/value information

#define SZ_REG_FILE_EXT TEXT(".hta")
#define SZ_REG_PROGID TEXT("htafile")
#define SZ_REG_CONTENT_TYPE TEXT("application/hta")
#define SZ_REG_FILE_READABLE_STRING TEXT("HTML Application")
#define SZ_REG_DEF_ICON_ID TEXT(",1")

#define TEST(hr) if ((hr)) goto Cleanup;
#define TESTREG(lRet) if ((lRet) != ERROR_SUCCESS) goto Cleanup;
