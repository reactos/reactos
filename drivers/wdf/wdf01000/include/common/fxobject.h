/*
        IMPORTANT: Common code must call Initialize method of
        FxObject before using it

        Cannot put CreateAndInitialize method on this class as it
        cannot be instantiated
*/

#ifndef _FXOBJECT_H_
#define _FXOBJECT_H_

class FxObject {

public:
    //
    // Request that an object be deleted.
    //
    // This can be the result of a WDF API or a WDM event.
    //
    virtual
    VOID
    DeleteObject(
        VOID
        );
};

#endif //_FXOBJECT_H_