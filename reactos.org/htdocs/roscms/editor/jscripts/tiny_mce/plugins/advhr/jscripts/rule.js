function init() {
	tinyMCEPopup.resizeToInnerSize();

	var formObj = document.forms[0];
	formObj.width.value  = tinyMCE.getWindowArg('width');
	formObj.size.value   = tinyMCE.getWindowArg('size');
	formObj.insert.value = tinyMCE.getLang('lang_' + tinyMCE.getWindowArg('mceDo'),'Insert',true);
	if (tinyMCE.getWindowArg('noshade')) {
		formObj.noshade.checked = true;
	}
	if (tinyMCE.getWindowArg('width').lastIndexOf('%')!=-1) {
		formObj.width2.value = "%";
		formObj.width.value  = formObj.width.value.substring(0,formObj.width.value.length-1);
	}
}

function insertHR() {
	var formObj = document.forms[0];
	var width   = formObj.width.value;
	var size    = formObj.size.value;
	var html = '<hr';
	if (size!='' && size!=0) {
		html += ' size="' + size + '"';
	}
	if (width!='' && width!=0) {
		html += ' width="' + width;
		if (formObj.width2.value=='%') {
			html += '%';
		}
		html += '"';
	}
	if (formObj.noshade.checked==true) {
		html += ' noshade="noshade"';
	}
	html += ' />';

	tinyMCEPopup.execCommand("mceInsertContent", true, html);
	tinyMCEPopup.close();
}

function cancelAction() {
	tinyMCEPopup.close();
}
