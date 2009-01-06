


/**
 * requests a form for a new entry specified by subject
 * the subject is interpreted in index.php
 *
 * @param string subject 'acl','group' or 'lang'
 */ 
function showNew( subject )
{
  makeRequest('?page=data_out&d_f=admin&d_u=' + subject + '&action=new', 'GET');
}



/**
 * @FILLME
 */
function submitNew( subject )
{
  makeRequest('?page=data_out&d_f=admin&d_u=' + subject + '&action=new&submit=true', 'POST');
  return false;
}



/**
 * @FILLME
 *
 * @param string subject 'acl','group' or 'lang'
 * @param string type 'edit' or 'delete'
 */ 
function showSearch( subject, type )
{
  makeRequest('?page=data_out&d_f=admin&d_u=' + subject + '&action=search&for='+type, 'GET');
}



/**
 * @FILLME
 *
 * @param string subject 'acl','group' or 'lang'
 * @param string type 'edit' or 'delete'
 */ 
function submitSearch( subject, type )
{
  makeRequest('?page=data_out&d_f=admin&d_u=' + subject + '&action=search&submit=true&for='+type, 'POST');
}


/**
 * @FILLME
 *
 * @param string subject 'acl','group' or 'lang'
 */ 
function showEdit( subject )
{
  makeRequest('?page=data_out&d_f=admin&d_u=' + subject + '&action=edit', 'GET');
}



/**
 * @FILLME
 */
function submitEdit( subject )
{
  makeRequest('?page=data_out&d_f=admin&d_u=' + subject + '&action=edit&submit=true', 'POST');
  return false;
}



/**
 * @FILLME
 */
function submitDelete( subject )
{
  var uf = confirm("Do you really want delete this entry ?");
  if (uf==true) {
    makeRequest('?page=data_out&d_f=admin&d_u=' + subject + '&action=delete&submit=true', 'POST');
  }
  return false;
}




function getFormData( )
{
  var postdata = '';
  var value;

  for(var i = 0; i < document.forms[0].elements.length; i++) {
    if (postdata != '') postdata += "&";

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

    if (value != undefined) {
      postdata = postdata + document.forms[0].elements[i].name+"="+value;
    }

  }
  
  return postdata;
}



/**
 * starts a new AJAX request
 * if kind is 'POST' the form data will automatically used as params
 *
 * @param string url
 * @param string kind
 */
function makeRequest( url, kind )
{
  var http_request = false;

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
  if (http_request.overrideMimeType) {
    http_request.overrideMimeType('text/html');
  }

  http_request.onreadystatechange = function() { alertContents(http_request); };

  if (kind == 'POST') {
  
    // put form data as params
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
}



/**
 * inserts request results
 *
 * @param object reference to AJAX-Object
 */
function alertContents( http_request )
{
  try {
    if (http_request.readyState == 4) {
      if (http_request.status == 200) {
        document.getElementById('ajaxloadinginfo').style.display = 'none';
        document.getElementById('adminarea').innerHTML = http_request.responseText;
      }
      else {
        alert('There was a problem with the request ['+http_request.status+' / '+http_request.readyState+']. \n\nA client (browser) or server problem. Please check and try to update your browser. \n\nIf this error happens more than once or twice, contac the website admin.');
      }
    }
  }
  catch( e ) {
    alert('Caught Exception: ' + e.description +'\n\nIf this error occur more than once or twice, please contact the website admin with the exact error message. \n\nIf you use the Safari browser, please make sure you run the latest version.');
  }

  // to prevent memory leak
  http_request = null;
}