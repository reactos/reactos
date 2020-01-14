#ifndef _FXSTRING_H_
#define _FXSTRING_H_

#include "common/fxobject.h"

class FxString : public FxObject {
public:
    //
    // Length describes the length of the string in bytes (not WCHARs)
    // MaximumLength describes the size of the buffer in bytes
    //
    UNICODE_STRING m_UnicodeString;
};

#endif //_FXSTRING_H_
