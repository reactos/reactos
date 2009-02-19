    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007 Klemens Friedl <frik85@reactos.org>
                  2009 Danny Götte <dangerground@web.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    */

var search_phrase = 'Enter name to search for';

var mousex, mousey;
// check for quirks / standard mode
var IEmode = ( typeof document.compatMode != "undefined" && document.compatMode != "BackCompat") ? "documentElement" : "body";

var timerTooltip;


document.onmousemove = getMousePosition;



/**
 *
 * @param e
 */
function getMousePosition( e )
{
  // update mouse position for some browsers
  if (e) {
    mousex = e.pageX;
    mousey = e.pageY;
  }

  // other browser
  else {
    mousex = window.event.x;
    mousey = window.event.y;
  }

  // IE needs additional scrollbar position
  if (document.all && !document.captureEvents) {
    mousex    += document[docEl].scrollLeft;
    mousex    += document[docEl].scrollTop;
  }
  
  
  if (document.getElementById('tooltip').style.display == 'block') {
    setTooltipPosition();
  }

} // end of function getMousePosition



/**
 *
 */
function setTooltipPosition( )
{
  if (document.getElementById('tooltip').style.display == 'block') {
    document.getElementById('tooltip').style.top=(mousey+17)+"px";
    document.getElementById('tooltip').style.left=(mousex+17)+"px";
  }

} // end of function getMousePosition



/**
 * inverts checkbox selection
 *
 * @param string cbid checkbox id
 */
function selectRow( cbid )
{
  // check for checkbox id
  if (cbid.substr(0,2) === 'cb') {
    cbid = cbid.substr(2);
  }

  if (document.getElementById("cb"+cbid).checked === true) {
    document.getElementById("cb"+cbid).checked = false;
  }
  else {
    document.getElementById("cb"+cbid).checked = true;
  } 
} // end of function selectRow



/**
 * selects/deselects all rows
 *
 * @param string status true=select all / false=deselect all
 */
function selectAll( status )
{
  var rowstatus;
  var i;

  // select/deselect all rows
  for (i=1; i<=nres; i++) {
    if (status) {
      setRowColor('tr'+i,"#ffcc99");
    }
    else {
      rowstatus = document.getElementById("tr"+i).className;
      setRowColorByStatus('tr'+i, rowstatus);
    }
  }

  // select/deselect all checkboxes
  for (i=1; i<=nres; i++) {
    document.getElementById("cbtr"+i).checked = status;
  }
} // end of function selectAll



/**
 * returns a list with selected rows in the entry table
 */
function selectedEntries( )
{
  var n_ids=0; //number of ids in cookie str
  var mvstr = " ";
  var currentcolor = "";

  for (var i=1; i<=nres; i++) {
    currentcolor = document.getElementById("tr"+i).getElementsByTagName('td')[1].style.backgroundColor;
    if(currentcolor == "rgb(255, 204, 153)" || currentcolor == "#ffcc99") {
      n_ids++;
      if (n_ids>500) {
        alert("Maximum: 500 entries");
        break;
      }

      var tsplit1 = document.getElementById('bstar_'+i).parentNode.className.split("|");
      var tsplit2 = tsplit1[1].split("-");

      mvstr += "-"+ i +"_"+ tsplit2[0];
    }
  }
  return n_ids + "|"+ mvstr.substr(2, mvstr.length);
} // end of function selectedEntries



/**
 * Inverts selected rows
 */
function selectInverse( )
{
  var currentcolor, rowstatus;

  for (var i=1; i<=nres; i++) {
    currentcolor = document.getElementById("tr"+i).getElementsByTagName('td')[1].style.backgroundColor;
    if(currentcolor == "rgb(255, 204, 153)" || currentcolor == "#ffcc99") {

      rowstatus = document.getElementById("tr"+i).className;
      setRowColorByStatus('tr'+i, rowstatus);
      document.getElementById("cbtr"+i).checked = false;
    }
    else {
      setRowColor(i,"#ffcc99");
      document.getElementById("cbtr"+i).checked = true;
     }
  }
} // end of function selectInverse



/**
 * Selects all bookmarked/non-bookmarked rows
 *
 * @param bool status true - select bookmarked rows, false - select non-bookmarked rows
 */
function selectStars( status )
{
  // deselect all
  selectAll(false);

  var sstar = status?'cStarOn':'cStartOff';

  // select choosen ones
  for (var i=1; i<=nres; i++) {
    if (document.getElementById("tr"+i).getElementsByTagName('td')[1].getElementsByTagName('div')[0].className == sstar) {
      setRowColor('tr'+i,"#ffcc99");
      document.getElementById("cbtr"+i).checked = true;
    }
  }
} // end of function selectStars



/**
 * selects all entries from the given type
 *
 * @param string type see switch statement to get typenames
 */
function selectType( type )
{
  // deselect all rows
  selectAll(false); /* deselect all */

  var sstar1 = '';
  var sstar2 = '';

  switch (type) {
    case 'new':
      sstar1 = 'new';
      break;
    case 'draft':
      sstar1 = 'draft';
      break;
    case 'transg':
      sstar1 = 'transg';
      break;
    case 'transb':
      sstar1 = 'transb';
      break;
    case 'transr':
      sstar1 = 'transr';
      break;
    case 'stable':
      sstar1 = 'odd';
      sstar2 = 'even';
      break;
    default:
    case 'unknown':
      sstar1 = 'unknown';
      break;
  } // end switch

  // select items
  for (var i=1; i<=nres; i++) {
    if (document.getElementById("tr"+i).className == sstar1 || document.getElementById("tr"+i).className == sstar2) {
      setRowColor('tr'+i,"#ffcc99");
      document.getElementById("cbtr"+i).checked = true;
    }
  }
} // end of function selectType



/**
 * set or reset search filter (in filter loading process)
 *
 * @param object objid
 * @param string objval
 * @param string objhint
 * @param bool clear
 */
function searchFilter( objid, objval, objhint, clear )
{
  if (clear && objval == objhint) {
    document.getElementById(objid).value = '';
    document.getElementById(objid).style.color ='#000000';
  }
  else if (!clear && objval === '') {
    document.getElementById(objid).value = objhint;
    document.getElementById(objid).style.color = '#999999';
  }
} // end of function searchFilter



/**
 * Requests tooltip data
 *
 * @param string id_set set of data & rev ids in the following format data_id|rev_id
 */
function loadTooltip( id_set)
{
  var qistr = id_set.split('|');

  // deactivate tooltip-timer
  window.clearTimeout(timerTooltip);

  // perform request
  makeRequest('?page=backend&type=text&subtype=tt&d_val=ptm&d_id='+encodeURIComponent(qistr[0].substr(2))+'&d_r_id='+encodeURIComponent(qistr[1]), 'tt', 'tooltip', 'html', 'GET', '');
} // end of function loadTooltip



/**
 * Disables Tooltip view
 */
function clearTooltip( )
{
  // deactivate tooltip-timer
  window.clearTimeout(timerTooltip);

  document.getElementById('tooltip').style.display = 'none';
} // end of function clearTooltip



/**
 * wrapper for addUserFilterShared
 *
 * @param string uf_str
 */
function addUserFilter( uf_str )
{
  addUserFilterShared(uf_str, 'labtitel2c', 'ufs');
} // end of function addUserFilter



/**
 * wrapper for deleteUserFilterShared
 *
 * @param string uf_id
 * @param string uf_name
 */
function deleteUserFilter( uf_id, uf_name )
{
  deleteUserFilterShared( uf_id, uf_name, 'labtitel2c', 'ufs' );
} // end of function deleteUserFilter



/**
 * loads a user filter setting
 *
 * @param string ufiltstr
 * @param int ufilttype
 * @param string ufilttitel
 */
function selectUserFilter( ufiltstr, ufilttitel )
{
  var tentrs = selectedEntries().split("|");

  if (tentrs[0] > 0) {
    makeRequest('?page=backend&type=text&subtype=mef&d_fl=changetags&count='+encodeURIComponent(tentrs[0])+'&d_val2='+encodeURIComponent(tentrs[1])+'&d_val3=tg&d_val4='+ufilttitel, 'mef', 'changetags', 'html', 'GET', '');
  }
  else {
    highlightTab('smenutab8');

    filtstring2 = ufiltstr;

    // reset search box:
    filtstring1 = '';
    document.getElementById('txtfind').value = '';
    searchFilter('txtfind', document.getElementById('txtfind').value, search_phrase, false);

    selectAll(false); /* deselect all */

    loadEntryTable('all');
    htmlFilterChoices(ufiltstr);
  }
} // end of function selectUserFilter



/**
 * loads a tagged entries by tag value
 *
 * @param string value
 */
function selectUserTag( value )
{
  var tag_filter = 'a_is_'+value;

  highlightTab('smenutab8');

  filtstring2 = tag_filter;

  // reset search box:
  filtstring1 = '';
  document.getElementById('txtfind').value = '';
  searchFilter('txtfind', document.getElementById('txtfind').value, search_phrase, false);

  selectAll(false); /* deselect all */

  loadEntryTable('all');
  htmlFilterChoices(tag_filter);
} // end of function selectUserTag



/**
 * setup a new active menu entry (left side)
 *
 * @param string objid item to highlight
 */
function highlightTab( objid )
{
  for (var i=1; i<=smenutabs; i++) {

    // highlight menu_tab_button
    if (i == objid.substring(8)) {
      document.getElementById('smenutab'+i).className = 'lmItemTopSelected';
    }
    else {
      document.getElementById('smenutab'+i).className = 'lmItemTop';
    }
  }
} // end of function highlightTab



/**
 * table mouse events
 */
function registerMouseActions( )
{
  var j;

  if (!document.getElementById) {
    return;
  }


  // Shows Tooltip window for a specific time, and then hides it again 
  function localStartActive()
  {
    hlRow(this.id,1);

    // deactivate Tooltip-timer
    window.clearTimeout(timerTooltip); 

    timerTooltip = window.setTimeout("loadTooltip('"+this.getElementsByTagName('td')[3].className+"')", 500);
  } // end of inner function localStartActive 


  //sets a timeout to remove Tooltip
  function localStopActive() {
    hlRow(this.id,2);

    // deactivate Tooltip-timer
    window.clearTimeout(timerTooltip); 

    clearTooltip();
  } // end of inner function localStopActive


  function localSetSelected() {
    hlRow(this.parentNode.id,3);
    selectRow(this.parentNode.id);
  } // end of inner function localSetSelected


  function localSetBookmark() {
    setBookmark(this.className, document.getElementById('bstar_'+this.parentNode.id.substr(2,4)).className, roscms_intern_account_id, 'bstar_'+this.parentNode.id.substr(2,4));
  } // end of inner function localSetBookmark


  function localStartEditor() {
    loadEditor(roscms_current_page, this.className);
  } // end of inner function localStartEditor


  //row highlighting
  for (var i=1; i<=nres; i++) {

    document.getElementById("tr"+i).onmouseover = localStartActive;
    document.getElementById("tr"+i).onmouseout = localStopActive;
    document.getElementById("tr"+i).getElementsByTagName('td')[0].onclick = localSetSelected;
    document.getElementById("tr"+i).getElementsByTagName('td')[1].onclick = localSetBookmark;

    // support up to 13 columns
    for(j=2; j<13; j++) {
      if (document.getElementById("tr"+i).getElementsByTagName('td')[j]) {
        document.getElementById("tr"+i).getElementsByTagName('td')[j].onclick = localStartEditor;
      }
      else {
        break;
      }
    }
  } // end for
} // end of function registerMouseActions



/**@CLEAR
 * loads Table with entries
 *
 * @param string objevent
 */
function loadEntryTable( objevent )
{
  if (document.getElementById('frametable').style.display !== 'block') {
    document.getElementById('frametable').style.display = 'block';
    document.getElementById('frameedit').style.display = 'none';
    document.getElementById('previewarea').style.display = 'none';
    document.getElementById('newentryarea').style.display = 'none';

    // deactivate alert-timer
    alertboxClose(0);
  }

  // update table-cmdbar
  switch (objevent) {
    case 'new':
      htmlCommandBar('new');
      htmlSelectPresets('basic');
      break;
    case 'page':
    case 'dynamic':
    case 'content':
      htmlCommandBar('page');
      htmlSelectPresets('basic');
      break;
    case 'script':
      htmlCommandBar('script');
      htmlSelectPresets('basic');
      break;
    case 'draft':
      htmlCommandBar('draft');
      htmlSelectPresets('basic');
      break;
    case 'translate':
      htmlCommandBar('trans');
      htmlSelectPresets('trans');
      break;
    case 'archive':
      htmlCommandBar('archive');
      htmlSelectPresets('archive');
      break;
    case 'starred':
      htmlCommandBar('all');
      htmlSelectPresets('bookmark');
      break;
    default:
      htmlCommandBar('all');
      htmlSelectPresets('common');
      break;
  } // end switch

  // send request
  makeRequest('?page=backend&type=xml&subtype=ptm&d_fl='+objevent+'&d_filter='+filtstring1+'&d_filter2='+filtstring2+'&d_cp=0', 'ptm', 'tablist', 'xml', 'GET', '');
} // end of function loadEntryTable



/**
 * Reloads the entry table with an offset
 *
 * @param int offset
 */
function reloadEntryTableWithOffset( offset )
{
  // deactivate alert-timer
  window.clearTimeout(alertactiv);
  document.getElementById('alertbox').style.visibility = 'hidden';
  document.getElementById('alertboxc').innerHTML = '&nbsp;';

  // deselect all table entries
  selectAll(false); 

  makeRequest('?page=backend&type=xml&subtype=ptm&d_fl='+roscms_current_page+'&d_filter='+filtstring1+'&d_filter2='+filtstring2+'&d_cp='+offset, 'ptm', 'tablist', 'xml', 'GET', '');
} // end of function reloadEntryTableWithOffset



/**
 * closes edit frame, and loads the entry table again
 *
 * @param int offset
 */
function loadEntryTableWithOffset( offset )
{
  if (document.getElementById('frametable').style.display !== 'block') {
    document.getElementById('frametable').style.display = 'block';
    document.getElementById('frameedit').style.display = 'none';
    document.getElementById('previewarea').style.display = 'none';
    document.getElementById('newentryarea').style.display = 'none';
  }

  reloadEntryTableWithOffset( offset );
} // end of function loadEntryTableWithOffset



/**
 * reloads the entry table (maybe selected editor before)
 */
function reloadEntryTable( )
{
  if (document.getElementById('frametable').style.display !== 'block') {
    document.getElementById('frametable').style.display = 'block';
    document.getElementById('frameedit').style.display = 'none';
    document.getElementById('previewarea').style.display = 'none';
    document.getElementById('newentryarea').style.display = 'none';
  }

  // deactivate alert-timer
  window.clearTimeout(alertactiv); 
  document.getElementById('alertbox').style.visibility = 'hidden';
  document.getElementById('alertboxc').innerHTML = '&nbsp;';

  // deselect all table entries
  selectAll(false);

  makeRequest('?page=backend&type=xml&subtype=ptm&d_fl='+roscms_current_page+'&d_filter='+filtstring1+'&d_filter2='+filtstring2, 'ptm', 'tablist', 'xml', 'GET', '');
} // end of function reloadEntryTable



/**
 * shows the edit frame which allows to edit an entry
 */
function showEditor( )
{
  if (document.getElementById('frameedit').style.display !== 'block') {
    document.getElementById('frametable').style.display = 'none';
    document.getElementById('previewarea').style.display = 'none';
    document.getElementById('newentryarea').style.display = 'none';
    document.getElementById('frameedit').style.display = 'block';
  }

  // deactivate alert-timer
  window.clearTimeout(alertactiv);
  document.getElementById('alertbox').style.visibility = 'hidden';
  document.getElementById('alertboxc').innerHTML = '&nbsp;';

  // deselect all table entries
  selectAll(false); 
} // end of function showEditor



/**
 * calls the new entry dialog
 *
 * @param string objevent
 * @param string entryid
 */
function newEntryDialog( objevent)
{
  makeRequest('?page=backend&type=text&subtype=ned&action=dialog&tab=single', 'ned', 'newentryarea', 'html', 'GET', '');
} // end of function newEntryDialog



/**
 * initialises the editor
 *
 * @param string objevent
 * @param string entryid
 */
function loadEditor( objevent, entryid )
{
  switch (objevent) {
    case 'diffentry':
      showEditor();
      document.getElementById('frmedithead').innerHTML = '<span class="virtualLink" onclick="loadEntryTableWithOffset(roscms_current_tbl_position)"><strong>&laquo; Back</strong></span> &nbsp; <strong>Compare two Entries</strong>';
      break;

    default:
      if (entryid.indexOf("|") > -1) {
        var devideids1 = entryid.indexOf("|");
        var devideids2 = entryid.substr(2, devideids1-2);
        var devideids3 = entryid.substr(devideids1+1);

        if (devideids2.substr(0,2) === 'tr') {
          var uf_check = confirm("Do you want to translate this entry?");
          if (!uf_check) {
            break;
          }
          alertbox('Translation copy created.');
        }
        else if (devideids2 === 'notrans') {
          alertbox('You don\'t have enough rights to translate this entry.');
          break;
        }

        showEditor();

        roscms_prev_page = roscms_current_page;
        roscms_current_page = objevent;

        document.getElementById('frmedithead').innerHTML = '<span class="virtualLink" onclick="loadEntryTableWithOffset(roscms_current_tbl_position)"><strong>&laquo; Back</strong></span> &nbsp; <strong>Edit Entry</strong>';

        // enable autosave
        autosave_timer = window.setTimeout("tryAutosave()", autosave_coundown);

        // loading screen:
        document.getElementById('editzone').innerHTML = '<div style="background:white; border-bottom: 1px solid #bbb; border-right: 1px solid #bbb;margin:10px;padding: 2em 0px;width:95%;text-align: center;"><img src="'+roscms_intern_webserver_roscms+'images/ajax_loading.gif" alt="loading ..." style="width:13px;height:13px;" />&nbsp;&nbsp;loading ...</div>';

        makeRequest('?page=backend&type=text&subtype=mef&d_fl='+objevent+'&d_id='+devideids2+'&d_r_id='+devideids3+'&d_r_lang='+userlang, 'mef', 'editzone', 'html', 'GET', '');
      }
      else {
        alert('bad request: loadEditor('+objevent+', '+entryid+')');
      }
      break;
  } // end switch
} // end of function loadEditor



/**
 * is another filter selected -> we need to update the possible filter values
 *
 * @param string objid
 */
function isFilterChanged( objid )
{
  var filtselstr = '';
  var filtselstr2 = '';

  getActiveFilters();

  filtselstr = filtstring2.split('|');
  filtselstr[(objid.substring(3)-1)] = document.getElementById(objid).value+'__';

  filtselstr2 = '';
  for (var i=0; i < filtselstr.length; i++) {
    filtselstr2 += filtselstr[i]+'|';
  }

  // remove last '|'
  filtselstr2 = filtselstr2.substr(0,filtselstr2.length-1); 

  htmlFilterChoices(filtselstr2);
} // end of function isFilterChanged



/**
 * sets the search filter back to his standard text
 */
function clearSearchFilter( )
{
  document.getElementById('filtersct').innerHTML = '';
  document.getElementById('txtfind').value = '';
  searchFilter('txtfind', document.getElementById('txtfind').value, search_phrase, false);
  filtercounter = 0;
  filterid = 0;
} // end of function clearSearchFilter



/**
 * resets all filters
 */
function clearAllFilter( )
{
  clearSearchFilter();

  filtstring1 = '';
  filtstring2 = '';
  loadEntryTable(roscms_current_page);
} // end of function clearAllFilter



/**
 * get all filters + searchbox
 */
function getAllActiveFilters( )
{
  filtstring1 = '';

  // without filters visible
  if (document.getElementById('filtersc').style.display === 'none') { 
    if (document.getElementById('txtfind').value != search_phrase) {
      if (document.getElementById('txtfind').value.length > 1) {
        filtstring1 = document.getElementById('txtfind').value;
      }
    }

    getActiveFilters();
    loadEntryTable(roscms_current_page);
  }
} // end of function getAllActiveFilters



/**
 * performs a new search by current filter settings
 */
function searchByFilters( )
{
  filtstring1 = '';
  if (document.getElementById('txtfind').value != search_phrase && document.getElementById('txtfind').value.length > 1) {
    filtstring1 = document.getElementById('txtfind').value;
  }

  getActiveFilters();
  loadEntryTable(roscms_current_page);
} // end of function searchByFilters



/**
 * setup a list with presets for selectable rows
 *
 * @param string preset
 */
function htmlSelectPresets( preset )
{
  var selbarstr = '';

  // prepare selections
  var selhtml_all = '<option onclick="selectAll(true)">All</option>';
  var selhtml_none = '<option onclick="selectAll(false)">None</option>';
  var selhtml_inv = '<option onclick="selectInverse()">Inverse Selection</option>';
  var selhtml_star = '<option onclick="selectStars(true)">Bookmarked</option>';
  var selhtml_nostar = '<option onclick="selectStars(false)">Not Bookmarked</option>';
  var selhtml_stable = '<option onclick="selectType(\'stable\')">Published</option>';
  var selhtml_new = '<option onclick="selectType(\'new\')">Pending</option>';
  var selhtml_draft = '<option onclick="selectType(\'draft\')">Draft</option>';
  var selhtml_uptodate = '<option onclick="selectType(\'transg\')">Up-to-date Translations</option>';
  var selhtml_outdated = '<option onclick="selectType(\'transr\')">Outdated Translations</option>';
  var selhtml_notrans = '<option onclick="selectType(\'transb\')">Missing Translations</option>';

  // use for all types
  selbarstr += '<select>'
    + '<option selected="selected">Select:</option>'
    + selhtml_all
    + selhtml_none
    + selhtml_inv;

  // build selection thing
  switch (preset) {
    case 'common':
      selbarstr += selhtml_star
        + selhtml_nostar
        + selhtml_stable
        + selhtml_new
        + selhtml_draft;
      break;

    case 'trans':
      selbarstr += selhtml_star
        + selhtml_nostar
        + selhtml_uptodate
        + selhtml_outdated
        + selhtml_notrans;
      break;

    case 'basic':
      selbarstr += selhtml_star
        + selhtml_nostar;
      break;

    case 'bookmark':
      selbarstr += selhtml_stable
        + selhtml_new
        + selhtml_draft;
      break;

    case 'archive':
      // cut off last space
      selbarstr = selbarstr.substring(0, selbarstr.length-2);
      break;
  } // end select

  selbarstr += '</select>';

  // update current
  document.getElementById('tabselect1').innerHTML = selbarstr;
  document.getElementById('tabselect2').innerHTML = selbarstr;
} // end of function htmlSelectPresets



/**
 * requests the revisions tab in the entry details
 *
 * @param int drid revision id
 */
function showEditorTabRevisions( drid )
{
  alertbox('Change fields only if you know what you are doing.');
  makeRequest('?page=backend&type=text&subtype=mef&d_fl=showentry&d_r_id='+drid, 'mef', 'frmedittagsc2', 'html', 'GET', '');
} // end of function showEditorTabRevision



/**
 * saves changes made in the editor entry details revisions tab
 *
 * @param int drid revision id
 */
function saveRevisionData( drid )
{
  var uf_check = confirm("Please double check your changes.\n\nDo you want to continue?");

  if (uf_check === true) {
    var d_lang_str = document.getElementById('cbmentrylang').value;
    var d_revnbr_str = document.getElementById('vernbr').value;
    var d_usr_str = beautifystr2(document.getElementById('verusr').value);
    var d_date_str = document.getElementById('verdate').value;
    var d_time_str = document.getElementById('vertime').value;

    // remove leading space character
    if (d_usr_str.substr(0, 1) === ' ') {
      d_usr_str = d_usr_str.substr(1, d_usr_str.length-1); 
    }

    makeRequest('?page=backend&type=text&subtype=mef&d_fl=alterentry&d_r_id='+drid+'&d_val='+d_lang_str+'&d_val2='+d_revnbr_str+'&d_val3='+d_usr_str+'&d_val4='+d_date_str+'&d_val5='+d_time_str, 'mef', 'editalterentry', 'html', 'GET', '');
  }
} // end of function saveRevisionData



/**
 * requests the security tab in the entry details
 *
 * @param int did data id
 * @param int drid revision id
 */
function showEditorTabSecurity( drid )
{
  alertbox('Changes will affect all related entries (see \'History\').');
  makeRequest('?page=backend&type=text&subtype=mef&d_fl=showsecurity&d_r_id='+drid, 'mef', 'frmedittagsc2', 'html', 'GET', '');
} // end of function showEditorTabSecurity



/**
 * saves changes made in the editor entry details security tab
 *
 * @param int did data id
 * @param int drid revision id
 */
function saveSecurityData( did, drid )
{
  var uf_check = confirm("Please double check your changes. \n\nOnly a limited ASCII charset is allowed. \nYou will be on the save side, if you use only A-Z, 0-9, underscore, comma, dot, plus, minus. \n\nDo you want to continue?");

  if (uf_check === true) {
    var d_name_str = beautifystr2(document.getElementById('secdataname').value);
    var d_type_str = document.getElementById('cbmdatatype').value;
    var d_acl_str = document.getElementById('cbmdataacl').value;
    var d_name_update = document.getElementById('chdname').checked;

    // remove leading space character
    if (d_name_str.substr(0, 1) === ' ') {
      d_name_str = d_name_str.substr(1, d_name_str.length-1); 
    }

    makeRequest('?page=backend&type=text&subtype=mef&d_fl=altersecurity&d_id='+did+'&d_r_id='+drid+'&d_val='+d_name_str+'&d_val2='+d_type_str+'&d_val3='+d_acl_str+'&d_val4='+d_name_update, 'mef', 'editaltersecurity', 'html', 'GET', '');
  }
} // end of function saveSecurityData



/**
 * saves changes made in the editor entry details fields tab
 *
 * @param int did data id
 * @param int drid revision id
 */
function saveFieldData( did, drid )
{
  var i;
  var uf_check = confirm("Please double check your changes. \n\nOnly a limited ASCII charset is allowed. \nYou will be on the save side, if you use only A-Z, 0-9, comma, dot, plus, minus. \n\nDo you want to continue?");

  if (uf_check === true) {
    var stext_str = '';
    var text_str = '';
    var tmp_str;

    // process short texts
    for (i=1; i <= document.getElementById('editaddstextcount').innerHTML; i++) {
      tmp_str = beautifystr(document.getElementById('editstext'+i).value);

      // remove leading space character
      if (tmp_str.substr(0, 1) === ' ') {
        tmp_str = tmp_str.substr(1, tmp_str.length-1); 
      }

      stext_str += document.getElementById('editstextorg'+i).value +'='+ tmp_str +'='+ document.getElementById('editstextdel'+i).checked +'|';
    } // end for
    stext_str = stext_str.substr(0, stext_str.length-1);

    // process texts
    for (i=1; i <= document.getElementById('editaddtextcount').innerHTML; i++) {
      tmp_str = beautifystr(document.getElementById('edittext'+i).value);

      // remove leading space character
      if (tmp_str.substr(0, 1) === ' ') {
        tmp_str = tmp_str.substr(1, tmp_str.length-1); 
      }

      text_str += document.getElementById('edittextorg'+i).value +'='+ tmp_str +'='+ document.getElementById('edittextdel'+i).checked +'|';
    } // end for
    text_str = text_str.substr(0, text_str.length-1);
    makeRequest('?page=backend&type=text&subtype=mef&d_fl=alterfields2&d_id='+did+'&d_r_id='+drid+'&d_val='+stext_str+'&d_val2='+text_str, 'mef', 'editalterfields', 'html', 'GET', '');
  } // end if uf_check
} // end of function saveFieldData



/**
 * add a new short text field
 */
function addShortTextField( )
{
  var textcount = document.getElementById('editaddstextcount').innerHTML*1 + 1;

  document.getElementById('editaddstext').innerHTML += '<input type="text" name="editstext'+textcount+'" id="editstext'+textcount+'" size="25" maxlength="100" value="" />&nbsp;';
  document.getElementById('editaddstext').innerHTML += '<input type="checkbox" name="editstextdel'+textcount+'" id="editstextdel'+textcount+'" value="del" /><label for="editstextdel'+textcount+'">delete?</label>';
  document.getElementById('editaddstext').innerHTML += '<input name="editstextorg'+textcount+'" id="editstextorg'+textcount+'" type="hidden" value="new" /><br /><br />';
  document.getElementById('editaddstextcount').innerHTML = textcount;
} // end of function addShortTextField



/**
 * add a new text field
 */
function addTextField( )
{
  var textcount = document.getElementById('editaddtextcount').innerHTML*1 + 1;

  document.getElementById('editaddtext').innerHTML += '<input type="text" name="edittext'+textcount+'" id="edittext'+textcount+'" size="25" maxlength="100" value="" />&nbsp;';
  document.getElementById('editaddtext').innerHTML += '<input type="checkbox" name="edittextdel'+textcount+'" id="edittextdel'+textcount+'" value="del" /><label for="edittextdel'+textcount+'">delete?</label>';
  document.getElementById('editaddtext').innerHTML += '<input name="edittextorg'+textcount+'" id="edittextorg'+textcount+'" type="hidden" value="new" /><br /><br />';
  document.getElementById('editaddtextcount').innerHTML = textcount;
} // end of function addTextField



/**
 * requests the metadata tab in the entry details
 *
 * @param int drid revision id
 */
function showEditorTabMetadata( drid )
{
  makeRequest('?page=backend&type=text&subtype=mef&d_fl=showtag&d_r_id='+drid, 'mef', 'frmedittagsc2', 'html', 'GET', '');
} // end of function showEditorTabMetadata



/**
 * requests the history tab in the entry details
 *
 * @param int drid revision id
 */
function showEditorTabHistory( drid )
{
  makeRequest('?page=backend&type=text&subtype=mef&d_fl=showhistory&d_r_id='+drid, 'mef', 'frmedittagsc2', 'html', 'GET', '');
} // end of function showEditorTabHistory



/**
 * requests the depencies tab in the entry details
 *
 * @param int drid revision id
 */
function showEditorTabDepencies(  drid )
{
  makeRequest('?page=backend&type=text&subtype=mef&d_fl=showdepencies&d_r_id='+drid, 'mef', 'frmedittagsc2', 'html', 'GET', '');
} // end of function showEditorTabDepencies



/**
 * requests the fields tab in the entry details
 *
 * @param int drid revision id
 * @param int dusr user
 */
function showEditorTabFields( drid, dusr )
{
  alertbox('Change fields only if you know what you are doing.');
  makeRequest('?page=backend&type=text&subtype=mef&d_fl=alterfields&d_r_id='+drid+'&d_val3='+dusr, 'mef', 'frmedittagsc2', 'html', 'GET', '');
} // end of function showEditorTabFields



/**
 * adds a new user label, system metadata/label
 *
 * @param int did data id
 * @param int drid revision id
 * @param string dtn tag name
 * @param string dtv tag value
 * @param int dusr user
 */
function addLabelOrTag( drid, dtn, dtv, dusr )
{
  var dtna = '';
  var dtva = '';

  if (dtn === 'tag') {
    dtna = 'tag';
  }
  else {
    dtna = document.getElementById(dtn).value;
  }

  dtva = document.getElementById(dtv).value;

  if (dtna !== '' && dtva !== '') {
    makeRequest('?page=backend&type=text&subtype=mef&d_fl=addtag&d_r_id='+drid+'&d_val='+encodeURIComponent(dtna)+'&d_val2='+encodeURIComponent(dtva)+'&d_val3='+dusr, 'mef', 'frmedittagsc2', 'html', 'GET', '');
  }
} // end of function addLabelOrTag



/**
 * deletes a user label, system metadata/label
 *
 * @param int tag_id
 */
function delLabelOrTag( tag_id )
{
  if (tag_id > 0) {
    makeRequest('?page=backend&type=text&subtype=mef&d_fl=deltag&d_val='+tag_id, 'mef', 'frmedittagsc2', 'html', 'GET', '');
  }
} // end of function delLabelOrTag



/**
 * updates or changes a specific tag, mostly star
 *
 * @param int rev_id revision id
 * @param string value tag value
 * @param string objid
 */
function updateBookmark( rev_id, value, objid )
{
  if (value !== '') {
    makeRequest('?page=backend&type=text&subtype=eta&d_fl=setbookmark&rev='+rev_id+'&tag_value='+encodeURIComponent(value), 'eta', objid, 'html', 'GET', '');
  }
} // end of function updateBookmark



/**
 * prepares the short texts & texts for submit via POST 
 */
function getEditorTexts( )
{
  var poststr = "";
  var instatinymce;
  var i;

  try {
    // short text
    poststr += "pstextsum="+document.getElementById("estextcount").className;

    for (i=1; i <= document.getElementById("estextcount").className; i++) {
      poststr += "&pdstext"+i+"=" + encodeURIComponent(document.getElementById("edstext"+i).innerHTML);
      poststr += "&pstext"+i+"=" + encodeURIComponent(document.getElementById("estext"+i).value);
    }

    // text
    poststr += "&plmsum="+document.getElementById("elmcount").className;

    for (i=1; i <= document.getElementById("elmcount").className; i++) {
      poststr += "&pdtext"+i+"=" + encodeURIComponent(document.getElementById("textname"+i).innerHTML);

      // get the content from TinyMCE
      instatinymce = ajaxsaveContent("elm"+i); 
      if (instatinymce !== null) {
        poststr += "&plm"+i+"=" + encodeURIComponent(instatinymce);
      }
      else {
        poststr += "&plm"+i+"=" + encodeURIComponent(document.getElementById("elm"+i).value);
      }
    }
    return poststr;
  }
  catch (e) {
    // destroy old rich text editor instances
    rtestop(); 
  }
  return false;
} // end of function getEditorTexts



/**
 * save the current entry as draft
 *
 * @param int did data id
 * @param int drid revision id
 */
function saveAsDraft( did, drid )
{
  document.getElementById("bsavedraft").disabled = true;

  if (addOrReplaceDraft(did, drid)) {

    // destroy old rich text editor instances
    rtestop(); 

    loadEntryTableWithOffset(roscms_current_tbl_position);
    window.clearTimeout(autosave_timer);
  }
} // end of function saveAsDraft



/**
 * adds or updates the draft (sends request to save)
 *
 * @param int did data id
 * @param int drid revision id
 */
function addOrReplaceDraft( did, drid )
{
  var poststr = getEditorTexts();

  if (poststr !== false) {
    makeRequest('?page=backend&type=text&subtype=asi&data_id='+encodeURIComponent(did)+'&rev_id='+encodeURIComponent(drid)+'&lang_id='+encodeURIComponent(document.getElementById("mefrlang").innerHTML), 'asi', 'mefasi', 'html', 'POST', poststr.substr(1));
    return true;
  }
  else {
    alertbox('Cannot save draft.');
    return false;
  }
} // end of function addOrReplaceDraft



/**
 * tries to autosave an entry
 */
function tryAutosave( )
{
  window.clearTimeout(autosave_timer);

  try {
    if (document.getElementById("editautosavemode").value === 'false') {
      window.clearTimeout(autosave_timer);
      return;
    }
  } 
  catch (e) {
    window.clearTimeout(autosave_timer);
    return;
  }

  if (autosave_cache != getEditorTexts() && autosave_cache !== '') {
    addOrReplaceDraft(document.getElementById("entrydataid").className, document.getElementById("entrydatarevid").className);
  }

  autosave_timer = window.setTimeout("tryAutosave()", autosave_coundown); // 10000
} // end of function tryAutosave



/**
 * loads another tab in the 'new entry' editor interface
 *
 * @param string menumode 'single','dynamic'
 */
function changeNewEntryTab( mode )
{
  if (mode === 'single' || mode === 'dynamic' || 'template') {
    makeRequest('?page=backend&type=text&subtype=ned&action=dialog&tab='+mode, 'ned', 'newentryarea', 'html', 'GET', '');
  }
} // end of function changeNewEntryTab



/**
 * search the table rows to perform a diff between 2 rows (otherwise -> error)
 */
function compareEntries( )
{
  var tentrs = selectedEntries().split("|");

  if (tentrs[0] < 2) {
    alertbox("Select two entries to compare them!");
  } 
  else if (tentrs[0] == 2) {
    var tentrs2 = tentrs[1].split("-");
    var tentrs20 = tentrs2[0].split("_");
    var tentrs21 = tentrs2[1].split("_");

    if (tentrs20[1] < tentrs21[1]) {
      getDiffEntries(tentrs20[1], tentrs21[1]);
    }
    else {
      getDiffEntries(tentrs21[1], tentrs20[1]);
    }
  }
  else {
    alertbox("Select two entries to compare them!");
  }
} // end of function compareEntries



/**
 * used by the compare button
 *
 * @param int revid1
 * @param int revid2
 */
function getDiffEntries( revid1, revid2 )
{
  if (revid1 > 0 && revid2 > 0) {
    makeRequest('?page=backend&type=text&subtype=mef&d_fl=diff&d_val='+encodeURIComponent(revid1)+'&d_val2='+encodeURIComponent(revid2), 'mef', 'diff2', 'html', 'GET', '');
  }
} // end of function getDiffEntries



/**
 * closes diff area
 *
 * @param int revid1
 * @param int revid2
 */
function openOrCloseDiffArea( revid1, revid2 )
{
  document.getElementById('frmdiff').innerHTML = '';

  if (document.getElementById('frmdiff').style.display === 'none') {
    document.getElementById('frmdiff').style.display = 'block';
    document.getElementById('bshowdiffi').src = roscms_intern_webserver_roscms+'images/tab_open.gif';
    getDiffEntries(revid1, revid2);
  }
  else {
    document.getElementById('frmdiff').style.display = 'none';
    document.getElementById('bshowdiffi').src = roscms_intern_webserver_roscms+'images/tab_closed.gif';
  }
} // end of function openOrCloseDiffArea



/**
 * change entry tag 'status' for selected entries
 *
 * @param string ctk tag action 'ms' / 'mn'
 */
function changeSelectedTags( ctk )
{
  if (ctk === 'ms' || ctk === 'mn') {
    var tentrs = selectedEntries().split("|");

    if (tentrs[0] < 1 || tentrs[0] === '') {
      alertbox("No entry selected.");
    }
    else {
      var tmp_obj = 'changetags';

      if (ctk === 'ms') {
        tmp_obj = 'changetags2';
        alertbox('Please be patient, related pages get generated ...');
      }

      makeRequest('?page=backend&type=text&subtype=eta&d_fl=changetags&count='+encodeURIComponent(tentrs[0])+'&d_val2='+encodeURIComponent(tentrs[1])+'&d_val3='+encodeURIComponent(ctk), 'mef', tmp_obj, 'html', 'GET', '');
    }
  }


  try {
    document.getElementById('cmddiff').focus();
  }
  catch (e) {
  }
} // end of function changeSelectedTags



/**
 * starts a new AJAX request
 *
 * @param string url
 * @param string action
 * @param string objid
 * @param string format
 * @param string kind
 * @param string parameters
 */
function makeRequest( url, action, objid, format, kind, parameters )
{
  var http_request = false;

  loadingSplash(true); 

  if (window.XMLHttpRequest) { // Mozilla, Safari,...
    http_request = new XMLHttpRequest();
  }
  else if (window.ActiveXObject) { // IE
    try {
      http_request = new ActiveXObject("Msxml2.XMLHTTP");
    } catch (e) {
      try {
        http_request = new ActiveXObject("Microsoft.XMLHTTP");
      } catch (e2) {
      }
    }
  }

  if (!http_request) { // stop if browser doesn't support AJAX
    alert('Cannot create an XMLHTTP instance. \nMake sure that your browser does support AJAX. \nMake sure that your browser does support AJAX. \nTry out IE 5.5+ (with ActiveX enabled), IE7+, Mozilla, Opera 9+ or Safari 3+.');
    return false;
  }

  // override mime
  if (http_request.overrideMimeType && format=='xml') {
    http_request.overrideMimeType('text/xml');
  }
  else if (http_request.overrideMimeType && format=='text') {
    http_request.overrideMimeType('text/plain');
  }
  else if (http_request.overrideMimeType && format=='html') {
    http_request.overrideMimeType('text/html');
  }

  // enter archive mode
  if (roscms_archive) { 
    url = url + '&d_arch=true';
  }

  http_request.onreadystatechange = function()
  {
    try {
      if (http_request.readyState === 4) {
        if (http_request.status === 200) {
          loadingSplash(false); 

          switch (action) {

            // page table main
            case 'ptm': 
              buildEntryTable(http_request, objid);
              break;

            // main edit frame
            case 'mef': 
              applyToEditor(http_request, objid);
              break;

            // entry table action
            case 'eta': 
              entryTableActionPerformed(http_request, objid);
              break;

            // new entry dialog
            case 'ned': 
              showNewEntryDialog(http_request, objid);
              break;

            // auto save info
            case 'asi': 
              showAutosaveInfo(http_request, objid);
              break;

              // user quick info
            case 'tt': 
              document.getElementById('tooltip').style.display='block';
              setTooltipPosition();
            case 'ufs': // user filter storage
            case 'ut': // user tags

              // apply to content box
              document.getElementById(objid).innerHTML = http_request.responseText;
              break;

              // preview
            case 'prv': 
              showPreview(http_request, objid);
              break;

            default:
              alert('Unknown-AJAX-LoadAction: '+ action);
              break;
          }
        }
        else {
          alert('There was a problem with the request ['+http_request.status+' / '+http_request.readyState+']. \n\nA client (browser) or server problem. Please make sure you use an up-to-date browser client. \n\nIf this error happens more than once or twice, contact the website admin.');
        }
      }
    }
    catch (e) {
      if (roscms_page_load_finished === true) {
        alertbox('RosCMS caught an exception to prevent data loss. If you see this message several times, please make sure you use an up-to-date browser client. If the issue still occur, tell the website admin the following information:<br />Name: '+ e.name +'; Number: '+ e.number +'; Message: '+ e.message +'; Description: '+ e.description);
      }
    }

  }; // internal function end

  if (kind === 'POST') {
    http_request.open('POST', roscms_intern_webserver_roscms+url, true);
    http_request.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    http_request.setRequestHeader("Content-length", parameters.length);
    http_request.setRequestHeader("If-Modified-Since", "Sat, 1 Jan 2000 00:00:00 GMT");	// Bypass the IE Cache
    http_request.setRequestHeader("Connection", "close");
    http_request.send(parameters);
  }
  else {
    http_request.open('GET', roscms_intern_webserver_roscms+url, true);
    http_request.setRequestHeader("If-Modified-Since", "Sat, 1 Jan 2000 00:00:00 GMT");	// Bypass the IE Cache
    http_request.send(null);
  }

  return true;
} // end of function makeRequest



/**
 * updates things in the editor frame
 *
 * @param object http_request
 * @param string objid
 */
function applyToEditor( http_request, objid )
{
  var tsplits = objid.split('_');

  switch (tsplits[0]) {

    case 'bstar':
      var tsplitsa = document.getElementById("tr"+tsplits[1]).getElementsByTagName('td')[1].className.split('-');
      document.getElementById("tr"+tsplits[1]).getElementsByTagName('td')[1].className = tsplitsa[0]+'-'+http_request.responseText;
      break;

    case 'editstar':
      document.getElementById(objid).className = http_request.responseText;
      break;

    case 'addnew':
    case 'addnew2':
      document.getElementById('editzone').innerHTML = http_request.responseText;
      break;

    case 'diff':
    case 'diff2': 
      document.getElementById('frmdiff').innerHTML = http_request.responseText;
      loadEditor('diffentry');
      document.getElementById('frmeditdiff').innerHTML = WDiffShortenOutput(WDiffString(document.getElementById('frmeditdiff1').innerHTML, document.getElementById('frmeditdiff2').innerHTML));
      break;

    case 'changetags':
      reloadEntryTableWithOffset(0);
      if (http_request.responseText === '') {
        alertbox('Action performed');
      }
      else {
        alertbox("Error while requested action:\n"+http_request.responseText);
      }
      break;

    case 'changetags2':
      reloadEntryTableWithOffset(0);
      alertbox(http_request.responseText);
      break;

    case 'editalterfields':
      document.getElementById('editzone').innerHTML = http_request.responseText;
      alertbox('Entry fields updated');
      break;

    case 'editaltersecurity':
      document.getElementById('editzone').innerHTML = http_request.responseText;
      alertbox('Data fields updated');
      break;

    case 'editalterentry':
      document.getElementById('editzone').innerHTML = http_request.responseText;
      alertbox('Entry updated');
      break;

    case 'updatedepencies':
      alertbox(http_request.responseText);
      makeRequest('?page=backend&type=text&subtype=mef&d_fl=showdepencies&d_r_id='+document.getElementById('mefrrevid').innerHTML, 'mef', 'frmedittagsc2', 'html', 'GET', '');
      break;

    case 'updatetag':
      selectUserTags();
      objid = tsplits[1];

    default:
      document.getElementById(objid).innerHTML = http_request.responseText;
      autosave_cache = getEditorTexts();
      break;
  } // end switch
} // end of function applyToEditor



/**
 * shows that the editor has autosaved an entry + time
 *
 * @param object http_request
 * @param string objid
 */
function showAutosaveInfo( http_request, objid )
{
  switch (objid) {
    case 'mefasi':
      var tempcache = getEditorTexts();
      var d = new Date();
      var curr_hour = d.getHours();
      var curr_min = d.getMinutes();

      if (curr_hour.length === 1) {
        curr_hour = '0'+curr_hour;
      }
      if (curr_min.length === 1) {
        curr_min = '0'+curr_min;
      }

      if (autosave_cache != tempcache) {
        autosave_cache = tempcache;
      }
      document.getElementById('mefasi').innerHTML = 'Draft saved at '+ curr_hour +':'+ curr_min;
      
      if (http_request.responseText !== '') {
        alertbox('Error: '+http_request.responseText);
      }
      else {
        alertbox('Draft saved');
      }
      break;

    case 'alert':
      window.clearTimeout(autosave_timer);
      loadEntryTable(roscms_prev_page);
      break;

    default:
      alert('showAutosaveInfo() with no args');
      break;
  }
} // end of function showAutoSaveInfo



/**
 * sets a new star
 *
 * @param int entryid
 * @param string dtv
 * @param int dusr
 * @param string objid
 */
function setBookmark( entryid, dtv, dusr, objid )
{
  if (entryid.indexOf("|") > -1) { 
    var devide1 = '';
    var devide2 = '';
    var devideids1 = '';
    var devideids2 = '';
    var devideids3 = '';

    devide1 = entryid.split('_');
    devide2 = devide1[1].split('-');
    devideids1 = devide2[0].indexOf("|");
    devideids2 = devide2[0].substr(0, devideids1);
    devideids3 = devide2[0].substr(devideids1+1);

    if (devideids2.substr(0,2) === 'tr') {
      alertbox('Cannot bookmark not translated entries.');
    }
    else {
      if (dtv === 'cStarOff') {
        dtv = 'on';
        document.getElementById(objid).className = 'cStarOn';
      }
      else {
        dtv = 'off';
        document.getElementById(objid).className = 'cStarOff';
      }
      updateBookmark(devideids3, dtv, objid);
    }
  }
} // end of function setBookmark



/**
 * construct a new row for the entry table
 *
 * @param 
 * @param 
 * @param 
 * @param 
 * @param 
 * @param 
 * @param 
 * @param 
 * @param 
 * @param 
 * @param 
 * @param 
 * @param 
 * @param 
 * @param 
 */
function htmlEntryTableRow( bnr, bclass, bid, bdname, btype, brid, brver, brlang, brdate, bstar, bstarid, brusrid, security, xtrtblcol, bdesc )
{
  var xtrtblcols2 = '';
  var lstBody = '';
  var tmpdesc = '';

  if (bstar == 1) {
    bstar = 'cStarOn';
  }
  else {
    bstar = 'cStarOff';
  }

  xtrtblcols2 = xtrtblcol.split('|');

  lstBody = '<tr class="'+bclass+'" id="tr'+(bnr+1)+'">'
    + '<td align="right"><input id="cbtr'+(bnr+1)+'" type="checkbox" onclick="selectRow(this.id)" /></td>'
    + '<td class="tstar_'+bid+'|'+brid+'-'+bstarid+'"><div id="bstar_'+(bnr+1)+'" class="'+bstar+'">&nbsp;</div></td>'
    + '<td class="rv'+bid+'|'+brid+'">'+bdname+'</td>'
    + '<td class="rv'+bid+'|'+brid+'" class="cell-height tcp">';

  // not found -> readonly
  if (security.indexOf("write") < 0 ) {
    lstBody += '<img src="'+roscms_intern_webserver_roscms+'images/locked.gif" alt="read-only" style="width:11px; height:12px;" /> ';
  }

  try {
    tmpdesc = unescape(decodeURI(bdesc));
    tmpdesc = tmpdesc.replace(/\+/g, ' ');
    if (tmpdesc.length === 0) {
      tmpdesc = '&nbsp;';
    }
  } catch (e) {
    tmpdesc = '<em>check the title or description field, it contains non UTF-8 chars</em>';
  }
  lstBody += tmpdesc+'</td>';

  if (xtrtblcol !== '' && xtrtblcols2.length > 1) {
    for (var i=1; i < xtrtblcols2.length-1; i++) {
      lstBody += '<td class="rv'+bid+'|'+brid+'">'+xtrtblcols2[i]+'</td>';
    }
  }

  lstBody += '<td class="rv'+bid+'|'+brid+'">'+brdate.substr(0, 10)+'</td>'
    + '</tr>';

  return lstBody;
} // end of function htmlEntryTableRow



/**
 * opens a new tab on the left side
 *
 * @param string objid
 */
function loadMenu( objid )
{
  var translang = '';
  var menu_item = null;

  roscms_archive = false;

  // deselect all
  selectAll(false);

  // reset search box:
  filtstring1 = '';
  document.getElementById('txtfind').value = '';
  searchFilter('txtfind', document.getElementById('txtfind').value, search_phrase, false);

  window.clearTimeout(autosave_timer);
  autosave_cache = '';

  if (document.getElementById('smenutab'+objid.substring(8)).className !== 'lmItemTopSelected') {
    highlightTab(objid);
  }

  switch (objid.substring(8)) {

    case '1':
      filtstring2 = '';
      newEntryDialog();
      break;

    case '2':
      filtstring2 = 'k_is_new_0|c_is_user_0|c_is_type_0|l_is_'+userlang+'_1|o_desc_date_1';
      menu_item = 'new';
      break;

    case '3':
      filtstring2 = 'y_is_page_0|k_is_stable_0|c_is_language_1|o_asc_name_1';
      menu_item = 'page';
      break;

    case '5':
      filtstring2 = 'y_is_dynamic_0|k_is_stable_0|c_is_language_1|o_asc_name_1';
      menu_item = 'dynamic';
      break;

    case '4':
      filtstring2 = 'y_is_content_0|k_is_stable_0|l_is_'+userlang+'_1|o_asc_name_1';
      menu_item = 'content';
      break;

    case '6':
      filtstring2 = 'y_is_script_0|k_is_stable_0|c_is_language_1|o_asc_name_1';
      menu_item = 'script';
      break;

    case '7':
      if (userlang == roscms_standard_language) {
        alertbox('You can\'t translate entries, because you have the standard language as your user language.');
        return false;
      }
      else {
        translang = userlang;
      }
      filtstring2 = 'y_is_content_1|k_is_stable_0|i_is_translate_0|c_is_user_1|l_is_'+roscms_standard_language+'_0|r_is_'+translang+'_1|o_desc_date_1';
      menu_item = 'translate';
      break;

    case '8':
    default:
      filtstring2 = 'c_is_type_1|l_is_'+userlang+'_1|o_desc_date_1';
      menu_item = 'all';
      break;

    case '9':
      filtstring2 = 's_is_true_0|c_is_type_1|l_is_'+userlang+'_1|o_desc_date_1';
      menu_item = 'starred';
      break;

    case '10':
      filtstring2 = 'k_is_draft_0|u_is_'+roscms_intern_login_check_username+'_0|c_is_type_1|o_desc_date_1';
      menu_item = 'draft';
      break;

    case '11':
      filtstring2 = 'u_is_'+roscms_intern_login_check_username+'_0|c_is_type_1|o_desc_date_1';
      menu_item = 'my';
      break;

    case '12':
      filtstring2 = 'k_is_archive_0|c_is_version_1|c_is_type_1|l_is_'+userlang+'_1|o_desc_date_1';
      roscms_archive = true; /* activate archive mode*/
      menu_item = 'archive';
      break;
  } // end switch

  // setup current filter settings
  if (menu_item !== null) {
    roscms_prev_page = roscms_current_page;
    roscms_current_page = menu_item;
    htmlFilterChoices(filtstring2);
    loadEntryTable(menu_item);
  }

  return true;
} // end of function loadMenu



/**
 * sets a new language for the current session
 *
 * @param string favlang
 */
function setLang( favlang )
{
  var tmp_regstr;
  var transcheck = filtstring2.search(/r_is_/);

  // translation view
  if (transcheck != -1) { 
    tmp_regstr = new RegExp('r_is_'+userlang, "g");
    filtstring2 = filtstring2.replace(tmp_regstr, 'r_is_'+favlang);
  }
  else {
    tmp_regstr = new RegExp('l_is_'+userlang, "g");
    filtstring2 = filtstring2.replace(tmp_regstr, 'l_is_'+favlang);
  }

  userlang = favlang;
  htmlFilterChoices(filtstring2);
  reloadEntryTable(roscms_current_tbl_position);
} // end of function setLang



/**
 * toggles bookmark setting for given entry
 *
 * @param int did
 * @param int drid
 * @param string dtn
 * @param string dtv
 * @param int dusr
 * @param string objid
 */
function toggleBookmark( did, drid, dusr, objid )
{
  if (did > 0 && drid > 0) {
    if (document.getElementById(objid).src == roscms_intern_webserver_roscms+'images/star_on_small.gif') {
      document.getElementById(objid).src = roscms_intern_webserver_roscms+'images/star_off_small.gif';
      updateBookmark(drid, 'off', objid);
    }
    else {
      document.getElementById(objid).src = roscms_intern_webserver_roscms+'images/star_on_small.gif';
      updateBookmark(drid, 'on', objid);
    }
  }
} // end of function ToggleBookmark



/**
 * creates a new entry with the 'new entry' interface
 *
 * @param int menumode
 */
function createNewEntry( menumode )
{
  switch (menumode) {

    // new single entry
    case 0:
      if (document.getElementById('txtaddentryname').value !== "") {
        makeRequest('?page=backend&type=text&subtype=ned&action=newentry&name='+encodeURIComponent(document.getElementById('txtaddentryname').value)+'&data_type='+encodeURIComponent(document.getElementById('txtaddentrytype').value), 'ned', 'newentryarea', 'html', 'GET', '');
      }
      else {
        alertbox('Entry name is required');
      }
      break;

    // new dynamic entry
    case 1:
      makeRequest('?page=backend&type=text&subtype=ned&action=newdynamic&data_id='+encodeURIComponent(document.getElementById('txtadddynsource').value), 'ned', 'newentryarea', 'html', 'GET', '');
      break;

    // new page & content
    case 2:
      if (document.getElementById('txtaddentryname3').value !== "") {
        makeRequest('?page=backend&type=text&subtype=ned&action=newcombo&name='+encodeURIComponent(document.getElementById('txtaddentryname3').value)+'&template='+encodeURIComponent(document.getElementById('txtaddtemplate').value), 'ned', 'newentryarea', 'html', 'GET', '');
      }
      else {
        alertbox('Entry name is required');
      }
  } // end switch
} // end of function createNewEntry



/**
 * construct a new header for the entry table
 *
 * @param string xtrtblcol
 */
function htmlEntryTableHeader( xtrtblcol )
{
  var xtrtblcols2 ='';
  var lstHeader = '';

  xtrtblcols2 = xtrtblcol.split('|');

  lstHeader = '<table class="roscmsTable" cellpadding="0" cellspacing="0">'
    + '<thead><tr>'
    + '<th scope="col" class="cMark">&nbsp;</th>'
    + '<th scope="col" class="cStar">&nbsp;</th>'
    + '<th scope="col" class="cCid sortable" onclick="sortEntryTable(this, \'name\');">Name</th>'
    + '<th scope="col" class="cRev">Title</th>';


  if (xtrtblcol !== '' && xtrtblcols2.length > 1) {
    for (var i=1; i < xtrtblcols2.length-1; i++) {
      lstHeader += '<th scope="col" class="cExtraCol sortable" onclick="sortEntryTable(this, \''+xtrtblcols2[i]+'\');">'+xtrtblcols2[i]+'</th>';
    }
  }

  lstHeader+= '<th scope="col" class="cDate sortable" onclick="sortEntryTable(this, \'date\');">Date</th>'
    + '</tr></thead><tbody>';

  return lstHeader;
} // end of function htmlEntryTableHeader



/**
 * gives a preview of >one< selected entry
 */
function previewPage( )
{
  var tentrs = selectedEntries().split("|");
  var tentrs2 = tentrs[1].split("_");

  if (tentrs[0] == 1) {
    makeRequest('?page=backend&type=text&subtype=prv&rev_id='+tentrs2[1], 'prv', 'previewarea', 'html', 'GET', '');
  }
  else {
    alertbox("Select one entry to preview a page!");
  }
} // previewPage



/**
 * put the previewed page into an iframe and hide other elements like entry table and editor
 *
 * @param object http_request
 * @param string objid
 */
function showPreview( http_request, objid )
{
  if (document.getElementById('previewarea').style.display !== 'block') {
    document.getElementById('frametable').style.display = 'none';
    document.getElementById('frameedit').style.display = 'none';
    document.getElementById('newentryarea').style.display = 'none';
    document.getElementById('previewarea').style.display = 'block';
  }

  document.getElementById('previewzone').innerHTML = http_request.responseText;
  document.getElementById('previewhead').innerHTML = '<span class="virtualLink" onclick="loadEntryTableWithOffset(roscms_current_tbl_position)"><strong>&laquo; Back</strong></span> &nbsp; <strong>Preview</strong>';
} // end of function showPreview



/**
 * add a new manual depency
 *
 * @param int rev_id
 */
function addDepency( rev_id )
{
  var name = document.getElementById('dep_name').value;

  // check if name is given
  if (name !== '') {
    makeRequest('?page=backend&type=text&subtype=mef&d_fl=adddepency&rev_id='+rev_id+'&dep_name='+encodeURIComponent(name)+'&dep_type='+encodeURIComponent(document.getElementById('dep_type').value), 'mef', 'updatedepencies', 'html', 'GET', '');
  }
} // addDepency



/**
 * delete a new manual depency
 *
 * @param int dep_id depency id
 */
function deleteDepency( dep_id )
{
  makeRequest('?page=backend&type=text&subtype=mef&d_fl=deletedepency&dep_id='+dep_id, 'mef', 'updatedepencies', 'html', 'GET', '');
} // deleteDepency



/**
 * put the new entry dialog as current view
 *
 * @param object http_request
 * @param string objid
 */
function showNewEntryDialog( http_request, objid )
{
  if (document.getElementById('newentryarea').style.display !== 'block') {
    document.getElementById('frametable').style.display = 'none';
    document.getElementById('frameedit').style.display = 'none';
    document.getElementById('previewarea').style.display = 'none';
    document.getElementById('newentryarea').style.display = 'block';
  }

  document.getElementById('newentryzone').innerHTML = http_request.responseText;
  document.getElementById('newentryhead').innerHTML = '<span class="virtualLink" onclick="loadEntryTableWithOffset(roscms_current_tbl_position)"><strong>&laquo; Back</strong></span> &nbsp; <strong>New Entry</strong>';
} // end of function showNewEntryDialog



/**
 * gives html for commands like compare, preview, make stable, mark new and the dropdown
 *
 * @param string opt
 */
function htmlCommandBar( preset )
{
  var cmdbarstr = '';

  // prepare some commands
  var cmdhtml_space = '&nbsp;';
  var cmdhtml_diff = '<div class="button" onclick="compareEntries()"><img src="'+roscms_intern_webserver_roscms+'images/compare.gif" alt="" /><span class="text">Compare</span></div>';
  var cmdhtml_preview = '<div class="button" onclick="previewPage()"><img src="'+roscms_intern_webserver_roscms+'images/preview.gif" alt="" /><span class="text">Preview</span></div>';
  var cmdhtml_ready = '<div class="button" onclick="changeSelectedTags(\'mn\')"><img src="'+roscms_intern_webserver_roscms+'images/edit.gif" alt="" /><span class="text">Suggest</span></div>';

  
  var cmdhtml_stable = '';
  var cmdhtml_archive = '';
  var cmdhtml_delete = '';

  // mark stable / generate
  if (roscms_access.make_stable) {
    cmdhtml_stable = '<div class="button" onclick="changeSelectedTags(\'ms\')"><img src="'+roscms_intern_webserver_roscms+'images/mail.gif" alt="" /><span class="text">Publish</span></div>';
  }

  // delete entries
  if (roscms_access.del_entry) {
    cmdhtml_archive = '<div class="button" onclick="changeSelectedTags(\'va\')"><img src="'+roscms_intern_webserver_roscms+'images/rospc.gif" alt="" /><span class="text">to archive</span></div>';
    cmdhtml_delete = '<div class="button" onclick="changeSelectedTags(\'xe\')"><img src="'+roscms_intern_webserver_roscms+'images/delete.gif" alt="" /><span class="text">Delete</span></div>';

  }

  switch (preset) {

    case 'all':
      cmdbarstr += cmdhtml_diff
        + cmdhtml_preview
        + cmdhtml_ready
        + cmdhtml_stable
        + cmdhtml_archive
        + cmdhtml_delete;
      break;

    case 'trans':
      cmdbarstr += cmdhtml_preview;
      break;

    case 'new':
      cmdbarstr += cmdhtml_diff
        + cmdhtml_preview
        + cmdhtml_stable
        + cmdhtml_archive
        + cmdhtml_delete;
      break;

    case 'page':
      cmdbarstr += cmdhtml_preview
        + cmdhtml_diff
        + cmdhtml_archive
        + cmdhtml_delete;
      break;

    case 'script':
      cmdbarstr += cmdhtml_diff
        + cmdhtml_archive
        + cmdhtml_delete;
      break;

    case 'draft':
      cmdbarstr += cmdhtml_diff
        + cmdhtml_preview
        + cmdhtml_ready
        + cmdhtml_stable
        + cmdhtml_archive
        + cmdhtml_delete;
      break;

    case 'archive':
      break;
  } 

  document.getElementById('toolbarExtension').innerHTML = cmdbarstr;
} // end of function htmlCommandBar



/**
 * build filter dropdown
 *
 * @param string filtpopstr;
 */
function htmlFilterChoices( filtpopstr )
{
  var lstfilterstr = '';
  var lstfilterstr2 = '';

  document.getElementById('filtersct').innerHTML = '';
  filtercounter = 0;
  filterid = 0;

  if (filtpopstr !== '') {
    var indexid = '';
    var filtvisibility = false;
    var filtpopstr2 = filtpopstr.split('|');
    var i;

    for (i=0; i < filtpopstr2.length; i++) {
      lstfilterstr2 = filtpopstr2[i].split('_');

      if (lstfilterstr2[3] == '0' && !roscms_access.dont_hide_filter) {
        filtvisibility = false;
        lstfilterstr +=  '<span style="display: none;">';
      }
      else {
        filtvisibility = true;
        lstfilterstr +=  '<span style="font-style: normal;">';
      }

      indexid = i + 1;
      lstfilterstr +=  '<div id="filt'+indexid+'" class="filterbar2">and&nbsp;';

      // hidden filter entries don't need a combobox (only for SecLev = 1 user)
      if (lstfilterstr2[3] == '0' && !roscms_access.dont_hide_filter) {  
        lstfilterstr +=  '<input type="hidden" name="sfa'+indexid+'" id="sfa'+indexid+'" value="" />';
      }
      else {
        lstfilterstr +=  '<select id="sfa'+indexid+'" onchange="isFilterChanged(this.id)">'
          + '<option value="k">Status</option>'
          + '<option value="n">Name</option>'
          + '<option value="s">Bookmarked</option>'
          + '<option value="l">Language</option>';

        if (roscms_access.more_filter) {
          lstfilterstr += '<option value="y">Type</option>'
            + '<option value="r">Translate</option>'
            + '<option value="i">Security</option>'
            + '<option value="m">Metadata</option>'
            + '<option value="u">User</option>';
        }

        lstfilterstr += '<option value="d">Date</option>'
          + '<option value="c">Column</option>'
          + '<option value="o">Order</option>'
          + '</select>&nbsp;';
      }

      lstfilterstr +=  htmlFilterValues(lstfilterstr2[0], lstfilterstr2[3], indexid)
        +  '&nbsp;&nbsp;&nbsp;<span id="fdel'+indexid+'" class="virtualButton" onclick="removeFilter(this.id)"><img src="'+roscms_intern_webserver_roscms+'images/remove.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Delete</span>'
        +  '</div>';

      if (lstfilterstr2[3] == '0') {
        lstfilterstr +=  '<span id="sfv'+indexid+'" class="filthidden"></span>'; // store visibility-status
      }
      else {
        lstfilterstr +=  '<span id="sfv'+indexid+'" class="filtvisible"></span>'; // store visibility-status
      }
      lstfilterstr +=  '</span>';
    } // end for

    // apply builded filter
    document.getElementById('filtersct').innerHTML = lstfilterstr;

    for (i=0; i < filtpopstr2.length; i++) {
      lstfilterstr2 = filtpopstr2[i].split('_');
      indexid = i + 1;
      document.getElementById('sfa'+indexid).value = lstfilterstr2[0];

      if (lstfilterstr2[1] !== '') {
        document.getElementById('sfb'+indexid).value = lstfilterstr2[1];
      }

      if (lstfilterstr2[2] !== '') {
        document.getElementById('sfc'+indexid).value = lstfilterstr2[2];
      }
    }

    filtercounter = filtpopstr2.length;
  }
} // end of function htmlFilterChoices



function entryTableActionPerformed(http_request, objid)
{
  if (http_request.responseText !== '') {
    alertbox(http_request.responseText);
  }
} // end of function entryTableActionPerformed
