var action, orgTableWidth, orgTableHeight;

function insertTable() {
	var formObj = document.forms[0];
	var inst = tinyMCE.selectedInstance;
	var cols = 2, rows = 2, border = 0, cellpadding = -1, cellspacing = -1, align, width, height, className, caption;
	var html = '', capEl;
	var elm = tinyMCE.tableElm;
	var cellLimit, rowLimit, colLimit;

	if (!AutoValidator.validate(formObj)) {
		alert(tinyMCE.getLang('lang_invalid_data'));
		return false;
	}

	tinyMCEPopup.restoreSelection();

	// Get form data
	cols = formObj.elements['cols'].value;
	rows = formObj.elements['rows'].value;
	border = formObj.elements['border'].value != "" ? formObj.elements['border'].value  : 0;
	cellpadding = formObj.elements['cellpadding'].value != "" ? formObj.elements['cellpadding'].value : "";
	cellspacing = formObj.elements['cellspacing'].value != "" ? formObj.elements['cellspacing'].value : "";
	align = formObj.elements['align'].options[formObj.elements['align'].selectedIndex].value;
	width = formObj.elements['width'].value;
	height = formObj.elements['height'].value;
	bordercolor = formObj.elements['bordercolor'].value;
	bgcolor = formObj.elements['bgcolor'].value;
	className = formObj.elements['class'].options[formObj.elements['class'].selectedIndex].value;
	id = formObj.elements['id'].value;
	summary = formObj.elements['summary'].value;
	style = formObj.elements['style'].value;
	dir = formObj.elements['dir'].value;
	lang = formObj.elements['lang'].value;
	background = formObj.elements['backgroundimage'].value;
	caption = formObj.elements['caption'].checked;

	cellLimit = tinyMCE.getParam('table_cell_limit', false);
	rowLimit = tinyMCE.getParam('table_row_limit', false);
	colLimit = tinyMCE.getParam('table_col_limit', false);

	// Validate table size
	if (colLimit && cols > colLimit) {
		alert(tinyMCE.getLang('lang_table_col_limit', '', true, {cols : colLimit}));
		return false;
	} else if (rowLimit && rows > rowLimit) {
		alert(tinyMCE.getLang('lang_table_row_limit', '', true, {rows : rowLimit}));
		return false;
	} else if (cellLimit && cols * rows > cellLimit) {
		alert(tinyMCE.getLang('lang_table_cell_limit', '', true, {cells : cellLimit}));
		return false;
	}

	// Update table
	if (action == "update") {
		inst.execCommand('mceBeginUndoLevel');

		tinyMCE.setAttrib(elm, 'cellPadding', cellpadding, true);
		tinyMCE.setAttrib(elm, 'cellSpacing', cellspacing, true);
		tinyMCE.setAttrib(elm, 'border', border, true);
		tinyMCE.setAttrib(elm, 'align', align);
		tinyMCE.setAttrib(elm, 'class', className);
		tinyMCE.setAttrib(elm, 'style', style);
		tinyMCE.setAttrib(elm, 'id', id);
		tinyMCE.setAttrib(elm, 'summary', summary);
		tinyMCE.setAttrib(elm, 'dir', dir);
		tinyMCE.setAttrib(elm, 'lang', lang);

		capEl = elm.getElementsByTagName('caption')[0];

		if (capEl && !caption)
			capEl.parentNode.removeChild(capEl);

		if (!capEl && caption) {
			capEl = elm.ownerDocument.createElement('caption');
			capEl.innerHTML = '&nbsp;';
			elm.insertBefore(capEl, elm.firstChild);
		}

		// Not inline styles
		if (!tinyMCE.getParam("inline_styles"))
			tinyMCE.setAttrib(elm, 'width', width, true);

		// Remove these since they are not valid XHTML
		tinyMCE.setAttrib(elm, 'borderColor', '');
		tinyMCE.setAttrib(elm, 'bgColor', '');
		tinyMCE.setAttrib(elm, 'background', '');
		tinyMCE.setAttrib(elm, 'height', '');

		if (background != '')
			elm.style.backgroundImage = "url('" + background + "')";
		else
			elm.style.backgroundImage = '';

		if (tinyMCE.getParam("inline_styles"))
			elm.style.borderWidth = border + "px";

		if (tinyMCE.getParam("inline_styles")) {
			if (width != '')
				elm.style.width = getCSSSize(width);
		}

		if (bordercolor != "") {
			elm.style.borderColor = bordercolor;
			elm.style.borderStyle = elm.style.borderStyle == "" ? "solid" : elm.style.borderStyle;
			elm.style.borderWidth = border == "" ? "1px" : border;
		} else
			elm.style.borderColor = '';

		elm.style.backgroundColor = bgcolor;
		elm.style.height = getCSSSize(height);

		tinyMCE.handleVisualAid(tinyMCE.tableElm, false, inst.visualAid, inst);

		// Fix for stange MSIE align bug
		tinyMCE.tableElm.outerHTML = tinyMCE.tableElm.outerHTML;

		tinyMCE.handleVisualAid(inst.getBody(), true, inst.visualAid, inst);
		tinyMCE.triggerNodeChange();
		inst.execCommand('mceEndUndoLevel');

		// Repaint if dimensions changed
		if (formObj.width.value != orgTableWidth || formObj.height.value != orgTableHeight)
			inst.repaint();

		tinyMCEPopup.close();
		return true;
	}

	// Create new table
	html += '<table';

	html += makeAttrib('id', id);
	html += makeAttrib('border', border);
	html += makeAttrib('cellpadding', cellpadding);
	html += makeAttrib('cellspacing', cellspacing);
	html += makeAttrib('width', width);
	//html += makeAttrib('height', height);
	//html += makeAttrib('bordercolor', bordercolor);
	//html += makeAttrib('bgcolor', bgcolor);
	html += makeAttrib('align', align);
	html += makeAttrib('class', tinyMCE.getVisualAidClass(className, border == 0));
	html += makeAttrib('style', style);
	html += makeAttrib('summary', summary);
	html += makeAttrib('dir', dir);
	html += makeAttrib('lang', lang);
	html += '>';

	if (caption)
		html += '<caption>&nbsp;</caption>';

	for (var y=0; y<rows; y++) {
		html += "<tr>";

		for (var x=0; x<cols; x++)
			html += '<td>&nbsp;</td>';

		html += "</tr>";
	}

	html += "</table>";

	inst.execCommand('mceBeginUndoLevel');
	inst.execCommand('mceInsertContent', false, html);
	tinyMCE.handleVisualAid(inst.getBody(), true, tinyMCE.settings['visual']);
	inst.execCommand('mceEndUndoLevel');

	tinyMCEPopup.close();
}

function makeAttrib(attrib, value) {
	var formObj = document.forms[0];
	var valueElm = formObj.elements[attrib];

	if (typeof(value) == "undefined" || value == null) {
		value = "";

		if (valueElm)
			value = valueElm.value;
	}

	if (value == "")
		return "";

	// XML encode it
	value = value.replace(/&/g, '&amp;');
	value = value.replace(/\"/g, '&quot;');
	value = value.replace(/</g, '&lt;');
	value = value.replace(/>/g, '&gt;');

	return ' ' + attrib + '="' + value + '"';
}

function init() {
	tinyMCEPopup.resizeToInnerSize();

	document.getElementById('backgroundimagebrowsercontainer').innerHTML = getBrowserHTML('backgroundimagebrowser','backgroundimage','image','table');
	document.getElementById('backgroundimagebrowsercontainer').innerHTML = getBrowserHTML('backgroundimagebrowser','backgroundimage','image','table');
	document.getElementById('bordercolor_pickcontainer').innerHTML = getColorPickerHTML('bordercolor_pick','bordercolor');
	document.getElementById('bgcolor_pickcontainer').innerHTML = getColorPickerHTML('bgcolor_pick','bgcolor');

	var cols = 2, rows = 2, border = tinyMCE.getParam('table_default_border', '0'), cellpadding = tinyMCE.getParam('table_default_cellpadding', ''), cellspacing = tinyMCE.getParam('table_default_cellspacing', '');
	var align = "", width = "", height = "", bordercolor = "", bgcolor = "", className = "";
	var id = "", summary = "", style = "", dir = "", lang = "", background = "", bgcolor = "", bordercolor = "";
	var inst = tinyMCE.selectedInstance;
	var formObj = document.forms[0];
	var elm = tinyMCE.getParentElement(inst.getFocusElement(), "table");

	tinyMCE.tableElm = elm;
	action = tinyMCE.getWindowArg('action');
	if (action == null)
		action = tinyMCE.tableElm ? "update" : "insert";

	if (tinyMCE.tableElm && action != "insert") {
		var rowsAr = tinyMCE.tableElm.rows;
		var cols = 0;
		for (var i=0; i<rowsAr.length; i++)
			if (rowsAr[i].cells.length > cols)
				cols = rowsAr[i].cells.length;

		cols = cols;
		rows = rowsAr.length;

		st = tinyMCE.parseStyle(tinyMCE.getAttrib(tinyMCE.tableElm, "style"));
		border = trimSize(getStyle(elm, 'border', 'borderWidth'));
		cellpadding = tinyMCE.getAttrib(tinyMCE.tableElm, 'cellpadding', "");
		cellspacing = tinyMCE.getAttrib(tinyMCE.tableElm, 'cellspacing', "");
		width = trimSize(getStyle(elm, 'width', 'width'));
		height = trimSize(getStyle(elm, 'height', 'height'));
		bordercolor = convertRGBToHex(getStyle(elm, 'bordercolor', 'borderLeftColor'));
		bgcolor = convertRGBToHex(getStyle(elm, 'bgcolor', 'backgroundColor'));
		align = tinyMCE.getAttrib(tinyMCE.tableElm, 'align', align);
		className = tinyMCE.getVisualAidClass(tinyMCE.getAttrib(tinyMCE.tableElm, 'class'), false);
		id = tinyMCE.getAttrib(tinyMCE.tableElm, 'id');
		summary = tinyMCE.getAttrib(tinyMCE.tableElm, 'summary');
		style = tinyMCE.serializeStyle(st);
		dir = tinyMCE.getAttrib(tinyMCE.tableElm, 'dir');
		lang = tinyMCE.getAttrib(tinyMCE.tableElm, 'lang');
		background = getStyle(elm, 'background', 'backgroundImage').replace(new RegExp("url\\('?([^']*)'?\\)", 'gi'), "$1");
		formObj.caption.checked = tinyMCE.tableElm.getElementsByTagName('caption').length > 0;

		orgTableWidth = width;
		orgTableHeight = height;

		action = "update";
	}

	addClassesToList('class', "table_styles");

	// Update form
	selectByValue(formObj, 'align', align);
	selectByValue(formObj, 'class', className);
	formObj.cols.value = cols;
	formObj.rows.value = rows;
	formObj.border.value = border;
	formObj.cellpadding.value = cellpadding;
	formObj.cellspacing.value = cellspacing;
	formObj.width.value = width;
	formObj.height.value = height;
	formObj.bordercolor.value = bordercolor;
	formObj.bgcolor.value = bgcolor;
	formObj.id.value = id;
	formObj.summary.value = summary;
	formObj.style.value = style;
	formObj.dir.value = dir;
	formObj.lang.value = lang;
	formObj.backgroundimage.value = background;
	formObj.insert.value = tinyMCE.getLang('lang_' + action, 'Insert', true); 

	updateColor('bordercolor_pick', 'bordercolor');
	updateColor('bgcolor_pick', 'bgcolor');

	// Resize some elements
	if (isVisible('backgroundimagebrowser'))
		document.getElementById('backgroundimage').style.width = '180px';

	// Disable some fields in update mode
	if (action == "update") {
		formObj.cols.disabled = true;
		formObj.rows.disabled = true;
	}
}

function changedSize() {
	var formObj = document.forms[0];
	var st = tinyMCE.parseStyle(formObj.style.value);

	var width = formObj.width.value;
	if (width != "")
		st['width'] = tinyMCE.getParam("inline_styles") ? getCSSSize(width) : "";
	else
		st['width'] = "";

	var height = formObj.height.value;
	if (height != "")
		st['height'] = getCSSSize(height);
	else
		st['height'] = "";

	formObj.style.value = tinyMCE.serializeStyle(st);
}

function changedBackgroundImage() {
	var formObj = document.forms[0];
	var st = tinyMCE.parseStyle(formObj.style.value);

	st['background-image'] = "url('" + formObj.backgroundimage.value + "')";

	formObj.style.value = tinyMCE.serializeStyle(st);
}

function changedBorder() {
	var formObj = document.forms[0];
	var st = tinyMCE.parseStyle(formObj.style.value);

	// Update border width if the element has a color
	if (formObj.border.value != "" && formObj.bordercolor.value != "")
		st['border-width'] = formObj.border.value + "px";

	formObj.style.value = tinyMCE.serializeStyle(st);
}

function changedColor() {
	var formObj = document.forms[0];
	var st = tinyMCE.parseStyle(formObj.style.value);

	st['background-color'] = formObj.bgcolor.value;

	if (formObj.bordercolor.value != "") {
		st['border-color'] = formObj.bordercolor.value;

		// Add border-width if it's missing
		if (!st['border-width'])
			st['border-width'] = formObj.border.value == "" ? "1px" : formObj.border.value + "px";
	}

	formObj.style.value = tinyMCE.serializeStyle(st);
}

function changedStyle() {
	var formObj = document.forms[0];
	var st = tinyMCE.parseStyle(formObj.style.value);

	if (st['background-image'])
		formObj.backgroundimage.value = st['background-image'].replace(new RegExp("url\\('?([^']*)'?\\)", 'gi'), "$1");
	else
		formObj.backgroundimage.value = '';

	if (st['width'])
		formObj.width.value = trimSize(st['width']);

	if (st['height'])
		formObj.height.value = trimSize(st['height']);

	if (st['background-color']) {
		formObj.bgcolor.value = st['background-color'];
		updateColor('bgcolor_pick','bgcolor');
	}

	if (st['border-color']) {
		formObj.bordercolor.value = st['border-color'];
		updateColor('bordercolor_pick','bordercolor');
	}
}
