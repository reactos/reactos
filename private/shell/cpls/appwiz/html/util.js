// util.js


// Keycode values

var KC_ESCAPE = 27;         // Escape key
var KC_RETURN = 13;         // return key

// App capability flags
// (these values match the APPACTION_* values in shappmgr.idl)

var APPCAP_INSTALL      = 0x0001;
var APPCAP_UNINSTALL    = 0x0002;
var APPCAP_MODIFY       = 0x0004;
var APPCAP_REPAIR       = 0x0008;
var APPCAP_UPGRADE      = 0x0010;
var APPCAP_CANGETSIZE   = 0x0020;
// var APPCAP_???       = 0x0040;
var APPCAP_MODIFYREMOVE = 0x0080;
var APPCAP_ADDLATER     = 0x0100;
var APPCAP_UNSCHEDULE   = 0x0200;



/*-------------------------------------------------------------------------
Purpose: Apply the right styles to the expanded property table that is
         databound.
*/
function ApplyExtraStyles(tblElem, bFocus)
{
    var szFocusClass;
    
    if (bFocus)
        szFocusClass = "Focus";
    else
        szFocusClass = "";
        
    // Apply the selection class to the extended property table 
    // that is inserted by databinding.

    // NOTE: there's something to keep in mind here.  When Trident
    // databinds this span, the inserted table wipes out any class
    // settings that we may set to the existing table.  This means
    // that this function may be called and the class set, and then
    // the whole table is wiped away by a fresh new table (inserted
    // by the databound span element).  The end result is you don't
    // see the effects you want.
    //
    // Currently, the work-around is to make sure the inserted table
    // has the class already set.  This requires the DSO to provide 
    // that class name.
    
    var tblProps = tblElem.all('idTblExtendedProps');
    if (tblProps)
    {
        tblProps.className = szFocusClass;
    }

    // Set the right styles for the anchors

    var rganchor = tblElem.all.tags("A");
    var canchor = rganchor.length;
    for (i = 0; i < canchor; i++)
    {
        rganchor[i].className = szFocusClass;
    }
}


/*-------------------------------------------------------------------------
Purpose: Return the control given the named string
*/
function Dso_GetCtl(szDso)
{
    return g_docAll.idCtlAppsDso;

    /* Fake version
    var ctl = null;
    
    switch (szDso)
    {
    case "Remove":
        ctl = g_docAll.idCtlRemoveApps;
        break;

    case "Add":
        ctl = g_docAll.idCtlAddApps;
        break;
        
    case "Categories":
        ctl = g_docAll.idCtlCategory;
        break;
        
    case "ocsetup":
        ctl = g_docAll.idCtlOcsetup;
        break;
    }
    return ctl;
    */
}


/*-------------------------------------------------------------------------
Purpose: Retrieves the recordset of the given DSO.
*/
function Dso_GetRecordset(szDso)
{
    return g_docAll.idCtlAppsDso.namedRecordset(szDso);

    /* Fake version
    var ctl = Dso_GetCtl(szDso);
    return ctl.recordset;
    */
}


/*-------------------------------------------------------------------------
Purpose: Sorts the given DSO.
*/
function Dso_Sort(szDso, szKey)
{
    g_docAll.idCtlAppsDso.Sort = szKey;
    g_docAll.idCtlAppsDso.Reset(szDso);

    /* Fake version
    var ctl = Dso_GetCtl(szDso);
    ctl.Sort = szKey;
    ctl.Reset();
    */
}


/*-------------------------------------------------------------------------
Purpose: Filters the given DSO.
*/
function Dso_Filter(szDso, szFilter)
{
    g_docAll.idCtlAppsDso.Category = szFilter;
    g_docAll.idCtlAppsDso.Reset(szDso);

    /* Fake version
    var ctl = Dso_GetCtl(szDso);
    ctl.Filter = "cat_id = " + szFilter;
    ctl.Reset();
    */
}


/*-------------------------------------------------------------------------
Purpose: Set the feedback for the add page
*/
function _SetPubWaitingFeedback()
{
    // set the feedback in the addpage. 
    var L_RetrievingApps_Text = "Searching the network for available programs...";
    
    g_docAll.idAddListbox.feedBack = L_RetrievingApps_Text;
}

/*-------------------------------------------------------------------------
Purpose: Triggers the DSO to refresh (reenumerate list and so on)
*/
function Dso_Refresh(szDso)
{
    g_docAll.idCtlAppsDso.Dirty = true;

    if ("Add" == szDso)
        _SetPubWaitingFeedback();

    g_docAll.idCtlAppsDso.Reset(szDso);          // Now reenumerate
    
    /* Fake version
    var ctl = Dso_GetCtl(szDso);
    
    // Reset the enumarea to itself, this will make the list dirty
    ctl.DataURL = ctl.DataURL;
    ctl.Reset();                        // Now reenumerate
    */
}


/*-------------------------------------------------------------------------
Purpose: Set the feedBack property on the given listbox control if
         the dataset is empty.
*/
function Dso_FeedbackIfEmpty(szDso, idListbox, szEmptyFeedback)
{
    // If there is no record available, show the user.  Otherwise, 
    // set the feedback feature to nothing.
    
    var rs = Dso_GetRecordset(szDso); 
    if (rs && rs.state != 0 && rs.RecordCount > 0)
        idListbox.feedBack = "";
    else 
        idListbox.feedBack = szEmptyFeedback;
}

