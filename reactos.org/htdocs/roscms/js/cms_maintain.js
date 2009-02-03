


/**
 * sends an signal to DB to optimize tables
 */
function optimizeDB( )
{
  makeRequest('?page=backend&type=maintain&action=optimize');
}



/**
 * generates all pages
 */
function generateAllPages( )
{
  var uf_check = confirm("Do you want to continue?");

  if (uf_check === true) {
    document.getElementById('maintainarea').style.display = 'block';
    document.getElementById('maintainarea').innerHTML = 'generating all pages, may take several seconds ...';
    makeRequest('?page=backend&type=maintain&action=genall');
  }
}



/**
 * rebuild the depency tree
 */
function rebuildDepencies( )
{
  var uf_check = confirm("Do you want to continue?");

  if (uf_check === true) {
    document.getElementById('maintainarea').style.display = 'block';
    document.getElementById('maintainarea').innerHTML = 'rebuilding depencies, this may take a while ...';
    makeRequest('?page=backend&type=maintain&action=rebuilddepencies');
  }
}



/**
 * generates a single page
 */
function generatePage( )
{
  document.getElementById('maintainarea').style.display = 'block';
  makeRequest('?page=backend&type=maintain&action=genone&name='+encodeURIComponent(document.getElementById('textfield').value)+'&data_type='+encodeURIComponent(document.getElementById('txtaddentrytype').value)+'&lang='+encodeURIComponent(document.getElementById('txtaddentrylang').value));
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
      } catch (e2) {}
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

  // inserts request results
  http_request.onreadystatechange = function()
  {
    try {
      if (http_request.readyState === 4) {
        if (http_request.status === 200) {
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
  };
  http_request.open('GET', url, true);
  http_request.setRequestHeader("If-Modified-Since", "Sat, 1 Jan 2000 00:00:00 GMT");	// Bypass the IE Cache
  http_request.send(null);
  return true;
}
