// preload.js
//
// This file contains the script necessary for the initial load of the page.
// We want to minimize this size as much as possible to mitigate the initial
// hit at load time.
//



/*-------------------------------------------------------------------------
Purpose: Display a generic message box if we hit a script error.
*/
function EatErrors(szMsg, szUrl, iLine)
{
    // Prevent scripting errors from displaying ugly messages
    alert("An unexpected error occurred.\n\n" + szMsg + "\n" + szUrl + "\nLine: " + iLine);
    window.event.returnValue = true;    // Suppress IE error messaging
}


/*-------------------------------------------------------------------------
Purpose: Load the given script file into the tree
*/
function LoadScriptFile(szScriptID, szUrl)
{
    // Is the script loaded?
    if (null == g_docAll(szScriptID))
    {
        // No; create it
        var elemScript = document.createElement("<SCRIPT id=" + szScriptID + " src='" + szUrl + "' language='javascript'></SCRIPT>");

        if (elemScript)
        {
            // Load it by adding it to the body
            document.body.insertBefore(elemScript);
        }
    }
}


/*-------------------------------------------------------------------------
Purpose: Returns true or false for the given policy
*/
function Dso_IsRestricted(szPolicy)
{
    var bResult;
    
    bResult = g_docAll.idCtlAppsDso.IsRestricted(szPolicy);

    /* Fake version
    // alert("IsRestricted: " + szPolicy);
    
    bResult = false;     // default
    
    switch (szPolicy)
    {
    case "ShowPostSetup":           bResult = true;     break;
    case "NoRemovePage":            bResult = false;     break;
    case "NoAddPage":               bResult = false;     break;
    case "NoWindowsSetupPage":      bResult = false;     break;
    case "NoAddFromCDorFloppy":     bResult = false;     break;
    case "NoAddFromInternet":       bResult = false;     break;
    case "NoAddFromNetwork":        bResult = false;     break;
    case "NoComponents":            bResult = false;     break;
    case "NoServices":              bResult = false;     break;
    case "NoSupportInfo":           bResult = false;     break;
    }
    */
    
    return bResult;
}






// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------



/*-------------------------------------------------------------------------
Purpose: Determine which page to show at startup
*/
function _ParseCmdLine()
{
    var szCmdLine = g_docAll.idAppARP.commandLine;
    
    // The command line will be something like: 
    //  "c:\lfnpath\default.hta <params>"
    
    var ichParam = szCmdLine.indexOf("default.hta") + 12;
    var cch = szCmdLine.length;

    while (ichParam < cch)
    {
        if (szCmdLine.charAt(ichParam) != ' ')
            break;
        ichParam++;
    }

    // First parameter is the button index indicating which page
    // to open

    var nPage = parseInt(szCmdLine.charAt(ichParam));
    if (isNaN(nPage) || 2 < nPage)
        nPage = g_iPageDefault;

    // Set the page by pushing the button
    g_docAll.idPlaces.currentButton = nPage;
}


/*-------------------------------------------------------------------------
Purpose: Initialize the Places bar.
*/
function _InitPlacesBar()
{
    var _idPlaces = g_docAll.idPlaces;
    
    if (Dso_IsRestricted("NoRemovePage"))
    {
        _idPlaces.ShowButton('idChangeRemove', false);
        g_iPageDefault++;
    }

    if (Dso_IsRestricted("NoAddPage"))
    {
        _idPlaces.ShowButton('idAdd', false);
        if (1 == g_iPageDefault)
            g_iPageDefault++;
    }
        
    if (Dso_IsRestricted("NoWindowsSetupPage"))
    {
        _idPlaces.ShowButton('idConfig', false);
        if (2 == g_iPageDefault)
            g_iPageDefault++;
    }
}


/*-------------------------------------------------------------------------
Purpose: Handle the 'onload' event for the whole page. 
         This function processes the commandline paremeters passed in and launches 
         the correct page. 
*/

function Body_OnLoad()
{
    window.onerror = EatErrors;     // Catch onerror events

    // Bind to some events
    document.body.onkeydown = Body_OnKeyDown;
    g_docAll.idBtnClose.onclick = _CloseWindow;

    // Determine whether to make the Config page simply exec the OC Manager or not
    var bExecOCMgr;
    
    bExecOCMgr = !g_docAll.idCtlAppsDso.ShowPostSetup;
    
    /* Fake version
    bExecOCMgr = true;
    */

    // The 'ShowPostSetup' policy overrides this.  If this is true, show the config page.
    if (Dso_IsRestricted("ShowPostSetup"))
        bExecOCMgr = false;
    else if (Dso_IsRestricted("NoServices"))
        bExecOCMgr = true;

    // Allow the places bar to switch to this page using ctrl-tab
    // (normally clicking the button would exec the Optional Components Manager)
    var _idPlaces = g_docAll.idPlaces;
    
    _idPlaces.SetExecButton("idConfig", bExecOCMgr);

    // Set the width of the places column.  We set this once at start up.  We avoid
    // expressions since those are more expensive.
    //
    // Trick: we need to set the width of the dummy image too, to make sure 
    // idColPlaces isn't squeezed.
    
    g_docAll.idColPlaces.width = g_docAll.idPlaces.offsetWidth;
    g_docAll.idImgDummy1.width = g_docAll.idPlaces.offsetWidth;
    g_docAll.idImgDummy2.width = g_docAll.idPlaces.offsetWidth;
    g_docAll.idImgDummy3.width = g_docAll.idPlaces.offsetWidth;

    // Parse the command line and set the active page
    _ParseCmdLine();
}


/*-------------------------------------------------------------------------
Purpose: Handle the 'onkeydown' for the documents
*/
function Body_OnKeyDown() 
{
    // Is this a F5 key?
    if (KC_F5 == event.keyCode) 
    {
        // Call the correct refresh functions
        switch (g_idCur)
        {
        case 'idChangeRemove':
            Dso_Refresh("Remove");
            break;

        case 'idAdd':
            Dso_Refresh("Categories");
            Dso_Refresh("Add");
            break;

        case 'idConfig':
            Dso_Refresh("ocsetup");
            break;
        }

        // Block this event
        event.returnValue=false;
    }
}


function _CloseWindow()
{
    window.parent.close();
}


/*-------------------------------------------------------------------------
Purpose: Switch pages based upon the given idElem (button id)
*/
function _SetPage(idElem)
{
    // Load the corresponding script first
    switch (idElem)
    {
    case 'idChangeRemove':
        LoadScriptFile("idScriptRemove", "remove.js");
        break;

    case 'idAdd':
        LoadScriptFile("idScriptAdd", "add.js");
        break;

    case 'idConfig':
        LoadScriptFile("idScriptConfig", "config.js");
        break;
    }


    // Clean up after the current page
    switch (g_idCur)
    {
    case 'idChangeRemove':
        Remove_Deactivate();
        break;

    case 'idAdd':
        Add_Deactivate();
        break;

    case 'idConfig':
        Config_Deactivate();
        break;
    }


    // Now set up for the new page
    switch (idElem)
    {
    case 'idChangeRemove':
        Remove_Activate();
        break;

    case 'idAdd':
        Add_Activate();
        break;

    case 'idConfig':
        Config_Activate(false);
        break;
    }

    g_idCur = idElem;
}


/*-------------------------------------------------------------------------
Purpose: Handle the 'onexecitem' for the places bar.
*/
function Places_OnExecItem()
{
    var idElem = window.event.srcID;
    
    if ('idConfig' == idElem)
    {
        LoadScriptFile("idScriptConfig", "config.js");
        Config_Activate(true);
    }
}


/*-------------------------------------------------------------------------
Purpose: Handle the 'onselectitem' for the places bar
*/
function Places_OnSelectItem()
{
    if (g_idCur == window.event.srcID)
    {
        switch (g_idCur)
        {
        case 'idChangeRemove':
            Remove_SetFocus();
            break;

        case 'idAdd':
            Add_SetFocus();
            break;

        case 'idConfig':
            Config_SetFocus();
            break;
        }
        return;
    }

    _SetPage(window.event.srcID);
}


/*-------------------------------------------------------------------------
Purpose: Handle the 'onreadystatechange' for the places bar
*/
function Places_OnComplete()
{
    // Depending on the speed of the machine, this event might get
    // fired before the inline <SCRIPT> in default.hta is run.  If this
    // is the case, then initialize g_docAll here, since this code uses 
    // it.
    if ("undefined" == typeof g_docAll)
        g_docAll = document.all;
        
    _InitPlacesBar();
}

var KC_SPACE    = 32;
var KC_RETURN   = 13;

/*-------------------------------------------------------------------------
Purpose: Handle the onKeyDown event
*/
function _OnKeyDownFakeAnchor()
{
    var keyCode = window.event.keyCode;

    if (KC_SPACE == keyCode || KC_RETURN == keyCode)
    {
        window.event.srcElement.click();
    }
}

