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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "msi.h"
#include "msipriv.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef UINT (*EVENTHANDLER)(MSIPACKAGE*,LPCWSTR,msi_dialog *);

struct _events {
    LPCSTR event;
    EVENTHANDLER handler;
};

struct subscriber {
    struct list entry;
    msi_dialog *dialog;
    LPWSTR event;
    LPWSTR control;
    LPWSTR attribute;
};

static UINT ControlEvent_HandleControlEvent(MSIPACKAGE *, LPCWSTR, LPCWSTR, msi_dialog*);

/*
 * Create a dialog box and run it if it's modal
 */
static UINT event_do_dialog( MSIPACKAGE *package, LPCWSTR name, msi_dialog *parent, BOOL destroy_modeless )
{
    msi_dialog *dialog;
    UINT r;

    /* create a new dialog */
    dialog = msi_dialog_create( package, name, parent,
                                ControlEvent_HandleControlEvent );
    if( dialog )
    {
        /* kill the current modeless dialog */
        if( destroy_modeless && package->dialog )
        {
            msi_dialog_destroy( package->dialog );
            package->dialog = NULL;
        }

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
        package->CurrentInstallState = ERROR_SUCCESS;
    else if (lstrcmpW(argument, szReturn) == 0)
    {
        msi_dialog *parent = msi_dialog_get_parent(dialog);
        msi_free(package->next_dialog);
        package->next_dialog = (parent) ? strdupW(msi_dialog_get_name(parent)) : NULL;
        package->CurrentInstallState = ERROR_SUCCESS;
    }
    else
    {
        ERR("Unknown argument string %s\n",debugstr_w(argument));
        package->CurrentInstallState = ERROR_FUNCTION_FAILED;
    }

    ControlEvent_CleanupDialogSubscriptions(package, msi_dialog_get_name( dialog ));
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
    /* don't destroy a modeless dialogs that might be our parent */
    event_do_dialog( package, argument, dialog, FALSE );
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
    ACTION_PerformAction(package,argument,-1,TRUE);
    return ERROR_SUCCESS;
}

static UINT ControlEvent_AddLocal(MSIPACKAGE* package, LPCWSTR argument, 
                                  msi_dialog* dialog)
{
    static const WCHAR szAll[] = {'A','L','L',0};
    MSIFEATURE *feature = NULL;

    if (lstrcmpW(szAll,argument))
    {
        MSI_SetFeatureStateW(package,argument,INSTALLSTATE_LOCAL);
    }
    else
    {
        LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
            msi_feature_set_state(package, feature, INSTALLSTATE_LOCAL);

        ACTION_UpdateComponentStates(package,argument);
    }
    return ERROR_SUCCESS;
}

static UINT ControlEvent_Remove(MSIPACKAGE* package, LPCWSTR argument, 
                                msi_dialog* dialog)
{
    static const WCHAR szAll[] = {'A','L','L',0};
    MSIFEATURE *feature = NULL;

    if (lstrcmpW(szAll,argument))
    {
        MSI_SetFeatureStateW(package,argument,INSTALLSTATE_ABSENT);
    }
    else
    {
        LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
            msi_feature_set_state(package, feature, INSTALLSTATE_ABSENT);

        ACTION_UpdateComponentStates(package,argument);
    }
    return ERROR_SUCCESS;
}

static UINT ControlEvent_AddSource(MSIPACKAGE* package, LPCWSTR argument, 
                                   msi_dialog* dialog)
{
    static const WCHAR szAll[] = {'A','L','L',0};
    MSIFEATURE *feature = NULL;

    if (lstrcmpW(szAll,argument))
    {
        MSI_SetFeatureStateW(package,argument,INSTALLSTATE_SOURCE);
    }
    else
    {
        LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
            msi_feature_set_state(package, feature, INSTALLSTATE_SOURCE);
        ACTION_UpdateComponentStates(package,argument);
    }
    return ERROR_SUCCESS;
}

static UINT ControlEvent_SetTargetPath(MSIPACKAGE* package, LPCWSTR argument, 
                                   msi_dialog* dialog)
{
    LPWSTR path = msi_dup_property( package, argument );
    MSIRECORD *rec = MSI_CreateRecord( 1 );
    UINT r;

    static const WCHAR szSelectionPath[] = {'S','e','l','e','c','t','i','o','n','P','a','t','h',0};

    MSI_RecordSetStringW( rec, 1, path );
    ControlEvent_FireSubscribedEvent( package, szSelectionPath, rec );

    /* failure to set the path halts the executing of control events */
    r = MSI_SetTargetPathW(package, argument, path);
    msi_free(path);
    msi_free(&rec->hdr);
    return r;
}

static UINT ControlEvent_Reset(MSIPACKAGE* package, LPCWSTR argument, 
                                   msi_dialog* dialog)
{
    msi_dialog_reset(dialog);
    return ERROR_SUCCESS;
}

/*
 * Subscribed events
 */
static void free_subscriber( struct subscriber *sub )
{
    msi_free(sub->event);
    msi_free(sub->control);
    msi_free(sub->attribute);
    msi_free(sub);
}

VOID ControlEvent_SubscribeToEvent( MSIPACKAGE *package, msi_dialog *dialog,
                                    LPCWSTR event, LPCWSTR control, LPCWSTR attribute )
{
    struct subscriber *sub;

    sub = msi_alloc(sizeof (*sub));
    if( !sub )
        return;
    sub->dialog = dialog;
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

    LIST_FOR_EACH_ENTRY( sub, &package->subscriptions, struct subscriber, entry )
    {
        if (lstrcmpiW(sub->event, event))
            continue;
        msi_dialog_handle_event( sub->dialog, sub->control,
                                 sub->attribute, rec );
    }
}

VOID ControlEvent_CleanupDialogSubscriptions(MSIPACKAGE *package, LPWSTR dialog)
{
    struct list *i, *t;
    struct subscriber *sub;

    LIST_FOR_EACH_SAFE( i, t, &package->subscriptions )
    {
        sub = LIST_ENTRY( i, struct subscriber, entry );

        if ( lstrcmpW( msi_dialog_get_name( sub->dialog ), dialog ))
            continue;

        list_remove( &sub->entry );
        free_subscriber( sub );
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
    r = event_do_dialog( package, szDialogName, NULL, TRUE );
    while( r == ERROR_SUCCESS && package->next_dialog )
    {
        LPWSTR name = package->next_dialog;

        package->next_dialog = NULL;
        r = event_do_dialog( package, name, NULL, TRUE );
        msi_free( name );
    }

    if( r == ERROR_IO_PENDING )
        r = ERROR_SUCCESS;

    return r;
}

static UINT ControlEvent_SetInstallLevel(MSIPACKAGE* package, LPCWSTR argument,
                                          msi_dialog* dialog)
{
    int iInstallLevel = atolW(argument);

    TRACE("Setting install level: %i\n", iInstallLevel);

    return MSI_SetInstallLevel( package, iInstallLevel );
}

static UINT ControlEvent_DirectoryListUp(MSIPACKAGE *package, LPCWSTR argument,
                                         msi_dialog *dialog)
{
    return msi_dialog_directorylist_up( dialog );
}

static UINT ControlEvent_ReinstallMode(MSIPACKAGE *package, LPCWSTR argument,
                                       msi_dialog *dialog)
{
    static const WCHAR szReinstallMode[] = {'R','E','I','N','S','T','A','L','L','M','O','D','E',0};
    return MSI_SetPropertyW( package, szReinstallMode, argument );
}

static const struct _events Events[] = {
    { "EndDialog",ControlEvent_EndDialog },
    { "NewDialog",ControlEvent_NewDialog },
    { "SpawnDialog",ControlEvent_SpawnDialog },
    { "SpawnWaitDialog",ControlEvent_SpawnWaitDialog },
    { "DoAction",ControlEvent_DoAction },
    { "AddLocal",ControlEvent_AddLocal },
    { "Remove",ControlEvent_Remove },
    { "AddSource",ControlEvent_AddSource },
    { "SetTargetPath",ControlEvent_SetTargetPath },
    { "Reset",ControlEvent_Reset },
    { "SetInstallLevel",ControlEvent_SetInstallLevel },
    { "DirectoryListUp",ControlEvent_DirectoryListUp },
    { "SelectionBrowse",ControlEvent_SpawnDialog },
    { "ReinstallMode",ControlEvent_ReinstallMode },
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
            msi_free(wevent);
            rc = Events[i].handler(package,argument,dialog);
            return rc;
        }
        msi_free(wevent);
        i++;
    }
    FIXME("unhandled control event %s arg(%s)\n",
          debugstr_w(event), debugstr_w(argument));
    return rc;
}
