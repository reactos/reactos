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

var alertactiv = '';
var filtercounter = 0;
var filterid = 0;
var roscms_page_load_finished = false;
var roscms_intern_entry_per_page = 25;
var roscms_current_tbl_position = 0;
var nres=0;

var filtstring1 = '';
var filtstring2 = '';



/**@CLEAR
 * load another CMS branch
 *
 * @param branch CMS branch to be loaded
 */
function loadBranch( )
{
  exitmsg = '';
} // end of function loadBranch



/**@CLEAR
 * clear search text box
 */
function clearSearchBox( id )
{
  if (id.value == search_phrase) {
    id.value = '';
  }
  else if (id.value === '') {
    id.value = search_phrase;
  }
} // end of function clearSearchBox



/**@CLEAR
 * colors (background) a row in a given color
 *
 * @param int num number of the row
 * @param string color new color of the row
 */
function setRowColor( id, color )
{
  // check for needed internal functions
  if (!document.getElementById || !document.getElementsByTagName || !document.getElementById(id)) {
    return;
  }

  // select row
  var cell_arr = document.getElementById(id).getElementsByTagName('td');

  // set background color for columns
  for (var i=0; i<cell_arr.length; i++) {
    cell_arr[i].style.backgroundColor = color;
  }
} // end of function setRowColor



/**@CLEAR
 * colors a rows background by status
 */
function setRowColorByStatus( id, status )
{
  if (status === 'odd') {
    setRowColor(id,"#dddddd");
  }
  else if (status === 'even') {
    setRowColor(id,"#eeeeee");
  }
  else if (status === 'new') {
    setRowColor(id,"#B5EDA3");
  }
  else if (status === 'draft') {
    setRowColor(id,"#FFE4C1");
  }
  else if (status === 'transg') {
    setRowColor(id,"#A3EDB4");
  }
  else if (status === 'transb') {
    setRowColor(id,"#D6CAE4");
  }
  else if (status === 'transr') {
    setRowColor(id,"#FAA5A5");
  }
  else {
    setRowColor(id,"#FFCCFF");
  }
} // end of function setRowColorByStatus



/**@CLEAR
 * highlights a row
 *
 * @param int num number of the row
 * @param int hlmode highlight mode (mouseover, mouseout, mouseclick)
 */
function hlRow( id, hlmode )
{
  var rowstatus = document.getElementById(id).className;

  switch (hlmode) {

    //on mouseover
    case 1:
      setRowColor(id,"#ffffcc");
      break;

    //on mouseout
    case 2:
      if (document.getElementById("cb"+id) && document.getElementById("cb"+id).checked) {
        setRowColor(id,"#ffcc99");
      }
      else {
        setRowColorByStatus(id, rowstatus);
      }
      break;

    //on click
    case 3:
      if (!document.getElementById("cb"+id).checked) {
        setRowColor(id,"#ffcc99");
      }
      else {
        setRowColorByStatus(id, rowstatus);
      }
      break;
  } // end switch
} // end of function hlRow


/**@CLEAR
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
      window.clearTimeout(alertactiv); 
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
} // end of function alertboxClose



/**@CLEAR
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
} // end of function alertbox



/**@CLEAR
 * stores a new cookie
 *
 * @param string name cookie name
 * @param string value cookie value
 * @param int days how many days the cookie is stored
 */
function createCookie( name, value, days )
{
  var expires = "";

  if (days) {
    var date = new Date();
    date.setTime(date.getTime()+(days*24*60*60*1000));
    expires = "; expires="+date.toGMTString();
  }
  
  document.cookie = name+"="+value+expires+"; path=/";
} // end of function createCookie



/**@CLEAR
 * gets the value of a cookie
 *
 * @param string name cookie to access
 */
function readCookie( name )
{
  var nameEQ = name + "=";
  var ca = document.cookie.split(';');
  var c;

  for(var i=0;i < ca.length;i++) {
    c = ca[i];
    while (c.charAt(0)==' ') {
      c = c.substring(1,c.length);
    }
    if (c.indexOf(nameEQ) === 0) {
      return c.substring(nameEQ.length,c.length);
    }
  }
  return null;
} // end of function readCookie



/**@CLEAR
 * Saves smart filter setting or user label
 *
 * @param string uf_str
 */
function addUserFilterShared( uf_str, objid, type )
{
  var uf_name = window.prompt("Input a new Smart Filter name:", "");

  if (uf_name !== '' && uf_name.length < 50) {
    makeRequest('?page=backend&type=text&subtype='+type+'&action=add&title='+encodeURIComponent(uf_name)+'&setting='+encodeURIComponent(uf_str), type, objid, 'html', 'GET', '');
  }
} // end of function addUserFilterShared



/**@CLEAR
 * Deletes smart filter setting or user label
 *
 * @param string uf_id
 * @param string uf_type 'label'/'filter'
 * @param string uf_name
 */
function deleteUserFilterShared( uf_id, uf_name, objid, type )
{
  var uf_check = confirm("Do you want to delete Smart Filter '"+uf_name+"' ?");

  if (uf_check === true) {
    makeRequest('?page=backend&type=text&subtype='+type+'&action=del&id='+encodeURIComponent(uf_id), type, objid, 'html', 'GET', '');
  }
} // end of function deleteUserFilterShared



/**@CLEAR
 * toggles arrow right/bottom
 *
 * @param string objid affected object
 */
function TabOpenClose( objid )
{
  if (document.getElementById(objid +'c').style.display === 'none') {
    document.getElementById(objid +'c').style.display = 'block';
    document.getElementById(objid +'i').src = roscms_intern_webserver_roscms+'images/tab_open.gif';
  }
  else {
    document.getElementById(objid +'c').style.display = 'none';
    document.getElementById(objid +'i').src = roscms_intern_webserver_roscms+'images/tab_closed.gif';
  }
} // end of function TabOpenClose



/**@CLEAR
 * toggles arrow right/bottom and saves the status in a cookie
 *
 * @param string objid affected object
 */
function TabOpenCloseEx( objid )
{
  // save status
  if (document.getElementById(objid +'c').style.display === 'none') {
    createCookie(objid,'1',365); // 365 days
  }
  else {
    createCookie(objid,'0',365); // 365 days
  }

  // toggle
  TabOpenClose(objid);
} // end of function TabOpenCloseEx



/**@CLEAR
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
} // end of function loadingSplash



/**@CLEAR
 * strips invalid characters out
 *
 * @param string str
 */
function beautifystr2( str )
{
  // remove invalid characters
  str = str.replace(/\|/g, '');
  str = str.replace(/\=/g, '');
  str = str.replace(/&/g, '');
  return str;
} // end of function beautifystr2



/**@CLEAR
 * strips invalid characters out
 *
 * @param string str
 */
function beautifystr( str )
{
  // remove invalid characters
  str = beautifystr2(str);
  str = str.replace(/_/g, '');
  return str;
} // end of function beautifystr



/**@CLEAR
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
      if (document.getElementById('sfv'+i).id && document.getElementById('sfv'+i).className === "filthidden") {
        filtstring2 += '_0';
      }
      else {
        filtstring2 += '_1';
      }

      filtstring2 += '|';
    }
  } // end for

  // remove last '|'
  filtstring2 = filtstring2.substr(0,filtstring2.length-1);
} // end of function getActiveFilters



/**@CLEAR
 * get active filters
 */
function sortEntryTable( header, by )
{
  var direction;
  var setting;
  var check;

  // get active settings
  getActiveFilters();
  setting = filtstring2;

  // detect direction of new sort (invert it, if already was set)
  check = new RegExp('o_(asc|desc)_'+by+'_[01]', 'g');
  matches = check.exec(setting);

  if (matches === null || matches[1] != 'asc') {
    direction = 'asc';
  }
  else {
    direction = 'desc';
  }

  // remove old sorting
  setting = setting.replace(/(^|\|)o_(asc|desc)_[A-Za-z0-9]+_[01](\||$)/g, '');

  // add new sorting
  if (setting !== '') {
    setting += '|';
  }
  setting += 'o_'+direction+'_'+by+'_1';

  // apply new setting
  if (setting != filtstring2) {
    filtstring2 = setting;
    htmlFilterChoices(setting);
    loadEntryTable();
  }
} // end of function sortEntryTable



/**@CLEAR
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



/**@CLEAR
 * adds a new filter to the actual list
 */
function addFilter( )
{
  var new_filter = 'c_is_';

  // fill filtstring2 with content, if available
  getActiveFilters();

  if (filtstring2 === '') {
    htmlFilterChoices(new_filter);
  }
  else {
    htmlFilterChoices(filtstring2+'|'+new_filter);
  }
} // end of function addFilter



/**@CLEAR
 * removes an filter from current setting
 *
 * @param string objid
 */
function removeFilter( objid )
{
  document.getElementById('filt'+objid.substr(4)).style.display = 'none';
  document.getElementById('filt'+objid.substr(4)).innerHTML = '';
} // end of function removeFilter



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
    document.getElementById('mtblnav').innerHTML = '&nbsp;';
    document.getElementById('mtbl2nav').innerHTML = '&nbsp;';
    lstData = '<div class="tableswhitespace"><strong>No results, due an error in the filter settings or the data metadata.</strong><br /><br />If this happens more than a few times, please contact the website admin with the following information:<br />Name: '+ e.name +'<br />Number: '+ e.number +'<br />Message: '+ e.message +'<br />ObjID: '+ objid +'<br />Request: <pre>'+ http_request +'</pre></div>';
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
      mtblnavstr += '<span class="virtualLink" onclick="reloadEntryTableWithOffset(0)"><strong>&laquo;</strong></span>&nbsp;&nbsp;';
    }

    // table navigation previous page
    if (xview[0].getAttributeNode("curpos").value > 0) {
      mtblnavstr += '<span class="virtualLink" onclick="reloadEntryTableWithOffset(';
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
      mtblnavstr += '&nbsp;&nbsp;<span class="virtualLink" onclick="reloadEntryTableWithOffset(';
      mtblnavstr += xview[0].getAttributeNode("curpos").value*1+roscms_intern_entry_per_page*1;
      mtblnavstr += ')"><b>Next &rsaquo;</b></span>';
    }

    if (xview[0].getAttributeNode("curpos").value < (xview[0].getAttributeNode("pagemax").value*1-roscms_intern_entry_per_page*2)) {
      mtblnavstr += '&nbsp;&nbsp;<span class="virtualLink" onclick="reloadEntryTableWithOffset(';
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
      lstData += '<div class="tableswhitespace">&nbsp;</div>';
    }

    nres = (temp_counter_loop+1);
    window.setTimeout("registerMouseActions()", 100);
  }
  else {
    nres = 0;
    document.getElementById('mtblnav').innerHTML = '&nbsp;';
    document.getElementById('mtbl2nav').innerHTML = '&nbsp;';
    lstData = '<div class="tableswhitespace"><strong>No results.</strong></div>';
  }

  // update table
  document.getElementById(objid).innerHTML = lstData;
} // end of function buildEntryTable
