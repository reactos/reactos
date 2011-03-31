//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    audfilter.h
//
// Abstract:    
//      This is the header file for C++ classes that expose
//      functionality of KS filters that support PCM audio formats 
//

#pragma once
#ifndef __KSAUDFILTER_H
#define __KSAUDFILTER_H

// Forward Declarations
class CKsAudPin;
class CKsAudRenPin;
class CKsAudCapPin;

////////////////////////////////////////////////////////////////////////////////
//
//  class CKsAudFilter
//
//  Class Description:
//      This is the base class for a Audio Render Filter.
//
//
//

class CKsAudFilter : public CKsFilter
{
public:
    virtual HRESULT EnumeratePins(void);
    
protected:
    CKsAudFilter
    (
        LPCTSTR  pszName,
        HRESULT  *phr
    );
};


////////////////////////////////////////////////////////////////////////////////
//
//  class CKsAudRenFilter
//
//  Class Description:
//      This is the base class for a Audio Render Filter.
//
//
//

class CKsAudRenFilter : public CKsAudFilter
{
public:
    CKsAudRenFilter
    (
        LPCTSTR  pszName,
        HRESULT  *phr
    );

    CKsAudRenPin* CreateRenderPin(const WAVEFORMATEX* pwfx, BOOL fLooped);
    BOOL            CanCreateRenderPin(const WAVEFORMATEX* pwfx);
private:
    CKsAudRenPin* FindViablePin(const WAVEFORMATEX* pwfx);
};


////////////////////////////////////////////////////////////////////////////////
//
//  class CKsAudCapFilter
//
//  Class Description:
//      This is the base class for an Audio Capture Filter.
//
//
//

class CKsAudCapFilter : public CKsAudFilter
{
public:
    CKsAudCapFilter
    (
        LPCTSTR  pszName,
        HRESULT* phr
    );

    HRESULT GetCapturePin(CKsAudCapPin** ppPin);

    CKsAudCapPin* CreateCapturePin(WAVEFORMATEX* pwfx, BOOL fLooped);

private:
    CKsAudCapPin* FindViablePin(WAVEFORMATEX* pwfx);
};

#endif //__KSAUDFILTER_H
