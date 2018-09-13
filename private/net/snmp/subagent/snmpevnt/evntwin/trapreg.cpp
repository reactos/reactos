//***********************************************************************
// trapreg.cpp
//
// This file contains the implementation of the classes for the objects
// that are read from the registry, manipulated and written back to the
// registry.
//
// Author: Larry A. French
//
// History:
//      20-Febuary-1996     Larry A. French
//          Totally rewrote it to fix the spagetti code and huge
//          methods.  The original author seemed to have little or
//          no ability to form meaningful abstractions.
//
//
// Copyright (C) 1995, 1996 Microsoft Corporation.  All rights reserved.
//
//************************************************************************
//
// Some of the interesting class implementations contained here are:
//
// CTrapReg
//      This is the container class for the registry information.  It is
//      composed of the configuration "parameters" and an EventLog array.
//
// CXEventLogArray
//      This class implements an array of CXEventLog objects, where the
//      event logs are "application", "security", "system" and so on.
//
// CXEventLog
//      This class implements a single event log.  All information
//      relevent to an event log can be accesssed through this class.
//
// CXEventSourceArray
//      Each event log contains an event source array.  The event source
//      represents an application that can generate an Event.
//
// CXEventSource
//      An eventsource represents an application that can generate some
//      number of event-log events.  The event source contains a CXEventArray
//      and CXMessageArray.  The CXEventArray contains all the events
//      that will be converted to traps.  The CXMessageArray contains all the
//      possible messages that a particular event source can generate.
//
// CXMessageArray
//      This class implements an array of CXMessage objects.
//
// CXMessage
//      This class contains all the information relevent to a message
//      that a message source can generate.
//
//
// CXEventArray
//      This class implements an array of CXEvent objects.
//
// CXEvent
//      This class represents an event that the user has selected to be
//      converted to a trap.  The event contains a message plus some
//      additional information.
//
//**************************************************************************
// The Registry:
//
// These classes are loaded from the registry and written back to the
// registry when the user clicks OK.  To understand the format of the
// registry, use the registry editor while looking though the "Serialize"
// and "Deserialize" member function for each of these classes.
//**************************************************************************


#include "stdafx.h"
#include "trapreg.h"
#include "regkey.h"
#include "busy.h"
#include "utils.h"
#include "globals.h"
#include "utils.h"
#include "dlgsavep.h"
#include "remote.h"

///////////////////////////////////////////////////////////////////
// Class: CBaseArray
//
// This class extends the CObArray class by adding the DeleteAll
// method.
//
//////////////////////////////////////////////////////////////////

//****************************************************************
// CBaseArray::DeleteAll
//
// Delete all the objects contained in this array.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//****************************************************************
void CBaseArray::DeleteAll()
{
    LONG nObjects = (LONG)GetSize();
    for (LONG iObject = nObjects-1; iObject >= 0; --iObject) {
        CObject* pObject = GetAt(iObject);
        delete pObject;
    }

    RemoveAll();
}


/////////////////////////////////////////////////////////////////////////////////////
// Class: CTrapReg
//
// This is the container class for all the registry information for eventrap.exe.
//
////////////////////////////////////////////////////////////////////////////////////
CTrapReg::CTrapReg() : m_pdlgLoadProgress(FALSE), m_pbtnApply(FALSE)
{
    m_bNeedToCloseKeys = FALSE;
    m_pdlgSaveProgress = NULL;
    m_pdlgLoadProgress = NULL;
    m_bDidLockRegistry = FALSE;
    m_bRegIsReadOnly = FALSE;
    SetDirty(FALSE);
    m_nLoadSteps = LOAD_STEPS_IN_TRAPDLG;

    m_bShowConfigTypeBox = TRUE;
    m_dwConfigType = CONFIG_TYPE_CUSTOM;
}

CTrapReg::~CTrapReg()
{
    delete m_pdlgSaveProgress;
    delete m_pdlgLoadProgress;


    if (!g_bLostConnection) {
        if (m_bDidLockRegistry) {
            UnlockRegistry();
        }

        if (m_bNeedToCloseKeys) {
            m_regkeySource.Close();
            m_regkeySnmp.Close();
            m_regkeyEventLog.Close();
        }
    }
}




//*********************************************************************************
// CTrapReg::SetConfigType
//
// Set the configuration type to CONFIG_TYPE_CUSTOM or CONFIG_TYPE_DEFAULT
// When the configuration type is changed, the change is reflected in the
// registry immediately so that the config tool can know whether or not it
// should update the event to trap configuration.
//
// Parameters:
//      DWORD dwConfigType
//          This parameter must be CONFIG_TYPE_CUSTOM or CONFIG_TYPE_DEFAULT.
//
// Returns:
//      SCODE
//          S_OK if the configuration type was set, otherwise E_FAIL.
//
//*********************************************************************************
SCODE CTrapReg::SetConfigType(DWORD dwConfigType)
{
    ASSERT(dwConfigType==CONFIG_TYPE_CUSTOM || dwConfigType==CONFIG_TYPE_DEFAULT_PENDING);
    if (dwConfigType != m_dwConfigType) {
        SetDirty(TRUE);
    }
    m_dwConfigType = dwConfigType;
    return S_OK;
}





//*********************************************************************************
// CTrapReg::LockRegistry
//
// Lock the registry to prevent two concurrent edits of the event-to-trap configuration
// information.
//
// Parameters:
//      None.
//
// Returns:
//      SCODE
//          S_OK if successful.
//          E_FAIL if the configuration information was already locked.
//          E_REGKEY_NO_CREATE if the "CurrentlyOpen" registry key can't
//          be created.
//
//**********************************************************************************
SCODE CTrapReg::LockRegistry()
{
    if (g_bLostConnection) {
        return E_REGKEY_LOST_CONNECTION;
    }

    CRegistryKey regkey;
    if (m_regkeyEventLog.GetSubKey(SZ_REGKEY_CURRENTLY_OPEN, regkey)) {
        if (g_bLostConnection) {
            return E_REGKEY_LOST_CONNECTION;
        }

        if (AfxMessageBox(IDS_ERR_REGISTRY_BUSY, MB_YESNO | MB_ICONSTOP | MB_DEFBUTTON2) == IDNO)
        {
            regkey.Close();
            return E_FAIL;
        }
    }


    // Create the "CurrentlyOpen" key as a volatile key so that it will disappear the next
    // time the machine is restarted in the event that the application that locked the
    // event-to-trap configuration crashed before it could clear this lock.
    if (!m_regkeyEventLog.CreateSubKey(SZ_REGKEY_CURRENTLY_OPEN, regkey, NULL, NULL, TRUE)) {
        if (g_bLostConnection) {
            return E_REGKEY_LOST_CONNECTION;
        }

        AfxMessageBox(IDS_WARNING_CANT_WRITE_CONFIG, MB_OK | MB_ICONSTOP);
        return E_REGKEY_NO_CREATE;
    }
    regkey.Close();
    m_bDidLockRegistry = TRUE;
    return S_OK;
}



//***********************************************************************
// CTrapReg::UnlockRegistry
//
// Unlock the event-to-trap configuration so that others can edit it.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//***********************************************************************
void CTrapReg::UnlockRegistry()
{
    m_regkeyEventLog.DeleteSubKey(SZ_REGKEY_CURRENTLY_OPEN);
}




 	

//***********************************************************************
// CTrapReg::Connect
//
// Connect to a registry.  The registry may exist on a remote computer.
//
// Parameters:
//      LPCTSTR pszComputerName
//          The computer who's registry you want to edit.  An empty string
//          specifies a request to connect to the local machine.
//
// Returns:
//      SCODE
//          S_OK if the connection was made.
//          E_FAIL if an error occurred.  In this event, the appropriate
//          error message boxes will have already been displayed.
//
//***********************************************************************
SCODE CTrapReg::Connect(LPCTSTR pszComputerName, BOOL bIsReconnecting)
{
    SCODE sc;

    g_bLostConnection = FALSE;

    if (pszComputerName) {
        m_sComputerName = pszComputerName;
    }

    // There are eight steps here, plus there are three initial steps in
    // CTrapReg::Deserialize.  After that the step count will be reset
    // and then stepped again for each log where each log will have
    // ten sub-steps.
    if (!bIsReconnecting) {
        m_pdlgLoadProgress->SetStepCount(LOAD_STEP_COUNT);
    }

    CRegistryValue regval;
    CRegistryKey regkeyEventLog;


    if (m_regkeySource.Connect(pszComputerName) != ERROR_SUCCESS) {
        if (m_regkeySource.m_lResult == ERROR_ACCESS_DENIED) {
            AfxMessageBox(IDS_ERR_REG_NO_ACCESS, MB_OK | MB_ICONSTOP);
            return E_ACCESS_DENIED;
        }
        goto CONNECT_FAILURE;
    }

    if (!bIsReconnecting) {
        if (m_pdlgLoadProgress->StepProgress()) {
            return S_LOAD_CANCELED;
        }
        ++m_nLoadSteps;
    }

    if (m_regkeySnmp.Connect(pszComputerName) != ERROR_SUCCESS) {
        if (m_regkeySnmp.m_lResult == ERROR_ACCESS_DENIED) {
            AfxMessageBox(IDS_ERR_REG_NO_ACCESS, MB_OK | MB_ICONSTOP);
            return E_ACCESS_DENIED;
        }
        goto CONNECT_FAILURE;
    }
    if (!bIsReconnecting) {
        if (m_pdlgLoadProgress->StepProgress()) {
            return S_LOAD_CANCELED;
        }
        ++m_nLoadSteps;
    }


    // SOFTWARE\\Microsoft\\SNMP_EVENTS
    if (m_regkeySnmp.Open(SZ_REGKEY_SNMP_EVENTS, KEY_READ | KEY_WRITE | KEY_CREATE_SUB_KEY) != ERROR_SUCCESS) {
        if (m_regkeySnmp.Open(SZ_REGKEY_SNMP_EVENTS, KEY_READ) == ERROR_SUCCESS) {
            m_bRegIsReadOnly = TRUE;
        }
        else {
            // At this point we know the SNMP_EVENTS key could not be opened.  This
            // could either be because we don't have access to the registry or we
            // weren't installed yet. We now check to see if we can access the
            // registry at all.
            CRegistryKey regkeyMicrosoft;
            if (regkeyMicrosoft.Open(SZ_REGKEY_MICROSOFT, KEY_READ) == ERROR_SUCCESS) {
                regkeyMicrosoft.Close();
                AfxMessageBox(IDS_ERR_NOT_INSTALLED, MB_OK | MB_ICONSTOP);
            }
            else {
                // We couldn't even access SOFTWARE\Microsoft, so we know that
                // we don't have access to the registry.
                AfxMessageBox(IDS_ERR_REG_NO_ACCESS, MB_OK | MB_ICONSTOP);
                return E_ACCESS_DENIED;
            }
        }
        return E_FAIL;

    }
    if (!bIsReconnecting) {
        if (m_pdlgLoadProgress->StepProgress()) {
            return S_LOAD_CANCELED;
        }
        ++m_nLoadSteps;
    }


    //  SYSTEM\\CurrentControlSet\\Services\\EventLog
    if (m_regkeySource.Open(SZ_REGKEY_SOURCE_EVENTLOG, KEY_ENUMERATE_SUB_KEYS | KEY_READ | KEY_QUERY_VALUE ) != ERROR_SUCCESS) {
        m_regkeySnmp.Close();
        AfxMessageBox(IDS_ERR_REG_NO_ACCESS, MB_OK | MB_ICONSTOP);
        return E_ACCESS_DENIED;
    }

    if (!bIsReconnecting) {
        if (m_pdlgLoadProgress->StepProgress()) {
            return S_LOAD_CANCELED;
        }
        ++m_nLoadSteps;
    }



    if (!m_regkeySnmp.GetSubKey(SZ_REGKEY_EVENTLOG, m_regkeyEventLog)) {
        if (m_regkeySnmp.m_lResult == ERROR_ACCESS_DENIED) {
            AfxMessageBox(IDS_ERR_REG_NO_ACCESS, MB_OK | MB_ICONSTOP);
            sc = E_ACCESS_DENIED;
        }
        else {
            AfxMessageBox(IDS_WARNING_CANT_READ_CONFIG, MB_OK | MB_ICONSTOP);
            sc = E_REGKEY_NOT_FOUND;
        }
        m_regkeySnmp.Close();
        m_regkeySource.Close();
        return sc;
    }

    if (!bIsReconnecting) {
        if (m_pdlgLoadProgress->StepProgress()) {
            return S_LOAD_CANCELED;
        }
        ++m_nLoadSteps;
    }

    m_bNeedToCloseKeys = TRUE;

    sc = LockRegistry();

    if (FAILED(sc)) {
        if (sc == E_REGKEY_LOST_CONNECTION) {
            return sc;
        }
        else {
            return E_REGKEY_NO_CREATE;
        }
    }
    if (!bIsReconnecting) {
        if (m_pdlgLoadProgress->StepProgress()) {
            return S_LOAD_CANCELED;
        }
        ++m_nLoadSteps;
    }

    if (!bIsReconnecting) {
        if (m_pdlgLoadProgress->StepProgress()) {
            return S_LOAD_CANCELED;
        }
        ++m_nLoadSteps;
    }

    m_bShowConfigTypeBox = TRUE;

    if (FAILED(sc)) {
        if (sc == E_ACCESS_DENIED) {
            AfxMessageBox(IDS_ERR_REG_NO_ACCESS, MB_OK | MB_ICONSTOP);
            return E_ACCESS_DENIED;
        }
        else {
            goto CONNECT_FAILURE;
        }
    }
    if (!bIsReconnecting) {
        if (m_pdlgLoadProgress->StepProgress()) {
            return S_LOAD_CANCELED;
        }
        ++m_nLoadSteps;
    }

    return S_OK;

CONNECT_FAILURE:
        CString sMessage;
        sMessage.LoadString(IDS_CANTCONNECT);
        if (pszComputerName != NULL) {
            sMessage += pszComputerName;
        }
        AfxMessageBox((LPCTSTR) sMessage, MB_OK | MB_ICONSTOP);
        return E_FAIL;
}


//****************************************************************************
// CTrapReg::BuildSourceHasTrapsMap
//
// This method fills the m_mapEventSources CMapStringToPtr object with the
// names of all the event sources that actually have events configured for them.
// When this map is used later, we only need to know whether or not a particular
// entry exists in the map, so the value associated with each entry is irrelevant.
//
// Why do we need m_mapEventSources?  The reason is that we need a quick way to
// determine whether or not a particular source has events configured for it.
// This is used when all the event sources are being enumerated and we need to know
// whether or not to load the messages for the event source (an expensive operation).
// If a particular event source has events configured for it, then we need to load
// the messages so that the message text can be displayed.  This is because the
// event configuration stored in the registry only contains the event id and not the
// message text.
//
// Parameters:
//      None.
//
// Returns:
//      SCODE
//          S_OK if successful, otherwise E_FAIL.
//
//******************************************************************************
SCODE CTrapReg::BuildSourceHasTrapsMap()
{


    CRegistryKey regkey;
    if (!g_reg.m_regkeySnmp.GetSubKey(SZ_REGKEY_SOURCES, regkey)) {
        // For a fresh installation, there is no source subkey.
        if (g_bLostConnection) {
            return E_REGKEY_LOST_CONNECTION;
        }
        return S_OK;
    }

    CStringArray* pasEventSources = regkey.EnumSubKeys();
    regkey.Close();

    if (pasEventSources == NULL) {
        if (g_bLostConnection) {
            return E_REGKEY_LOST_CONNECTION;
        }
        return S_OK;
    }

    CString sEventSource;
    LONG nEventSources = (LONG)pasEventSources->GetSize();
    for (LONG iEventSource = 0; iEventSource < nEventSources; ++iEventSource) {
        sEventSource = pasEventSources->GetAt(iEventSource);
		sEventSource.MakeUpper();
        m_mapSourceHasTraps.SetAt(sEventSource, NULL);
    }
    delete pasEventSources;
    return S_OK;
}


//**************************************************************************
// CTrapReg::Deserialize
//
// Read all the registry information (not including the event source messages) that
// is required by eventrap.exe into this object.  Reading the messages for most
// event sources is delayed until the user actually requests it by selecting
// an event source in the event source tree control.  If an event source has
// events that are being mapped into traps, then the messages for that event
// source are loaded because an event description in the registry does not contain
// the message text.
//
// Parameters:
//      None.
//
// Returns:
//      SCODE
//          S_OK if successful.
//          E_FAIL if a failure was detected. In the event of a failure, all
//          of the appropriate message boxes will have been displayed.
//
//***************************************************************************
SCODE CTrapReg::Deserialize()
{
    m_bSomeMessageWasNotFound = FALSE;
    SetDirty(FALSE);

    // Get the value for the configuration type.
    CRegistryValue regval;
    if (m_regkeyEventLog.GetValue(SZ_NAME_REGVAL_CONFIGTYPE, regval)) {
        m_dwConfigType = *(DWORD*)regval.m_pData;
    }
    else {
        if (g_bLostConnection) {
            AfxMessageBox(IDS_ERROR_NOT_RESPONDING);
            return E_REGKEY_LOST_CONNECTION;
        }

        // If the config type value doesn't exist, assume a custom configuration.
        // This can happen because the setup program doesn't necessarily create
        // this value.
        m_dwConfigType = CONFIG_TYPE_CUSTOM;
    }
    if (m_pdlgLoadProgress->StepProgress()) {
        return S_LOAD_CANCELED;
    }
    ++m_nLoadSteps;


    SCODE sc = BuildSourceHasTrapsMap();
    if (SUCCEEDED(sc)) {
        if (m_pdlgLoadProgress->StepProgress()) {
            return S_LOAD_CANCELED;
        }
        ++m_nLoadSteps;

        // Load the event log list, the current event list and so on.
        sc = m_params.Deserialize();
        if (sc == S_LOAD_CANCELED) {
            return sc;
        }

        if (SUCCEEDED(sc)) {
            if (m_pdlgLoadProgress->StepProgress()) {
                return S_LOAD_CANCELED;
            }
            ++m_nLoadSteps;

            sc = m_aEventLogs.Deserialize();
            if (sc == S_LOAD_CANCELED) {
                return sc;
            }

            if (SUCCEEDED(sc)) {
                if (m_nLoadSteps < LOAD_STEP_COUNT) {
                    if (m_pdlgLoadProgress->StepProgress(LOAD_STEP_COUNT - m_nLoadSteps)) {
                        return S_LOAD_CANCELED;
                    }
                }
            }
        }
    }


    if (FAILED(sc)) {
        if (sc == E_REGKEY_LOST_CONNECTION) {
            AfxMessageBox(IDS_ERROR_NOT_RESPONDING);
        }
        else {
            AfxMessageBox(IDS_WARNING_CANT_READ_CONFIG);
        }

    }
    return sc;
}



//**************************************************************************
// CTrapReg::GetSaveProgressStepCount
//
// Get the number of steps for the save progress dialog.  The number of steps
// is the number of events that will be written to SNMP_EVENTS\EventLog in
// the registry.
//
// Parameters:
//      None.
//
// Returns:
//      The number of steps to use for the save progress dialog.
//
//*************************************************************************
LONG CTrapReg::GetSaveProgressStepCount()
{
    LONG nSteps = 0;
    LONG nEventLogs = m_aEventLogs.GetSize();
    for (LONG iEventLog = 0; iEventLog < nEventLogs; ++iEventLog) {
        CXEventLog* pEventLog = m_aEventLogs[iEventLog];

        LONG nEventSources = pEventLog->m_aEventSources.GetSize();
        for (LONG iEventSource = 0; iEventSource < nEventSources; ++iEventSource) {
            CXEventSource* pEventSource = pEventLog->m_aEventSources.GetAt(iEventSource);
            nSteps += pEventSource->m_aEvents.GetSize();
        }
    }
    return nSteps;
}


//**************************************************************************
// CTrapReg::Serialize
//
// Write eventrap's current configuration out to the registry.
//
// Parameters:
//      None.
//
// Returns:
//      SCODE
//          S_OK if successful.
//          E_FAIL if a failure was detected.  In the event of a failure, all
//          of the appropriate message boxes will have been displayed.
//
//***************************************************************************
SCODE CTrapReg::Serialize()
{
    SCODE sc;
    if (g_bLostConnection) {
        sc = Connect(m_sComputerName, TRUE);
        if (FAILED(sc)) {
            if (g_bLostConnection) {
                AfxMessageBox(IDS_ERROR_NOT_RESPONDING);
                return E_REGKEY_LOST_CONNECTION;
            }
            return S_SAVE_CANCELED;
        }
    }

    if (!m_bIsDirty) {
        // The configuration state was not changed, so there is nothing to do.
        return S_OK;
    }

    LONG nProgressSteps = GetSaveProgressStepCount();
    if (nProgressSteps > 0) {
        m_pdlgSaveProgress = new CDlgSaveProgress;
        m_pdlgSaveProgress->Create(IDD_SAVE_PROGRESS);

        m_pdlgSaveProgress->SetStepCount( nProgressSteps );
    }

    CRegistryValue regval;
    regval.Set(SZ_NAME_REGVAL_CONFIGTYPE, REG_DWORD, sizeof(DWORD), (LPBYTE)&m_dwConfigType);
    if (!m_regkeyEventLog.SetValue(regval)) {
        if (g_bLostConnection) {
            AfxMessageBox(IDS_ERROR_NOT_RESPONDING);
            sc = E_REGKEY_LOST_CONNECTION;
        }
        else {
            AfxMessageBox(IDS_WARNING_CANT_WRITE_CONFIG);
            sc = S_SAVE_CANCELED;
        }
    }
    else {
        sc = m_aEventLogs.Serialize();
        if (sc != S_SAVE_CANCELED) {

            if (SUCCEEDED(sc)) {
                sc = m_params.Serialize();
            }

            if (sc != S_SAVE_CANCELED)
                SetDirty(FALSE);

            if (FAILED(sc)) {
                if (g_bLostConnection) {
                    AfxMessageBox(IDS_ERROR_NOT_RESPONDING);
                }
                else {
                    AfxMessageBox(IDS_WARNING_CANT_WRITE_CONFIG);
                }
            }
        }
    }

    delete m_pdlgSaveProgress;
    m_pdlgSaveProgress = NULL;
    return sc;
}


void CTrapReg::SetDirty(BOOL bDirty)
{
    m_bIsDirty = bDirty;
    if (m_pbtnApply)
    {
        m_pbtnApply->EnableWindow(m_bIsDirty);
    }
}


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
// Class: CTrapParams
//
// This class represents the information stored in the
// SNMP_EVENTS\EventLog\Parameters registry key.
//
// Question:  Why is it that the horizontal space in the gap between
// the lines at the top of this header appears to be very irregular?
//////////////////////////////////////////////////////////////////


//****************************************************************
// CTrapParams::CTrapParams
//
// Constructor for CTrapParams.
//
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//****************************************************************
CTrapParams::CTrapParams()
{
    m_trapsize.m_bTrimFlag = TRUE;
    m_trapsize.m_dwMaxTrapSize = 4096;
    m_trapsize.m_bTrimMessages = FALSE;
}



//********************************************************************
// CTrapParams::Deserialize
//
// Read the contents of this CTrapParams object from the registry.
//
// Parameters:
//      None.
//
// Returns:
//      SCODE
//          S_OK if successful.
//          E_FAIL if there was a problem reading the required information
//          from the registry.
//********************************************************************
SCODE CTrapParams::Deserialize()
{
    CRegistryKey regkeyParams;
    if (!g_reg.m_regkeySnmp.GetSubKey(SZ_REGKEY_PARAMETERS, regkeyParams)) {
        if (g_bLostConnection) {
            return E_REGKEY_LOST_CONNECTION;
        }
        else {
            return E_REGKEY_NOT_FOUND;
        }
    }


    CRegistryValue regval;

    // !!!CR: There is no longer any reason to load the BASE OID
    if (!regkeyParams.GetValue(SZ_REGKEY_PARAMS_BASE_ENTERPRISE_OID, regval))
        goto REGISTRY_FAILURE;
    m_sBaseEnterpriseOID = (LPCTSTR)regval.m_pData;

    if (!regkeyParams.GetValue(SZ_REGKEY_PARAMS_TRIMFLAG, regval))
        m_trapsize.m_bTrimFlag = FALSE;
    else
        m_trapsize.m_bTrimFlag = (*(DWORD*)regval.m_pData == 1);

    if (!regkeyParams.GetValue(SZ_REGKEY_PARAMS_MAXTRAP_SIZE, regval))
        m_trapsize.m_dwMaxTrapSize = MAX_TRAP_SIZE;
    else
        m_trapsize.m_dwMaxTrapSize = *(DWORD*)regval.m_pData;

    if (!regkeyParams.GetValue(SZ_REGKEY_PARAMS_TRIM_MESSAGE, regval))
        m_trapsize.m_bTrimMessages = TRUE;
    else
        m_trapsize.m_bTrimMessages = (*(DWORD*)regval.m_pData) != 0;


    if (!regkeyParams.GetValue(SZ_REGKEY_PARAMS_THRESHOLDENABLED, regval))
        m_throttle.m_bIsEnabled = TRUE;
    else
        m_throttle.m_bIsEnabled = (*(DWORD*)regval.m_pData) != THROTTLE_DISABLED;


    // Threshold trap count.
    if (!regkeyParams.GetValue(SZ_REGKEY_PARAMS_THRESHOLDCOUNT, regval) ||
        *(DWORD*)regval.m_pData < 2)
        m_throttle.m_nTraps = THRESHOLD_COUNT;
    else
        m_throttle.m_nTraps = *(DWORD*)regval.m_pData;

    // Threshold time in seconds
    if (!regkeyParams.GetValue(SZ_REGKEY_PARAMS_THRESHOLDTIME, regval))
        m_throttle.m_nSeconds = THRESHOLD_TIME;
    else
        m_throttle.m_nSeconds = *(DWORD*)regval.m_pData;


    if (regkeyParams.Close() != ERROR_SUCCESS) {
        goto REGISTRY_FAILURE;
    }
    return S_OK;

REGISTRY_FAILURE:
    if (g_bLostConnection) {
        return E_REGKEY_LOST_CONNECTION;
    }
    else {
        return E_FAIL;
    }
}

//****************************************************************
// CTrapParams::Serialize
//
// Write SNMP_EVENTS\EventLog\Parameters information to the
// registry.
//
// Parameters:
//      None.
//
// Returns:
//      S_OK if everything went OK.
//      E_REGKEY_NOT_FOUND if an expected registry key was missing.
//*****************************************************************
SCODE CTrapParams::Serialize()
{
    if (g_bLostConnection) {
        return E_REGKEY_LOST_CONNECTION;
    }


    // Open the Parameters key.
    // Create simply opens the key if already present.
    CRegistryKey regkey;
    if (!g_reg.m_regkeySnmp.CreateSubKey(SZ_REGKEY_PARAMETERS, regkey)) {
        if (g_bLostConnection) {
            return E_REGKEY_LOST_CONNECTION;
        }
        else {
            return E_REGKEY_NOT_FOUND;
        }
    }

    CRegistryValue regval;

    // Save the Message Length and the TrimMessage.
    DWORD dwTrim;
    if (m_trapsize.m_bTrimFlag)
        dwTrim = 1;
    else
        dwTrim = 0;
    regval.Set(SZ_REGKEY_PARAMS_TRIMFLAG, REG_DWORD, sizeof(DWORD), (LPBYTE)&dwTrim);
    regkey.SetValue(regval);
    if (g_bLostConnection) {
        return E_REGKEY_LOST_CONNECTION;
    }

    if (m_trapsize.m_bTrimFlag)
    {
        // Save the maximum trap size
        regval.Set(SZ_REGKEY_PARAMS_MAXTRAP_SIZE, REG_DWORD, sizeof(DWORD), (LPBYTE)&m_trapsize.m_dwMaxTrapSize);
        regkey.SetValue(regval);
        if (g_bLostConnection) {
            return E_REGKEY_LOST_CONNECTION;
        }


        // Save the trim message length
        DWORD dwTrimMessages = m_trapsize.m_bTrimMessages;
        regval.Set(SZ_REGKEY_PARAMS_TRIM_MESSAGE, REG_DWORD, sizeof(DWORD), (LPBYTE)&dwTrimMessages);
        regkey.SetValue(regval);
        if (g_bLostConnection) {
            return E_REGKEY_LOST_CONNECTION;
        }
    }

    // Threshold enabled flag
    DWORD dwValue = (m_throttle.m_bIsEnabled ? THROTTLE_ENABLED : THROTTLE_DISABLED);
    regval.Set(SZ_REGKEY_PARAMS_THRESHOLDENABLED, REG_DWORD, sizeof(DWORD), (LPBYTE)&dwValue);
    regkey.SetValue(regval);
    if (g_bLostConnection) {
        return E_REGKEY_LOST_CONNECTION;
    }

    // If throttle is not enabled, do not write the ThresholdCount and ThresholdTime parameters
    if (m_throttle.m_bIsEnabled)
    {
        // Threshold trap count.
        regval.Set(SZ_REGKEY_PARAMS_THRESHOLDCOUNT, REG_DWORD, sizeof(DWORD), (LPBYTE)&m_throttle.m_nTraps);
        regkey.SetValue(regval);
        if (g_bLostConnection) {
            return E_REGKEY_LOST_CONNECTION;
        }

        // Threshold time in seconds
        regval.Set(SZ_REGKEY_PARAMS_THRESHOLDTIME, REG_DWORD, sizeof(DWORD), (LPBYTE)&m_throttle.m_nSeconds);
        regkey.SetValue(regval);
        if (g_bLostConnection) {
            return E_REGKEY_LOST_CONNECTION;
        }
    }

    regkey.Close();
    if (g_bLostConnection) {
        return E_REGKEY_LOST_CONNECTION;
    }
    return S_OK;
}




//*******************************************************************
// CTrapParams::ResetExtensionAgent
//
// Reset the extension agent.  This is done by setting the "Threshold"
// parameter to zero in the registry.  The extension agent monitors this
// value and will reset itself when a zero is written there.
//
// The user may want to reset the extension agent if its throttle limit
// has been tripped.
//
// Parameters:
//      None.
//
// Returns:
//      SCODE
//          S_OK if successful.  E_FAIL if the extension agent could not
//          be reset.  If a failure occurs, the appropriate message box
//          is displayed.
//
//*********************************************************************
SCODE CTrapParams::ResetExtensionAgent()
{
    CRegistryKey regkey;
    if (!g_reg.m_regkeySnmp.GetSubKey(SZ_REGKEY_PARAMETERS, regkey)) {
        return E_REGKEY_NOT_FOUND;
    }
    CRegistryValue regval;

    // Set the "Threshold" value under the Parameters key to zero to reset
    // the extension agent.
    DWORD dwValue = THROTTLE_RESET;
    SCODE sc = S_OK;
    regval.Set(SZ_REGKEY_PARAMS_THRESHOLD, REG_DWORD, sizeof(DWORD), (LPBYTE)&dwValue);
    if (!regkey.SetValue(regval)) {
        AfxMessageBox(IDS_WARNING_CANT_WRITE_CONFIG);
        sc = E_FAIL;
    }

    regkey.Close();
    return sc;
}

//***********************************************************************
// CTrapParams::ThrottleIsTripped
//
// Check the registry to determine whether or not the extension agent
// throttle was tripped.
//
// Parameters:
//      None.
//
// Returns:
//      TRUE if the extension agent's throttle was tripped, FALSE otherwise.
//
//************************************************************************
BOOL CTrapParams::ThrottleIsTripped()
{
    CRegistryKey regkey;
    if (!g_reg.m_regkeySnmp.GetSubKey(SZ_REGKEY_PARAMETERS, regkey)) {
        return FALSE;
    }
    CRegistryValue regval;

    // SNMP_EVENTS\Parameters\Threshold value
    BOOL bThrottleIsTripped = FALSE;
    if (regkey.GetValue(SZ_REGKEY_PARAMS_THRESHOLD, regval)) {
        if (*(DWORD*)regval.m_pData == THROTTLE_TRIPPED) {
            bThrottleIsTripped = TRUE;
        }
    }

    regkey.Close();
    return bThrottleIsTripped;
}


///////////////////////////////////////////////////////////////////
// Class: CXEventLogArray
//
// This class implements an array of CXEventLog objects.
//
//////////////////////////////////////////////////////////////////


//****************************************************************
// CXEventLogArray::Deserialize
//
// Examine the registry find all the event logs and load all the
// relevent information for all the event logs into this array.
//
// Parameters:
//      None.
//
// Returns:
//      S_OK if successful.
//      E_FAIL if a failure was detected.
//
//****************************************************************
SCODE CXEventLogArray::Deserialize()
{
    if (g_bLostConnection) {
        return E_REGKEY_LOST_CONNECTION;
    }

    CStringArray* pasEventLogs = g_reg.m_regkeySource.EnumSubKeys();
    SCODE sc = S_OK;

    // Iterate through all the event log names and create each log.
	LONG nEventLogs = (LONG)pasEventLogs->GetSize();
    if (nEventLogs > 0) {
        g_reg.m_nLoadStepsPerLog = LOAD_LOG_ARRAY_STEP_COUNT / nEventLogs;
    }
    LONG nUnusedSteps = LOAD_LOG_ARRAY_STEP_COUNT -  (nEventLogs * g_reg.m_nLoadStepsPerLog);

    for (LONG iEventLog=0; iEventLog < nEventLogs; ++iEventLog)
    {
        CString sEventLog = pasEventLogs->GetAt(iEventLog);
        CXEventLog* pEventLog = new CXEventLog(sEventLog);
        sc = pEventLog->Deserialize();
        if ((sc==S_LOAD_CANCELED) || FAILED(sc)) {
            delete pEventLog;
            break;
        }
        else if (sc == S_NO_SOURCES) {
            delete pEventLog;
            sc = S_OK;
        }
        else {
            Add(pEventLog);
        }
    }
    delete pasEventLogs;
    if (g_reg.m_pdlgLoadProgress->StepProgress(nUnusedSteps)) {
        sc = S_LOAD_CANCELED;
    }

    return sc;
}


//****************************************************************
// CXEventLogArray::Serialize
//
// Write the current configuration of all the EventLogs out to the
// registry.  Only those logs and sources that actually have events
// are written.
//
// Parameters:
//      None.
//
// Returns:
//      S_OK if successful.
//      E_FAIL if a failure was detected.
//
//****************************************************************
SCODE CXEventLogArray::Serialize()
{
    if (g_bLostConnection) {
        return E_REGKEY_LOST_CONNECTION;
    }

    // This is where the eventlog stuff should be cleaned up.

    CRegistryKey regkey;
    if (!g_reg.m_regkeySnmp.CreateSubKey(SZ_REGKEY_EVENTLOG, regkey)) {
        if (g_bLostConnection) {
            return E_REGKEY_LOST_CONNECTION;
        }
        else {
            return E_REGKEY_NOT_FOUND;
        }
    }
    regkey.Close();


    if (!g_reg.m_regkeySnmp.CreateSubKey(SZ_REGKEY_SOURCES, regkey)) {
        if (g_bLostConnection) {
            return E_REGKEY_LOST_CONNECTION;
        }
        else {
            return E_REGKEY_NOT_FOUND;
        }
    }

    // Delete the keys for the sources and events for which we no longer
    // trap. I'm going to be lazy and just delete them all.
    // !!!CR: It could potentially save a lot of time if this was made smarter
    // !!!CR: so that it only replaced items that had been deleted.
    LONG nEventSources, iEventSource;
    CStringArray* pasEventSources = regkey.EnumSubKeys();
    nEventSources = (LONG)pasEventSources->GetSize();
    for (iEventSource=0; iEventSource<nEventSources; iEventSource++)
    {
        CString sSource;
        sSource = pasEventSources->GetAt(iEventSource);
        regkey.DeleteSubKey(sSource);
    }
    delete pasEventSources;


    SCODE sc = S_OK;
    LONG nEventLogs = GetSize();
    for (LONG iEventLog = 0; iEventLog < nEventLogs; ++iEventLog) {
        sc = GetAt(iEventLog)->Serialize(regkey);
        if (sc == S_SAVE_CANCELED) {
            break;
        }
        else if (g_bLostConnection) {
            sc = E_REGKEY_LOST_CONNECTION;
            break;
        }
    }
    regkey.Close();

    return sc;
}



//****************************************************************
// CXEventLogArray::FindEventSource
//
// Given the name of an event log and the name of the event source
// within the event log, return a pointer to the requested CXEventSource.
//
// Parameters:
//      CString& sLog
//          The name of the event log.
//
//      CString& sEventSource
//          The name of the event source.
//
// Returns:
//      CXEventSource*
//          A pointer to the requested event source if it was found.  NULL
//          if no such event source exists.
//
//****************************************************************
CXEventSource* CXEventLogArray::FindEventSource(CString& sLog, CString& sEventSource)
{
    LONG nLogs = GetSize();
    for (LONG iLog = 0; iLog < nLogs; ++iLog) {
        CXEventLog* pEventLog = GetAt(iLog);
        if (pEventLog->m_sName.CompareNoCase(sLog) == 0) {
            return pEventLog->FindEventSource(sEventSource);
        }
    }
    return NULL;
}





///////////////////////////////////////////////////////////////////
// Class: CXEventLog
//
// This class contains all the information for a particular event log.
//
//////////////////////////////////////////////////////////////////


//************************************************************************
// CXEventLog::Deserialize
//
// Load the contents of this EventLog object from the registry.
//
// Parameters:
//      g_reg is a global parameter.
//
// Returns:
//      SCODE
//          S_OK or S_NO_SOURCES if successful.  E_FAIL if there was
//          a failure of any kind.
//
//************************************************************************
SCODE CXEventLog::Deserialize()
{
    return m_aEventSources.Deserialize(this);
}


//************************************************************************
// CXEventLog::Serialize
//
// Write the current configuration for this log to the registry.
//
// Parameters:
//      CRegistryKey& regkey
//          This registry key points to SOFTWARE\Microsoft\SNMP_EVENTS\EventLog
//
// Returns:
//      SCODE
//          S_OK or S_SAVE_CANCELED if successful.  E_FAIL for an error condition.
//          a failure of any kind.
//
//************************************************************************
SCODE CXEventLog::Serialize(CRegistryKey& regkey)
{
    return m_aEventSources.Serialize(regkey);
}







///////////////////////////////////////////////////////////////////
// Class: CXEventSourceArray
//
// This class implements an array of CXEventSource pointers and
// related methods.
//
//////////////////////////////////////////////////////////////////



//*************************************************************************
// CXEventSourceArray::Deserialize
//
// Load all the information pertaining to the event sources associated with
// the given event log.  This information is loaded from the registry.
//
// Parameters:
//      CXEventLog* pEventLog
//          Pointer to the event log.  The sources associated with this
//          event log are loaded into this object.
//
// Returns:
//      SCODE
//          S_OK or S_NO_SOURCES if successful.  E_FAIL if there was
//          a failure of any kind.
//*************************************************************************
SCODE CXEventSourceArray::Deserialize(CXEventLog* pEventLog)
{

	// Get the registry entry for this log.  This registry key will be
	// used to enumerate the event sources for this log.
    CRegistryKey regkey;
    if (!g_reg.m_regkeySource.GetSubKey(pEventLog->m_sName, regkey)) {
        if (g_reg.m_pdlgLoadProgress->StepProgress(g_reg.m_nLoadStepsPerLog)) {
            return S_LOAD_CANCELED;
        }
        if (g_bLostConnection) {
            return E_REGKEY_LOST_CONNECTION;
        }
        else {
            return E_FAIL;
        }
    }


    SCODE sc = S_OK;

	// Enumerate the event sources for this log.
    CStringArray* pasSources = regkey.EnumSubKeys();
    if (pasSources == NULL) {
        regkey.Close();
        if (g_reg.m_pdlgLoadProgress->StepProgress(g_reg.m_nLoadStepsPerLog)) {
            return S_LOAD_CANCELED;
        }
        if (g_bLostConnection) {
            return E_REGKEY_LOST_CONNECTION;
        }
        else {
            return E_FAIL;
        }
    }


	// Iterate though all the event sources and add them as a sub-item
	// under the log.
	LONG nEventSources = (LONG)pasSources->GetSize();
    LONG nScaledStepSize = 0;
    g_reg.m_nLoadStepsPerSource = 0;
    if (nEventSources > 0) {
        nScaledStepSize = (g_reg.m_nLoadStepsPerLog * 1000) / nEventSources;
        g_reg.m_nLoadStepsPerSource = g_reg.m_nLoadStepsPerLog / nEventSources;
    }
    LONG nLoadSteps = 0;
    LONG nProgress = 0;


    // Set the load progress step count.  Since we don't know how many events are saved
    // for each event source, we will assume some small number for LOAD_STEPS_FOR_SOURCE
    // and divide the actual number of steps up as evenly as possible once we know the actual
    // event count.
    for (LONG iEventSource=0; iEventSource< nEventSources; ++iEventSource)
    {
        nProgress += nScaledStepSize;
        g_reg.m_nLoadStepsPerSource = nProgress / 1000;
        if (g_reg.m_nLoadStepsPerSource > 0) {
            nProgress -= g_reg.m_nLoadStepsPerSource * 1000;
            nLoadSteps += g_reg.m_nLoadStepsPerSource;
        }

        CString sEventSource = pasSources->GetAt(iEventSource);
        CXEventSource* pEventSource = new CXEventSource(pEventLog, sEventSource);
        sc = pEventSource->Deserialize(regkey);
        if ((sc==S_LOAD_CANCELED) || FAILED(sc)) {
            delete pEventSource;
            break;
        }
        else if (sc == S_NO_EVENTS) {
            // If there are no events, then this is not a valid event source.
            delete pEventSource;
            sc = S_OK;
        }
        else {
            Add(pEventSource);
        }
    }
	delete pasSources;
    if (SUCCEEDED(sc)) {
        // We only close the registry key if we succeeded to avoid hanging if we loose
        // a remote connection.
        regkey.Close();
        if (GetSize() == 0) {
            sc = S_NO_SOURCES;
        }
    }
    if (nLoadSteps < g_reg.m_nLoadStepsPerLog) {
        if (g_reg.m_pdlgLoadProgress->StepProgress(g_reg.m_nLoadStepsPerLog - nLoadSteps)) {
            return S_LOAD_CANCELED;
        }
        g_reg.m_nLoadSteps += g_reg.m_nLoadStepsPerLog - nLoadSteps;
    }
    return sc;
}


//************************************************************************
// CXEventSourceArray::Serialize
//
// Write the current configuration for this event source array to the registry.
//
// Parameters:
//      CRegistryKey& regkey
//          This registry key points to SOFTWARE\Microsoft\SNMP_EVENTS\EventLog\Sources
//
// Returns:
//      SCODE
//          S_OK or S_SAVE_CANCELED if successful.  E_FAIL for an error condition.
//          a failure of any kind.
//
//************************************************************************
SCODE CXEventSourceArray::Serialize(CRegistryKey& regkey)
{
    // Write the subkeys under SNMP_EVENTS\EventLog
    SCODE sc = S_OK;
    LONG nEventSources = GetSize();
    for (LONG iEventSource = 0; iEventSource < nEventSources; ++iEventSource) {
        SCODE scTemp = GetAt(iEventSource)->Serialize(regkey);
        if (g_bLostConnection) {
            sc = E_REGKEY_LOST_CONNECTION;
            break;
        }
        if (FAILED(scTemp)) {
            sc = E_FAIL;
            break;
        }
        if (scTemp == S_SAVE_CANCELED) {
            sc = S_SAVE_CANCELED;
            break;
        }
    }
    return sc;
}


//************************************************************************
// CXEventSourceArray::FindEventSource
//
// Given an event source name, find the specified event source in this
// event source array.
//
// Parameters:
//      CString& sEventSource
//          The name of the event source to search for.
//
// Returns:
//      CXEventSource*
//          Pointer to the event source if it is found, otherwise NULL.
//
//***********************************************************************
CXEventSource* CXEventSourceArray::FindEventSource(CString& sEventSource)
{
    LONG nSources = GetSize();
    for (LONG iSource = 0; iSource < nSources; ++iSource) {
        CXEventSource* pEventSource = GetAt(iSource);
        if (pEventSource->m_sName.CompareNoCase(sEventSource)==0) {
            return pEventSource;
        }
    }
    return NULL;
}


///////////////////////////////////////////////////////////////////
// Class: CXEventSource
//
// This class implements an an event source object.  An event source
// corresponds to an application that can generate events.  The
// event sources are enumerated from the registry in
// "SYSTEM\CurrentControlSet\Services\EventLogs" under each particular
// eventlog found there.
//
// An event source has an array of messages and an array of events
// associated with it.
//
// The message array comes from the message .DLL file(s) pointed to by
// the "EventMessageFile" value attached to the source's key in the registry.
// The message array is read-only in the sense that it is loaded from the
// registry and never written back to it.
//
// The event array comes from SNMP_EVENTS\EventLog\<source-subkey>.  These
// events are loaded when the configuration program starts up and written
// back out when the user clicks "OK".  Note that the events stored in the
// registry contain the event ID, but not the message text.  The message text
// for an event is found by searching the message array in the CXEventSource
// object for the event's ID.
//
//////////////////////////////////////////////////////////////////

//*************************************************************************
// CXEventSource::CXEventSource
//
// Construct the CXEventSource object.
//
// Parameters:
//      CXEventLog* pEventLog
//          Pointer to the event log that contains this event source.
//
//      CString& sName
//          The name of this event source.
//
// Returns:
//      Nothing.
//
//*************************************************************************
CXEventSource::CXEventSource(CXEventLog* pEventLog, CString& sName)
{
    m_pEventLog = pEventLog;
    m_sName = sName;
    m_aMessages.Initialize(this);
}




//************************************************************************
// CXEventSource::~CXEventSource
//
// Destroy thus event source object.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//************************************************************************
CXEventSource::~CXEventSource()
{
    // We must explicitly delete the contents of the event array and message
    // array.  Note that this is different behavior from the CXEventLogArray
    // and CXEventSourceArray.  This is because it was useful to create
    // message and event arrays as temporary containers for a set of pointers.
    // Thus, there were situations where you did not want to delete the
    // objects contained in these arrays when the arrays were destroyed.
    m_aEvents.DeleteAll();
    m_aMessages.DeleteAll();
}


//**********************************************************************
// CXEventSource::Deserialize
//
// Load this event source from the registry given the registry key
// for the event log that contains this source.
//
// Parameters:
//      CRegistryKey& regkeyLog
//          An open registry key for the event log containing this
//          event source.  This key points to somewhere in
//          SYSTEM\CurrentControlSet\Services\EventLog
//
// Returns:
//      SCODE
//          S_OK = the source has events and no errors were encountered.
//          S_NO_EVENTS = the source has no events and no errors were encountered.
//          E_FAIL = an condition was encountered.
//
//***********************************************************************
SCODE CXEventSource::Deserialize(CRegistryKey& regkeyLog)
{
    CRegistryKey regkeySource;
    if (!regkeyLog.GetSubKey(m_sName, regkeySource)) {
        if (g_reg.m_pdlgLoadProgress->StepProgress(g_reg.m_nLoadStepsPerSource)) {
            return S_LOAD_CANCELED;
        }
        g_reg.m_nLoadSteps += g_reg.m_nLoadStepsPerSource;
        if (g_bLostConnection) {
            return E_REGKEY_LOST_CONNECTION;
        }
        else {
            return E_FAIL;
        }
    }

    SCODE sc = E_FAIL;
    if (SUCCEEDED(GetLibPath(regkeySource))) {
        sc = m_aEvents.Deserialize(this);
    }
    else {
        if (g_bLostConnection) {
            return E_REGKEY_LOST_CONNECTION;
        }

        if (g_reg.m_pdlgLoadProgress->StepProgress(g_reg.m_nLoadStepsPerSource)) {
            return S_LOAD_CANCELED;
        }
        g_reg.m_nLoadSteps += g_reg.m_nLoadStepsPerSource;
        sc = S_NO_EVENTS;
    }


    regkeySource.Close();
    if (g_bLostConnection) {
        return E_REGKEY_LOST_CONNECTION;
    }

    // Delay deserializing the messages for this source until they are
    // needed.
    return sc;
}


#if 0
//*************************************************************************
// CXEventSource::GetLibPath
//
// Get the path the the EventMessageFile for this event source.
//
// Parameters:
//      CRegistryKey& regkeySource
//          An open registry key corresponding to this source in
//          SYSTEM\CurrentControlSet\Services\EventLog\<event log>
//
// Returns:
//      SCODE
//          S_OK if successful, otherwise E_FAIL.
//
//*************************************************************************
SCODE CXEventSource::GetLibPath(CRegistryKey& regkeySource)
{
    CRegistryValue regval;
    if (!regkeySource.GetValue(SZ_REGKEY_SOURCE_EVENT_MESSAGE_FILE, regval))
        return E_FAIL;

	TCHAR szLibPath[MAX_STRING];
    if (ExpandEnvironmentStrings((LPCTSTR)regval.m_pData, szLibPath, sizeof(szLibPath)) == 0)
        return E_FAIL;

    m_sLibPath = szLibPath;
    return S_OK;
}
#else
//*************************************************************************
// CXEventSource::GetLibPath
//
// Get the path the the EventMessageFile for this event source.
//
// Parameters:
//      CRegistryKey& regkeySource
//          An open registry key corresponding to this source in
//          SYSTEM\CurrentControlSet\Services\EventLog\<event log>
//
// Returns:
//      SCODE
//          S_OK if successful, otherwise E_FAIL.
//
//*************************************************************************
SCODE CXEventSource::GetLibPath(CRegistryKey& regkeySource)
{
    static CEnvCache cache;



    CRegistryValue regval;
    if (!regkeySource.GetValue(SZ_REGKEY_SOURCE_EVENT_MESSAGE_FILE, regval))
        return E_FAIL;

    SCODE sc = S_OK;
    if (g_reg.m_sComputerName.IsEmpty()) {
        // Editing the local computer computer's registry, so the local environment
        // variables are in effect.

    	TCHAR szLibPath[MAX_STRING];
        if (ExpandEnvironmentStrings((LPCTSTR)regval.m_pData, szLibPath, sizeof(szLibPath)))  {
            m_sLibPath = szLibPath;
        }
        else {
            sc = E_FAIL;
        }
    }
    else {
        // Editing a remote computer's registry, so the remote environment strings are in
        // effect.  Also, file paths must be mapped to the UNC path for the machine.  For
        // example, C:Foo will be mapped to \\Machine\C$\Foo
        m_sLibPath = regval.m_pData;
        sc = RemoteExpandEnvStrings(g_reg.m_sComputerName, cache, m_sLibPath);
        if (SUCCEEDED(sc)) {
            sc = MapPathToUNC(g_reg.m_sComputerName, m_sLibPath);
        }
    }

    return S_OK;
}

#endif



//************************************************************************
// CXEventSource::Serialize
//
// Write the configuration information for this event source to the registry.
//
// Parameters:
//      CRegistryKey& regkeyParent
//          An open registry key pointing to SNMP_EVENTS\EventLog\Sources
//
// Returns:
//      SCODE
//          S_OK if successful.
//          S_SAVE_CANCELED if no errors, but the user canceled the save.
//
//************************************************************************
SCODE CXEventSource::Serialize(CRegistryKey& regkeyParent)
{
    if (g_bLostConnection) {
        return E_REGKEY_LOST_CONNECTION;
    }

    SCODE sc = S_OK;
    if (m_aEvents.GetSize() > 0) {
        CRegistryKey regkey;
        if (!regkeyParent.CreateSubKey(m_sName, regkey)) {
            if (g_bLostConnection) {
                return E_REGKEY_LOST_CONNECTION;
            }
            else {
                return E_REGKEY_NOT_FOUND;
            }
        }

        CString sEnterpriseOID;
        GetEnterpriseOID(sEnterpriseOID);
        CRegistryValue regval;


        regval.Set(SZ_REGKEY_SOURCE_ENTERPRISE_OID,
                   REG_SZ, (sEnterpriseOID.GetLength() + 1) * sizeof(TCHAR),
                   (LPBYTE)(LPCTSTR)sEnterpriseOID);
        regkey.SetValue(regval);


        DWORD dwAppend = 1;
        regval.Set(SZ_REGKEY_SOURCE_APPEND, REG_DWORD, sizeof(DWORD), (LPBYTE) &dwAppend);
        regkey.SetValue(regval);

        sc = m_aEvents.Serialize(regkey);
        regkey.Close();
    }

    if (g_bLostConnection) {
        return E_REGKEY_LOST_CONNECTION;
    }
    return sc;
}


//*******************************************************************
// CXEventSource::GetEnterpriseOID
//
// Get the enterprise OID for this event source.  The enterprise OID
// is composed of a prefix and suffix string concatenated together.  The
// prefix string is an ASCII decimal value for the length of the suffix
// string.  The suffix string is composed by separating each character of
// the name of this source by a "." character.
//
// Parameters:
//      CString& sEnterpriseOID
//          A reference to the string where the enterprise OID for this
//          source will be returned.
//
// Returns:
//      The enterprise OID in via the sEnterpriseOID reference.
//
//********************************************************************
void CXEventSource::GetEnterpriseOID(CString& sEnterpriseOID, BOOL bGetFullID)
{
    CString sValue;


    // Form the prefix string in sEnterpriseOID and compute the length
    // of the prefix and suffix strings.
    DecString(sValue, m_sName.GetLength());
    if (bGetFullID) {
        sEnterpriseOID = g_reg.m_params.m_sBaseEnterpriseOID + _T('.') + sValue;
    }
    else {
        sEnterpriseOID = sValue;
    }

    // Append the suffix string to the prefix string by getting a pointer to
    // the sEnterpriseOID buffer and allocating enough space to hold the
    // combined strings.
    LPCTSTR pszSrc = m_sName;

    // Append the suffix by copying it to the destination buffer and inserting the
    // "." separator characters as we go.
    LONG iCh;
    while (iCh = *pszSrc++) {
        switch(sizeof(TCHAR)) {
        case 1:
            iCh &= 0x0ff;
            break;
        case 2:
            iCh &= 0x0ffffL;
            break;
        default:
            ASSERT(FALSE);
            break;
        }

        DecString(sValue, iCh);
        sEnterpriseOID += _T('.');
        sEnterpriseOID += sValue;
    }
}






///////////////////////////////////////////////////////////////////
// Class: CXEventArray
//
// This class implements an array of pointers to CXEvent objects.
// The events contained in this array correspond to the events that
// the user has configured in the main dialog.  Don't confuse events
// with messages.  Events are the subset of the messages that the
// user has selected to be translated into traps.
//
// For further information on how this CXEventArray fits into the
// scheme of things, please see the CXEventSource class header.
//////////////////////////////////////////////////////////////////


//************************************************************************
// CXEventArray::Deserialize
//
// Read an array of events from the registry for the given
// source.
//
// Parameters:
//      CXEventSource* pEventSource
//          Pointer to the event source who's events should be read.
//
// Returns:
//      SCODE
//          S_OK if successful.
//          E_FAIL if an error occurs.
//
//************************************************************************
SCODE CXEventArray::Deserialize(CXEventSource* pEventSource)
{
    if (!g_reg.SourceHasTraps(pEventSource->m_sName)) {
        if (g_reg.m_pdlgLoadProgress->StepProgress(g_reg.m_nLoadStepsPerSource)) {
            return S_LOAD_CANCELED;
        }
        g_reg.m_nLoadSteps += g_reg.m_nLoadStepsPerSource;
        return S_OK;
    }

    // Control comes here if we know that there are events configured
    // for the event source that this event array is part of.  We now
    // need to load the events for this source by enumerating them
    // from SNMP_EVENTS\EventLog\<event source>

    CString sKey;
    sKey = sKey + SZ_REGKEY_SOURCES + _T("\\") + pEventSource->m_sName;
    CRegistryKey regkey;
    if (!g_reg.m_regkeySnmp.GetSubKey(sKey, regkey)) {
        if (g_reg.m_pdlgLoadProgress->StepProgress(g_reg.m_nLoadStepsPerSource)) {
            return S_LOAD_CANCELED;
        }
        g_reg.m_nLoadSteps += g_reg.m_nLoadStepsPerSource;
        return S_OK;
    }


	// Enumerate the events for this source
    CStringArray* pasEvents = regkey.EnumSubKeys();
    if (pasEvents == NULL) {
        if (g_bLostConnection) {
            return E_REGKEY_LOST_CONNECTION;
        }

        regkey.Close();
        if (g_reg.m_pdlgLoadProgress->StepProgress(g_reg.m_nLoadStepsPerSource)) {
            return S_LOAD_CANCELED;
        }
        g_reg.m_nLoadSteps += g_reg.m_nLoadStepsPerSource;

        return E_FAIL;
    }



	// Iterate though all the events and add them as a sub-item
	// under the event source.
	LONG nEvents = (LONG)pasEvents->GetSize();
    LONG nStepsDone = 0;
    LONG nEventsPerStep = 0;
    if (g_reg.m_nLoadStepsPerSource > 0) {
        nEventsPerStep = nEvents / g_reg.m_nLoadStepsPerSource;
    }

    for (LONG iEvent=0; iEvent< nEvents; ++iEvent)
    {
        CString sEvent = pasEvents->GetAt(iEvent);
        CXEvent* pEvent = new CXEvent(pEventSource);
        SCODE sc = pEvent->Deserialize(regkey, sEvent);
        if (sc == E_MESSAGE_NOT_FOUND) {
            delete pEvent;
            if (!g_reg.m_bSomeMessageWasNotFound) {
                AfxMessageBox(IDS_ERR_MESSAGE_NOT_FOUND, MB_OK | MB_ICONEXCLAMATION);
                g_reg.m_bSomeMessageWasNotFound = TRUE;
                g_reg.SetDirty(TRUE);
            }

            continue;
        }


        if ((sc == S_LOAD_CANCELED) || FAILED(sc) ) {
            delete pEvent;
            delete pasEvents;
            return sc;
        }

        if (nEventsPerStep > 0) {
            if ((iEvent % nEventsPerStep) == (nEventsPerStep - 1)) {
                if (g_reg.m_pdlgLoadProgress->StepProgress()) {
                    delete pasEvents;
                    return S_LOAD_CANCELED;
                }
                ++g_reg.m_nLoadSteps;
                ++nStepsDone;
            }
        }
    }
	delete pasEvents;
    regkey.Close();
    if (nStepsDone < g_reg.m_nLoadStepsPerSource) {
        if (g_reg.m_pdlgLoadProgress->StepProgress(g_reg.m_nLoadStepsPerSource - nStepsDone)) {
            return S_LOAD_CANCELED;
        }
        g_reg.m_nLoadSteps += g_reg.m_nLoadStepsPerSource - nStepsDone;
    }
    return S_OK;
}


//************************************************************************
// CXEventArray::Serialize
//
// Write the current configuration for the events contained in this array
// out to the registry.
//
// Parameters:
//      CRegistryKey& regkeyParent
//          An open registry key for the source that owns these events.
//          The source key is located in SNMP_EVENTS\EventLogs\<source-key>
//
// Returns:
//      SCODE
//          S_OK = All events saved without errors.
//          S_SAVE_CANCELED = No errors, but the user canceled the save.
//          E_FAIL = An error occurs.
//
//************************************************************************
SCODE CXEventArray::Serialize(CRegistryKey& regkeyParent)
{
    SCODE sc = S_OK;
    LONG nEvents = GetSize();
    for (LONG iEvent = 0; iEvent < nEvents; ++iEvent) {
        SCODE scTemp = GetAt(iEvent)->Serialize(regkeyParent);
        if (scTemp == S_SAVE_CANCELED) {
            sc = S_SAVE_CANCELED;
            break;
        }

        if (FAILED(scTemp)) {
            if (g_bLostConnection) {
                sc = E_REGKEY_LOST_CONNECTION;
            }
            else {
                sc = E_FAIL;
            }
            break;
        }
    }
    return sc;
}


//***********************************************************************
// CXEventArray::Add
//
// Add an event pointer to this array.  Note that there is no assumption
// that the array owns the pointer.  Someone must explicitly call the DeleteAll
// member to delete the pointers stored in this array.
//
// Parameters:
//      CXEvent* pEvent
//          Pointer to the event to add to this array.
//
// Returns:
//      Nothing.
//
//***********************************************************************
void CXEventArray::Add(CXEvent* pEvent)
{
    CBaseArray::Add(pEvent);
}	



//***********************************************************************
// CXEventArray::FindEvent
//
// Given an event id, find the corresponding event in this array.
//
// Note that this array should never contain duplicate events.
//
// Parameters:
//      DWORD dwId
//          The event ID.
//
// Returns:
//      CXEvent*
//          A pointer to the desired event.  NULL if the event was
//          not found.
//
//***********************************************************************
CXEvent* CXEventArray::FindEvent(DWORD dwId)
{
    LONG nEvents = GetSize();
    for (LONG iEvent=0; iEvent < nEvents; ++iEvent) {
        CXEvent* pEvent = GetAt(iEvent);
        if (pEvent->m_message.m_dwId == dwId) {
            return pEvent;
        }
    }
    return NULL;
}



//***********************************************************************
// CXEventArray::FindEvent
//
// Given an event pointer, remove the event from this array.
//
// Parameters:
//      CXEvent* pEventRemove
//          A pointer to the event to remove.
//
// Returns:
//      SCODE
//          S_OK if the event was removed.
//          E_FAIL if the event was not found in this array.
//
//***********************************************************************
SCODE CXEventArray::RemoveEvent(CXEvent* pEventRemove)
{
    // Iterate through the event array to search for the specified event.
    LONG nEvents = GetSize();
    for (LONG iEvent=0; iEvent < nEvents; ++iEvent) {
        CXEvent* pEvent = GetAt(iEvent);
        if (pEvent == pEventRemove) {
            RemoveAt(iEvent);
            return S_OK;
        }
    }
    return E_FAIL;
}




///////////////////////////////////////////////////////////////////
// Class: CXEvent
//
// This class implements an event.  Events are the subset of the
// messages that the user selects to be translated into traps.
// Events, and not messages, are what the user configures.
//
// For further information on how this class fits into the
// scheme of things, please see the CXEventSource class header.
//////////////////////////////////////////////////////////////////

//*********************************************************************
// CXEvent::CXEvent
//
// Construct the event.
//
// Parameters:
//      CXEventSource* pEventSource
//          Pointer to the event source that has the potential to generate
//          this event.
//
// Returns:
//      Nothing.
//
//*********************************************************************
CXEvent::CXEvent(CXEventSource* pEventSource) : m_message(pEventSource)
{
    m_dwCount = 0;
    m_dwTimeInterval = 0;
    m_pEventSource = pEventSource;
    m_pEventSource->m_aEvents.Add(this);
}



//**********************************************************************
// CXEvent::CXEvent
//
// Construct an event.  This form of the constructor creates an event
// directly from a CXMessage object.  This is possible because the
// CXMessage object contains a back-pointer to its source.
//
// Parameters:
//      CXMessage* pMessage
//          Pointer to the message that is used as the event template.
//
// Returns:
//      Nothing.
//**********************************************************************
CXEvent::CXEvent(CXMessage* pMessage) : m_message(pMessage->m_pEventSource)
{
    m_pEventSource = pMessage->m_pEventSource;
    m_message = *pMessage;
    m_dwCount = 0;
    m_dwTimeInterval = 0;
    m_pEventSource->m_aEvents.Add(this);
}


//**********************************************************************
// CXEvent::~CXEvent
//
// Destroy this event.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//**********************************************************************
CXEvent::~CXEvent()
{
    // Remove this event from the source
    m_pEventSource->m_aEvents.RemoveEvent(this);
}


//**********************************************************************
// CXEvent::Deserialize
//
// Read this event from the registry.
//
// Parameters:
//      CRegistryKey& regkeyParent
//          An open registry key pointing to the event source in
//          SNMP_EVENTS\EventLog
//
//      CString& sName
//          The name of the event to load.
//
// Returns:
//      SCODE
//          S_OK if successful.
//          E_FAIL if an error occurred.
//
//*********************************************************************
SCODE CXEvent::Deserialize(CRegistryKey& regkeyParent, CString& sName)
{
    CRegistryKey regkey;
    if (!regkeyParent.GetSubKey(sName, regkey)) {
        return E_FAIL;
    }

    SCODE sc = E_FAIL;
    CRegistryValue regval;

    // Get the count and time interval.
    m_dwCount = 0;
    m_dwTimeInterval = 0;
    if (regkey.GetValue(SZ_REGKEY_EVENT_COUNT, regval))  {
        m_dwCount = *(DWORD*)regval.m_pData;
        if (regkey.GetValue(SZ_REGKEY_EVENT_TIME, regval))  {
            m_dwTimeInterval = *(DWORD*)regval.m_pData;
        }
    }


    if (regkey.GetValue(SZ_REGKEY_EVENT_FULLID, regval))   {
        DWORD dwFullId = *(DWORD*)regval.m_pData;

        CXMessage* pMessage = m_pEventSource->FindMessage(dwFullId);
        if (pMessage == NULL) {
            sc = E_MESSAGE_NOT_FOUND;
        }
        else {
            m_message = *pMessage;
            sc = S_OK;
        }
    }

    regkey.Close();
    return sc;
}



//**********************************************************************
// CXEvent::Deserialize
//
// Write the configuration for this event to the registry.
//
// Parameters:
//      CRegistryKey& regkeyParent
//          An open registry key pointing to the event source in
//          SNMP_EVENTS\EventLog
//
// Returns:
//      SCODE
//          S_OK = the event was successful written out.
//          S_SAVE_CANCELED = no errors, but the user canceled the save.
//          E_FAIL = if an error occurred.
//
//*********************************************************************
SCODE CXEvent::Serialize(CRegistryKey& regkeyParent)
{
    if (g_reg.m_pdlgSaveProgress) {
        if (g_reg.m_pdlgSaveProgress->StepProgress()) {
            return S_SAVE_CANCELED;
        }
    }


    CRegistryKey regkey;

    CString sName;
    GetName(sName);
    if (!regkeyParent.CreateSubKey(sName, regkey)) {
        return E_REGKEY_NO_CREATE;
    }

    CRegistryValue regval;
    if (m_dwCount > 0) {
        regval.Set(SZ_REGKEY_EVENT_COUNT, REG_DWORD, sizeof(DWORD), (LPBYTE) &m_dwCount);
        regkey.SetValue(regval);

        if (m_dwTimeInterval > 0) {
            regval.Set(SZ_REGKEY_EVENT_TIME, REG_DWORD, sizeof(DWORD), (LPBYTE) &m_dwTimeInterval);
            regkey.SetValue(regval);
        }
    }

    regval.Set(SZ_REGKEY_EVENT_FULLID, REG_DWORD, sizeof(DWORD), (LPBYTE) &m_message.m_dwId);
    regkey.SetValue(regval);
    regkey.Close();
    return S_OK;
}


//*************************************************************************
// CXEvent::GetCount
//
// Get the ASCII decimal value for the m_dwCount member.
//
// Using this method to do the conversion ensures that the count value is
// presented to the user in a consistent form throughout the program.
//
// Parameters:
//      CString& sText
//          This is where the count value is returned.
//
// Returns:
//      The ASCII value for the count is returned via sText.
//
// Note: m_dwCount and m_dwTimeInterval work together.  A trap is sent only if
// m_dwCount events are registered withing m_dwTimeInterval seconds.
//*************************************************************************
void CXEvent::GetCount(CString& sText)
{
    DecString(sText, (long) m_dwCount);
}



//*************************************************************************
// CXEvent::GetTimeInterval
//
// Get the ASCII decimal value for the m_dwTimeInterval member.
//
// Using this method to do the conversion ensures that the time-interval value is
// presented to the user in a consistent form throughout the program.
//
// Parameters:
//      CString& sText
//          This is where the count value is returned.
//
// Returns:
//      The ASCII value for the count is returned via sText.
//
// Note: m_dwCount and m_dwTimeInterval work together.  A trap is sent only if
// m_dwCount events are registered withing m_dwTimeInterval seconds.
//*************************************************************************
void CXEvent::GetTimeInterval(CString& sText)
{
    DecString(sText, (long) m_dwTimeInterval);
}






///////////////////////////////////////////////////////////////////
// Class: CXMessage
//
// This class implements a message.  Each event source has some
// number of messages associated with it.  A user may select some
// subset of the messages to be converted into "events".  The user
// configures events, not messages.
//
// For further information on how this class fits into the
// scheme of things, please see the CXEventSource class header.
//////////////////////////////////////////////////////////////////


CXMessage::CXMessage(CXEventSource* pEventSource)
{
    m_pEventSource = pEventSource;
}


CXMessage& CXMessage::operator=(CXMessage& message)
{
    m_pEventSource = message.m_pEventSource;
    m_dwId = message.m_dwId;
    m_sText = message.m_sText;
    return *this;
}



//***************************************************************************
//
//  CMessage::GetSeverity
//
//  Get the severity level of the event.  This is the human-readable string
//	corresponding to the top two bits of the event ID.
//
//  Parameters:
//		CString& sSeverity
//			A reference to the place to return the severity string.
//
//  Returns:
//		Nothing.
//
//  Status:
//
//***************************************************************************
void CXMessage::GetSeverity(CString& sSeverity)
{
	MapEventToSeverity(m_dwId, sSeverity);
}



//***************************************************************************
//
//  CMessage::GetTrappingString
//
//  This method returns the trapping string "yes" if the event is being
//	trapped and "no" if its not being trapped.
//
//  Parameters:
//		CString& sTrapping
//			A reference to the place to return the trapping string.
//
//  Returns:
//		Nothing.
//
//  Status:
//
//***************************************************************************
void CXMessage::IsTrapping(CString& sIsTrapping)
{
    CXEvent* pEvent = m_pEventSource->FindEvent(m_dwId);
    sIsTrapping.LoadString( pEvent != NULL ? IDS_IS_TRAPPING : IDS_NOT_TRAPPING);
}


//****************************************************************************
//
// CMessage::SetAndCleanText
//
// Set the m_sText data member to a cleaned up version of a source string.
// The text is cleaned by converting all funny whitespace characters such
// as carriage return, tabs and so on to ordinary space characters.  All
// leading space is stripped from the beginning of the string.
//
//****************************************************************************
void CXMessage::SetAndCleanText(PMESSAGE_RESOURCE_ENTRY pEntry)
{
    BOOL    bIsLeadingSpace = TRUE;
    USHORT  i;

    if (pEntry->Flags == 0x00000)   // ANSI char set
    {
        CHAR *pszSrc = (CHAR *)pEntry->Text;
        CHAR chSrc;
        LPTSTR pszDst = m_sText.GetBuffer(strlen(pszSrc) + 1);

        for (i=0; i<pEntry->Length && *pszSrc; i++, pszSrc++)
        {
            chSrc = *pszSrc;
            if (chSrc >= 0x09 && chSrc <= 0x0d)
                chSrc = ' ';
            if (chSrc == ' ' && bIsLeadingSpace)
                    continue;

            *pszDst++ = (TCHAR)chSrc;
            if (bIsLeadingSpace)    // testing only is less costly
                bIsLeadingSpace = FALSE;
        }
        *pszDst = _T('\0');
    }
    else    // UNICODE char set
    {
        wchar_t *pwszSrc = (wchar_t *)pEntry->Text;
        wchar_t wchSrc;
        LPTSTR pszDst = m_sText.GetBuffer(wcslen(pwszSrc) + 1);

        for (i=0; i<pEntry->Length/sizeof(wchar_t) && *pwszSrc; i++, pwszSrc++)
        {
            wchSrc = *pwszSrc;
            if (wchSrc >= (wchar_t)0x09 && wchSrc <= (wchar_t)0x0d)
                wchSrc = (wchar_t)' ';
            if (wchSrc == (wchar_t)' ' && bIsLeadingSpace)
                continue;

            *pszDst++ = (TCHAR)wchSrc;
            if (bIsLeadingSpace)    // testing only is less costly
                bIsLeadingSpace = FALSE;
        }
        *pszDst = _T('\0');
    }

    m_sText.ReleaseBuffer();
}



//****************************************************************************
// CXMessage::GetShortId
//
// This method returns the message's "short ID" that users see for events and
// messages.  The short ID is the ASCII decimal value for the low-order 16 bits
// of the message ID.
//
// Using this method to do the conversion ensures that the short-ID value is
// presented to the user in a consistent form throughout the program.
//
// Parameters:
//      CString& sShortId
//          This is where the ID string is returned.
//
// Returns:
//      The message ID string is returned via sShortId
//
//****************************************************************************
void CXMessage::GetShortId(CString& sShortId)
{
    TCHAR szBuffer[MAX_STRING];
    _ltot((LONG) LOWORD(m_dwId), szBuffer, 10);
    sShortId = szBuffer;
}



///////////////////////////////////////////////////////////////////
// Class: CXMessageArray
//
// This class implements an array of pointers to CXMessage objects.
//
// For further information on how this CXMessageArray fits into the
// scheme of things, please see the CXEventSource class header.
//////////////////////////////////////////////////////////////////


//****************************************************************
// CXMessageArray::CXMessageArray
//
// Constructor.
//
// Parameters:
//      None.
//
// Returns:
//      Nothing.
//
//****************************************************************
CXMessageArray::CXMessageArray()
{
    m_bDidLoadMessages = FALSE;
    m_pEventSource = NULL;
}




//*******************************************************************
// CXMessageArray::FindMessage
//
// Search this array for a message given its ID.
//
// Parameters:
//      DWORD dwId
//          The full message ID
//
// Returns:
//      CXMessage*
//          Pointer to the message if it was found.  NULL if it was
//          not found.
//
// Note:
//      Duplicate messages are not allowed in the array, but no code
//      enforces this for the sake of efficiency.
//
//*******************************************************************
CXMessage* CXMessageArray::FindMessage(DWORD dwId)
{
    if (!m_bDidLoadMessages) {
        if (FAILED(LoadMessages())) {
            return NULL;
        }
    }

    LONG nMessages = GetSize();
    for (LONG iMessage = 0; iMessage < nMessages; ++iMessage) {
        CXMessage* pMessage = GetAt(iMessage);
        if (pMessage->m_dwId == dwId) {
            return pMessage;
        }
    }

    return NULL;
}





//****************************************************************************
//
// XProcessMsgTable
//
// This function processes a the message table contained in a message .DLL file
// and adds all the messages it contains to the given CXMessageArray object.
//
// Parameters:
//      HANDLE hModule
//          The module handle for the .DLL file.
//
//      LPCTSTR lpszType
//          Ignored.
//
//      LPTSTR lpszName
//          The name of the module.
//
//      LONG lParam
//          A pointer to a CXMessageArray object where the messages will be
//          stored.
//
// Returns:
//      BOOL
//          Always returns TRUE.
//
//
//****************************************************************************
static BOOL CALLBACK XProcessMsgTable(HANDLE hModule, LPCTSTR lpszType,
    LPTSTR lpszName, LONG lParam)
{
    CXMessageArray* paMessages = (CXMessageArray*)(LPVOID)lParam;

    // Found a message table.  Process it!
    HRSRC hResource = FindResource((HINSTANCE)hModule, lpszName,
        RT_MESSAGETABLE);
    if (hResource == NULL)
        return TRUE;

    HGLOBAL hMem = LoadResource((HINSTANCE)hModule, hResource);
    if (hMem == NULL)
        return TRUE;

    PMESSAGE_RESOURCE_DATA pMsgTable = (PMESSAGE_RESOURCE_DATA)::LockResource(hMem);
    if (pMsgTable == NULL)
        return TRUE;

    ULONG ulBlock, ulId, ulOffset;

    for (ulBlock=0; ulBlock<pMsgTable->NumberOfBlocks; ulBlock++)
    {
        ulOffset = pMsgTable->Blocks[ulBlock].OffsetToEntries;
        for (ulId = pMsgTable->Blocks[ulBlock].LowId;
            ulId <= pMsgTable->Blocks[ulBlock].HighId; ulId++)

        {
            PMESSAGE_RESOURCE_ENTRY pEntry =
                (PMESSAGE_RESOURCE_ENTRY)((ULONG_PTR)pMsgTable + ulOffset);
            CXMessage *pMessage = new CXMessage(paMessages->m_pEventSource);
            pMessage->m_dwId = (DWORD) ulId;
            pMessage->SetAndCleanText(pEntry);
            paMessages->Add(pMessage);
            ulOffset += pEntry->Length;
        }
    }

    return TRUE;
}


//****************************************************************************
// CXMessageArray::LoadMessages
//
// Load the messages from the message .DLL file(s) for the source into this
// message array.
//
// Parameters:
//      None.
//
// Returns:
//      SCODE
//          S_OK if successful.
//          E_FAIL if an error occurs.
//
//*****************************************************************************
SCODE CXMessageArray::LoadMessages()
{
    ASSERT(m_pEventSource != NULL);
    if (m_bDidLoadMessages) {
        return S_OK;
    }


    CBusy busy;
    CString sLibPathList = m_pEventSource->m_sLibPath;
    CString sLibPath;

	while (GetNextPath(sLibPathList, sLibPath) != E_FAIL) {

	    // Load the library and get a list of all the messages.
	    HINSTANCE hInstMsgFile = LoadLibraryEx((LPCTSTR) sLibPath, NULL,
	        LOAD_LIBRARY_AS_DATAFILE);
	    if (hInstMsgFile == NULL) {
            TCHAR szMessage[MAX_STRING];
            CString sFormat;
            sFormat.LoadString(IDS_ERR_LOAD_MESSAGE_FILE_FAILED);
            _stprintf(szMessage, (LPCTSTR) sFormat, (LPCTSTR) sLibPath);
            AfxMessageBox(szMessage, MB_OK | MB_ICONSTOP);
			continue;
		}

	    EnumResourceNames(hInstMsgFile, RT_MESSAGETABLE,
	        (ENUMRESNAMEPROC)XProcessMsgTable, (LONG_PTR) this);

        GetLastError();

	    FreeLibrary(hInstMsgFile);
	}


    m_bDidLoadMessages = TRUE;
    return S_OK;
}


//**************************************************************
// CXMessageArray::GetNextPath
//
// This function extracts the next path element from a list
// of semi-colon separated paths.  It also removes the extracted
// element and the semi-colon from the path list.
//
// Paramters:
//		CString& sPathlist
//			A reference to a string consisting of one or more paths separated
//			by semi-colons.
//
//		CString& sPath
//			A reference to the place where the extracted path string
//			will be returned.
//
// Returns:
//		SCODE
//			S_OK if a path was extracted, E_FAIL otherwise.
//
//		The path is returned via sPath.  sPathlist is updated
//		so that sPath and the trailing semi-colon is removed
//
//**************************************************************
SCODE CXMessageArray::GetNextPath(CString& sPathlist, CString& sPath)
{
	CString sPathTemp;

	sPath.Empty();
	while (sPath.IsEmpty() && !sPathlist.IsEmpty()) {
		// Copy the next path from the sPathlist to sPath and
		// remove it from sPathlist
		INT ich = sPathlist.Find(_T(';'));
		if (ich == -1) {
			sPathTemp = sPathlist;
			sPathlist = "";
		}
		else {
			sPathTemp = sPathlist.Left(ich);
			sPathlist = sPathlist.Right( sPathlist.GetLength() - (ich + 1));
		}

		// Trim any leading or trailing space characters from
		// the path.

		// Find the first non-space character
		LPTSTR pszStart = sPathTemp.GetBuffer(sPathTemp.GetLength() + 1);
		while (*pszStart) {
			if (!isspace(*pszStart)) {
				break;
			}			
			++pszStart;
		}	

		// Find the first space following the path
		LPTSTR pszEnd = pszStart;
		while (*pszEnd) {
			if (isspace(*pszEnd)) {
				break;
			}
			++pszEnd;
		}
		*pszEnd = 0;
		sPath = pszStart;
		sPathTemp.ReleaseBuffer();
	}
	
	if (sPath.IsEmpty()) {
		return E_FAIL;
	}
	else {
		return S_OK;
	}
}





