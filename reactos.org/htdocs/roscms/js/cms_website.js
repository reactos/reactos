    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>

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



/**
 * inverts checkbox selection
 *
 * @param string cbid checkbox id
 */
function selectRow( cbid )
{
  // check for checkbox id
  if (cbid.substr(0,2) == 'cb') {
    cbid = cbid.substr(2);
  }

  if (document.getElementById("cb"+cbid).checked == true) {
    document.getElementById("cb"+cbid).checked = false;
  }
  else {
    document.getElementById("cb"+cbid).checked = true;
  } 
}



/**
 * colors (background) a row in a given color
 *
 * @param int num number of the row
 * @param string color new color of the row
 */
function setRowColor( num, color )
{
  // check for needed internal functions
  if (!document.getElementById || !document.getElementsByTagName || !document.getElementById("tr"+num)) {
    return;
  }

  // select row
  var cell_arr = document.getElementById("tr"+num).getElementsByTagName('td');

  // set background color for cells
  for (var i=0; i<cell_arr.length; i++) {
    cell_arr[i].style.backgroundColor = color;
  }
}



/**
 * colors a rows background by status
 */
function setRowColorStatus( num, status )
{
  if (status == 'odd' || status == 'even') {
    if (num%2) setRowColor(num,"#dddddd");
    else setRowColor(num,"#eeeeee");
  }
  else if(status == 'new') {
    setRowColor(num,"#B5EDA3");
  }
  else if(status == 'draft') {
    setRowColor(num,"#FFE4C1");
  }
  else if(status == 'transg') {
    setRowColor(num,"#A3EDB4");
  }
  else if(status == 'transb') {
    setRowColor(num,"#D6CAE4");
  }
  else if(status == 'transr') {
    setRowColor(num,"#FAA5A5");
  }
  else {
    setRowColor(num,"#FFCCFF");
  }
}



/**
 * highlights a row
 *
 * @param int num number of the row
 * @param int hlmode highlight mode (mouseover, mouseout, mouseclick)
 */
function hlRow( rownum, hlmode )
{
  var rowstatus = document.getElementById("tr"+rownum).className;

  switch (hlmode) {

    //on mouseover
    case 1:
      setRowColor(rownum,"#ffffcc");
      break;

    //on mouseout
    case 2:
      if (document.getElementById("cb"+rownum).checked) {
        setRowColor(rownum,"#ffcc99");
      }
      else {
        setRowColorStatus(rownum, rowstatus);
      }
      break;

    //on click
    case 3:
      if (!document.getElementById("cb"+rownum).checked) {
        setRowColor(rownum,"#ffcc99");
      }
      else {
        setRowColorStatus(rownum, rowstatus);
      }
      break;
  } // end switch
}



/**
 * selects/deselects all rows
 *
 * @param string status true=select all / false=deselect all
 */
function selectAll( status )
{
  var rowstatus;

  // select/deselect all rows
  for (var i=1; i<=nres; i++) {
    if (status) {
      setRowColor(i,"#ffcc99");
    }
    else {
      rowstatus = document.getElementById("tr"+i).className;
      setRowColorStatus(i, rowstatus);
    }
  }

  // select/deselect all checkboxes
  for (var i=1; i<=nres; i++) {
    document.getElementById("cb"+i).checked = status;
  }
}



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
}



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
      setRowColorStatus(i, rowstatus);
      document.getElementById("cb"+i).checked = false;
    }
    else {
      setRowColor(i,"#ffcc99");
      document.getElementById("cb"+i).checked = true;
     }
  }
}



/**
 * Selects all starred/non-starred rows
 *
 * @param bool status true - select starred rows, false - select non-starred rows
 */
function selectStars( status )
{
  // deselect all
  selectAll(false);

  var sstar = status?'cStarOn':'cStartOff';

  // select choosen ones
  for (var i=1; i<=nres; i++) {
    if (document.getElementById("tr"+i).getElementsByTagName('td')[1].getElementsByTagName('div')[0].className == sstar) {
      setRowColor(i,"#ffcc99");
      document.getElementById("cb"+i).checked = true;
    }
  }
}



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
      setRowColor(i,"#ffcc99");
      document.getElementById("cb"+i).checked = true;
    }
  }
}



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
  else if (!clear && objval == '') {
    document.getElementById(objid).value = objhint;
    document.getElementById(objid).style.color = '#999999';
  }
}



/**
 * strips invalid characters out
 *
 * @param string str
 */
function beautifystr( str )
{
  // remove invalid characters
  str = str.replace(/\|/g, '');
  str = str.replace(/=/g, '');
  str = str.replace(/&/g, '');
  str = str.replace(/_/g, '');
  return str;
}



/**
 * strips invalid characters out
 *
 * @param string str
 */
function beautifystr2( str )
{
  // remove invalid characters
  str = str.replace(/\|/g, '');
  str = str.replace(/=/g, '');
  str = str.replace(/&/g, '');
  return str;
}



/**
 * Load Quickinfo data
 * Shows quickinfo window for a specific time, and then hides it again 
 *
 * @param string id_set set of data & rev ids in the following format data_id|rev_id
 */
function showQuickinfo( id_set )
{
  // only if the quick info box is 'visible'
  if (document.getElementById('labtitel1c').style.display == 'block') { 

    // deactivate quickinfo-timer
    window.clearTimeout(timerquickinfo); 

    document.getElementById('qiload').style.display = 'none';
    timerquickinfo = window.setTimeout("loadQuickinfo('"+id_set+"')", 700);
  }
}



/**
 * Requests quickinfo data
 *
 * @param string id_set set of data & rev ids in the following format data_id|rev_id
 */
function loadQuickinfo( id_set)
{
  var qistr = id_set.split('|');

  // deactivate quickinfo-timer
  window.clearTimeout(timerquickinfo);

  // perform request
  document.getElementById('qiload').style.display = 'block';
  makeRequest('?page=data_out&d_f=text&d_u=uqi&d_val=ptm&d_id='+encodeURIComponent(qistr[0].substr(2))+'&d_r_id='+encodeURIComponent(qistr[1]), 'uqi', 'lablinks1', 'html', 'GET', '');
}



/**
 * sets a timeout to remove Quickinfo
 */
function stopQuickinfo( )
{
  // only if the quick info box is 'visible'
  if (document.getElementById('labtitel1c').style.display == 'block') {

    // deactivate quickinfo-timer
    window.clearTimeout(timerquickinfo); 

    document.getElementById('qiload').style.display = 'none';
    timerquickinfo = window.setTimeout("clearQuickinfo()", 5000);
  }
}



/**
 * Disables Quickinfo view
 */
function clearQuickinfo( )
{
  // deactivate quickinfo-timer
  window.clearTimeout(timerquickinfo);

  document.getElementById('qiload').style.display = 'none';
  document.getElementById('lablinks1').innerHTML = '<span style="color:#FF6600;">Move the mouse over an item to get some details</span>';
}



/**
 * Saves smart filter setting or user label
 *
 * @param string uf_type 'label'/'filter'
 * @param string uf_str
 */
function addUserFilter( uf_str )
{
  var uf_name = '';
  var uf_objid = '';

  try {
    uf_name = window.prompt("Input a new Smart Filter name:", "");
    uf_objid = 'labtitel2c';
  }
  catch (e) {
  }

  // cancel button
  if (!uf_name) { 
    return;
  }

  if (uf_name != '' && uf_name.length < 50) {
    makeRequest('?page=data_out&d_f=text&d_u=ufs&d_val=add&d_val2='+encodeURIComponent(uf_type)+'&d_val3='+encodeURIComponent(uf_name)+'&d_val4='+encodeURIComponent(uf_str), 'ufs', uf_objid, 'html', 'GET', '');
  }
}



/**
 * Deletes smart filter setting or user label
 *
 * @param string uf_id
 * @param string uf_type 'label'/'filter'
 * @param string uf_name
 */
function deleteUserFilter( uf_id, uf_type, uf_name )
{
  var uf_check = confirm("Do you want to delete Smart Filter '"+uf_name+"' ?");
  uf_objid = 'labtitel2c';

  if (uf_check == true) {
    makeRequest('?page=data_out&d_f=text&d_u=ufs&d_val=del&d_val2='+encodeURIComponent(uf_type)+'&d_val3='+encodeURIComponent(uf_id), 'ufs', uf_objid, 'html', 'GET', '');
  }
}



/**
 * gets another stored filter setting
 */
function loadUserFilter( )
{
  document.getElementById('labtitel2c').innerHTML = '<div align="right"><img src="images/ajax_loading.gif" alt="loading ..." style="width:13px; height:13px;" /></div>';
  makeRequest('?page=data_out&d_f=text&d_u=ufs&d_val=load', 'ufs', 'labtitel2c', 'html', 'GET', '');
}




/**
 * gets user tags
 */
function loadUserTags( )
{
  document.getElementById('labtitel3c').innerHTML = '<div align="right"><img src="images/ajax_loading.gif" alt="loading ..." style="width:13px; height:13px;" /></div>';
  makeRequest('?page=data_out&d_f=text&d_u=ut', 'ut', 'labtitel3c', 'html', 'GET', '');
}



/**
 * toggles arrow right/bottom
 *
 * @param string objid affected object
 */
function TabOpenClose( objid )
{
  if (document.getElementById(objid +'c').style.display == 'none') {
    document.getElementById(objid +'c').style.display = 'block';
    document.getElementById(objid +'i').src = 'images/tab_open.gif';
  }
  else {
    document.getElementById(objid +'c').style.display = 'none';
    document.getElementById(objid +'i').src = 'images/tab_closed.gif';
  }
}



/**
 * toggles arrow right/bottom and saves the status in a cookie
 *
 * @param string objid affected object
 */
function TabOpenCloseEx( objid )
{
  if (document.getElementById(objid +'c').style.display == 'none') {
    document.getElementById(objid +'c').style.display = 'block';
    document.getElementById(objid +'i').src = 'images/tab_open.gif';
    createCookie(objid,'1',365); // 365 days
  }
  else {
    document.getElementById(objid +'c').style.display = 'none';
    document.getElementById(objid +'i').src = 'images/tab_closed.gif';
    createCookie(objid,'0',365); // 365 days
  }
}



/**
 * stores a new cookie
 *
 * @param string name cookie name
 * @param string value cookie value
 * @param int days how many days the cookie is stored
 */
function createCookie( name, value, days )
{
  var expires = ""

  if (days) {
    var date = new Date();
    date.setTime(date.getTime()+(days*24*60*60*1000));
    expires = "; expires="+date.toGMTString();
  }
  
  document.cookie = name+"="+value+expires+"; path=/";
}



/**
 * gets the value of a cookie
 *
 * @param string name cookie to access
 */
function readCookie( name )
{
  var nameEQ = name + "=";
  var ca = document.cookie.split(';');
  var c

  for(var i=0;i < ca.length;i++) {
    c = ca[i];
    while (c.charAt(0)==' ') {
      c = c.substring(1,c.length);
    }
    if (c.indexOf(nameEQ) == 0) {
      return c.substring(nameEQ.length,c.length);
    }
  }
  return null;
}



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

  if (ufilttype == 2 && tentrs[0] > 0 && tentrs[0] != '') {
    makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=changetags&d_val='+encodeURIComponent(tentrs[0])+'&d_val2='+encodeURIComponent(tentrs[1])+'&d_val3=tg&d_val4='+ufilttitel, 'mef', 'changetags', 'html', 'GET', '');
  }
  else {
    highlightTab('smenutab8');

    filtstring2 = ufiltstr;

    // reset search box:
    filtstring1 = '';
    document.getElementById('txtfind').value = '';
    searchFilter('txtfind', document.getElementById('txtfind').value, 'Search & Filters', false);

    chtabtext = null;
    selectAll(false); /* deselect all */

    loadEntryTable('all');
    htmlFilterChoices(ufiltstr);
  }
}



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
  searchFilter('txtfind', document.getElementById('txtfind').value, 'Search & Filters', false);

  chtabtext = null;
  selectAll(false); /* deselect all */

  loadEntryTable('all');
  htmlFilterChoices(tag_filter);
}



/**
 * setup a new active menu entry (left side)
 *
 * @param string objid item to highlight
 */
function highlightTab( objid )
{
  var chtabtext;

  for (var i=1; i<=smenutabs; i++) {

    // remove all 'bold' html-tags
    if (document.getElementById('smenutab'+i).className == 'subma' && i != 1) {
      chtabtext = '';
      chtabtext = document.getElementById('smenutabc'+i).innerHTML;
      chtabtext = chtabtext.replace(/<B>/, '');
      chtabtext = chtabtext.replace(/<\/B>/, '');
      chtabtext = chtabtext.replace(/<b>/, '');
      chtabtext = chtabtext.replace(/<\/b>/, '');
      document.getElementById('smenutabc'+i).innerHTML = chtabtext;
    }

    // highlight menu_tab_button
    if (i == objid.substring(8)) {
      document.getElementById('smenutab'+i).className = 'subma';
      document.getElementById('smenutabc'+i).innerHTML = '<b>'+document.getElementById('smenutabc'+i).innerHTML+'</b>';
    }
    else {
      document.getElementById('smenutab'+i).className = 'submb';
    }
  }
  // clear
  chtabtext = '';
}



/**
 * loads Table with entries
 *
 * @param string objevent
 */
function loadEntryTable( objevent )
{
  if (document.getElementById('frametable').style.display != 'block') {
    document.getElementById('frametable').style.display = 'block';
    document.getElementById('frameedit').style.display = 'none';

    // deactivate alert-timer
    window.clearTimeout(alertactiv); 
    document.getElementById('alertbox').style.visibility = 'hidden';
    document.getElementById('alertboxc').innerHTML = '&nbsp;';

    // deselect all table entries
    selectAll(false); 
  }

  // function was called via loadMenu
  if (submenu_button == true) {
    roscms_prev_page = roscms_current_page;
    roscms_current_page = objevent;
    htmlFilterChoices(filtstring2);
    submenu_button = '';
  }

  // update table-cmdbar
  switch (objevent) {
    case 'new':
      htmlCommandBar('new');
      htmlSelectPresets('basic');
      break;
    case 'page':
    case 'content':
      htmlCommandBar('page');
      htmlSelectPresets('basic');
      break;
    case 'script':
    case 'template':
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
  makeRequest('?page=data_out&d_f=xml&d_u=ptm&d_fl='+objevent+'&d_filter='+filtstring1+'&d_filter2='+filtstring2+'&d_cp=0', 'ptm', 'tablist', 'xml', 'GET', '');
}



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

  makeRequest('?page=data_out&d_f=xml&d_u=ptm&d_fl='+roscms_current_page+'&d_filter='+filtstring1+'&d_filter2='+filtstring2+'&d_cp='+offset, 'ptm', 'tablist', 'xml', 'GET', '');
}



/**
 * closes edit frame, and loads the entry table again
 *
 * @param int offset
 */
function loadEntryTableWithOffset( offset )
{
  if (document.getElementById('frametable').style.display != 'block') {
    document.getElementById('frametable').style.display = 'block';
    document.getElementById('frameedit').style.display = 'none';
  }

  reloadEntryTableWithOffset( offset );
}



/**
 * reloads the entry table (maybe selected editor before)
 */
function reloadEntryTable( )
{
  if (document.getElementById('frametable').style.display != 'block') {
    document.getElementById('frametable').style.display = 'block';
    document.getElementById('frameedit').style.display = 'none';
  }

  // deactivate alert-timer
  window.clearTimeout(alertactiv); 
  document.getElementById('alertbox').style.visibility = 'hidden';
  document.getElementById('alertboxc').innerHTML = '&nbsp;';

  // deselect all table entries
  selectAll(false);

  makeRequest('?page=data_out&d_f=xml&d_u=ptm&d_fl='+roscms_current_page+'&d_filter='+filtstring1+'&d_filter2='+filtstring2, 'ptm', 'tablist', 'xml', 'GET', '');
}



/**
 * shows the edit frame which allows to edit an entry
 */
function showEditor( )
{
  if (document.getElementById('frameedit').style.display != 'block') {
    document.getElementById('frametable').style.display = 'none';
    document.getElementById('frameedit').style.display = 'block';
  }

  // deactivate alert-timer
  window.clearTimeout(alertactiv);
  document.getElementById('alertbox').style.visibility = 'hidden';
  document.getElementById('alertboxc').innerHTML = '&nbsp;';

  // deselect all table entries
  selectAll(false); 
}



/**
 * initialises the editor
 *
 * @param string objevent
 * @param string entryid
 */
function loadEditor( objevent, entryid )
{
  switch (objevent) {
    case 'newentry':
      showEditor();
      roscms_prev_page = roscms_current_page;
      roscms_current_page = objevent;

      document.getElementById('frmedithead').innerHTML = '<b>New Entry</b>';
      document.getElementById('editzone').innerHTML = '<p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p><p>&nbsp;</p>';
      makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=newentry&d_id=new&d_val=0', 'mef', 'addnew', 'html', 'GET', '');
      break;

    case 'diffentry':
      showEditor();
      document.getElementById('frmedithead').innerHTML = '<span class="button" onclick="loadEntryTableWithOffset(roscms_current_tbl_position)"><strong>&laquo; Back</strong></span> &nbsp; <strong>Compare two Entries</strong>';
      break;

    default:
      if (entryid.indexOf("|") > -1) {
        var devideids1 = entryid.indexOf("|");
        var devideids2 = entryid.substr(2, devideids1-2);
        var devideids3 = entryid.substr(devideids1+1);

        if (devideids2.substr(0,2) == 'tr') {
          var uf_check = confirm("Do you want to translate this entry?");
          if (uf_check != true) {
            break;
          }
          alertbox('Translation copy created.');
        }
        else if (devideids2 == 'notrans') {
          alertbox('You don\'t have enough rights to translate this entry.');
          break;
        }

        showEditor();

        roscms_prev_page = roscms_current_page;
        roscms_current_page = objevent;

        document.getElementById('frmedithead').innerHTML = '<span class="button" onclick="loadEntryTableWithOffset(roscms_current_tbl_position)"><strong>&laquo; Back</strong></span> &nbsp; <b>Edit Entry</b>';

        // enable autosave
        autosave_timer = window.setTimeout("tryAutosave()", autosave_coundown);

        // loading screen:
        document.getElementById('editzone').innerHTML = '<div style="background:white; border-bottom: 1px solid #bbb; border-right: 1px solid #bbb;margin:10px;padding: 2em 0px;width:95%;text-align: center;"><img src="images/ajax_loading.gif" alt="loading ..." style="width:13px;height:13px;" />&nbsp;&nbsp;loading ...</div>';

        makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl='+objevent+'&d_id='+devideids2+'&d_r_id='+devideids3+'&d_r_lang='+userlang, 'mef', 'editzone', 'html', 'GET', '');
      }
      else {
        alert('bad request: loadEditor('+objevent+', '+entryid+')');
      }
      break;
  } // end switch
}



/**
 * displays/closes the orange alertbox on top of the page
 *
 * @param int action
 */
function alertboxClose( action )
{
  switch (action) {

    // close and delete
    case 0:
      document.getElementById('alertbox').style.visibility = 'hidden';
      document.getElementById('alertboxc').innerHTML = '&nbsp;';
      break;

    // close
    case 1:
      document.getElementById('alertbox').style.visibility = 'hidden';
      alertactiv = window.setTimeout("alertboxClose(2)", 500);
      break;

    // open
    default:
      document.getElementById('alertbox').style.visibility = 'visible';
      alertactiv = window.setTimeout("alertboxClose(0)", 6000);
      break;
  }
}



/**
 * setup text for a new alertbox and shows it
 *
 * @param string text text to be setup in the alert box
 */
function alertbox( text )
{
  window.clearTimeout(alertactiv);
  document.getElementById('alertbox').style.visibility = 'visible';
  document.getElementById('alertboxc').innerHTML = text;
  alertactiv = window.setTimeout("alertboxClose(1)", 500);
}



/**
 * removes an filter from current setting
 *
 * @param string objid
 */
function removeFilter( objid )
{
  document.getElementById('filt'+objid.substr(4)).style.display = 'none';
  document.getElementById('filt'+objid.substr(4)).innerHTML = '';
}



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
}



/**
 * sets the search filter back to his standard text
 */
function clearSearchFilter( )
{
  document.getElementById('filtersct').innerHTML = '';
  document.getElementById('txtfind').value = '';
  searchFilter('txtfind', document.getElementById('txtfind').value, 'Search & Filters', false);
  filtercounter = 0;
  filterid = 0;
}



/**
 * resets all filters
 */
function clearAllFilter( )
{
  clearSearchFilter();

  filtstring1 = '';
  filtstring2 = '';
  loadEntryTable(roscms_current_page);
}



/**
 * get active filters
 */
function getActiveFilters( )
{
  filtstring2 = '';

  for (var i=1; i <= filtercounter; i++) {
    if (document.getElementById('sfa'+i)) {
      filtstring2 += beautifystr(document.getElementById('sfa'+i).value);
      filtstring2 += '_';
      filtstring2 += beautifystr(document.getElementById('sfb'+i).value);
      filtstring2 += '_';
      filtstring2 += beautifystr(document.getElementById('sfc'+i).value);

      // care about visibility-status
      if (document.getElementById('sfv'+i).id && document.getElementById('sfv'+i).className == "filthidden") {
        filtstring2 += '_0';
      }

      filtstring2 += '|';
    }
  } // end for

  // remove last '|'
  filtstring2 = filtstring2.substr(0,filtstring2.length-1);
}



/**
 * get all filters + searchbox
 */
function getAllActiveFilters( )
{
  filtstring1 = '';

  // without filters visible
  if (document.getElementById('filtersc').style.display == 'none') { 
    if (document.getElementById('txtfind').value != 'Search & Filters') {
      if (document.getElementById('txtfind').value.length > 1) {
        filtstring1 = document.getElementById('txtfind').value;
      }
    }

    getActiveFilters();
    loadEntryTable(roscms_current_page);	
  }
}



/**
 * performs a new search by current filter settings
 */
function searchByFilters( )
{
  filtstring1 = '';
  if (document.getElementById('txtfind').value != 'Search & Filters' && document.getElementById('txtfind').value.length > 1) {
    filtstring1 = document.getElementById('txtfind').value;
  }

  getActiveFilters();
  loadEntryTable(roscms_current_page);
}



/**
 * setup a list with presets for selectable rows
 *
 * @param string preset
 */
function htmlSelectPresets( preset )
{
  var selbarstr = '';

  // prepare selections
  var selhtml_space = ', ';
  var selhtml_all = '<span class="button" onclick="selectAll(true)">All</span>';
  var selhtml_none = '<span class="button" onclick="javascript:selectAll(false)">None</span>';
  var selhtml_inv = '<span class="button" onclick="selectInverse()">Inverse</span>';
  var selhtml_star = '<span class="button" onclick="selectStars(true)">Starred</span>';
  var selhtml_nostar = '<span class="button" onclick="selectStars(false)">Unstarred</span>';
  var selhtml_stable = '<span class="button" onclick="selectType(\'stable\')">Stable</span>';
  var selhtml_new = '<span class="button" onclick="selectType(\'new\')">New</span>';
  var selhtml_draft = '<span class="button" onclick="selectType(\'draft\')">Draft</span>';
  var selhtml_uptodate = '<span class="button" onclick="selectType(\'transg\')">Current</span>';
  var selhtml_outdated = '<span class="button" onclick="selectType(\'transr\')">Dated</span>';
  var selhtml_notrans = '<span class="button" onclick="selectType(\'transb\')">Missing</span>';
  var selhtml_unknown = '<span class="button" onclick="selectType(\'unknown\')">Unknown</span>';

  // use for all types
  selbarstr += selhtml_all + selhtml_space
    + selhtml_none + selhtml_space
    + selhtml_inv + selhtml_space;

  // build selection thing
  switch (preset) {
    case 'common':
      selbarstr += selhtml_star + selhtml_space
        + selhtml_nostar + selhtml_space
        + selhtml_stable + selhtml_space
        + selhtml_new + selhtml_space
        + selhtml_draft;
      break;

    case 'trans':
      selbarstr += selhtml_star + selhtml_space
        + selhtml_nostar + selhtml_space
        + selhtml_uptodate + selhtml_space
        + selhtml_outdated + selhtml_space
        + selhtml_notrans;
      break;

    case 'basic':
      selbarstr += selhtml_star + selhtml_space
        + selhtml_nostar;
      break;

    case 'bookmark':
      selbarstr += selhtml_stable + selhtml_space
        + selhtml_new + selhtml_space
        + selhtml_draft;
      break;

    case 'archive':
      // cut off last space
      selbarstr = selbarstr.substring(0, selbarstr.length-2);
      break;
  } // end select

  // update current
  document.getElementById('tabselect1').innerHTML = selbarstr;
  document.getElementById('tabselect2').innerHTML = selbarstr;
}



/**
 * requests the revisions tab in the entry details
 *
 * @param int drid revision id
 */
function showEditorTabRevisions( drid )
{
  alertbox('Change fields only if you know what you are doing.');
  makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=showentry&d_r_id='+drid, 'mef', 'frmedittagsc2', 'html', 'GET', '');
}



/**
 * saves changes made in the editor entry details revisions tab
 *
 * @param int drid revision id
 */
function saveRevisionData( drid )
{
  var uf_check = confirm("Please double check your changes.\n\nDo you want to continue?");

  if (uf_check == true) {
    var d_lang_str = document.getElementById('cbmentrylang').value;
    var d_revnbr_str = document.getElementById('vernbr').value;
    var d_usr_str = beautifystr2(document.getElementById('verusr').value);
    var d_date_str = document.getElementById('verdate').value;
    var d_time_str = document.getElementById('vertime').value;

    // remove leading space character
    if (d_usr_str.substr(0, 1) == ' ') {
      d_usr_str = d_usr_str.substr(1, d_usr_str.length-1); 
    }

    makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=alterentry&d_r_id='+drid+'&d_val='+d_lang_str+'&d_val2='+d_revnbr_str+'&d_val3='+d_usr_str+'&d_val4='+d_date_str+'&d_val5='+d_time_str, 'mef', 'editalterentry', 'html', 'GET', '');
  }
}



/**
 * requests the security tab in the entry details
 *
 * @param int did data id
 * @param int drid revision id
 */
function showEditorTabSecurity( drid )
{
  alertbox('Changes will affect all related entries (see \'History\').');
  makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=showsecurity&d_r_id='+drid, 'mef', 'frmedittagsc2', 'html', 'GET', '');
}



/**
 * saves changes made in the editor entry details security tab
 *
 * @param int did data id
 * @param int drid revision id
 */
function saveSecurityData( did, drid )
{
  var uf_check = confirm("Please double check your changes. \n\nOnly a limited ASCII charset is allowed. \nYou will be on the save side, if you use only A-Z, 0-9, underscore, comma, dot, plus, minus. \n\nDo you want to continue?");

  if (uf_check == true) {
    var d_name_str = beautifystr2(document.getElementById('secdataname').value);
    var d_type_str = document.getElementById('cbmdatatype').value;
    var d_acl_str = document.getElementById('cbmdataacl').value;
    var d_name_update = document.getElementById('chdname').checked;

    // remove leading space character
    if (d_name_str.substr(0, 1) == ' ') {
      d_name_str = d_name_str.substr(1, d_name_str.length-1); 
    }

    makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=altersecurity&d_id='+did+'&d_r_id='+drid+'&d_val='+d_name_str+'&d_val2='+d_type_str+'&d_val3='+d_acl_str+'&d_val4='+d_name_update, 'mef', 'editaltersecurity', 'html', 'GET', '');
  }
}



/**
 * saves changes made in the editor entry details fields tab
 *
 * @param int did data id
 * @param int drid revision id
 */
function saveFieldData( did, drid )
{
  var uf_check = confirm("Please double check your changes. \n\nOnly a limited ASCII charset is allowed. \nYou will be on the save side, if you use only A-Z, 0-9, comma, dot, plus, minus. \n\nDo you want to continue?");

  if (uf_check == true) {
    var stext_str = '';
    var text_str = '';

    // process short texts
    for (var i=1; i <= document.getElementById('editaddstextcount').innerHTML; i++) {
      var tmp_str = beautifystr(document.getElementById('editstext'+i).value);

      // remove leading space character
      if (tmp_str.substr(0, 1) == ' ') {
        tmp_str = tmp_str.substr(1, tmp_str.length-1); 
      }

      stext_str += document.getElementById('editstextorg'+i).value +'='+ tmp_str +'='+ document.getElementById('editstextdel'+i).checked +'|';
    } // end for
    stext_str = stext_str.substr(0, stext_str.length-1);

    // process texts
    for (var i=1; i <= document.getElementById('editaddtextcount').innerHTML; i++) {
      var tmp_str = document.getElementById('edittext'+i).value;

      tmp_str = beautifystr(tmp_str);

      // remove leading space character
      if (tmp_str.substr(0, 1) == ' ') {
        tmp_str = tmp_str.substr(1, tmp_str.length-1); 
      }

      text_str += document.getElementById('edittextorg'+i).value +'='+ tmp_str +'='+ document.getElementById('edittextdel'+i).checked +'|';
    } // end for
    text_str = text_str.substr(0, text_str.length-1);
    makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=alterfields2&d_id='+did+'&d_r_id='+drid+'&d_val='+stext_str+'&d_val2='+text_str, 'mef', 'editalterfields', 'html', 'GET', '');
  } // end if uf_check
}



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
}



/**
 * add a new text field
 */
function addTextField( )
{
  var textcount = document.getElementById('editaddtextcount').innerHTML*1 + 1;

  document.getElementById('editaddtext').innerHTML += '<input type="text" name="edittext'+textcount+'" id="edittext'+textcount+'" size="25" maxlength="100" value="" />&nbsp;';
  document.getElementById('editaddtext').innerHTML += '<input type="checkbox" name="edittextdel'+textcount+'" id="edittextdel'+textcount+'" value="del" /><label for="edittextdel'+textcount+'">delete?</label>'
  document.getElementById('editaddtext').innerHTML += '<input name="edittextorg'+textcount+'" id="edittextorg'+textcount+'" type="hidden" value="new" /><br /><br />';
  document.getElementById('editaddtextcount').innerHTML = textcount;
}



/**
 * requests the metadata tab in the entry details
 *
 * @param int drid revision id
 */
function showEditorTabMetadata( drid )
{
  makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=showtag&d_r_id='+drid, 'mef', 'frmedittagsc2', 'html', 'GET', '');
}



/**
 * requests the history tab in the entry details
 *
 * @param int drid revision id
 */
function showEditorTabHistory( drid )
{
  makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=showhistory&d_r_id='+drid, 'mef', 'frmedittagsc2', 'html', 'GET', '');
}



/**
 * requests the depencies tab in the entry details
 *
 * @param int drid revision id
 */
function showEditorTabDepencies(  drid )
{
  makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=showdepencies&d_r_id='+drid, 'mef', 'frmedittagsc2', 'html', 'GET', '');
}



/**
 * requests the fields tab in the entry details
 *
 * @param int drid revision id
 * @param int dusr user
 */
function showEditorTabFields( drid, dusr )
{
  alertbox('Change fields only if you know what you are doing.');
  makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=alterfields&d_r_id='+drid+'&d_val3='+dusr, 'mef', 'frmedittagsc2', 'html', 'GET', '');
}



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

  if (dtn == 'tag') dtna = 'tag';
  else dtna = document.getElementById(dtn).value;

  dtva = document.getElementById(dtv).value;

  if (dtna != '' && dtva != '') {
    makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=addtag&d_r_id='+drid+'&d_val='+encodeURIComponent(dtna)+'&d_val2='+encodeURIComponent(dtva)+'&d_val3='+dusr, 'mef', 'frmedittagsc2', 'html', 'GET', '');
  }
}



/**
 * deletes a user label, system metadata/label
 *
 * @param int tag_id
 */
function delLabelOrTag( tag_id )
{
  if (tag_id > 0) {
    makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=deltag&d_val='+tag_id, 'mef', 'frmedittagsc2', 'html', 'GET', '');
  }
}



/**
 * updates or changes a specific tag, mostly status or star
 *
 * @param int did data id
 * @param int drid revision id
 * @param string dtn tag name
 * @param string dtv tag value
 * @param int dusr user
 * @param int dtid tag id
 * @param string objid
 * @param string dbflag
 */
function updateTag( did, drid, dtn, dtv, dusr, dtid, objid, dbflag )
{
  if (dtn != '' && dtv != '') {
    makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=changetag'+encodeURIComponent(dbflag)+'&d_id='+did+'&d_r_id='+drid+'&d_val='+encodeURIComponent(dtn)+'&d_val2='+encodeURIComponent(dtv)+'&d_val3='+dusr+'&d_val4='+dtid, 'mef', objid, 'html', 'GET', '');
  }
}



/**
 * prepares the short texts & texts for submit via POST 
 */
function getEditorTexts( )
{
  var poststr = "";
  var instatinymce;

  try {
    // short text
    poststr += "pstextsum="+document.getElementById("estextcount").className;

    for (var i=1; i <= document.getElementById("estextcount").className; i++) {
      poststr += "&pdstext"+i+"=" + encodeURIComponent(document.getElementById("edstext"+i).innerHTML);
      poststr += "&pstext"+i+"=" + encodeURIComponent(document.getElementById("estext"+i).value);
    }

    // text
    poststr += "&plmsum="+document.getElementById("elmcount").className;
    
    for (var i=1; i <= document.getElementById("elmcount").className; i++) {
      poststr += "&pdtext"+i+"=" + encodeURIComponent(document.getElementById("textname"+i).innerHTML);

      // get the content from TinyMCE
      instatinymce = ajaxsaveContent("elm"+i); 
      if (instatinymce != null) {
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
}



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
    alertbox('Draft saved');
  }
}



/**
 * adds or updates the draft (sends request to save)
 *
 * @param int did data id
 * @param int drid revision id
 */
function addOrReplaceDraft( did, drid )
{
  var poststr = getEditorTexts();

  if (poststr != false) {
    makeRequest('?page=data_out&d_f=text&d_u=asi&d_fl=new&d_id='+encodeURIComponent(did)+'&d_r_id='+encodeURIComponent(drid)+'&d_r_lang='+encodeURIComponent(document.getElementById("mefrlang").innerHTML)+'&d_r_ver='+encodeURIComponent(document.getElementById("mefrverid").innerHTML)+'&d_val='+encodeURIComponent(document.getElementById("estextcount").className)+'&d_val2='+encodeURIComponent(document.getElementById("elmcount").className)+'&d_val3=draft&d_val4='+encodeURIComponent(document.getElementById("entryeditdynnbr").innerHTML), 'asi', 'mefasi', 'html', 'POST', poststr.substr(1));
    return true;
  }
  else {
    alertbox('Cannot save draft.');
    return false;
  }
}



/**
 * tries to autosave an entry
 */
function tryAutosave( )
{
  window.clearTimeout(autosave_timer);

  try {
    if (document.getElementById("editautosavemode").value == 'false') {
      window.clearTimeout(autosave_timer);
      return;
    }
  } 
  catch (e) {
    window.clearTimeout(autosave_timer);
    return;
  }

  if (autosave_cache != getEditorTexts() && autosave_cache != '') {
    addOrReplaceDraft(document.getElementById("entrydataid").className, document.getElementById("entrydatarevid").className);
  }

  autosave_timer = window.setTimeout("tryAutosave()", autosave_coundown); // 10000
}



/**
 * loads another tab in the 'new entry' editor interface
 *
 * @param string menumode 'single','dynamic','template'
 */
function changeNewEntryTab( mode )
{
  if (mode == 'single' || mode == 'dynamic' || mode == 'template') {
    makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=newentry&d_id=new&d_val='+mode, 'mef', 'addnew', 'html', 'GET', '');
  }
}



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
    var tentrs30, tentrs31;

    if (tentrs20[1] < tentrs21[1]) {
      getDiffEntries(tentrs20[1], tentrs21[1])
    }
    else {
      getDiffEntries(tentrs21[1], tentrs20[1])
    }
  }
  else {
    alertbox("Select two entries to compare them!");
  }
}



/**
 * used by the compare button
 *
 * @param int revid1
 * @param int revid2
 */
function getDiffEntries( revid1, revid2 )
{
  if (revid1 != '' && revid2 != '') {
    makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=diff&d_val='+encodeURIComponent(revid1)+'&d_val2='+encodeURIComponent(revid2), 'mef', 'diff2', 'html', 'GET', '');
  }
}



/**
 * closes diff area
 *
 * @param int revid1
 * @param int revid2
 */
function openOrCloseDiffArea( revid1, revid2 )
{
  document.getElementById('frmdiff').innerHTML = '';

  if (document.getElementById('frmdiff').style.display == 'none') {
    document.getElementById('frmdiff').style.display = 'block';
    document.getElementById('bshowdiffi').src = 'images/tab_open.gif';
    getDiffEntries(revid1, revid2);
  }
  else {
    document.getElementById('frmdiff').style.display = 'none';
    document.getElementById('bshowdiffi').src = 'images/tab_closed.gif';
  }
}



/**
 * change entry tag 'status' for selected entries
 *
 * @param string ctk tag action 'ms' / 'mn'
 */
function changeSelectedTags( ctk )
{
  if ((document.getElementById('extraopt').value != 'sel' && document.getElementById('extraopt').value != 'no') || ctk == 'ms' || ctk == 'mn') {
    var tentrs = selectedEntries().split("|");

    if (tentrs[0] < 1 || tentrs[0] == '') {
      alertbox("No entry selected.");
    }
    else {
      var tmp_obj = 'changetags';

      if (ctk == 'ms') {
        tmp_obj = 'changetags2';
        alertbox('Please be patient, related pages get generated ...');
      }

      makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=changetags&d_val='+encodeURIComponent(tentrs[0])+'&d_val2='+encodeURIComponent(tentrs[1])+'&d_val3='+encodeURIComponent(ctk), 'mef', tmp_obj, 'html', 'GET', '');
    }
  }

  document.getElementById('extraopt').value = 'sel';

  try {
    document.getElementById('cmddiff').focus();
  }
  catch (e) {
  }
}



/**
 * hides or shows the ajax loading graphic
 *
 * @param bool show
 */
function loadingSplash( show )
{
  if (show) {
    document.getElementById('ajaxloadinginfo').style.visibility = 'visible';
  }
  else {
    document.getElementById('ajaxloadinginfo').style.visibility = 'hidden';
  }
}



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
      } catch (e) {
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

  http_request.onreadystatechange = function() { alertContents(http_request, action, objid); };

  if (kind == 'POST') {
    http_request.open('POST', url, true);
    http_request.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    http_request.setRequestHeader("Content-length", parameters.length);
    http_request.setRequestHeader("If-Modified-Since", "Sat, 1 Jan 2000 00:00:00 GMT");	// Bypass the IE Cache
    http_request.setRequestHeader("Connection", "close");
    http_request.send(parameters);
  }
  else {
    http_request.open('GET', url, true);
    http_request.setRequestHeader("If-Modified-Since", "Sat, 1 Jan 2000 00:00:00 GMT");	// Bypass the IE Cache
    http_request.send(null);
  }

  return true;
}



/**
 * processes requested actions for the requests
 *
 * @param object http_request
 * @param string action
 * @param string objid
 */
function alertContents( http_request, action, objid )
{
  try {
    if (http_request.readyState == 4) {
      if (http_request.status == 200) {
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

          // auto save info
          case 'asi': 
            showAutosaveInfo(http_request, objid);
            break;

            // user filter storage
          case 'ufs': 
            updateUserFilter(http_request, objid);
            break;

            // user tags
          case 'ut': 
            updateUserTags(http_request, objid);
            break;

            // user quick info
          case 'uqi': 
            updateQuickinfo(http_request, objid);
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
    if (roscms_page_load_finished == true) {
      alertbox('RosCMS caught an exception to prevent data loss. If you see this message several times, please make sure you use an up-to-date browser client. If the issue still occur, tell the website admin the following information:<br />Name: '+ e.name +'; Number: '+ e.number +'; Message: '+ e.message +'; Description: '+ e.description);
    }
  }

  // to prevent memory leak
  http_request = null;
}



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

  lstHeader = '<table id="entryTable" cellpadding="1" cellspacing="0">'
    + '<thead><tr class="head">'
    + '<th scope="col" class="cMark">&nbsp;</th>'
    + '<th scope="col" class="cStar">&nbsp;</th>'
    + '<th scope="col" class="cCid">Name</th>'
    + '<th scope="col" class="cSpace">&nbsp;</th>'
    + '<th scope="col" class="cRev">Title</th>';


  if (xtrtblcol != '' && xtrtblcols2.length > 1) {
    for (var i=1; i < xtrtblcols2.length-1; i++) {
      lstHeader += '<th scope="col" class="cSpace">&nbsp;</th>'
        + '<th scope="col" class="cExtraCol">'+xtrtblcols2[i]+'</th>';
    }
  }

  lstHeader+= '<th scope="col" class="cSpace">&nbsp;</th>'
    + '<th scope="col" class="cDate">Date</th>'
    + '</tr></thead><tbody>\n';

  return lstHeader;
}



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
    + '<td align="right"><input id="cb'+(bnr+1)+'" type="checkbox" onclick="selectRow(this.id)" /></td>'
    + '<td class="tstar_'+bid+'|'+brid+'-'+bstarid+'"><div id="bstar_'+(bnr+1)+'" class="'+bstar+'">&nbsp;</div></td>'
    + '<td class="rv'+bid+'|'+brid+'">'+bdname+'</td>'
    + '<td class="rv'+bid+'|'+brid+'">&nbsp;</td>'
    + '<td class="rv'+bid+'|'+brid+'" class="cell-height">';

  // not found -> readonly
  if (security.indexOf("write") < 0 ) { 
    lstBody += '<img src="images/locked.gif" alt="read-only" style="width:11px; height:12px; border:0px;" /> ';
  }

  try {
    tmpdesc = unescape(decodeURI(bdesc));
    tmpdesc = tmpdesc.replace(/\+/g, ' ');
    if (tmpdesc.length == 0) tmpdesc = '&nbsp;';
  } catch (e) {
    tmpdesc = '<em>check the title or description field, it contains non UTF-8 chars</em>';
  }
  lstBody += '<span class="tcp">'+tmpdesc+'</span></td>';

  if (xtrtblcol != '' && xtrtblcols2.length > 1) {
    for (var i=1; i < xtrtblcols2.length-1; i++) {
      lstBody += '<td class="rv'+bid+'|'+brid+'">&nbsp;</td>'
        + '<td class="rv'+bid+'|'+brid+'">'+xtrtblcols2[i]+'</td>';
    }
  }

  lstBody += '<td class="rv'+bid+'|'+brid+'">&nbsp;</td>'
    + '<td class="rv'+bid+'|'+brid+'">'+brdate.substr(0, 10)+'</td>'
    + '</tr>';

  return lstBody;
}



/**
 * displays the given number of empty lines
 *
 * @param int spacelines
 */
function htmlEntryTableMargin( spacelines )
{
  var lstSpace = "<br />";

  for (var i=0; i < spacelines; i++) {
    lstSpace += "<br />";
  }

  return lstSpace;
}



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
      document.getElementById('editzone').innerHTML = '<div id="frmdiff">'+ http_request.responseText + '</div>';
      loadEditor('diffentry');
      document.getElementById('frmeditdiff').innerHTML = CDiffString(document.getElementById('frmeditdiff1').innerHTML, document.getElementById('frmeditdiff2').innerHTML);
      break;

    // update diff-area with new entries start diff-process; called from within diff-area
    case 'diff2': 
      document.getElementById('frmdiff').innerHTML = http_request.responseText;
      document.getElementById('frmeditdiff').innerHTML = CDiffString(document.getElementById('frmeditdiff1').innerHTML, document.getElementById('frmeditdiff2').innerHTML);
      break;

    case 'changetags':
      reloadEntryTableWithOffset(0);
      alertbox('Action performed');
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

    case 'updatetag':
      selectUserTags();
      objid = tsplits[1];

    default:
      document.getElementById(objid).innerHTML = http_request.responseText;
      autosave_cache = getEditorTexts();
      break;
  } // end switch
}



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

      if (curr_hour.length == 1) curr_hour = '0'+curr_hour;
      if (curr_min.length == 1) curr_min = '0'+curr_min;

      if (autosave_cache != tempcache) {
        autosave_cache = tempcache;
      }
      document.getElementById('mefasi').innerHTML = 'Draft saved at '+ curr_hour +':'+ curr_min;
      break;

    case 'alert':
      window.clearTimeout(autosave_timer);
      loadEntryTable(roscms_prev_page);
      break;

    default:
      alert('showAutosaveInfo() with no args');
      break;
  }
}



/**
 * update users filter list
 *
 * @param object http_request
 * @param string objid
 */
function updateUserFilter( http_request, objid )
{
  document.getElementById(objid).innerHTML = http_request.responseText;
}




/**
 * update users tags
 *
 * @param object http_request
 * @param string objid
 */
function updateUserTags( http_request, objid )
{
  document.getElementById(objid).innerHTML = http_request.responseText;
}



/**
 * show the current quickinfo
 *
 * @param object http_request
 * @param string objid
 */
function updateQuickinfo( http_request, objid )
{
  document.getElementById('qiload').style.display = 'none';
  document.getElementById(objid).innerHTML = http_request.responseText;
}



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
    var devide1 = ''
    var devide2 = ''
    var devideids1 = ''
    var devideids2 = ''
    var devideids3 = ''

    devide1 = entryid.split('_');
    devide2 = devide1[1].split('-');
    devideids1 = devide2[0].indexOf("|");
    devideids2 = devide2[0].substr(0, devideids1);
    devideids3 = devide2[0].substr(devideids1+1);

    if (devideids2.substr(0,2) == 'tr') {
      alertbox('Cannot bookmark not translated entries.');
    }
    else {
      if (dtv == 'cStarOff') {
        dtv = 'on';
        document.getElementById(objid).className = 'cStarOn';
      }
      else {
        dtv = 'off';
        document.getElementById(objid).className = 'cStarOff';
      }
      updateTag(devideids2, devideids3, 'star', dtv, dusr, devide2[1], objid, '3');
    }
  }
}



/**
 * table mouse events
 */
function registerMouseActions( )
{
  var j;

  if (!document.getElementById) return;

  //row highlighting
  if (hlRows) {
    for (var i=1; i<=nres; i++) {

      document.getElementById("tr"+i).onmouseover = function() {hlRow(this.id.substr(2,4),1); showQuickinfo(this.getElementsByTagName('td')[3].className);}
      document.getElementById("tr"+i).onmouseout = function() {hlRow(this.id.substr(2,4),2); stopQuickinfo();}
      document.getElementById("tr"+i).getElementsByTagName('td')[0].onclick = function() {hlRow(this.parentNode.id.substr(2,4),3);selectRow(this.parentNode.id.substr(2,4));}
      document.getElementById("tr"+i).getElementsByTagName('td')[1].onclick = function() {setBookmark(this.className, document.getElementById('bstar_'+this.parentNode.id.substr(2,4)).className, roscms_intern_account_id, 'bstar_'+this.parentNode.id.substr(2,4));}

      // support up to 13 rows
      for(j=2; j<13; j++) {
        if (document.getElementById("tr"+i).getElementsByTagName('td')[j]) {
          document.getElementById("tr"+i).getElementsByTagName('td')[j].onclick = function() {loadEditor(roscms_current_page, this.className);}
        }
        else {
          break;
        }
      }
    }
  }
}



/**
 * opens a new tab on the left side
 *
 * @param string objid
 */
function loadMenu( objid )
{
  var chtabtext = null;
  var translang = '';

  submenu_button = true;
  roscms_archive = false;

  // deselect all
  selectAll(false);

  // reset search box:
  filtstring1 = '';
  document.getElementById('txtfind').value = '';
  searchFilter('txtfind', document.getElementById('txtfind').value, 'Search & Filters', false);

  window.clearTimeout(autosave_timer);
  autosave_cache = '';

  if (document.getElementById('smenutab'+objid.substring(8)).className != 'subma') {
    highlightTab(objid);
  }

  if (getLang() == roscms_standard_language) {
    alertbox('You can\'t translate entries, because you don\'t have the standard language as your user language.');
    return false;
  }
  else {
    translang = getLang();
  }

  switch (objid.substring(8)) {

    case '1':
      filtstring2 = '';
      loadEditor('newentry', 'new');
      break;

    case '2':
      filtstring2 = 'k_is_new_0|c_is_type_0|l_is_'+getLang()+'_0|o_desc_datetime';
      loadEntryTable('new');
      break;

    case '3':
      filtstring2 = 'y_is_page_0|k_is_stable_0|l_is_'+getLang()+'_0|o_asc_name';
      loadEntryTable('page');
      break;

    case '4':
      filtstring2 = 'y_is_content_0|k_is_stable_0|l_is_'+getLang()+'_0|o_asc_name';
      loadEntryTable('content');
      break;

    case '5':
        filtstring2 = 'y_is_template_0|k_is_stable_0|l_is_'+getLang()+'_0|o_asc_name';
      loadEntryTable('template');
      break;

    case '6':
      filtstring2 = 'y_is_script_0|k_is_stable_0|l_is_'+getLang()+'_0|o_asc_name';
      loadEntryTable('script');
      break;

    case '7':
      filtstring2 = 'y_is_content_0|k_is_stable_0|i_is_default_0|c_is_user_0|l_is_'+roscms_standard_language+'_0|r_is_'+translang+'|o_asc_name';
      loadEntryTable('translate');
      break;

    case '8':
    default:
      filtstring2 = 'c_is_type_0|l_is_'+getLang()+'|o_desc_datetime';
      loadEntryTable('all');
      break;

    case '9':
      filtstring2 = 's_is_true_0|c_is_type_0|l_is_'+getLang()+'_0|o_desc_datetime';
      loadEntryTable('starred');
      break;

    case '10':
      filtstring2 = 'k_is_draft_0|u_is_'+roscms_intern_login_check_username+'_0|c_is_type_0|o_desc_datetime';
      loadEntryTable('draft');
      break;

    case '11':
      filtstring2 = 'u_is_'+roscms_intern_login_check_username+'_0|c_is_type_0|o_desc_datetime';
      loadEntryTable('my');
      break;

    case '12':
      filtstring2 = 'k_is_archive_0|c_is_version_0|c_is_type_0|l_is_'+getLang()+'_0|o_asc_name|o_desc_ver';
      roscms_archive = true; /* activate archive mode*/
      loadEntryTable('archive');
      break;
  } // end switch

  return true;
}



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
}



/**
 * gets the current selected session language
 */
function getLang( )
{
  return userlang;
}



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
      updateTag(did, drid, 'star', 'off', dusr, document.getElementById(objid).className, objid, '3');
    }
    else {
      document.getElementById(objid).src = roscms_intern_webserver_roscms+'images/star_on_small.gif';
      updateTag(did, drid, 'star', 'on', dusr, document.getElementById(objid).className, objid, '3');
    }
  }
}



/**
 * creates a new entry with the 'new entry' interface
 *
 * @param int menumode
 */
function createNewEntry( menumode )
{
  switch (menumode) {

    case 0:
      if (document.getElementById('txtaddentryname').value != "") {
        makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=newentry2&d_name='+encodeURIComponent(document.getElementById('txtaddentryname').value)+'&d_type='+encodeURIComponent(document.getElementById('txtaddentrytype').value)+'&d_r_lang='+encodeURIComponent(document.getElementById('txtaddentrylang').value), 'mef', 'addnew2', 'html', 'GET', '');
      }
      else {
        alertbox('Entry name is required');
      }
      break;

    case 1:
      makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=newentry4&d_name='+encodeURIComponent(document.getElementById('txtadddynsource').value)+'&d_type=content&d_r_lang='+roscms_standard_language, 'mef', 'addnew2', 'html', 'GET', '');
      break;

    case 2:
      if (document.getElementById('txtaddentryname3').value != "") {
        makeRequest('?page=data_out&d_f=text&d_u=mef&d_fl=newentry3&d_name='+encodeURIComponent(document.getElementById('txtaddentryname3').value)+'&d_type=content&d_r_lang='+roscms_standard_language+'&d_template='+encodeURIComponent(document.getElementById('txtaddtemplate').value), 'mef', 'addnew2', 'html', 'GET', '');
      }
      else {
        alertbox('Entry name is required');
      }
  } // end switch
}



/**
 * gives a preview of >one< selected entry
 */
function previewPage( )
{
  var tentrs = selectedEntries().split("|");
  var tentrs2 = tentrs[1].split("_");

  if (tentrs[0] == 1) {
    window.open(roscms_intern_page_link+"data_out&d_f=page&d_u=show&d_val="+tentrs2[1]+"&d_val2="+userlang+"&d_val3=", "RosCMSPagePreview");
  }
  else {
    alertbox("Select one entry to preview a page!");
  }

  document.getElementById('extraopt').value = 'sel';
}



/**
 * builds a new entry table with the requested data
 *
 * @param object http_request
 * @param string objid
 */
function buildEntryTable( http_request, objid )
{
  var lstData = "";
  var temp_counter_loop = 0;
  var xmldoc = http_request.responseXML;
  var root_node = xmldoc.getElementsByTagName('root').item(0);

  // check if server has send valid data
  try {
    if (root_node.firstChild.data) {
      // temp
    }
  }
  catch (e) {
    nres = 0;
    hlRows=false;
    document.getElementById('mtblnav').innerHTML = '&nbsp;';
    document.getElementById('mtbl2nav').innerHTML = '&nbsp;';
    lstData = '<div class="tableswhitespace"><br /><br /><b>No results, due an error in the filter settings or the data metadata.</b><br /><br />If this happens more than a few times, please contact the website admin with the following information:<br />Name: '+ e.name +'<br />Number: '+ e.number +'<br />Message: '+ e.message +'<br />ObjID: '+ objid +'<br />Request: <pre>'+ http_request +'</pre>'+ htmlEntryTableMargin(8)+'</div>';
    document.getElementById(objid).innerHTML = lstData;
    return;
  }

  if ((root_node.firstChild.data.search(/#none#/)) == -1) {
    var xrow = xmldoc.getElementsByTagName("row");
    var xview = xmldoc.getElementsByTagName("view");
    var mtblnavstr = '';
    var mtblnavfrom = xview[0].getAttributeNode("curpos").value*1+1;
    var mtblnavto = (xview[0].getAttributeNode("curpos").value*1) + (xview[0].getAttributeNode("pagelimit").value*1);

    roscms_current_tbl_position = xview[0].getAttributeNode("curpos").value;

    // table navigation next page
    if (xview[0].getAttributeNode("curpos").value >= roscms_intern_entry_per_page*2) {
      mtblnavstr += '<span class="button" onclick="reloadEntryTableWithOffset(0)"><strong>&laquo;</strong></span>&nbsp;&nbsp;';
    }

    // table navigation previous page
    if (xview[0].getAttributeNode("curpos").value > 0) {
      mtblnavstr += '<span class="button" onclick="reloadEntryTableWithOffset(';
      if (xview[0].getAttributeNode("curpos").value-roscms_intern_entry_per_page*1 >= 0) {
        mtblnavstr += xview[0].getAttributeNode("curpos").value-roscms_intern_entry_per_page*1;
      }
      else {
        mtblnavstr += '0';
      }
      mtblnavstr += ')"><strong>&lsaquo; Previous</strong></span>&nbsp;&nbsp;';
    }

    // current entry table begin
    mtblnavstr += '<strong>'+mtblnavfrom+'</strong> - <strong>';

    // foo from bar entries displayed
    if (mtblnavto > xview[0].getAttributeNode("pagemax").value) {
      mtblnavstr += xview[0].getAttributeNode("pagemax").value;
    }
    else {
      mtblnavstr += mtblnavto;
    }
    mtblnavstr += '</strong> of <strong>'+xview[0].getAttributeNode("pagemax").value+'</strong>';

    // display navigation
    if (xview[0].getAttributeNode("curpos").value < xview[0].getAttributeNode("pagemax").value-roscms_intern_entry_per_page*1) {
      mtblnavstr += '&nbsp;&nbsp;<span class="button" onclick="reloadEntryTableWithOffset(';
      mtblnavstr += xview[0].getAttributeNode("curpos").value*1+roscms_intern_entry_per_page*1;
      mtblnavstr += ')"><b>Next &rsaquo;</b></span>';
    }

    if (xview[0].getAttributeNode("curpos").value < (xview[0].getAttributeNode("pagemax").value*1-roscms_intern_entry_per_page*2)) {
      mtblnavstr += '&nbsp;&nbsp;<span class="button" onclick="reloadEntryTableWithOffset(';
      mtblnavstr += xview[0].getAttributeNode("pagemax").value*1-roscms_intern_entry_per_page*1;
      mtblnavstr += ')"><b>&raquo;</b></span>';
    }
    mtblnavstr += '&nbsp;&nbsp;';

    // update current navigation
    document.getElementById('mtblnav').innerHTML = mtblnavstr;
    document.getElementById('mtbl2nav').innerHTML = mtblnavstr;

    lstData += htmlEntryTableHeader(xview[0].getAttributeNode("tblcols").value);

    for (var i=0; i < xrow.length; i++) {

      // build current line
      lstData += htmlEntryTableRow(i, xrow[i].getAttributeNode("status").value, xrow[i].getAttributeNode("id").value, xrow[i].getAttributeNode("dname").value, xrow[i].getAttributeNode("type").value, xrow[i].getAttributeNode("rid").value, xrow[i].getAttributeNode("rver").value, xrow[i].getAttributeNode("rlang").value, xrow[i].getAttributeNode("rdate").value, xrow[i].getAttributeNode("star").value, xrow[i].getAttributeNode("starid").value, xrow[i].getAttributeNode("rusrid").value, xrow[i].getAttributeNode("security").value, xrow[i].getAttributeNode("xtrcol").value, xrow[i].firstChild.data);

      temp_counter_loop = i;
    }
    lstData += '</tbody></table>';

    // fillup whitespace
    if (xrow.length < 15) {
      lstData += '<div class="tableswhitespace"><br />'+htmlEntryTableMargin(15-xrow.length)+'</div>';
    }

    nres = (temp_counter_loop+1);
    hlRows=true;
    window.setTimeout("registerMouseActions()", 100);
  }
  else {
    nres = 0;
    hlRows=false;
    document.getElementById('mtblnav').innerHTML = '&nbsp;';
    document.getElementById('mtbl2nav').innerHTML = '&nbsp;';
    lstData = '<div class="tableswhitespace"><br /><br /><b>No results.</b>'+htmlEntryTableMargin(15)+'</div>';
  }

  // update table
  document.getElementById(objid).innerHTML = lstData;
}



/**
 * holds message for unloading the page
 *
 * @param string src
 */
function unloadMessage( )
{
  if (exitmsg != '') {
    return exitmsg;
  }
  return false;
}



/**
 * gives html for commands like compare, preview, make stable, mark new and the dropdown
 *
 * @param string opt
 */
function htmlCommandBar( preset )
{
  var cmdbarstr = '';
  var setbold = 'cmddiff';

  // prepare some commands
  var cmdhtml_space = '&nbsp;';
  var cmdhtml_diff = '<button type="button" id="cmddiff" onclick="compareEntries()">Compare</button>'+cmdhtml_space;
  var cmdhtml_preview = '<button type="button" id="cmdpreview" onclick="previewPage()">Preview</button>'+cmdhtml_space;
  var cmdhtml_ready = '<button type="button" id="cmdready" onclick="changeSelectedTags(\'mn\')">Ready</button>'+cmdhtml_space;
  var cmdhtml_refresh = '<button type="button" id="cmdrefresh" onclick="loadEntryTableWithOffset(roscms_current_tbl_position)">Refresh</button>'+cmdhtml_space;
  var cmdhtml_select_start = '<select name="extraopt" id="extraopt" style="vertical-align: top; width: 22ex;" onchange="changeSelectedTags(this.value)">'
        + '<option value="sel" style="color: rgb(119, 119, 119);">More actions...</option>';
  var cmdhtml_select_as = '<option value="as">&nbsp;&nbsp;&nbsp;Add star</option>';
  var cmdhtml_select_xs = '<option value="xs">&nbsp;&nbsp;&nbsp;Remove star</option>';
  var cmdhtml_select_no = '<option value="no" style="color: rgb(119, 119, 119);">&nbsp;&nbsp;&nbsp;-----</option>';
  var cmdhtml_select_mn = '<option value="mn">&nbsp;&nbsp;&nbsp;Mark as new</option>';
  var cmdhtml_select_xe2 = '<option value="xe">&nbsp;&nbsp;&nbsp;Delete</option>';
  var cmdhtml_select_end = '</select>';
  
  var cmdhtml_stable = '';
  var cmdhtml_select_ms = '';
  var cmdhtml_select_ge = '';
  var cmdhtml_select_va = '';
  var cmdhtml_select_xe = '';

  // special commands for access levels
  if (roscms_access_level >= 2) {
    cmdhtml_stable = '<button type="button" id="cmdstable" onclick="changeSelectedTags(\'ms\')">Stable</button>'+cmdhtml_space;
    cmdhtml_select_ms = '<option value="ms">&nbsp;&nbsp;&nbsp;Mark as stable</option>';
    cmdhtml_select_ge = '<option value="va">&nbsp;&nbsp;&nbsp;Generate page</option>';
    
    if (roscms_access_level == 3) {
      cmdhtml_select_va = '<option value="va">&nbsp;&nbsp;&nbsp;Move to archive</option>';
      cmdhtml_select_xe = '<option value="xe">&nbsp;&nbsp;&nbsp;Delete</option>';
    }
  }
  
  var cmdhtml_select_full = cmdhtml_select_start
        + cmdhtml_select_as
        + cmdhtml_select_xs
        + cmdhtml_select_no
        + cmdhtml_select_mn
        + cmdhtml_select_ms
        + cmdhtml_select_va
        + cmdhtml_select_xe
        + cmdhtml_select_end;

  switch (preset) {

    case 'all':
      cmdbarstr += cmdhtml_diff
        + cmdhtml_preview
        + cmdhtml_ready
        + cmdhtml_stable
        + cmdhtml_refresh
        + cmdhtml_select_full;
      break;

    case 'trans':
      cmdbarstr += cmdhtml_diff
        + cmdhtml_preview
        + cmdhtml_refresh
        + cmdhtml_select_full;
      break;

    case 'new':
      cmdbarstr += cmdhtml_diff
        + cmdhtml_preview
        + cmdhtml_stable
        + cmdhtml_refresh
        + cmdhtml_select_full;
      break;

    case 'page':
      cmdbarstr += cmdhtml_preview
        + cmdhtml_diff
        + cmdhtml_refresh
        + cmdhtml_select_full;
      setbold = 'cmdpreview';
      break;

    case 'script':
      cmdbarstr += cmdhtml_diff
        + cmdhtml_refresh
        + cmdhtml_select_full;
      break;

    case 'draft':
      cmdbarstr += cmdhtml_diff
        + cmdhtml_preview
        + cmdhtml_ready
        + cmdhtml_stable
        + cmdhtml_refresh
        + cmdhtml_select_start
        + cmdhtml_select_as
        + cmdhtml_select_xs
        + cmdhtml_select_no
        + cmdhtml_select_mn
        + cmdhtml_select_ms
        + cmdhtml_select_va
        + cmdhtml_select_no
        + cmdhtml_select_xe2
        + cmdhtml_select_end;
      break;

    case 'archive':
      cmdbarstr += cmdhtml_refresh
        + cmdhtml_select_start
        + cmdhtml_select_as
        + cmdhtml_select_xs
        + cmdhtml_select_end;
      setbold = 'cmdrefresh';
      break;
  } 

  document.getElementById('tablecmdbar').innerHTML = cmdbarstr;
  document.getElementById(setbold).style.fontWeight = 'bold';
}



/**
 * adds a new filter to the actual list
 */
function addFilter( )
{
  getActiveFilters();

  if (roscms_access_level > 1) { 
    tmp_security_new_filter = "k_is_stable";
  }
  else {
    tmp_security_new_filter = "a_is_";
  }

  if (filtstring2 == '') {
    htmlFilterChoices(tmp_security_new_filter);
  }
  else {
    htmlFilterChoices(filtstring2+'|'+tmp_security_new_filter);
  }
}



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

  if (filtpopstr != '') {
    var indexid = '';
    var filtvisibility = false;
    var filtpopstr2 = filtpopstr.split('|');

    for (var i=0; i < filtpopstr2.length; i++) {
      lstfilterstr2 = '';
      lstfilterstr2 = filtpopstr2[i].split('_');

      if (lstfilterstr2[3] == 0) {
        filtvisibility = false;

        if (roscms_access_level > 1) {
          lstfilterstr +=  '<span style="font-style: italic;">';
        }
        else {
          lstfilterstr +=  '<span style="display: none">';
        }
      }
      else {
        filtvisibility = true;
        lstfilterstr +=  '<span style="font-style: normal;">';
      }

      indexid = i + 1;
      lstfilterstr +=  '<div id="filt'+indexid+'" class="filterbar2">and&nbsp;';

      // hidden filter entries don't need a combobox (only for SecLev = 1 user)
      if (lstfilterstr2[3] == 0 && roscms_access_level == 1) {  
        lstfilterstr +=  '<input type="hidden" name="sfa'+indexid+'" id="sfa'+indexid+'" value="" />';
      }
      else {
        lstfilterstr +=  '<select id="sfa'+indexid+'" onchange="isFilterChanged(this.id)">';

        if (roscms_access_level > 1) { 
          lstfilterstr += '<option value="k"+roscms_cbm_hide+>Status</option>'
            + '<option value="y"+roscms_cbm_hide+>Type</option>';
        } 

        lstfilterstr += '<option value="n">Name</option>'
          + '<option value="v">Version</option>'
          + '<option value="s">Starred</option>'
          + '<option value="a">Tag</option>'
          + '<option value="l">Language</option>';

        if (roscms_access_level > 1) {
          lstfilterstr += '<option value="r"+roscms_cbm_hide+>Translate</option>'
            + '<option value="i"+roscms_cbm_hide+>Security</option>'
            + '<option value="m"+roscms_cbm_hide+>Metadata</option>'
            + '<option value="u"+roscms_cbm_hide+>User</option>';

          if (roscms_access_level == 3) {
            lstfilterstr += '<option value="e"+roscms_cbm_hide+>System</option>';
          }
        }

        lstfilterstr += '<option value="d">Date</option>'
          + '<option value="t">Time</option>'
          + '<option value="c">Column</option>'
          + '<option value="o">Order</option>'
          + '</select>&nbsp;';
      }

      lstfilterstr +=  htmlFilterValues(lstfilterstr2[0], lstfilterstr2[3], indexid)
        +  '&nbsp;&nbsp;&nbsp;<span id="fdel'+indexid+'" class="filterbutton" onclick="removeFilter(this.id)"><img src="images/remove.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Delete</span>'
        +  '</div>';

      if (lstfilterstr2[3] == 0) {
        lstfilterstr +=  '<span id="sfv'+indexid+'" class="filthidden"></span>'; // store visibility-status
      }
      else {
        lstfilterstr +=  '<span id="sfv'+indexid+'" class="filtvisible"></span>'; // store visibility-status
      }
      lstfilterstr +=  '</span>';
    } // end for

    // apply builded filter
    document.getElementById('filtersct').innerHTML = lstfilterstr;

    for (var i=0; i < filtpopstr2.length; i++) {
      lstfilterstr2 = filtpopstr2[i].split('_');
      indexid = i + 1;
      document.getElementById('sfa'+indexid).value = lstfilterstr2[0];

      if (lstfilterstr2[1] != '') {
        document.getElementById('sfb'+indexid).value = lstfilterstr2[1];
      }

      if (lstfilterstr2[2] != '') {
        document.getElementById('sfc'+indexid).value = lstfilterstr2[2];
      }
    }

    filtercounter = filtpopstr2.length;
  }
}