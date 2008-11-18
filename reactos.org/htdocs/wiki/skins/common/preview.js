/**
 * Live preview script for MediaWiki
 *
 * 2007-04-25 – Nikerabbit:
 *   Worked around text cutoff in mozilla-based browsers
 *   Support for categories
 */


lpIdPreview = 'wikiPreview';
lpIdCategories = 'catlinks';
lpIdDiff = 'wikiDiff';

/*
 * Returns XMLHttpRequest based on browser support or null
 */
function openXMLHttpRequest() {
	if( window.XMLHttpRequest ) {
		return new XMLHttpRequest();
	} else if( window.ActiveXObject && navigator.platform != 'MacPPC' ) {
		// IE/Mac has an ActiveXObject but it doesn't work.
		return new ActiveXObject("Microsoft.XMLHTTP");
	} else {
		return null;
	}
}

/**
 * Returns true if could open the request,
 * false otherwise (eg no browser support).
 */
function lpDoPreview(text, postUrl) {
	lpRequest = openXMLHttpRequest();
	if( !lpRequest ) return false;

	lpRequest.onreadystatechange = lpStatusUpdate;
	lpRequest.open("POST", postUrl, true);

	var postData = 'wpTextbox1=' + encodeURIComponent(text);
	lpRequest.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
	lpRequest.send(postData);
	return true;
}

function lpStatusUpdate() {

	/* We are at some stage of loading */
	if (lpRequest.readyState > 0 && lpRequest.readyState < 4) {
		notify(i18n(wgLivepreviewMessageLoading));
	}

	/* Not loaded yet */
	if(lpRequest.readyState != 4) {
		return;
	}

	/* We got response, bug it not what we wanted */
	if( lpRequest.status != 200 ) {
		var keys = new Array();
		keys[0] = lpRequest.status;
		keys[1] = lpRequest.statusText;
		window.alert(i18n(wgLivepreviewMessageError, keys));
		lpShowNormalPreview();
		return;
	}

	/* All good */
	dismissNotify(i18n(wgLivepreviewMessageReady), 750);

	
	var XMLObject = lpRequest.responseXML.documentElement;


	/* Work around Firefox (Gecko?) limitation where it shows only the first 4096
	 * bytes of data. Ref: http://www.thescripts.com/forum/thread482760.html
	 */
	XMLObject.normalize();

	var previewElement = XMLObject.getElementsByTagName('preview')[0];
	var categoryElement = XMLObject.getElementsByTagName('category')[0];

	/* Hide the active diff if it exists */
	var diff = document.getElementById(lpIdDiff);
	if ( diff ) { diff.style.display = 'none'; }

	/* Inject preview */
	var previewContainer = document.getElementById( lpIdPreview );
	if ( previewContainer && previewElement ) {
		previewContainer.innerHTML = previewElement.firstChild.data;
	} else {
		/* Should never happen */
		window.alert(i18n(wgLivepreviewMessageFailed));
		lpShowNormalPreview();
		return;
	}
		

	/* Inject categories */
	var categoryContainer  = document.getElementById( lpIdCategories );
	if ( categoryElement && categoryElement.firstChild ) {
		if ( categoryContainer ) {
			categoryContainer.innerHTML = categoryElement.firstChild.data;
			/* May be hidden */
			categoryContainer.style.display = 'block';
		} else {
			/* Just dump them somewhere */
	/*		previewContainer.innerHTML += categoryElement.firstChild.data;*/
		}
	} else {
		/* Nothing to show, hide old data */
		if ( categoryContainer ) {
			categoryContainer.style.display = 'none';
		}
	}

}

function lpShowNormalPreview() {
	var fallback = document.getElementById('wpPreview');
	if ( fallback ) { fallback.style.display = 'inline'; }
}


// TODO: move elsewhere
/* Small non-intrusive popup which can be used for example to notify the user
 * about completed AJAX action. Supports only one notify at a time.
 */
function notify(message) {
	var notifyElement = document.getElementById('mw-js-notify');
	if ( !notifyElement ) {
		createNotify();
		var notifyElement = document.getElementById('mw-js-notify');
	}
	notifyElement.style.display = 'block';
	notifyElement.innerHTML = message;
}

function dismissNotify(message, timeout) {
	var notifyElement = document.getElementById('mw-js-notify');
	if ( notifyElement ) {
		if ( timeout == 0 ) {
			notifyElement.style.display = 'none';
		} else {
			notify(message);
			setTimeout("dismissNotify('', 0)", timeout);
		}
	}
}

function createNotify() {
	var div = document.createElement("div");
	var txt = '###PLACEHOLDER###'
	var txtNode = document.createTextNode(txt);
	div.appendChild(txtNode);
	div.id = 'mw-js-notify';
	// TODO: move styles to css
	div.setAttribute('style',
		'display: none; position: fixed; bottom: 0px; right: 0px; color: white; background-color: DarkRed; z-index: 5; padding: 0.1em 1em 0.1em 1em; font-size: 120%;');
	var body = document.getElementsByTagName('body')[0];
	body.appendChild(div);
}



/* Helper function similar to wfMsgReplaceArgs() */
function i18n(message, keys) {
	var localMessage = message;
	if ( !keys ) { return localMessage; }
	for( var i = 0; i < keys.length; i++) {
		var myregexp = new RegExp("\\$"+(i+1), 'g');
		localMessage = localMessage.replace(myregexp, keys[i]);
	}
	return localMessage;
}