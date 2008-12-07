<?php include('../config.php'); ?>



/**
 * sends an signal to DB to optimize tables
 */
function optimizeDB( )
{
  makeRequest('?page=data_out&d_f=maintain&d_u=optimize');
}



/**
 * generates all pages
 */
function generateAllPages( )
{
  var uf_check = confirm("Do you want to continue?");

  if (uf_check == true) {
    document.getElementById('maintainarea').style.display = 'block';
    document.getElementById('maintainarea').innerHTML = 'generating all pages, may take several seconds ...';
    makeRequest('?page=data_out&d_f=maintain&d_u=genpages');
  }
}



/**
 * opens a second window with a preview of the selected page
 */
function previewPage( )
{
  window.open("<?php echo RosCMS::getInstance()->pathRosCMS(); ?>?page=data_out&d_f=page&d_u=show&d_val=index&d_val2=en", "RosCMSPagePreview");
}



/**
 * generates a single page
 */
function generatePage( )
{
  document.getElementById('maintainarea').style.display = 'block';
  makeRequest('?page=data_out&d_f=maintain&d_u=pupdate&d_val='+encodeURIComponent(document.getElementById('textfield').value)+'&d_val2='+encodeURIComponent(document.getElementById('txtaddentrytype').value)+'&d_val3='+encodeURIComponent(document.getElementById('txtaddentrylang').value)+'&d_val4='+encodeURIComponent(document.getElementById('dynnbr').value));
}



/**
 * make a new AJAX request
 *
 * @param string url requested url
 * @return bool
 */
function makeRequest( url )
{
  var http_request = false;

  document.getElementById('ajaxloading').style.display = 'block';
  document.getElementById('maintainarea').innerHTML = '';

  if (window.XMLHttpRequest) { // Mozilla, Safari,...
    http_request = new XMLHttpRequest();
  }
  else if (window.ActiveXObject) { // IE
    try {
      http_request = new ActiveXObject("Msxml2.XMLHTTP");
    } catch (e) {
      try {
        http_request = new ActiveXObject("Microsoft.XMLHTTP");
      } catch (e) {}
    }
  }

  // stop if browser doesn't support AJAX
  if (!http_request) { 
    alert('Cannot create an XMLHTTP instance. \nMake sure that your browser does support AJAX. \nMake sure that your browser does support AJAX. \nTry out IE 5.5+ (with ActiveX enabled), IE7+, Mozilla, Opera 9+ or Safari 3+.');
    return false;
  }

  // send out message
  if (http_request.overrideMimeType) {
    http_request.overrideMimeType('text/html');
  }
  http_request.onreadystatechange = function() { alertContents(http_request); };
  http_request.open('GET', url, true);
  http_request.setRequestHeader("If-Modified-Since", "Sat, 1 Jan 2000 00:00:00 GMT");	// Bypass the IE Cache
  http_request.send(null);
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
        document.getElementById('ajaxloading').style.display = 'none';
        document.getElementById('maintainarea').innerHTML = http_request.responseText;
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
