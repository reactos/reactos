tinyMCEPopup.requireLangPack();

function init() {
	var f = document.forms[0], v;

	tinyMCEPopup.resizeToInnerSize();

	f.numcols.value = tinyMCEPopup.getWindowArg('numcols', 1);
	f.numrows.value = tinyMCEPopup.getWindowArg('numrows', 1);
}

function mergeCells() {
	var args = [], f = document.forms[0];

	tinyMCEPopup.restoreSelection();

	if (!AutoValidator.validate(f)) {
		tinyMCEPopup.alert(tinyMCEPopup.getLang('invalid_data'));
		return false;
	}

	args["numcols"] = f.numcols.value;
	args["numrows"] = f.numrows.value;

	tinyMCEPopup.execCommand("mceTableMergeCells", false, args);
	tinyMCEPopup.close();
}

tinyMCEPopup.onInit.add(init);
