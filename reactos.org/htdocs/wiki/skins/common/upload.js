function licenseSelectorCheck() {
	var selector = document.getElementById( "wpLicense" );
	var selection = selector.options[selector.selectedIndex].value;
	if( selector.selectedIndex > 0 ) {
		if( selection == "" ) {
			// Option disabled, but browser is broken and doesn't respect this
			selector.selectedIndex = 0;
		}
	}
	// We might show a preview
	wgUploadLicenseObj.fetchPreview( selection );
}

function licenseSelectorFixup() {
	// for MSIE/Mac; non-breaking spaces cause the <option> not to render
	// but, for some reason, setting the text to itself works
	var selector = document.getElementById("wpLicense");
	if (selector) {
		var ua = navigator.userAgent;
		var isMacIe = (ua.indexOf("MSIE") != -1) && (ua.indexOf("Mac") != -1);
		if (isMacIe) {
			for (var i = 0; i < selector.options.length; i++) {
				selector.options[i].text = selector.options[i].text;
			}
		}
	}
}

var wgUploadWarningObj = {
	'responseCache' : { '' : '&nbsp;' },
	'nameToCheck' : '',
	'typing': false,
	'delay': 500, // ms
	'timeoutID': false,

	'keypress': function () {
		if ( !wgAjaxUploadDestCheck || !sajax_init_object() ) return;

		// Find file to upload
		var destFile = document.getElementById('wpDestFile');
		var warningElt = document.getElementById( 'wpDestFile-warning' );
		if ( !destFile || !warningElt ) return ;

		this.nameToCheck = destFile.value ;

		// Clear timer 
		if ( this.timeoutID ) {
			window.clearTimeout( this.timeoutID );
		}
		// Check response cache
		for (cached in this.responseCache) {
			if (this.nameToCheck == cached) {
				this.setWarning(this.responseCache[this.nameToCheck]);
				return;
			}
		}

		this.timeoutID = window.setTimeout( 'wgUploadWarningObj.timeout()', this.delay );
	},

	'checkNow': function (fname) {
		if ( !wgAjaxUploadDestCheck || !sajax_init_object() ) return;
		if ( this.timeoutID ) {
			window.clearTimeout( this.timeoutID );
		}
		this.nameToCheck = fname;
		this.timeout();
	},
	
	'timeout' : function() {
		if ( !wgAjaxUploadDestCheck || !sajax_init_object() ) return;
		injectSpinner( document.getElementById( 'wpDestFile' ), 'destcheck' );

		// Get variables into local scope so that they will be preserved for the 
		// anonymous callback. fileName is copied so that multiple overlapping 
		// ajax requests can be supported.
		var obj = this;
		var fileName = this.nameToCheck;
		sajax_do_call( 'UploadForm::ajaxGetExistsWarning', [this.nameToCheck], 
			function (result) {
				obj.processResult(result, fileName)
			}
		);
	},

	'processResult' : function (result, fileName) {
		removeSpinner( 'destcheck' );
		this.setWarning(result.responseText);
		this.responseCache[fileName] = result.responseText;
	},

	'setWarning' : function (warning) {
		var warningElt = document.getElementById( 'wpDestFile-warning' );
		var ackElt = document.getElementById( 'wpDestFileWarningAck' );
		this.setInnerHTML(warningElt, warning);

		// Set a value in the form indicating that the warning is acknowledged and 
		// doesn't need to be redisplayed post-upload
		if ( warning == '' || warning == '&nbsp;' ) {
			ackElt.value = '';
		} else {
			ackElt.value = '1';
		}
	},

	'setInnerHTML' : function (element, text) {
		// Check for no change to avoid flicker in IE 7
		if (element.innerHTML != text) {
			element.innerHTML = text;
		}
	}
}

function fillDestFilename(id) {
	if (!wgUploadAutoFill) {
		return;
	}
	if (!document.getElementById) {
		return;
	}
	var path = document.getElementById(id).value;
	// Find trailing part
	var slash = path.lastIndexOf('/');
	var backslash = path.lastIndexOf('\\');
	var fname;
	if (slash == -1 && backslash == -1) {
		fname = path;
	} else if (slash > backslash) {
		fname = path.substring(slash+1, 10000);
	} else {
		fname = path.substring(backslash+1, 10000);
	}

	// Capitalise first letter and replace spaces by underscores
	fname = fname.charAt(0).toUpperCase().concat(fname.substring(1,10000)).replace(/ /g, '_');

	// Output result
	var destFile = document.getElementById('wpDestFile');
	if (destFile) {
		destFile.value = fname;
		wgUploadWarningObj.checkNow(fname) ;
	}
}

function toggleFilenameFiller() {
	if(!document.getElementById) return;
	var upfield = document.getElementById('wpUploadFile');
	var destName = document.getElementById('wpDestFile').value;
	if (destName=='' || destName==' ') {
		wgUploadAutoFill = true;
	} else {
		wgUploadAutoFill = false;
	}
}

var wgUploadLicenseObj = {
	
	'responseCache' : { '' : '' },

	'fetchPreview': function( license ) {
		if( !wgAjaxLicensePreview || !sajax_init_object() ) return;
		for (cached in this.responseCache) {
			if (cached == license) {
				this.showPreview( this.responseCache[license] );
				return;
			}
		}
		injectSpinner( document.getElementById( 'wpLicense' ), 'license' );
		sajax_do_call( 'UploadForm::ajaxGetLicensePreview', [license],
			function( result ) {
				wgUploadLicenseObj.processResult( result, license );
			}
		);
	},

	'processResult' : function( result, license ) {
		removeSpinner( 'license' );
		this.showPreview( result.responseText );
		this.responseCache[license] = result.responseText;
	},

	'showPreview' : function( preview ) {
		var previewPanel = document.getElementById( 'mw-license-preview' );
		if( previewPanel.innerHTML != preview )
			previewPanel.innerHTML = preview;
	}
	
}

addOnloadHook( licenseSelectorFixup );