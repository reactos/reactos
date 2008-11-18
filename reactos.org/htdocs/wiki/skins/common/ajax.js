// remote scripting library
// (c) copyright 2005 modernmethod, inc
var sajax_debug_mode = false;
var sajax_request_type = "GET";

/**
* if sajax_debug_mode is true, this function outputs given the message into 
* the element with id = sajax_debug; if no such element exists in the document, 
* it is injected.
*/
function sajax_debug(text) {
	if (!sajax_debug_mode) return false;

	var e= document.getElementById('sajax_debug');

	if (!e) {
		e= document.createElement("p");
		e.className= 'sajax_debug';
		e.id= 'sajax_debug';

		var b= document.getElementsByTagName("body")[0];

		if (b.firstChild) b.insertBefore(e, b.firstChild);
		else b.appendChild(e);
	}

	var m= document.createElement("div");
	m.appendChild( document.createTextNode( text ) );

	e.appendChild( m );

	return true;
}

/**
* compatibility wrapper for creating a new XMLHttpRequest object.
*/
function sajax_init_object() {
	sajax_debug("sajax_init_object() called..")
	var A;
	try {
		// Try the new style before ActiveX so we don't
		// unnecessarily trigger warnings in IE 7 when
		// set to prompt about ActiveX usage
		A = new XMLHttpRequest();
	} catch (e) {
		try {
			A=new ActiveXObject("Msxml2.XMLHTTP");
		} catch (e) {
			try {
				A=new ActiveXObject("Microsoft.XMLHTTP");
			} catch (oc) {
				A=null;
			}
		}
	}
	if (!A)
		sajax_debug("Could not create connection object.");

	return A;
}

/**
* Perform an ajax call to mediawiki. Calls are handeled by AjaxDispatcher.php
*   func_name - the name of the function to call. Must be registered in $wgAjaxExportList
*   args - an array of arguments to that function
*   target - the target that will handle the result of the call. If this is a function,
*            if will be called with the XMLHttpRequest as a parameter; if it's an input
*            element, its value will be set to the resultText; if it's another type of
*            element, its innerHTML will be set to the resultText.
*
* Example:
*    sajax_do_call('doFoo', [1, 2, 3], document.getElementById("showFoo"));
*
* This will call the doFoo function via MediaWiki's AjaxDispatcher, with
* (1, 2, 3) as the parameter list, and will show the result in the element
* with id = showFoo
*/
function sajax_do_call(func_name, args, target) {
	var i, x, n;
	var uri;
	var post_data;
	uri = wgServer +
		((wgScript == null) ? (wgScriptPath + "/index.php") : wgScript) +
		"?action=ajax";
	if (sajax_request_type == "GET") {
		if (uri.indexOf("?") == -1)
			uri = uri + "?rs=" + encodeURIComponent(func_name);
		else
			uri = uri + "&rs=" + encodeURIComponent(func_name);
		for (i = 0; i < args.length; i++)
			uri = uri + "&rsargs[]=" + encodeURIComponent(args[i]);
		//uri = uri + "&rsrnd=" + new Date().getTime();
		post_data = null;
	} else {
		post_data = "rs=" + encodeURIComponent(func_name);
		for (i = 0; i < args.length; i++)
			post_data = post_data + "&rsargs[]=" + encodeURIComponent(args[i]);
	}
	x = sajax_init_object();
	if (!x) {
		alert("AJAX not supported");
		return false;
	}

	try {
		x.open(sajax_request_type, uri, true);
	} catch (e) {
		if (window.location.hostname == "localhost") {
			alert("Your browser blocks XMLHttpRequest to 'localhost', try using a real hostname for development/testing.");
		}
		throw e;
	}
	if (sajax_request_type == "POST") {
		x.setRequestHeader("Method", "POST " + uri + " HTTP/1.1");
		x.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
	}
	x.setRequestHeader("Pragma", "cache=yes");
	x.setRequestHeader("Cache-Control", "no-transform");
	x.onreadystatechange = function() {
		if (x.readyState != 4)
			return;

		sajax_debug("received (" + x.status + " " + x.statusText + ") " + x.responseText);

		//if (x.status != 200)
		//	alert("Error: " + x.status + " " + x.statusText + ": " + x.responseText);
		//else

		if ( typeof( target ) == 'function' ) {
			target( x );
		}
		else if ( typeof( target ) == 'object' ) {
			if ( target.tagName == 'INPUT' ) {
				if (x.status == 200) target.value= x.responseText;
				//else alert("Error: " + x.status + " " + x.statusText + " (" + x.responseText + ")");
			}
			else {
				if (x.status == 200) target.innerHTML = x.responseText;
				else target.innerHTML= "<div class='error'>Error: " + x.status + " " + x.statusText + " (" + x.responseText + ")</div>";
			}
		}
		else {
			alert("bad target for sajax_do_call: not a function or object: " + target);
		}

		return;
	}

	sajax_debug(func_name + " uri = " + uri + " / post = " + post_data);
	x.send(post_data);
	sajax_debug(func_name + " waiting..");
	delete x;

	return true;
}
