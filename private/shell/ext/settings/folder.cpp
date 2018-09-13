//
// This module was originally designed to provide category and topic extensibility
// through the registry and COM objects.  You could add new categories by adding
// the CLSID of a COM object to the registry.  The standard 3 categories were
// in fact implemented as the "standard" extension.  
// This code would enumerate these CLSIDs
// and instantiate the corresponding settings folder extension objects which 
// in turn enumerate the topics that they provide.  This extensibility
// was found to be unnecessary so I removed the registry-reading code 
// to reduce code size.  However, I left much of the 
// characteristics untouched in case the need for this flexibility returns
// at some future date.  BrianAu - 1/13/97
//
#include "precomp.hxx"
#pragma hdrstop

#include "folder.h"
#include "stdext.h"


SettingsFolder::SettingsFolder(
    VOID
    ) : m_iCurrentTopic(0)
{     
    DebugMsg(DM_CONSTRUCT, TEXT("SettingsFolder::SettingsFolder, 0x%08X"), this);
}


SettingsFolder::~SettingsFolder(
    VOID
    )
{
    PSETTINGS_FOLDER_TOPIC pITopic = NULL;

    //
    // Release the topic objects.
    //
    while(m_lstTopics.Remove((LPVOID *)&pITopic, 0))
    {
	    Assert(NULL != pITopic);
	    pITopic->Release();
	    pITopic = NULL;
    }
}


//
// For each extension specified in the registry,
// load the categories and topics supplied by the extension.
//
HRESULT
SettingsFolder::LoadTopics(
    VOID
    ) throw(OutOfMemory)
{
    HRESULT hResult = NO_ERROR;

	PointerList Extensions;
	PSETTINGS_FOLDER_EXT pExt = NULL;

	try
	{        
	    hResult = LoadExtensions(Extensions);
	    if (SUCCEEDED(hResult))
	    {
		    UINT cExtensions = Extensions.Count();
		    while(Extensions.Remove((LPVOID *)&pExt, 0))
		    {
		        Assert(NULL != pExt);
		        hResult = LoadExtensionTopics(pExt);   // Can throw OutOfMemory.
		        pExt->Release();                       // Release extension from list.
		    }
	    }
        if (E_OUTOFMEMORY == hResult)
        {
            //
            // The callers of this are set up to expect an 
            // OutOfMemory exception when we're out of memory.
            // 
            throw OutOfMemory();
        }

	}
	catch(...)
	{
	    //
	    // Clean up after exception.
	    //
	    while(Extensions.Remove((LPVOID *)&pExt, 0))
	    {
		    Assert(NULL != pExt);
		    pExt->Release();
	    }
	    throw;
	}

    return hResult;
}



HRESULT 
SettingsFolder::LoadExtensions(
    PointerList& list
    ) throw(OutOfMemory)
{
    PSETTINGS_FOLDER_EXT pExt = NULL;;

	try
	{
        pExt = new SettingsFolderExt();
        pExt->AddRef();

		//
		// Transfer ref count of 1 to list.
		//
		list.Append((LPVOID)pExt);  // Add extension to our list.
                 					// Can throw OutOfMemory.
	}
	catch(...)
	{
        if (NULL != pExt)
		    pExt->Release();
		throw;
	}
    return NO_ERROR;
}



//
// Load all individual topics supplied by the extension.
//
HRESULT
SettingsFolder::LoadExtensionTopics(
    PSETTINGS_FOLDER_EXT pExt
    ) throw(OutOfMemory)
{
    Assert(NULL != pExt);

    PENUM_SETTINGS_TOPICS pEnum = NULL;

    HRESULT hResult = NO_ERROR;
    hResult = pExt->EnumTopics(&pEnum);
    if (SUCCEEDED(hResult))
    {
	    PSETTINGS_FOLDER_TOPIC pITopic = NULL;
        try
        {
	        while(SUCCEEDED(hResult) && S_OK == pEnum->Next(1, &pITopic, NULL))
	        {
		        //
		        // Transfer ref count of 1 to topic descriptor.
		        //
		        hResult = AddTopic(pITopic);
                pITopic = NULL;
	        }
    	    pEnum->Release();
    	}
	    catch(...)
	    {
            if (NULL != pITopic)
		        pITopic->Release();
		    pEnum->Release();
		    throw;
	    }
    }
    return hResult;
}



//
// Assumes caller has AddRef'd pITopic.
//
HRESULT 
SettingsFolder::AddTopic(
    PSETTINGS_FOLDER_TOPIC pITopic
    ) throw(OutOfMemory)
{
    Assert(NULL != pITopic);

    m_lstTopics.Append((LPVOID)pITopic);
    return NO_ERROR;
}


PSETTINGS_FOLDER_TOPIC
SettingsFolder::GetTopic(
    INT iTopic
    )
{
    HRESULT hResult = NO_ERROR;
    PSETTINGS_FOLDER_TOPIC pITopic = NULL;

    if (m_lstTopics.Retrieve((LPVOID *)&pITopic, iTopic))
    {
	    //
	    // AddRef interface pointer before returning copy.
	    //
	    Assert(NULL != pITopic);
	    pITopic->AddRef();
    }

    return pITopic;
}


BOOL 
SettingsFolder::SetCurrentTopicIndex(
    INT iTopic
    )
{
    if (iTopic < (INT)m_lstTopics.Count())
    {
	    m_iCurrentTopic = iTopic;
	    return TRUE;
    }
    return FALSE;
}

