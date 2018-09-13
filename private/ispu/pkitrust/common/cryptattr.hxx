//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       CryptAttr.hxx
//
//  History:    31-Mar-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef CRYPTATTR_HXX
#define CRYPTATTR_HXX

#include    "Stack.hxx"

class CryptAttribute_
{
    public:
        CryptAttribute_(void);
        virtual ~CryptAttribute_(void);

        CRYPT_ATTRIBUTE     *Get(void);

        BOOL                Fill(DWORD cbAttributeData, BYTE *pbAttributeData, char *pszObjId);

    private:
        CRYPT_ATTRIBUTE     sAttribute;
};


class CryptAttributes_
{
    public:
        CryptAttributes_(void);
        virtual ~CryptAttributes_(void);

        BOOL                Add(CryptAttribute_ *pcCryptAttr);
        CRYPT_ATTRIBUTES    *Get(void);

    private:
        Stack_              sStack;
};

#endif // CRYPTATTR_HXX
