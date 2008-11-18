// this function generates the actual toolbar buttons with localized text
// we use it to avoid creating the toolbar where javascript is not enabled
function addButton(imageFile, speedTip, tagOpen, tagClose, sampleText, imageId) {
	// Don't generate buttons for browsers which don't fully
	// support it.
	mwEditButtons[mwEditButtons.length] =
		{"imageId": imageId,
		 "imageFile": imageFile,
		 "speedTip": speedTip,
		 "tagOpen": tagOpen,
		 "tagClose": tagClose,
		 "sampleText": sampleText};
}

// this function generates the actual toolbar buttons with localized text
// we use it to avoid creating the toolbar where javascript is not enabled
function mwInsertEditButton(parent, item) {
	var image = document.createElement("img");
	image.width = 23;
	image.height = 22;
	image.className = "mw-toolbar-editbutton";
	if (item.imageId) image.id = item.imageId;
	image.src = item.imageFile;
	image.border = 0;
	image.alt = item.speedTip;
	image.title = item.speedTip;
	image.style.cursor = "pointer";
	image.onclick = function() {
		insertTags(item.tagOpen, item.tagClose, item.sampleText);
		return false;
	};

	parent.appendChild(image);
	return true;
}

function mwSetupToolbar() {
	var toolbar = document.getElementById('toolbar');
	if (!toolbar) { return false; }

	var textbox = document.getElementById('wpTextbox1');
	if (!textbox) { return false; }

	// Don't generate buttons for browsers which don't fully
	// support it.
	if (!(document.selection && document.selection.createRange)
		&& textbox.selectionStart === null) {
		return false;
	}

	for (var i = 0; i < mwEditButtons.length; i++) {
		mwInsertEditButton(toolbar, mwEditButtons[i]);
	}
	for (var i = 0; i < mwCustomEditButtons.length; i++) {
		mwInsertEditButton(toolbar, mwCustomEditButtons[i]);
	}
	return true;
}

// apply tagOpen/tagClose to selection in textarea,
// use sampleText instead of selection if there is none
function insertTags(tagOpen, tagClose, sampleText) {
	var txtarea;
	if (document.editform) {
		txtarea = document.editform.wpTextbox1;
	} else {
		// some alternate form? take the first one we can find
		var areas = document.getElementsByTagName('textarea');
		txtarea = areas[0];
	}
	var selText, isSample = false;

	if (document.selection  && document.selection.createRange) { // IE/Opera

		//save window scroll position
		if (document.documentElement && document.documentElement.scrollTop)
			var winScroll = document.documentElement.scrollTop
		else if (document.body)
			var winScroll = document.body.scrollTop;
		//get current selection  
		txtarea.focus();
		var range = document.selection.createRange();
		selText = range.text;
		//insert tags
		checkSelectedText();
		range.text = tagOpen + selText + tagClose;
		//mark sample text as selected
		if (isSample && range.moveStart) {
			if (window.opera)
				tagClose = tagClose.replace(/\n/g,'');
			range.moveStart('character', - tagClose.length - selText.length); 
			range.moveEnd('character', - tagClose.length); 
		}
		range.select();   
		//restore window scroll position
		if (document.documentElement && document.documentElement.scrollTop)
			document.documentElement.scrollTop = winScroll
		else if (document.body)
			document.body.scrollTop = winScroll;

	} else if (txtarea.selectionStart || txtarea.selectionStart == '0') { // Mozilla

		//save textarea scroll position
		var textScroll = txtarea.scrollTop;
		//get current selection
		txtarea.focus();
		var startPos = txtarea.selectionStart;
		var endPos = txtarea.selectionEnd;
		selText = txtarea.value.substring(startPos, endPos);
		//insert tags
		checkSelectedText();
		txtarea.value = txtarea.value.substring(0, startPos)
			+ tagOpen + selText + tagClose
			+ txtarea.value.substring(endPos, txtarea.value.length);
		//set new selection
		if (isSample) {
			txtarea.selectionStart = startPos + tagOpen.length;
			txtarea.selectionEnd = startPos + tagOpen.length + selText.length;
		} else {
			txtarea.selectionStart = startPos + tagOpen.length + selText.length + tagClose.length;
			txtarea.selectionEnd = txtarea.selectionStart;
		}
		//restore textarea scroll position
		txtarea.scrollTop = textScroll;
	} 

	function checkSelectedText(){
		if (!selText) {
			selText = sampleText;
			isSample = true;
		} else if (selText.charAt(selText.length - 1) == ' ') { //exclude ending space char
			selText = selText.substring(0, selText.length - 1);
			tagClose += ' '
		} 
	}

}

/**
 * Restore the edit box scroll state following a preview operation,
 * and set up a form submission handler to remember this state
 */
function scrollEditBox() {
	var editBox = document.getElementById( 'wpTextbox1' );
	var scrollTop = document.getElementById( 'wpScrolltop' );
	var editForm = document.getElementById( 'editform' );
	if( editBox && scrollTop ) {
		if( scrollTop.value )
			editBox.scrollTop = scrollTop.value;
		addHandler( editForm, 'submit', function() {
			document.getElementById( 'wpScrolltop' ).value = document.getElementById( 'wpTextbox1' ).scrollTop; 
		} );
	}
}
hookEvent( 'load', scrollEditBox );
hookEvent( 'load', mwSetupToolbar );
