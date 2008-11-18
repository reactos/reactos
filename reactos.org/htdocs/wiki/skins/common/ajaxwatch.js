// dependencies:
// * ajax.js:
  /*extern sajax_init_object, sajax_do_call */
// * wikibits.js:
  /*extern changeText, akeytt, hookEvent, jsMsg */

// These should have been initialized in the generated js
/*extern wgAjaxWatch, wgPageName */

if(typeof wgAjaxWatch === "undefined" || !wgAjaxWatch) {
	var wgAjaxWatch = {
		watchMsg: "Watch",
		unwatchMsg: "Unwatch",
		watchingMsg: "Watching...",
		unwatchingMsg: "Unwatching..."
	};
}

wgAjaxWatch.supported = true; // supported on current page and by browser
wgAjaxWatch.watching = false; // currently watching page
wgAjaxWatch.inprogress = false; // ajax request in progress
wgAjaxWatch.timeoutID = null; // see wgAjaxWatch.ajaxCall
wgAjaxWatch.watchLinks = []; // "watch"/"unwatch" links

wgAjaxWatch.setLinkText = function(newText) {
	for (i = 0; i < wgAjaxWatch.watchLinks.length; i++) {
		changeText(wgAjaxWatch.watchLinks[i], newText);
	}
};

wgAjaxWatch.setLinkID = function(newId) {
	// We can only set the first one
	wgAjaxWatch.watchLinks[0].setAttribute( 'id', newId );
	akeytt(newId); // update tooltips for Monobook
};

wgAjaxWatch.setHref = function( string ) {
	for( i = 0; i < wgAjaxWatch.watchLinks.length; i++ ) {
		if( string == 'watch' ) {
			wgAjaxWatch.watchLinks[i].href = wgAjaxWatch.watchLinks[i].href
				.replace( /&action=unwatch/, '&action=watch' );
		} else if( string == 'unwatch' ) {
			wgAjaxWatch.watchLinks[i].href = wgAjaxWatch.watchLinks[i].href
				.replace( /&action=watch/, '&action=unwatch' );
		}
	}
}

wgAjaxWatch.ajaxCall = function() {
	if(!wgAjaxWatch.supported) {
		return true;
	} else if (wgAjaxWatch.inprogress) {
		return false;
	}
	if(!wfSupportsAjax()) {
		// Lazy initialization so we don't toss up
		// ActiveX warnings on initial page load
		// for IE 6 users with security settings.
		wgAjaxWatch.supported = false;
		return true;
	}

	wgAjaxWatch.inprogress = true;
	wgAjaxWatch.setLinkText( wgAjaxWatch.watching
		? wgAjaxWatch.unwatchingMsg : wgAjaxWatch.watchingMsg);
	sajax_do_call(
		"wfAjaxWatch",
		[wgPageName, (wgAjaxWatch.watching ? "u" : "w")], 
		wgAjaxWatch.processResult
	);
	// if the request isn't done in 10 seconds, allow user to try again
	wgAjaxWatch.timeoutID = window.setTimeout(
		function() { wgAjaxWatch.inprogress = false; },
		10000
	);
	return false;
};

wgAjaxWatch.processResult = function(request) {
	if(!wgAjaxWatch.supported) {
		return;
	}
	var response = request.responseText;
	if( response.match(/^<w#>/) ) {
		wgAjaxWatch.watching = true;
		wgAjaxWatch.setLinkText(wgAjaxWatch.unwatchMsg);
		wgAjaxWatch.setLinkID("ca-unwatch");
		wgAjaxWatch.setHref( 'unwatch' );
	} else if( response.match(/^<u#>/) ) {
		wgAjaxWatch.watching = false;
		wgAjaxWatch.setLinkText(wgAjaxWatch.watchMsg);
		wgAjaxWatch.setLinkID("ca-watch");
		wgAjaxWatch.setHref( 'watch' );
	} else {
		// Either we got a <err#> error code or it just plain broke.
		window.location.href = wgAjaxWatch.watchLinks[0].href;
		return;
	}
	jsMsg( response.substr(4), 'watch' );
	wgAjaxWatch.inprogress = false;
	if(wgAjaxWatch.timeoutID) {
		window.clearTimeout(wgAjaxWatch.timeoutID);
	}
	return;
};

wgAjaxWatch.onLoad = function() {
	// This document structure hardcoding sucks.  We should make a class and
	// toss all this out the window.
	var el1 = document.getElementById("ca-unwatch");
	var el2 = null;
	if (!el1) {
		el1 = document.getElementById("mw-unwatch-link1");
		el2 = document.getElementById("mw-unwatch-link2");
	}
	if(el1) {
		wgAjaxWatch.watching = true;
	} else {
		wgAjaxWatch.watching = false;
		el1 = document.getElementById("ca-watch");
		if (!el1) {
			el1 = document.getElementById("mw-watch-link1");
			el2 = document.getElementById("mw-watch-link2");
		}
		if(!el1) {
			wgAjaxWatch.supported = false;
			return;
		}
	}

	// The id can be either for the parent (Monobook-based) or the element
	// itself (non-Monobook)
	wgAjaxWatch.watchLinks.push( el1.tagName.toLowerCase() == "a"
		? el1 : el1.firstChild );

	if( el2 ) {
		wgAjaxWatch.watchLinks.push( el2 );
	}

	// I couldn't get for (watchLink in wgAjaxWatch.watchLinks) to work, if
	// you can be my guest.
	for( i = 0; i < wgAjaxWatch.watchLinks.length; i++ ) {
		wgAjaxWatch.watchLinks[i].onclick = wgAjaxWatch.ajaxCall;
	}
	return;
};

hookEvent("load", wgAjaxWatch.onLoad);

/**
 * @return boolean whether the browser supports XMLHttpRequest
 */
function wfSupportsAjax() {
	var request = sajax_init_object();
	var supportsAjax = request ? true : false;
	delete request;
	return supportsAjax;
}
