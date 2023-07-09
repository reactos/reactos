
// Functions for the Config page


/*-------------------------------------------------------------------------
Purpose: Called when the Config page is loaded
*/
function Config_Activate(bExec)
{
    // We need some utility functions
    LoadScriptFile("idScriptUtil", "util.js");

    // Should we show the post-setup list?
    if (bExec)
    {
        // No
        _RunOCMgr();
        return;
    }
    
    g_docAll.idTrHeadMargin_Config.style.display = 'block';
    g_docAll.idTrHeadComponents1_Config.style.display = 'block';
    g_docAll.idTrHeadComponents2_Config.style.display = 'block';
    g_docAll.idTrHeadServices_Config.style.display = 'block';
    g_docAll.idTrBody_Config.style.display = 'block';

    if (false == g_bConfigPageLoaded)
    {
        // Check policy restrictions to show the right sections.  Use the visibility
        // style to maintain text flow.
    
        if (Dso_IsRestricted("NoComponents"))
        {
            g_docAll.idTrHeadComponents1_Config.style.visibility = 'hidden';
            g_docAll.idTrHeadComponents2_Config.style.visibility = 'hidden';
        }

        // Connect the config listbox to the datasource
        g_docAll.idConfigListbox.dataSource = "idCtlAppsDso.ocsetup";
        
        /* Fake version
        g_docAll.idConfigListbox.dataSource = "idCtlOcsetup";
        */
        
        // Set the initial focus on the listbox
        g_docAll.idConfigListbox.Refresh();

        g_docAll.idBtnNTOptions.onclick = _RunOCMgr;
        
        g_bConfigPageLoaded = true;
    }

    Config_SetFocus();
}


/*-------------------------------------------------------------------------
Purpose: Set the initial focus
*/
function Config_SetFocus()
{
    g_docAll.idConfigListbox.focus();
}


/*-------------------------------------------------------------------------
Purpose: Called when the Config page is switched away
*/
function Config_Deactivate()
{
    g_docAll.idTrHeadMargin_Config.style.display = 'none';
    g_docAll.idTrHeadComponents1_Config.style.display = 'none';
    g_docAll.idTrHeadComponents2_Config.style.display = 'none';
    g_docAll.idTrHeadServices_Config.style.display = 'none';
    g_docAll.idTrBody_Config.style.display = 'none';
}


/*-------------------------------------------------------------------------
Purpose: Handler for the 'onSetFocus' listbox event.
*/
function Config_OnSetFocus()
{
    var evt = window.event;

    ApplyExtraStyles(evt.srcChild, evt.bFocus);
}


/*-------------------------------------------------------------------------
Purpose: Handler for the 'onCustomDraw' listbox event.  Fixup the element objects
         as appropriate.
         
*/
function Config_OnCustomDraw()
{
    var evt = window.event;
    var tblElem = evt.srcChild;      // the contents of the row is another table

    if (evt.bSelected && 'postpaint' == evt.drawStage)
    {
        // Attach events and stuff now that the elements have been added
        // to the document tree.
        
        tblElem.all('idBtnConfig').onclick = _Configure;
    }
}


/*-------------------------------------------------------------------------
Purpose: Run the Optional Components Manager
*/
function _RunOCMgr()
{
    g_docAll.idCtlAppsDso.Exec('ocsetup', 'ntoptions', 0);
    /* Fake version
    alert("Run OCManager");
    */
}


/*-------------------------------------------------------------------------
Purpose: Configure the current component
*/
function _Configure()
{
    var rsCur = Dso_GetRecordset("ocsetup");
    
    g_docAll.idCtlAppsDso.Exec("ocsetup", "install", rsCur.AbsolutePosition);

    /* Fake version
    alert('Configure ' + rsCur("displayname"));
    */

    Dso_Refresh("ocsetup");
}


