/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Aric Stewart for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


/*
http://msdn.microsoft.com/library/default.asp?url=/library/en-us/msi/setup/controlevent_overview.asp
*/

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "msi.h"
#include "msipriv.h"
#include "action.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef UINT (*EVENTHANDLER)(MSIPACKAGE*,LPCWSTR,msi_dialog *);

struct _events {
    LPCSTR event;
    EVENTHANDLER handler;
};

struct subscriber {
    struct list entry;
    LPWSTR event;
    LPWSTR control;
    LPWSTR attribute;
};

UINT ControlEvent_HandleControlEvent(MSIPACKAGE *, LPCWSTR, LPCWSTR, msi_dialog*);

/*
 * Create a dialog box and run it if it's modal
 */
static UINT event_do_dialog( MSIPACKAGE *package, LPCWSTR name )
{
    msi_dialog *dialog;
    UINT r;

    /* kill the current modeless dialog */
    if( package->dialog )
        msi_dialog_destroy( package->dialog );
    package->dialog = NULL;

    /* create a new dialog */
    dialog = msi_dialog_create( package, name,
                                ControlEvent_HandleControlEvent );
    if( dialog )
    {
        /* modeless dialogs return an error message */
        r = msi_dialog_run_message_loop( dialog );
        if( r == ERROR_SUCCESS )
            msi_dialog_destroy( dialog );
        else
            package->dialog = dialog;
    }
    else
        r = ERROR_FUNCTION_FAILED;

    return r;
}


/*
 * End a modal dialog box
 */
static UINT ControlEvent_EndDialog(MSIPACKAGE* package, LPCWSTR argument, 
                                   msi_dialog* dialog)
{
    static const WCHAR szExit[] = {
    'E','x','i','t',0};
    static const WCHAR szRetry[] = {
    'R','e','t','r','y',0};
    static const WCHAR szIgnore[] = {
    'I','g','n','o','r','e',0};
    static const WCHAR szReturn[] = {
    'R','e','t','u','r','n',0};

    if (lstrcmpW(argument,szExit)==0)
        package->CurrentInstallState = ERROR_INSTALL_USEREXIT;
    else if (lstrcmpW(argument, szRetry) == 0)
        package->CurrentInstallState = ERROR_INSTALL_SUSPEND;
    else if (lstrcmpW(argument, szIgnore) == 0)
        package->CurrentInstallState = -1;
    else if (lstrcmpW(argument, szReturn) == 0)
        package->CurrentInstallState = ERROR_SUCCESS;
    else
    {
        ERR("Unknown argument string %s\n",debugstr_w(argument));
        package->CurrentInstallState = ERROR_FUNCTION_FAILED;
    }

    ControlEvent_CleanupSubscriptions(package);
    msi_dialog_end_dialog( dialog );
    return ERROR_SUCCESS;
}

/*
 * transition from one modal dialog to another modal dialog
 */
static UINT ControlEvent_NewDialog(MSIPACKAGE* package, LPCWSTR argument, 
                                   msi_dialog *dialog)
{
    /* store the name of the next dialog, and signal this one to end */
    package->next_dialog = strdupW(argument);
    ControlEvent_CleanupSubscriptions(package);
    msi_dialog_end_dialog( dialog );
    return ERROR_SUCCESS;
}

/*
 * Create a new child dialog of an existing modal dialog
 */
static UINT ControlEvent_SpawnDialog(MSIPACKAGE* package, LPCWSTR argument, 
                              msi_dialog *dialog)
{
    event_do_dialog( package, argument );
    if( package->CurrentInstallState != ERROR_SUCCESS )
        msi_dialog_end_dialog( dialog );
    return ERROR_SUCCESS;
}

/*
 * Creates a dialog that remains up for a period of time
 * based on a condition
 */
static UINT ControlEvent_SpawnWaitDialog(MSIPACKAGE* package, LPCWSTR argument, 
                                  msi_dialog* dialog)
{
    FIXME("Doing Nothing\n");
    return ERROR_SUCCESS;
}

static UINT ControlEvent_DoAction(MSIPACKAGE* package, LPCWSTR argument, 
                                  msi_dialog* dialog)
{
    ACTION_PerformAction(package,argument,TRUE);
    return ERROR_SUCCESS;
}

static UINT ControlEvent_AddLocal(MSIPACKAGE* package, LPCWSTR argument, 
                                  msi_dialog* dialog)
{
    static const WCHAR szAll[] = {'A','L','L',0};
    int i;

    if (lstrcmpW(szAll,argument))
    {
        MSI_SetFeatureStateW(package,argument,INSTALLSTATE_LOCAL);
    }
    else
    {
        for (i = 0; i < package->loaded_features; i++)
        {
            package->features[i].ActionRequest = INSTALLSTATE_LOCAL;
            package->features[i].Action = INSTALLSTATE_LOCAL;
        }
        ACTION_UpdateComponentStates(package,argument);
    }
    return ERROR_SUCCESS;
}

static UINT ControlEvent_Remove(MSIPACKAGE* package, LPCWSTR argument, 
                                msi_dialog* dialog)
{
    static const WCHAR szAll[] = {'A','L','L',0};
    int i;

    if (lstrcmpW(szAll,argument))
    {
        MSI_SetFeatureStateW(package,argument,INSTALLSTATE_ABSENT);
    }
    else
    {
        for (i = 0; i < package->loaded_features; i++)
        {
            package->features[i].ActionRequest = INSTALLSTATE_ABSENT;
            package->features[i].Action= INSTALLSTATE_ABSENT;
        }
        ACTION_UpdateComponentStates(package,argument);
    }
    return ERROR_SUCCESS;
}

static UINT ControlEvent_AddSource(MSIPACKAGE* package, LPCWSTR argument, 
                                   msi_dialog* dialog)
{
    static const WCHAR szAll[] = {'A','L','L',0};
    int i;

    if (lstrcmpW(szAll,argument))
    {
        MSI_SetFeatureStateW(package,argument,INSTALLSTATE_SOURCE);
    }
    else
    {
        for (i = 0; i < package->loaded_features; i++)
        {
            package->features[i].ActionRequest = INSTALLSTATE_SOURCE;
            package->features[i].Action = INSTALLSTATE_SOURCE;
        }
        ACTION_UpdateComponentStates(package,argument);
    }
    return ERROR_SUCCESS;
}

static UINT ControlEvent_SetTargetPath(MSIPACKAGE* package, LPCWSTR argument, 
                                   msi_dialog* dialog)
{
    LPWSTR path = load_dynamic_property(package,argument, NULL);
    /* failure to set the path halts the executing of control events */
    return MSI_SetTargetPathW(package, argument, path);
}

/*
 * Subscribed events
 */
static void free_subscriber( struct subscriber *sub )
{
    HeapFree(GetProcessHeap(),0,sub->event);
    HeapFree(GetProcessHeap(),0,sub->control);
    HeapFree(GetProcessHeap(),0,sub->attribute);
    HeapFree(GetProcessHeap(),0,sub);
}

VOID ControlEvent_SubscribeToEvent( MSIPACKAGE *package, LPCWSTR event,
                                    LPCWSTR control, LPCWSTR attribute )
{
    struct subscriber *sub;

    sub = HeapAlloc(GetProcessHeap(),0,sizeof (*sub));
    if( !sub )
        return;
    sub->event = strdupW(event);
    sub->control = strdupW(control);
    sub->attribute = strdupW(attribute);
    list_add_tail( &package->subscriptions, &sub->entry );
}

VOID ControlEvent_UnSubscribeToEvent( MSIPACKAGE *package, LPCWSTR event,
                                      LPCWSTR control, LPCWSTR attribute )
{
    struct list *i, *t;
    struct subscriber *sub;

    LIST_FOR_EACH_SAFE( i, t, &package->subscriptions )
    {
        sub = LIST_ENTRY( i, struct subscriber, entry );

        if( lstrcmpiW(sub->control,control) )
            continue;
        if( lstrcmpiW(sub->attribute,attribute) )
            continue;
        if( lstrcmpiW(sub->event,event) )
            continue;
        list_remove( &sub->entry );
        free_subscriber( sub );
    }
}

VOID ControlEvent_FireSubscribedEvent( MSIPACKAGE *package, LPCWSTR event, 
                                       MSIRECORD *rec )
{
    struct subscriber *sub;

    TRACE("Firing Event %s\n",debugstr_w(event));

    if (!package->dialog)
        return;

    LIST_FOR_EACH_ENTRY( sub, &package->subscriptions, struct subscriber, entry )
    {
        if (lstrcmpiW(sub->event, event))
            continue;
        msi_dialog_handle_event( package->dialog, sub->control,
                                 sub->attribute, rec );
    }
}

VOID ControlEvent_CleanupSubscriptions(MSIPACKAGE *package)
{
    struct list *i, *t;
    struct subscriber *sub;

    LIST_FOR_EACH_SAFE( i, t, &package->subscriptions )
    {
        sub = LIST_ENTRY( i, struct subscriber, entry );

        list_remove( &sub->entry );
        free_subscriber( sub );
    }
}

/*
 * ACTION_DialogBox()
 *
 * Return ERROR_SUCCESS if dialog is process and ERROR_FUNCTION_FAILED
 * if the given parameter is not a dialog box
 */
UINT ACTION_DialogBox( MSIPACKAGE* package, LPCWSTR szDialogName )
{
    UINT r = ERROR_SUCCESS;

    if( package->next_dialog )
        ERR("Already a next dialog... ignoring it\n");
    package->next_dialog = NULL;

    /*
     * Dialogs are chained by filling in the next_dialog member
     *  of the package structure, then terminating the current dialog.
     *  The code below sees the next_dialog member set, and runs the
     *  next dialog.
     * We fall out of the loop below if we come across a modeless
     *  dialog, as it returns ERROR_IO_PENDING when we try to run
     *  its message loop.
     */
    r = event_do_dialog( package, szDialogName );
    while( r == ERROR_SUCCESS && package->next_dialog )
    {
        LPWSTR name = package->next_dialog;

        package->next_dialog = NULL;
        r = event_do_dialog( package, name );
        HeapFree( GetProcessHeap(), 0, name );
    }

    if( r == ERROR_IO_PENDING )
        r = ERROR_SUCCESS;

    return r;
}

struct _events Events[] = {
    { "EndDialog",ControlEvent_EndDialog },
    { "NewDialog",ControlEvent_NewDialog },
    { "SpawnDialog",ControlEvent_SpawnDialog },
    { "SpawnWaitDialog",ControlEvent_SpawnWaitDialog },
    { "DoAction",ControlEvent_DoAction },
    { "AddLocal",ControlEvent_AddLocal },
    { "Remove",ControlEvent_Remove },
    { "AddSource",ControlEvent_AddSource },
    { "SetTargetPath",ControlEvent_SetTargetPath },
    { NULL,NULL },
};

UINT ControlEvent_HandleControlEvent(MSIPACKAGE *package, LPCWSTR event,
                                     LPCWSTR argument, msi_dialog* dialog)
{
    int i = 0;
    UINT rc = ERROR_SUCCESS;

    TRACE("Handling Control Event %s\n",debugstr_w(event));
    if (!event)
        return rc;

    while( Events[i].event != NULL)
    {
        LPWSTR wevent = strdupAtoW(Events[i].event);
        if (lstrcmpW(wevent,event)==0)
        {
            HeapFree(GetProcessHeap(),0,wevent);
            rc = Events[i].handler(package,argument,dialog);
            return rc;
        }
        HeapFree(GetProcessHeap(),0,wevent);
        i++;
    }
    FIXME("unhandled control event %s arg(%s)\n",
          debugstr_w(event), debugstr_w(argument));
    return rc;
}
