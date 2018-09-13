
// Code for the Add pane


/*-------------------------------------------------------------------------
Purpose: Called by the containing document when the page is 'loaded'.
*/
function Add_Activate()
{
    // We need some utility functions
    LoadScriptFile("idScriptUtil", "util.js");

    if (false == g_bAddPageLoaded)
    {
        g_bIsOnDomain = g_docAll.idCtlAppsDso.OnDomain;
        
        /* Fake version
        g_bIsOnDomain = true;
        */
    }


    // We need to first set the display style to indicate the active page
    // has changed.
    g_docAll.idTrHeadMargin_Add.style.display = 'block';
    g_docAll.idTrHeadCDROM1_Add.style.display = 'block';
    g_docAll.idTrHeadCDROM2_Add.style.display = 'block';
    g_docAll.idTrHeadInternet1_Add.style.display = 'block';
    g_docAll.idTrHeadInternet2_Add.style.display = 'block';
    g_docAll.idTrHeadPub_Add.style.display = 'block';
    g_docAll.idTrBody2_Add.style.display = 'block';

    if (false == g_bAddPageLoaded)
    {
        // Check policy restrictions to show the right sections.  Use the visibility
        // style to maintain text flow.
        
        if (Dso_IsRestricted("NoAddFromCDorFloppy"))
        {
            g_docAll.idTrHeadCDROM1_Add.style.visibility = 'hidden';
            g_docAll.idTrHeadCDROM2_Add.style.visibility = 'hidden';
        }

        if (Dso_IsRestricted("NoAddFromInternet"))
        {
            g_docAll.idTrHeadInternet1_Add.style.visibility = 'hidden';
            g_docAll.idTrHeadInternet2_Add.style.visibility = 'hidden';
        }

        // Is this machine on a domain at all?
        if (false == g_bIsOnDomain || Dso_IsRestricted("NoAddFromNetwork"))
        {
            // No; don't bother showing the published list
            g_docAll.idTrHeadPub_Add.style.visibility = 'hidden';
            g_docAll.idTrBody2_Add.style.visibility = 'hidden';

            g_bIsOnDomain = false;      // override to persist restriction across panes
        }

        // Check the GPO policy for the default category selection.
        g_bSelectDefault = true;
    }
    
    if (false == g_bIsOnDomain)
    {
        // To maintain the Places Bar column, we turn on idTrBody1_Add in place
        // of idTrBody2_Add for this case.
        g_docAll.idTrBody1_Add.style.display = 'block';
    }
    
    g_bReenumInstalledList = false;     // Reset this each time we activate this pane


    // Is this an alpha machine?
    if (g_bIsAlpha)
    {
        // Yes; turn on the check box and set the 'force x86' property
        g_docAll.idTrFoot_Add.style.display = 'block';

        g_docAll.idChkAddForcex86.attachEvent("onclick", new Function("idCtlAppsDso.Forcex86 = idChkAddForcex86.checked"));
    }
    
    // Is this being loaded for the first time?
    if (false == g_bAddPageLoaded)
    {
        // Yes; apply some one-time settings

        // Handle button clicks
        g_docAll.idBtnCDFloppy.onclick = _AddApp;
        g_docAll.idBtnInternet.onclick = _AddApp;

        if (g_bIsOnDomain)
        {
            _SetPubWaitingFeedback();
            
            g_docAll.idSelCategory.onchange = _OnChangeCategories;

            // Bind our stub span element so the categories are enumerated
            g_docAll.idBindCategories.dataFld = 'displayname';
            
            g_docAll.idBindCategories.dataSrc = '#idCtlAppsDso.Categories';
            g_docAll.idSpnAddLaterSchedule.dataSrc = "#idCtlAppsDso.Add"; 
            g_docAll.idAddListbox.dataSource = "idCtlAppsDso.Add";
            /* Fake version
            g_docAll.idBindCategories.dataSrc = '#idCtlCategory';
            g_docAll.idSpnAddLaterSchedule.dataSrc = "#idCtlAddApps";
            g_docAll.idAddListbox.dataSource = "idCtlAddApps";
            */
            
            // Attach to the categories DSO so we know when to populate the dropdown
            Dso_GetCtl("Categories").attachEvent("ondatasetcomplete", Cat_OnDatasetComplete);
            Dso_GetCtl("Add").attachEvent("ondatasetcomplete", Add_OnDatasetComplete);

            // Set the initial focus to the listbox, and refresh the listbox 
            // so it gets its data.
            //
            g_docAll.idAddListbox.Refresh();
        }
        g_bAddPageLoaded = true;
    }

    Add_SetFocus();
}


/*-------------------------------------------------------------------------
Purpose: Set the initial focus
*/
function Add_SetFocus()
{
    g_docAll.idAddListbox.focus();
}


/*-------------------------------------------------------------------------
Purpose: Called by the containing document when the page is switched away.
*/
function Add_Deactivate()
{
    g_docAll.idTrHeadMargin_Add.style.display = 'none';
    g_docAll.idTrHeadCDROM1_Add.style.display = 'none';
    g_docAll.idTrHeadCDROM2_Add.style.display = 'none';
    g_docAll.idTrHeadInternet1_Add.style.display = 'none';
    g_docAll.idTrHeadInternet2_Add.style.display = 'none';
    g_docAll.idTrHeadPub_Add.style.display = 'none';
    g_docAll.idTrFoot_Add.style.display = 'none';
    g_docAll.idTrBody1_Add.style.display = 'none';
    g_docAll.idTrBody2_Add.style.display = 'none';
    
    // Reenumerate the list of installed apps (since we might be 
    // navigating to that page)?

    if (g_bReenumInstalledList)
    {
        // Yes; force a refresh of the installed apps
        Dso_Refresh("Remove");
    }
}


/*-------------------------------------------------------------------------
Purpose: Handler for the 'onSetFocus' listbox event.
*/
function Add_OnSetFocus()
{
    var evt = window.event;

    ApplyExtraStyles(evt.srcChild, evt.bFocus);
}


/*-------------------------------------------------------------------------
Purpose: Handler for the 'onCustomDraw' listbox event.  Fixup the element objects
         as appropriate.
         
*/
function Add_OnCustomDraw()
{
    var evt = window.event;
    var tblElem = evt.srcChild;      // the contents of the row is another table

    if (evt.bSelected)
    {
        // Item is selected
        var dwCapability =  evt.Recordset("capability");

        if ('prepaint' == evt.drawStage)
        {
            // Prepaint stuff
            
            // Show the right set of buttons
            
            // Does this support 'add later'?
            if (dwCapability & APPCAP_ADDLATER)
            {
                // Yes
                evt.srcElement.EnableTemplate('idTrMultiBtns', true);
                evt.srcElement.EnableTemplate('idTrSingleBtns', false);
            }
            else
            {
                // No
                evt.srcElement.EnableTemplate('idTrMultiBtns', false);
                evt.srcElement.EnableTemplate('idTrSingleBtns', true);
            }
        }
        else
        {
            // Postpaint stuff
            
            // Attach events and stuff now that the elements have been added
            // to the document tree.

            // Does this support 'add later'?
            if (dwCapability & APPCAP_ADDLATER)
            {
                // Yes
                tblElem.all("idBtnAdd").onclick = _AddApp;
                tblElem.all("idBtnAddLater").onclick = _AddLater;
            }
            else
            {
                // No
                tblElem.all("idBtnAdd").onclick = _AddApp;
            }

            // Does this app have a support URL?
            var szSupportUrl = evt.Recordset("supporturl");
            if ("" != szSupportUrl)
            {
                // Yes; show the "more info" string
                var spnMoreInfo = tblElem.all("idSpnMoreInfo");

                spnMoreInfo.all("idAMoreInfo").href = szSupportUrl;
                spnMoreInfo.style.display = 'block';
            }
            
            ApplyExtraStyles(evt.srcChild, evt.bFocus);
        }
    }
}


/*-------------------------------------------------------------------------
Purpose: Display the More Info dialog
*/
function _OpenMoreInfo()
{
    window.open(window.event.srcElement.href, "", "");

    // Don't let it navigate
    window.event.returnValue = false;
}


/*-------------------------------------------------------------------------
Purpose: Filter the categories according to the selection list
*/
function _FilterCategories(optElem)
{
    var szFilter;

    _SetPubWaitingFeedback();
    
    if (optElem.value == "all")
        szFilter = "";
    else
        szFilter = optElem.value;

    Dso_Filter("Add", szFilter);
}


/*-------------------------------------------------------------------------
Purpose: Handle the onchange event for the categories drop down
*/
function _OnChangeCategories()
{
    var elem = window.event.srcElement;
    var optElem = elem.options(elem.selectedIndex);

    _FilterCategories(optElem);
}


/*-------------------------------------------------------------------------
Purpose: Add the app 
*/
function _AddApp()
{
    var rsCur = Dso_GetRecordset("Add");
    
    switch(event.srcElement.id)
    {
    case "idBtnAdd":
        g_docAll.idCtlAppsDso.Exec("Add", "install", rsCur.AbsolutePosition);
        break;

    case "idBtnCDFloppy":
        g_docAll.idCtlAppsDso.Exec("Add", "generic install", 0);
        break;

    case "idBtnInternet":
        g_docAll.idCtlAppsDso.Exec("Add", "winupdate", 0);
        break;
    }

    /* Fake version
    switch(event.srcElement.id)
    {
    case "idBtnAdd":
        alert('Add ' + rsCur("displayname"));
        break;

    case "idBtnCDFloppy":
        alert('Add from CD or Floppy');
        break;

    case "idBtnInternet":
        alert('Add from Windows Update');
        break;
    }
    */

    if ("idBtnAdd" == event.srcElement.id || "idBtnInternet" == event.srcElement.id)
    {
        // Now cause the 'Remove' page to re-enumerate since an app may have been
        // installed.  Ideally we'd only do this when we know an app successfully
        // installed, but I'm lazy about trying to figure that out!
        g_bReenumInstalledList = true;
    }
}


/*-------------------------------------------------------------------------
Purpose: Add the app later on a schedule
*/
function _AddLater()
{
    var rsCur = Dso_GetRecordset("Add");
    
    g_docAll.idCtlAppsDso.Exec("Add", "addlater", rsCur.AbsolutePosition);

    /* Fake version
    alert('Add ' + rsCur("displayname") + ' later');
    */
    
    // Don't let the 'A' elem navigate
    window.event.returnValue = false;
    window.event.cancelBubble = true;
}


/*-------------------------------------------------------------------------
Purpose: Change the schedule of the scheduled app.
*/
function _Schedule()
{
    var elemSrc = window.event.srcElement;

    if ("idASchedule" == elemSrc.id)
    {
        var rsCur = Dso_GetRecordset("Add");
        
        g_docAll.idCtlAppsDso.Exec("Add", "addlater", rsCur.AbsolutePosition);

        /* Fake version
        alert('Change schedule for ' + rsCur("displayname"));
        */
    }
        
    // Don't let the 'A' elem navigate
    window.event.returnValue = false;
    window.event.cancelBubble = true;
}


/*-------------------------------------------------------------------------
Purpose: Reset the category list by removing all but the orginal "All" category
*/
function _ResetCategoryList()
{
    var i;
    var optElem;
    var colOptions = g_docAll.idSelCategory.options;

    for (i = 0; i < colOptions.length;)
    {
        optElem = colOptions[i];
        if (optElem.value != "all")         // this does not need to be localized
            optElem.removeNode();
        else
            i++;
    }
}


/*-------------------------------------------------------------------------
Purpose: Initialize the category selection list
*/
function _InitCategoryList()
{
    var rsCur = Dso_GetRecordset("Categories"); 

    if (rsCur.RecordCount > 0)
    {
        var i;
        var szDefault;
        var optElemSel = null;

        // Determine what the default category should be according
        // to the GPO policy.

        szDefault = g_docAll.idCtlAppsDso.DefaultCategory;
        /* Fake version
        szDefault = "{C}";
        */

        // Is there a default category?
        if ("" == szDefault)
        {
            // No; select the "All" category
            szDefault = "all";      // This does not need to be localized
        }

        // Now add the items to the dropdown
        rsCur.MoveFirst();
        for (i = 1; i <= rsCur.RecordCount; i++)
        {
            var optElem = document.createElement("option");

            // Right now we use the displayname as the id

            optElem.value = rsCur("displayname");

            /* Fake version
            optElem.value = rsCur("id");
            */

            optElem.text = rsCur("displayname");

            // Do we select this item?
            if (g_bSelectDefault && optElem.value == szDefault)
            {
                optElem.selected = true;        // Yes
                optElemSel = optElem;
            }
                
            g_docAll.idSelCategory.add(optElem);      // add it
            rsCur.MoveNext();
        }

        if (optElemSel)
            _FilterCategories(optElemSel);
            
        g_bSelectDefault = false;   // Reset this so we don't always reselect the default
    } 
}

    
/*-------------------------------------------------------------------------
Purpose: Handle 'ondatasetcomplete' event fired from DSO
*/
function Cat_OnDatasetComplete()
{
    // Is this dataset complete for categories
    if (window.event.qualifier == "Categories")
    {
        _ResetCategoryList();
        _InitCategoryList();
    }
}


/*-------------------------------------------------------------------------
Purpose: Handle 'ondatasetcomplete' event fired from DSO
*/
function Add_OnDatasetComplete()
{
    // Is this dataset complete for add?
    if (window.event.qualifier == "Add")
    {
        // Yes; show this text if the dataset is empty
        var L_AddNoneAvailable_Text = "No programs are available on the network";

        Dso_FeedbackIfEmpty("Add", g_docAll.idAddListbox, L_AddNoneAvailable_Text);
    }
}


