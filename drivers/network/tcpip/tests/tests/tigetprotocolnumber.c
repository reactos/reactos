#include "../../include/precomp.h"
#include "regtests.h"

static void RunTest() {
    UNICODE_STRING Str;
    int Proto;
    RtlInitUnicodeString( &Str, L"1" );
    TiGetProtocolNumber( &Str, (PULONG)&Proto );
    _AssertEqualValue(1, Proto);
}

_Dispatcher(TigetprotocolnumberTest, "TiGetProtocolNumber");
