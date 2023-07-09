NTSTATUS
NTAPI
NtWow64CsrBasepNlsSetUserInfo(
    IN LCTYPE   LCType,
    IN LPWSTR pData,
    IN ULONG DataLength
    );

NTSTATUS
NTAPI
NtWow64CsrBasepNlsSetMultipleUserInfo(
    IN DWORD dwFlags,
    IN int cchData,
    IN LPCWSTR pPicture,
    IN LPCWSTR pSeparator,
    IN LPCWSTR pOrder,
    IN LPCWSTR pTLZero,
    IN LPCWSTR pTimeMarkPosn
    );


NTSTATUS
NTAPI
NtWow64CsrBasepNlsCreateSection(
    IN UINT uiType,
    IN LCID Locale,
    OUT PHANDLE phSection
    );
