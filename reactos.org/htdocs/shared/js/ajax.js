/*
  PROJECT:    ReactOS Shared Website Components
  LICENSE:    GNU GPLv2 or any later version as published by the Free Software Foundation
  PURPOSE:    Some easy-to-use AJAX functions
  COPYRIGHT:  Copyright 2008-2009 Colin Finck <colin@reactos.org>
*/

/* This hint comes originally from: http://design-noir.de/webdev/JS/XMLHttpRequest-IE/ */
/*@cc_on @if (@_win32 && @_jscript_version >= 5) if (!window.XMLHttpRequest)
function XMLHttpRequest() { return new ActiveXObject('Microsoft.XMLHTTP') }
@end @*/

function PrepareParameters(data)
{
	var parameters = "";
	
	for(var elem in data)
		parameters += elem + "=" + encodeURIComponent(data[elem]) + "&";
	
	return parameters;
}

function AjaxGet(url, callback, data)
{
	HttpRequest = new XMLHttpRequest();

	if(!HttpRequest)
	{
		alert("Cannot create an XMLHTTP instance");
		return false;
	}
	
	var parameters = PrepareParameters(data);
	
	HttpRequest.open("GET", url + "?" + parameters, true);
	HttpRequest.setRequestHeader("If-Modified-Since", "Sat, 1 Jan 2000 00:00:00 GMT");			// Bypass the IE Cache
	HttpRequest.send(null);
	
	HttpRequest.onreadystatechange = function()
	{
		// Check for a HTTP error
		if(HttpRequest.readyState == 4)
		{
			if(HttpRequest.status == 200)
				eval(callback + "(HttpRequest, data)");
			else
				alert("AJAX Request problem!" + "\n\nError Code: " + HttpRequest.status + "\nError Text: " + HttpRequest.statusText + "\nURL: " + url);
		}
	}
}
