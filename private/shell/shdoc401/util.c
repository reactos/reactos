#include "priv.h"

DWORD
GetUrlSchemeA(IN LPCSTR pcszUrl)
{
    if(pcszUrl)
    {
        PARSEDURLA pu;
        pu.cbSize = SIZEOF(pu);
        if(SUCCEEDED(ParseURLA(pcszUrl, &pu)))
            return pu.nScheme;
    }
    return URL_SCHEME_INVALID;
}


DWORD
GetUrlSchemeW(IN LPCWSTR pcszUrl)
{
    if(pcszUrl)
    {
        PARSEDURLW pu;
        pu.cbSize = SIZEOF(pu);
        if(SUCCEEDED(ParseURLW(pcszUrl, &pu)))
            return pu.nScheme;
    }
    return URL_SCHEME_INVALID;
}


