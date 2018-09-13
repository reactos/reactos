
// Functions for the Remove page


/*-------------------------------------------------------------------------
Purpose: Called when ARP switches to the Remove pane
*/
function Remove_Activate()
{
    // We need some utility functions
    LoadScriptFile("idScriptUtil", "util.js");
    LoadScriptFile("idScriptBplate", "bplate.js");

    InitBoilerPlateClass();
    
    // Show the relavent rows
    g_docAll.idTrHeadMargin_Remove.style.display = 'block';
    g_docAll.idTrHead_Remove.style.display = 'block';
    g_docAll.idTrBody_Remove.style.display = 'block';

    // Is this an alpha machine?
    if (g_bIsAlpha)
    {
        // Yes; turn on the check box and set the 'force x86' property
        g_docAll.idTrFoot_Remove.style.display = 'block';

        g_docAll.idChkRemoveForcex86.attachEvent("onclick", new Function ("idCtlAppsDso.Forcex86 = idChkRemoveForcex86.checked"));
    }

    // Is this pane being activated for the first time?
    if (false == g_bRemovePageLoaded)
    {
        // Yes

        // Check policies for any restrictions
        if (Dso_IsRestricted("NoSupportInfo"))
            g_bShowSupportInfo = false;
        
        g_docAll.idSelSortBy.onchange = _SortDataSource;

        // Connect the remove listbox to the datasource
        g_docAll.idRemoveListbox.dataSource = "idCtlAppsDso.Remove";

        /* Fake version
        g_docAll.idRemoveListbox.dataSource = "idCtlRemoveApps";
        */
        
        Dso_GetCtl("Remove").attachEvent("ondatasetcomplete", Remove_OnDatasetComplete);

        // Set the initial focus to the listbox, and refresh the listbox 
        // so it gets its data.
        //
        g_docAll.idRemoveListbox.Refresh();

        g_bRemovePageLoaded = true;
    }

    g_bReenumAddList = false;
    
    Remove_SetFocus();
}


/*-------------------------------------------------------------------------
Purpose: Set the initial focus
*/
function Remove_SetFocus()
{
    g_docAll.idRemoveListbox.focus();
}


/*-------------------------------------------------------------------------
Purpose: Called when the Remove page is switched away
*/
function Remove_Deactivate()
{
    // Hide the relavent rows
    g_docAll.idTrHeadMargin_Remove.style.display = 'none';
    g_docAll.idTrHead_Remove.style.display = 'none';
    g_docAll.idTrFoot_Remove.style.display = 'none';
    g_docAll.idTrBody_Remove.style.display = 'none';

    if (g_bReenumAddList)
    {
        Dso_Refresh("Add");
    }
}


/*-------------------------------------------------------------------------
Purpose: Handler for the 'onSetFocus' listbox event.
*/
function Remove_OnSetFocus()
{
    var evt = window.event;

    ApplyExtraStyles(evt.srcChild, evt.bFocus);
}


/*-------------------------------------------------------------------------
Purpose: Handler for the 'onCustomDraw' listbox event.  Fixup the element objects
         as appropriate.
         
*/
function Remove_OnCustomDraw()
{
    var evt = window.event;
    var tblElem = evt.srcChild;      // the contents of the row is another table

    if (evt.bSelected)
    {
        // Item is being selected
        var dwCapability = evt.Recordset("capability");

        if ('prepaint' == evt.drawStage)
        {
            // Prepaint stuff

            // alert("Capability=" + dwCapability);
            
            // Show the right set of buttons
            
            // Does this support separate modify/remove buttons?
            if (dwCapability & APPCAP_MODIFYREMOVE)
            {
                // No
                evt.srcElement.EnableTemplate('idTrMultiBtns', false);
                evt.srcElement.EnableTemplate('idTrSingleBtns', true);
            }
            else
            {
                // Yes; show separate modify/remove buttons
                evt.srcElement.EnableTemplate('idTrMultiBtns', true);
                evt.srcElement.EnableTemplate('idTrSingleBtns', false);
            }
        }
        else
        {
            // Postpaint stuff
            var bplate = new BoilerPlate();

            bplate.Parse(evt.Recordset("supportinfo"));
            
            // Does this app have any need for the support info dialog?
            //
            // (Having just the 'displayname' as support info isn't helpful enough
            // to merit showing the dialog)
            //
            // alert("bplate.Length = " + bplate.Length() + "; displayname = " + bplate.Get("displayname"));
            
            if (!g_bShowSupportInfo ||
                !(dwCapability & APPCAP_REPAIR) && 
                (1 > bplate.Length() || 
                 1 == bplate.Length() && null != bplate.Get("displayname")))
            {
                // No; then hide the link to the dialog
                tblElem.all('idTdInfoDesc').style.visibility = 'hidden';
            }
            
            // Attach events and stuff now that the elements have been added
            // to the document tree.
            
            // Does this support separate modify and remove buttons?
            if (dwCapability & APPCAP_MODIFYREMOVE)
            {
                // No
                tblElem.all('idBtnBoth').onclick = _ModifyOrRemove;

                // Disable buttons according to policy
                if ( !(dwCapability & APPCAP_UNINSTALL) )
                    tblElem.all('idBtnBoth').disabled = true;
            }
            else
            {
                // Yes
                tblElem.all('idBtnModify').onclick = _ModifyOrRemove;
                tblElem.all('idBtnRemove').onclick = _ModifyOrRemove;

                // Disable buttons according to policy
                if ( !(dwCapability & APPCAP_MODIFY) )
                    tblElem.all('idBtnModify').disabled = true;
                    
                if ( !(dwCapability & APPCAP_UNINSTALL) )
                    tblElem.all('idBtnRemove').disabled = true;
            }

            // Are we sorting by name or size?
            var szSort = g_docAll.idSelSortBy.options(g_docAll.idSelSortBy.selectedIndex).value;

            if ("displayname" == szSort || "size" == szSort || "timesused" == szSort)
            {
                // Yes; then allow the size field in the properties (the 'index
                // value') to be an anchor so the user can click it for a 
                // definition.
                var spnValue = evt.srcRow.all("idSpnIndexValue");
                var tdValue = spnValue.parentElement;
                var tdId = ("timesused" == szSort ? "idAFrequency" : "idASize");
                
                tdValue._szInner = tdValue.innerHTML;

                // If there is nothing to go inside of the 'anchor', then don't create the 
                // span elements.  We will still replace it with the old _szInner later.
                if (tdValue.innerText != "")
                {
                    tdValue.innerHTML = 
                        "<SPAN id=" +
                        tdId +
                        " class='FakeAnchor' tabIndex=0 onKeyDown='_OnKeyDownFakeAnchor()' onClick='_OpenDefinition();'> " +
                        "&nbsp;<U>" +
                        tdValue._szInner + 
                        "</U></SPAN>";
                }
            }

            // Use the focus state provided by the event
            ApplyExtraStyles(tblElem, evt.bFocus);
        }
    }
    else
    {
        // Item is being deselected
        if ('prepaint' == evt.drawStage)
        {
            // Remove the anchor element from the value field
            var spnValue = evt.srcRow.all("idSpnIndexValue");
            var tdValue = spnValue.parentElement.parentElement.parentElement;
            
            if (null != tdValue._szInner)
            {
                tdValue.innerHTML = tdValue._szInner;
                tdValue._szInner = null;
            }

            // Say focus==false so the style reverts to the default setting
            ApplyExtraStyles(tblElem, false);
        }
    }
}


/*-------------------------------------------------------------------------
Purpose: Display the Support Info dialog
*/
function _OpenSupportInfo()
{
    // Display the Support Info dialog
    var szFeatures = g_szSupportInfoSize + "; resizable:no; help:no";
    
    window.showModalDialog("support.htm", window, szFeatures);
    
    // Don't let the 'A' elem navigate
    window.event.returnValue = false;
    window.event.cancelBubble = true;
}


/*-------------------------------------------------------------------------
Purpose: Display a definition dialog according to the field that was clicked.
*/
function _OpenDefinition()
{
    var elemSrc = window.event.srcElement;

    // We are trying to get the most specific id for looking up a 
    // definition.  We prefer idAFrequency or idASize.  If we
    // get nothing or idSpnIndexValue, check to see if the parent
    // tag is more specific.  It either will be or it will not exist.
    if (("idSpnIndexValue" == elemSrc.id) && ("" != elemSrc.parentElement.parentElement.id))
    {
        elemSrc = elemSrc.parentElement.parentElement;
    }
    
    if ("" == elemSrc.id)
    {
        elemSrc = elemSrc.parentElement;
    }

    if ("idAFrequency" == elemSrc.id)
    {
        var szFeatures = "dialogWidth:20em; dialogHeight:16em; resizable:no; help:no";

        window.showModalDialog("def_freq.htm", window, szFeatures);
    }
    else if ("idASize" == elemSrc.id || "idSpnIndexValue" == elemSrc.id)
    {
        var szFeatures = "dialogWidth:20em; dialogHeight:10.7em; resizable:no; help:no";

        window.showModalDialog("def_size.htm", window, szFeatures);
    }
        
    // Don't let the 'A' elem navigate
    window.event.returnValue = false;
    window.event.cancelBubble = true;
}


/*-------------------------------------------------------------------------
Purpose: Re-sort the given data source object according to the current selection
*/
function _SortDataSource()
{
    var selElem = window.event.srcElement;
    var optCur = selElem.options(selElem.selectedIndex);

    Dso_Sort("Remove", optCur.value);
}


/*-------------------------------------------------------------------------
Purpose: Modify or remove the current app (the current recordset)
*/
function _ModifyOrRemove()
{
    var rsCur = Dso_GetRecordset("Remove");
    var nRec = rsCur.AbsolutePosition;
    
    switch(event.srcElement.id)
    {
    case "idBtnBoth":
        g_docAll.idCtlAppsDso.Exec("Remove", "uninstall", nRec);
        break;

    case "idBtnModify":
        g_docAll.idCtlAppsDso.Exec("Remove", "modify", nRec);
        break;

    case "idBtnRemove":
        g_docAll.idCtlAppsDso.Exec("Remove", "uninstall", nRec);
        break;
    }

    /* Fake version
    switch(event.srcElement.id)
    {
    case "idBtnBoth":
        alert('Remove ' + rsCur("displayname"));
        break;

    case "idBtnModify":
        alert('Change ' + rsCur("displayname"));
        break;

    case "idBtnRemove":
        alert('Remove ' + rsCur("displayname"));
        break;
    }
    */

    if ("idBtnRemove" == event.srcElement.id)
    {
        // Now cause the 'Add' page to re-enumerate since an app may have been
        // removed.  Ideally we'd only do this when we know an app successfully
        // removed, but I'm lazy about trying to figure that out!
        g_bReenumAddList = true;
    }
}


/*-------------------------------------------------------------------------
Purpose: Called by the Support Info help window.  This repairs the given app.
*/
function SupportInfo_Repair(nRecordNumber)
{
    var rsCur = Dso_GetRecordset("Remove");
    var nRecordSav = rsCur.AbsolutePosition;

    window.focus();
    rsCur.AbsolutePosition = nRecordNumber;

    g_docAll.idCtlAppsDso.Exec("Remove", "repair", nRecordNumber);
    /* Fake version
    alert('Repair app ' + rsCur("displayname"));
    */
    
    rsCur.AbsolutePosition = nRecordSav;
}


/*-------------------------------------------------------------------------
Purpose: Called by the Support Info help window.  This returns the structured
         record string containing all info that the Support Info needs.
*/
function SupportInfo_Query()
{
    // Compose the record string.  See comments in support.htm for
    // details on the format.
    var rsCur = Dso_GetRecordset("Remove");
    
    var szRecord = "<recordnumber " + rsCur.AbsolutePosition + ">";
    szRecord += rsCur("supportinfo");
    szRecord += "<capability " + rsCur("capability") + ">";

    return szRecord;
}


/*-------------------------------------------------------------------------
Purpose: Called by the Support Info help window.  Returns the string
         specifying the intended size of the window.  This is a hack
         since Trident doesn't provide this for us.
*/
function SupportInfo_GetDlgSize()
{
    return g_szSupportInfoSize;
}


/*-------------------------------------------------------------------------
Purpose: Handle 'ondatasetcomplete' event fired from DSO
*/
function Remove_OnDatasetComplete()
{
    // Is this dataset complete for Remove?
    if (window.event.qualifier == "Remove")
    {
        // Yes; show this text if the dataset is empty
        var L_RemoveNoneAvailable_Text = "There are no programs installed on this computer";
        
        Dso_FeedbackIfEmpty("Remove", g_docAll.idRemoveListbox, L_RemoveNoneAvailable_Text);
    }
}
