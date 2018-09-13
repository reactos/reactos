//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       pertrust.h
//
//--------------------------------------------------------------------------

//
// PersonalTrustDB.h
// (pertrust.h)
//
// Interface to the personal trust database manager

#define IID_IPersonalTrustDB_Data { 0x4001b231, 0x8d76, 0x11cf, { 0xae, 0xce, 0x0, 0xaa, 0x0, 0x6c, 0x37, 0x6 } }
extern "C" const GUID IID_IPersonalTrustDB;


typedef struct TRUSTLISTENTRY
    {
    TCHAR               szToken[MAX_PATH];  // the name of this certificate
    LONG                iLevel;             // the level at which this fellow lives in the hierarchy
    TCHAR               szDisplayName[64];  // the display name to show in the UI
    } TRUSTLISTENTRY;

#undef  INTERFACE
#define INTERFACE IPersonalTrustDB

DECLARE_INTERFACE_(IPersonalTrustDB, IUnknown)
    {
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    //
    // Answer whether the indicated certificate is trusted at the 
    // indicated level of the certificate chain.
    //
    //      S_OK     == yes
    //      S_FALSE  == no
    //      other    == error, can't tell
    //
    STDMETHOD(IsTrustedCert)(DWORD dwEncoding, PCCERT_CONTEXT pCert, LONG iLevel, BOOL fCommercial) PURE;

    //
    // Add the given certificate to the trust data base
    //
    STDMETHOD(AddTrustCert)(PCCERT_CONTEXT pCert,       LONG iLevel, BOOL fLowerLevelsToo) PURE;

    //
    // Remove the given certificate from the trust data base
    //
    STDMETHOD(RemoveTrustCert)(PCCERT_CONTEXT pCert,       LONG iLevel, BOOL fLowerLevelsToo) PURE;
    STDMETHOD(RemoveTrustToken)(THIS_ LPTSTR szToken,   LONG iLevel, BOOL fLowerLevelsToo) PURE;

    //
    // Return the list of trusted entitities
    //
    STDMETHOD(GetTrustList)(THIS_ 
        LONG                iLevel,             // the cert chain level to get
        BOOL                fLowerLevelsToo,    // included lower levels, remove duplicates
        TRUSTLISTENTRY**    prgTrustList,       // place to return the trust list
        ULONG*              pcTrustList         // place to return the size of the returned trust list
        ) PURE;

    //
    // Answer whether commercial publishers are trusted
    //
    //      S_OK     == yes
    //      S_FALSE  == no
    //      other    == error, can't tell
    STDMETHOD(AreCommercialPublishersTrusted)(THIS) PURE;

    //
    // Set the commercial publisher trust setting
    //
    STDMETHOD(SetCommercialPublishersTrust)(THIS_ BOOL fTrusted) PURE;

    };

//
// Creation function for default implementation
//
HRESULT OpenTrustDB(IUnknown* punkOuter, REFIID iid, void** ppv);

