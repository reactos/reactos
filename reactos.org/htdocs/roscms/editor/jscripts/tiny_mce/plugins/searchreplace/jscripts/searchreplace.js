function init() {
	tinyMCEPopup.resizeToInnerSize();

	// start with appropiate tab
	var task = (tinyMCE.getWindowArg("replacestring") != null) ? "replace" : "search";
	mcTabs.displayTab(task + '_tab', task +'_panel');
	manageReplaceButtons();
	
	var formObj = document.forms[0];

	formObj[task + "_panel_searchstring"].value = tinyMCE.getWindowArg("searchstring");
	formObj["replace_panel_replacestring"].value = (tinyMCE.getWindowArg("replacestring") != null) ? tinyMCE.getWindowArg("replacestring") : "";
	formObj[task + "_panel_casesensitivebox"].checked = tinyMCE.getWindowArg("casesensitive");
	formObj[task + "_panel_backwardsu"].checked = tinyMCE.getWindowArg("backwards");
	formObj[task + "_panel_backwardsd"].checked = !tinyMCE.getWindowArg("backwards");
}

function searchNext(replacemode) {
	// "search" or "replace" mode of operation?
	var task = (document.getElementById("search_tab").className == "current") ? "search" : "replace";

	var formObj = document.forms[0];

	if (task == "replace") {
		// Whats the point?
		if (formObj[task + "_panel_searchstring"].value == "" || formObj[task + "_panel_searchstring"].value == formObj[task + "_panel_replacestring"].value)
			return false;
	}

	// Do search
	tinyMCEPopup.execCommand('mceSearch', false, { 
		string : formObj[task + "_panel_searchstring"].value,
		replacestring : formObj["replace_panel_replacestring"].value,
		replacemode : replacemode,
		casesensitive : formObj[task + "_panel_casesensitivebox"].checked,
		backwards : formObj[task + "_panel_backwardsu"].checked,
		win : window
		}, false);

	window.focus();

	return false;
}

function cancelAction() {
	tinyMCEPopup.close();
}

function manageReplaceButtons() {
	// "search" or "replace" mode of operation?
	var task = (document.getElementById("search_tab").className == "current") ? "search" : "replace";
	document.getElementById("replace_buttons").style.visibility = (task == "replace") ? "visible" : "hidden";
}

function copyValues(link) {
	// check if tab is already active
	var tab = link;
	while (tab.tagName && tab.tagName.toLowerCase() != "li") tab = tab.parentNode;
	if (tab.className) return false; // tab is already active -> no need to copy any values!

	// copy values from one panel to the other (if they exist there)
	var from_panel_name = tab.id.match(/^search/i) ? "replace_panel" : "search_panel";
	var to_panel_name = (from_panel_name == "search_panel") ? "replace_panel" : "search_panel";

	// find all elements with IDs to copy their values
	var elms = document.getElementById(from_panel_name).getElementsByTagName("*");
	for (var i = 0; i < elms.length; i++) {
		if (elms[i].id && elms[i].id != "") {
			var checked = "undefined";
			if (elms[i].type.toLowerCase() == "checkbox" || elms[i].type.toLowerCase() == "radio")
				checked = elms[i].checked;

			// copy values if element exists in other panel
			var to_elm_name = to_panel_name + elms[i].id.substring(from_panel_name.length, elms[i].id.length);
			var to_elm = document.getElementById(to_elm_name);
			if (to_elm) {
				if (checked != "undefined")
					to_elm.checked = checked;
				else
					to_elm.value = elms[i].value;
			}
		}
	}

	return false;
}