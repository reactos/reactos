#ifndef I_TRIAPI_HXX_
#define I_TRIAPI_HXX_
#pragma INCMSG("--- Beg 'triapi.hxx'")


MtExtern(CTridentAPI)

class CTridentAPI : public ITridentAPI
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTridentAPI))

    CTridentAPI();

    DECLARE_FORMS_STANDARD_IUNKNOWN(CTridentAPI);

    // *** ITridentAPI ***
    STDMETHODIMP ShowHTMLDialog(
                                HWND hwndParent,
                                IMoniker *pMk,
                                VARIANT *pvarArgIn,
                                WCHAR *pchOptions,
                                VARIANT *pvarArgOut,
                                IUnknown *punkHost
                                );

private:
    ~CTridentAPI() {}
};

#pragma INCMSG("--- End 'triapi.hxx'")
#else
#pragma INCMSG("*** Dup 'triapi.hxx'")
#endif

