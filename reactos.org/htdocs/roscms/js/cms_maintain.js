

var search_phrase = 'search account';
var filtstring1, filtstring2;
var exitmsg='';
var roscms_current_page = 'user';
var roscms_prev_page = 'user';



/**
 * requests user details
 *
 * @param int user_id details to this user are requested
 */
function getUserDetails( user_id )
{
  makeRequest('?page=backend&type=text&subtype=ud&user='+encodeURIComponent(user_id), 'userdetails', 'pageUserDetails', 'text', 'GET');
}



/**
 * add a new membership to a group
 *
 * @param int user_id affected user
 * @param string group_id group, where membership is added to
 */
function addMembership( user_id, group_id )
{
  makeRequest('?page=backend&type=text&subtype=ud&action=addmembership&user='+encodeURIComponent(user_id)+'&group='+encodeURIComponent(group_id), 'userdetails', 'pageUserDetails', 'text', 'GET');
}



/**
 * deletes a membership from a group
 *
 * @param int userid affected user
 * @param string member_id affected group
 */
function delMembership( user_id, member_id )
{
  var uf_check = confirm("Be careful! \n\nDo you want to delete this membership?");

  if (uf_check) {
    makeRequest('?page=backend&type=text&subtype=ud&action=delmembership&user='+encodeURIComponent(user_id)+'&group='+encodeURIComponent(member_id), 'userdetails', 'pageUserDetails', 'text', 'GET');
  }
}



/**
 * updates the users language setting
 *
 * @param int userid affected user
 * @param string membid new language
 */
function updateUserLang( user_id, language )
{
  var uf_check = confirm("Do you want to continue?");

  if (uf_check) {
    makeRequest('?page=backend&type=text&subtype=ud&action=updateusrlang&user='+encodeURIComponent(user_id)+'&lang='+encodeURIComponent(language), 'userdetails', 'pageUserDetails', 'text', 'GET');
  }
}



/**
 * enables or disables useraccounts
 *
 * @param int user_id affected user
 * @param string enable 'enable'/'disable'
 */
function setAccount( user_id, enable )
{
  var uf_check = confirm("Do you want to "+enable+" this membership?");

  if (uf_check) {
    makeRequest('?page=backend&type=text&subtype=ud&action=account'+enable+'&d_val='+encodeURIComponent(user_id), 'userdetails', 'pageUserDetails', 'text', 'GET');
  }
}


/**
 * sends an signal to DB to optimize tables
 */
function optimizeDB( )
{
  makeRequest('?page=backend&type=maintain&action=optimize', 'gen', '', 'text', 'GET');
}



/**
 * generates all pages
 */
function generateAllPages( )
{
  var uf_check = confirm("Do you want to continue?");

  if (uf_check) {
    document.getElementById('maintainarea').style.display = 'block';
    document.getElementById('maintainarea').innerHTML = 'generating all pages, may take some minutes ...';
    makeRequest('?page=backend&type=maintain&action=genall', 'gen', '', 'text', 'GET');
  }
}



/**
 * rebuild the dependency tree
 */
function rebuildDependencies( )
{
  var uf_check = confirm("Do you want to continue?");

  if (uf_check) {
    document.getElementById('maintainarea').style.display = 'block';
    document.getElementById('maintainarea').innerHTML = 'rebuilding dependencies, this may take a while ...';
    makeRequest('?page=backend&type=maintain&action=rebuilddependencies', 'gen', '', 'text', 'GET');
  }
}



/**
 * generates a single page
 */
function generatePage( )
{
  document.getElementById('maintainarea').style.display = 'block';
  makeRequest('?page=backend&type=maintain&action=genone&name='+encodeURIComponent(document.getElementById('textfield').value)+'&data_type='+encodeURIComponent(document.getElementById('txtaddentrytype').value)+'&lang='+encodeURIComponent(document.getElementById('txtaddentrylang').value), 'gen', '', 'text', 'GET');
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
  else if (!clear && objval === '') {
    document.getElementById(objid).value = objhint;
    document.getElementById(objid).style.color = '#999999';
  }
} // end of function searchFilter



/**
 * sets the search filter back to his standard text
 */
function clearSearchFilter( )
{
  document.getElementById('filtersct').innerHTML = '';
  document.getElementById('searchfield').value = '';
  searchFilter('searchfield', document.getElementById('searchfield').value, search_phrase, false);
  filtercounter = 0;
  filterid = 0;
} // end of function clearSearchFilter



/**
 * @FILLME
 */
function highlightMenu( obj )
{
  // remove highlight from other entries
  if (obj != 'User') {
    document.getElementById('lmUser').style.backgroundColor = 'white';
    document.getElementById('lmUser').style.fontWeight = 'normal';
    document.getElementById('pageUser').style.display = 'none';
  }
  if (obj != 'Generate') {
    document.getElementById('lmGenerate').style.backgroundColor = 'white';
    document.getElementById('lmGenerate').style.fontWeight = 'normal';
    document.getElementById('pageGenerate').style.display = 'none';
  }
  if (obj != 'Logs') {
    document.getElementById('lmLogs').style.backgroundColor = 'white';
    document.getElementById('lmLogs').style.fontWeight = 'normal';
    document.getElementById('pageLogs').style.display = 'none';
  }
  if (obj != 'Access') {
    document.getElementById('lmAccess').style.backgroundColor = 'white';
    document.getElementById('lmAccess').style.fontWeight = 'normal';
    document.getElementById('pageAccess').style.display = 'none';
  }
  if (obj != 'Groups') {
    document.getElementById('lmGroups').style.backgroundColor = 'white';
    document.getElementById('lmGroups').style.fontWeight = 'normal';
    document.getElementById('pageGroups').style.display = 'none';
  }
  if (obj != 'Languages') {
    document.getElementById('lmLanguages').style.backgroundColor = 'white';
    document.getElementById('lmLanguages').style.fontWeight = 'normal';
    document.getElementById('pageLanguages').style.display = 'none';
  }
  if (obj != 'System') {
    document.getElementById('lmSystem').style.backgroundColor = 'white';
    document.getElementById('lmSystem').style.fontWeight = 'normal';
    document.getElementById('pageSystem').style.display = 'none';
  }

  // highlight
  document.getElementById('page'+obj).style.display = 'block';
  document.getElementById('lm'+obj).style.backgroundColor = '#C9DAF8';
  document.getElementById('lm'+obj).style.fontWeight = 'bold';

} // end of function highlightMenu



/**
 * wrapper for addUserFilterShared
 *
 * @param string uf_str
 */
function addUserFilter( uf_str )
{
  addUserFilterShared(uf_str, 'userfilter2c', 'usf');
} // end of function addUserFilter



/**
 * wrapper for deleteUserFilterShared
 *
 * @param string uf_id
 * @param string uf_name
 */
function deleteUserFilter( uf_id, uf_name )
{
  deleteUserFilterShared( uf_id, uf_name, 'userfilter2c', 'usf' );
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

  // reset search box:
  filtstring1 = '';
  clearSearchFilter();

  loadUserSearch();

  filtstring2 = ufiltstr;
  htmlFilterChoices(ufiltstr);
} // end of function selectUserFilter



/**
 * @FILLME
 */
function loadUserSearch( )
{
  // reset search box:
  clearSearchFilter();

  // highlight left menu entry
  highlightMenu('User');

  // show table / hide details
  document.getElementById('pageUserTable').style.display = 'block';
  document.getElementById('pageUserDetails').style.display = 'none';

  // standard filter setting
  filtstring2 = 'b_is_name_1|c_is_Real-Name_1|c_is_edits_1|o_desc_edits_1';

  // apply standard filters
  htmlFilterChoices(filtstring2);
  return true;
} // end of function loadUserSearch



/**
 * @FILLME
 */
function loadGenerate( )
{
  // highlight left menu entry
  highlightMenu('Generate');
  return true;
} // end of function loadGenerate




/**
 * @FILLME
 */
function loadAccess( )
{

  // highlight left menu entry
  highlightMenu('Access');
  
  document.getElementById('accessList').style.display = 'block';
  document.getElementById('accessDetails').style.display = 'none';

  // get language list
  makeRequest('?page=backend&type=admin&subtype=acl&action=search', 'apply', 'accessListBody', 'html', 'GET');
  return true;
} // end of function loadGenerate



/**
 * @FILLME
 */
function addAccess( )
{
  // highlight left menu entry
  highlightMenu('Access');
  
  document.getElementById('accessList').style.display = 'none';
  document.getElementById('accessDetails').style.display = 'block';

  // get language list
  makeRequest('?page=backend&type=admin&subtype=acl&action=new', 'apply', 'accessDetails', 'html', 'GET');
  return true;
} // end of function loadGenerate



/**
 * @FILLME
 */
function submitAccessAdd( )
{
  makeRequest('?page=backend&type=admin&subtype=acl&action=new&submit=true', 'apply', 'accessDetails', 'html', 'POST');

  loadAccess();
}



/**
 * @FILLME
 */
function editAccess( id )
{
  // highlight left menu entry
  highlightMenu('Access');
  
  document.getElementById('accessList').style.display = 'none';
  document.getElementById('accessDetails').style.display = 'block';

  // get language list
  makeRequest('?page=backend&type=admin&subtype=acl&action=edit&access='+id, 'apply', 'accessDetails', 'html', 'GET');
  return true;
} // end of function loadGenerate



/**
 * @FILLME
 */
function submitAccessEdit( )
{
  makeRequest('?page=backend&type=admin&subtype=acl&action=edit&submit=true', 'apply', 'accessDetails', 'html', 'POST');
  
  loadAccess();
}



/**
 * @FILLME
 */
function loadSystem( )
{

  // highlight left menu entry
  highlightMenu('System');

  // get language list
  makeRequest('?page=backend&type=admin&subtype=system&action=apl', 'apply', 'pageSystem', 'html', 'GET');
  return true;
} // end of function loadGenerate



/**
 * @FILLME
 */
function submitSystemEdit( )
{
  makeRequest('?page=backend&type=admin&subtype=system&action=apl&submit=true', 'apply', 'pageSystem', 'html', 'POST');

  loadSystem();
}



/**
 * @FILLME
 */
function loadGroups( )
{

  // highlight left menu entry
  highlightMenu('Groups');
  
  document.getElementById('groupList').style.display = 'block';
  document.getElementById('groupDetails').style.display = 'none';

  // get language list
  makeRequest('?page=backend&type=admin&subtype=group&action=search', 'apply', 'groupListBody', 'html', 'GET');
  return true;
} // end of function loadGenerate



/**
 * @FILLME
 */
function addGroup( )
{
  // highlight left menu entry
  highlightMenu('Groups');
  
  document.getElementById('groupList').style.display = 'none';
  document.getElementById('groupDetails').style.display = 'block';

  // get language list
  makeRequest('?page=backend&type=admin&subtype=group&action=new', 'apply', 'groupDetails', 'html', 'GET');
  return true;
} // end of function loadGenerate



/**
 * @FILLME
 */
function submitGroupAdd( )
{
  makeRequest('?page=backend&type=admin&subtype=group&action=new&submit=true', 'apply', 'groupDetails', 'html', 'POST');

  loadGroups();
}



/**
 * @FILLME
 */
function editGroup( id )
{
  // highlight left menu entry
  highlightMenu('Groups');
  
  document.getElementById('groupList').style.display = 'none';
  document.getElementById('groupDetails').style.display = 'block';

  // get language list
  makeRequest('?page=backend&type=admin&subtype=group&action=edit&group='+id, 'apply', 'groupDetails', 'html', 'GET');
  return true;
} // end of function loadGenerate



/**
 * @FILLME
 */
function submitGroupEdit( )
{
  makeRequest('?page=backend&type=admin&subtype=group&action=edit&submit=true', 'apply', 'groupDetails', 'html', 'POST');
  
  loadGroups();
}



/**
 * @FILLME
 */
function loadLanguages( )
{
  // highlight left menu entry
  highlightMenu('Languages');
  
  document.getElementById('languageList').style.display = 'block';
  document.getElementById('languageDetails').style.display = 'none';

  // get language list
  makeRequest('?page=backend&type=admin&subtype=lang&action=search', 'apply', 'languageListBody', 'html', 'GET');
  return true;
} // end of function loadGenerate



/**
 * @FILLME
 */
function addLanguage( )
{
  // highlight left menu entry
  highlightMenu('Languages');
  
  document.getElementById('languageList').style.display = 'none';
  document.getElementById('languageDetails').style.display = 'block';

  // get language list
  makeRequest('?page=backend&type=admin&subtype=lang&action=new', 'apply', 'languageDetails', 'html', 'GET');
  return true;
} // end of function loadGenerate



/**
 * @FILLME
 */
function submitLanguageAdd( )
{
  makeRequest('?page=backend&type=admin&subtype=lang&action=new&submit=true', 'apply', 'languageDetails', 'html', 'POST');

  loadLanguages();
}



/**
 * @FILLME
 */
function editLanguage( id )
{
  // highlight left menu entry
  highlightMenu('Languages');
  
  document.getElementById('languageList').style.display = 'none';
  document.getElementById('languageDetails').style.display = 'block';

  // get language list
  makeRequest('?page=backend&type=admin&subtype=lang&action=edit&lang='+id, 'apply', 'languageDetails', 'html', 'GET');
  return true;
} // end of function loadGenerate



/**
 * @FILLME
 */
function submitLanguageEdit( )
{
  makeRequest('?page=backend&type=admin&subtype=lang&action=edit&submit=true', 'apply', 'languageDetails', 'html', 'POST');
  
  loadLanguages();
}




function getFormData( )
{
  var postdata = '';
  var value;

  for(var i = 0; i < document.forms[0].elements.length; i++) {
    if (postdata !== '') {
      postdata += "&";
    }

    switch (document.forms[0].elements[i].type) {
      case 'text':
      case 'hidden':
      case 'select-one':
        value = document.forms[0].elements[i].value;
        break;
      case 'checkbox':
        value = document.forms[0].elements[i].checked ? 'true' : 'false';
        break;
      default:
        value = undefined;
        break;
    }

    if (value !== undefined) {
      postdata = postdata + document.forms[0].elements[i].name+"="+value;
    }

  }
  
  return postdata;
}



/**
 * @FILLME
 */
function loadLogs( )
{
  // highlight left menu entry
  highlightMenu('Logs');
  return true;
} // end of function loadLogs



/**
 * performs a new search
 */
function getUser( )
{
  var soptckd = '';

  // get filter
  getActiveFilters();
  
  if (document.getElementById('searchfield').value != search_phrase) {
    filtstring1 = document.getElementById('searchfield').value;
  }
  else {
    filtstring1 = '';
  }

  // send request
  makeRequest('?page=backend&type=xml&subtype=ust&d_filter='+filtstring1+'&d_filter2='+filtstring2+'&d_cp=0', 'user', 'usertable', 'xml', 'GET');
} // end of function getUser



/**
 * @FILLME
 */
function loadUser( id )
{
  var user = document.getElementById(id).getElementsByTagName('td')[0].className;


  // send request
  makeRequest('?page=backend&type=user&user='+user, 'userdetails', 'pageUserDetails', 'text', 'GET');

  document.getElementById('pageUserTable').style.display = 'none';
  document.getElementById('pageUserDetails').style.display = 'block';
} // end of function getUser



/**
 * resets all filters
 */
function clearAllFilter( )
{
  clearSearchFilter();

  filtstring1 = '';
  filtstring2 = '';
  getUser();
} // end of function clearAllFilter



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
          + '<option value="b">Search Criteria</option>'
          + '<option value="l">Language</option>'
          + '<option value="g">Group</option>'
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
    + '<thead><tr class="head">'
    + '<th scope="col" class="cCid sortable" onclick="sortEntryTable(this, \'name\');">Name</th>';


  if (xtrtblcol !== '' && xtrtblcols2.length > 1) {
    for (var i=1; i < xtrtblcols2.length-1; i++) {
      lstHeader += '<th scope="col" class="cExtraCol sortable" onclick="sortEntryTable(this, \''+xtrtblcols2[i].toLowerCase()+'\');">'+xtrtblcols2[i]+'</th>';
    }
  }

  lstHeader += '</tr></thead><tbody>';

  return lstHeader;
} // end of function htmlEntryTableHeader



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


  xtrtblcols2 = xtrtblcol.split('|');

  lstBody = '<tr class="'+bclass+'" id="tru'+(bnr+1)+'">'
    + '<td class="'+brusrid+'">'+bdname+'</td>';

  if (xtrtblcol !== '' && xtrtblcols2.length > 1) {
    for (var i=1; i < xtrtblcols2.length-1; i++) {
      lstBody += '<td>'+xtrtblcols2[i]+'</td>';
    }
  }

  lstBody += '</tr>';

  return lstBody;
} // end of function htmlEntryTableRow



/**@CLEAR
 * table mouse events
 */
function registerMouseActions( )
{
  if (!document.getElementById) {
    return;
  }


  // Shows Tooltip window for a specific time, and then hides it again 
  function localStartActive()
  {
    hlRow(this.id,1);
  } // end of inner function localStartActive 


  //sets a timeout to remove Tooltip
  function localStopActive() {
    hlRow(this.id,2);
  } // end of inner function localStopActive


  function localEditUser() {
    loadUser(this.id);
  } // end of inner function localStartEditor


  //row highlighting
  for (var i=1; i<=nres; i++) {

    document.getElementById("tru"+i).onmouseover = localStartActive;
    document.getElementById("tru"+i).onmouseout = localStopActive;
    document.getElementById("tru"+i).onclick = localEditUser;

  }
} // end of function registerMouseActions



/**@CLEAR
 * Reloads the entry table with an offset
 *
 * @param int offset
 */
function reloadEntryTableWithOffset( offset )
{
  // deactivate alert-timer
  alertboxClose(0);

  makeRequest('?page=backend&type=xml&subtype=ust&d_fl='+roscms_current_page+'&d_filter='+filtstring1+'&d_filter2='+filtstring2+'&d_cp='+offset, 'user', 'usertable', 'xml', 'GET');
} // end of function reloadEntryTableWithOffset



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
function makeRequest( url, action, objid, format, kind )
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

  http_request.onreadystatechange = function()
  {
    try {
      if (http_request.readyState === 4) {
        if (http_request.status === 200) {
          loadingSplash(false); 

          switch (action) {

            // user search
            case 'user': 
              buildEntryTable(http_request, objid);
              break;

            case 'userdetails': 
              document.getElementById(objid).innerHTML = http_request.responseText;
              break;

            // generate
            case 'gen': 
              document.getElementById('maintainarea').innerHTML = http_request.responseText;
              break;

            // logs
            case 'logs': 
              entryTableActionPerformed(http_request, objid);
              break;

            // user search filter
            case 'usf': 
              document.getElementById(objid).innerHTML = http_request.responseText;
              break;

            // just apply content to the given objid
            case 'apply': 
              document.getElementById(objid).innerHTML = http_request.responseText;
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
        alertbox('RosCMS caught an exception to prevent data loss. If you see this message several times, please make sure you use an up-to-date browser client. If the issue still occur, tell the website admin the following information:<br />Name: '+ e.name +'; Number: '+ e.number +'; Message: '+ e.message +'; Description: '+ e.description+'; Object:'+objid);
      }
    }

  }; // internal function end

  if (kind === 'POST') {
    var parameters = getFormData();

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
} // end of function makeRequest
