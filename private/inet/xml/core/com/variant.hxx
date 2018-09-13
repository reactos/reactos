/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _VARIANT_HXX
#define _VARIANT_HXX

DEFINE_CLASS(Variant);

class Variant : public Base
{    
    DECLARE_CLASS_MEMBERS(Variant, Base);

    public: Variant();

    public: Variant(int i);

    public: Variant(String * s);

    protected: virtual void finalize();

    public: String * toString();

    public: Object * toObject();

    public:    VARIANT    variant;

    public: static IUnknown * QIForIID( VARIANT *, const IID * piid);

    public: static IUnknown* getUnknown(VARIANT* varObject);

};

#endif _VARIANT_HXX
