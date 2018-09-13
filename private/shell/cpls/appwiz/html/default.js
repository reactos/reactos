// default.js



// Run this code at startup...

var g_idCur = '';

window.onerror = EatErrors;     // Catch onerror events


// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------


/*-------------------------------------------------------------------------
Purpose: Handle the 'onload' event for the whole page. 
         This function processes the commandline paremeters passed in and launches 
         the correct page. 
*/

function Body_OnLoad()
{
    var szCmdLine = idAppARP.commandLine;
    
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
    if (isNaN(nPage))
        nPage = 0;

    if (2 < nPage)
        nPage = 0;
        
    idPlaces.currentButton = nPage;

    // Determine whether to make the Config page simply exec the OC Manager or not
    var bExecOCMgr;
    
    bExecOCMgr = !idCtlAppsDso.ShowPostSetup;
    /* Fake version
    bExecOCMgr = true;
    */
    
    // Allow the places bar to switch to this page using ctrl-tab
    // (normally clicking the button would exec the Optional Components Manager)
    idPlaces.SetExecButton("idConfig", bExecOCMgr);
}


function _CloseWindow()
{
    window.parent.close();
}


function _SetPage(idElem)
{
    // First clean up after the current page
    switch (g_idCur)
    {
    case 'idChangeRemove':
        idPageRemove.style.display = 'none';
        break;

    case 'idAdd':
        idPageAdd.style.display = 'none';
        Add_OnUnload();
        break;

    case 'idConfig':
        idPageConfig.style.display = 'none';
        break;
    }

    // Now set up for the new page
    switch (idElem)
    {
    case 'idChangeRemove':
        Remove_OnLoad();
        break;

    case 'idAdd':
        Add_OnLoad();
        break;

    case 'idConfig':
        Config_OnLoad(false);
        break;
    }

    g_idCur = idElem;
}


/*-------------------------------------------------------------------------
Purpose: Handle the 'onexecitem' for the places bar.
*/
function _OnExecItem()
{
    var idElem = window.event.srcID;
    
    if ('idConfig' == idElem)
    {
        Config_OnLoad(true);
    }
}


/*-------------------------------------------------------------------------
Purpose: Handle the 'onselectitem' for the places bar
*/
function _OnSelectItem()
{
    if (g_idCur == window.event.srcID)
        return;

    _SetPage(window.event.srcID);
}


var KC_F5 = 116;

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


/*-------------------------------------------------------------------------
Purpose: Handle the window's resize
*/
function Body_OnResize()
{
    Add_SizeTable();
}


