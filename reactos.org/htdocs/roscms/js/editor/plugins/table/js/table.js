tinyMCEPopup.requireLangPack();

var action, orgTableWidth, orgTableHeight, dom = tinyMCEPopup.editor.dom;

function insertTable() {
	var formObj = document.forms[0];
	var inst = tinyMCEPopup.editor, dom = inst.dom;
	var cols = 2, rows = 2, border = 0, cellpadding = -1, cellspacing = -1, align, width, height, className, caption, frame, rules;
	var html = '', capEl, elm;
	var cellLimit, rowLimit, colLimit;

	tinyMCEPopup.restoreSelection();

	if (!AutoValidator.validate(formObj)) {
		tinyMCEPopup.alert(inst.getLang('invalid_data'));
		return false;
	}

	elm = dom.getParent(inst.selection.getNode(), 'table');

	// Get form data
	cols = formObj.elements['cols'].value;
	rows = formObj.elements['rows'].value;
	border = formObj.elements['border'].value != "" ? formObj.elements['border'].value  : 0;
	cellpadding = formObj.elements['cellpadding'].value != "" ? formObj.elements['cellpadding'].value : "";
	cellspacing = formObj.elements['cellspacing'].value != "" ? formObj.elements['cellspacing'].value : "";
	align = formObj.elements['align'].options[formObj.elements['align'].selectedIndex].value;
	frame = formObj.elements['frame'].options[formObj.elements['frame'].selectedIndex].value;
	rules = formObj.elements['rules'].options[formObj.elements['rules'].selectedIndex].value;
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

	cellLimit = tinyMCEPopup.getParam('table_cell_limit', false);
	rowLimit = tinyMCEPopup.getParam('table_row_limit', false);
	colLimit = tinyMCEPopup.getParam('table_col_limit', false);

	// Validate table size
	if (colLimit && cols > colLimit) {
		tinyMCEPopup.alert(inst.getLang('table_dlg.col_limit').replace(/\{\$cols\}/g, colLimit));
		return false;
	} else if (rowLimit && rows > rowLimit) {
		tinyMCEPopup.alert(inst.getLang('table_dlg.row_limit').replace(/\{\$rows\}/g, rowLimit));
		return false;
	} else if (cellLimit && cols * rows > cellLimit) {
		tinyMCEPopup.alert(inst.getLang('table_dlg.cell_limit').replace(/\{\$cells\}/g, cellLimit));
		return false;
	}

	// Update table
	if (action == "update") {
		inst.execCommand('mceBeginUndoLevel');

		dom.setAttrib(elm, 'cellPadding', cellpadding, true);
		dom.setAttrib(elm, 'cellSpacing', cellspacing, true);
		dom.setAttrib(elm, 'border', border);
		dom.setAttrib(elm, 'align', align);
		dom.setAttrib(elm, 'frame', frame);
		dom.setAttrib(elm, 'rules', rules);
		dom.setAttrib(elm, 'class', className);
		dom.setAttrib(elm, 'style', style);
		dom.setAttrib(elm, 'id', id);
		dom.setAttrib(elm, 'summary', summary);
		dom.setAttrib(elm, 'dir', dir);
		dom.setAttrib(elm, 'lang', lang);

		capEl = inst.dom.select('caption', elm)[0];

		if (capEl && !caption)
			capEl.parentNode.removeChild(capEl);

		if (!capEl && caption) {
			capEl = elm.ownerDocument.createElement('caption');

			if (!tinymce.isIE)
				capEl.innerHTML = '<br mce_bogus="1"/>';

			elm.insertBefore(capEl, elm.firstChild);
		}

		if (width && inst.settings.inline_styles) {
			dom.setStyle(elm, 'width', width);
			dom.setAttrib(elm, 'width', '');
		} else {
			dom.setAttrib(elm, 'width', width, true);
			dom.setStyle(elm, 'width', '');
		}

		// Remove these since they are not valid XHTML
		dom.setAttrib(elm, 'borderColor', '');
		dom.setAttrib(elm, 'bgColor', '');
		dom.setAttrib(elm, 'background', '');

		if (height && inst.settings.inline_styles) {
			dom.setStyle(elm, 'height', height);
			dom.setAttrib(elm, 'height', '');
		} else {
			dom.setAttrib(elm, 'height', height, true);
			dom.setStyle(elm, 'height', '');
 		}

		if (background != '')
			elm.style.backgroundImage = "url('" + background + "')";
		else
			elm.style.backgroundImage = '';

/*		if (tinyMCEPopup.getParam("inline_styles")) {
			if (width != '')
				elm.style.width = getCSSSize(width);
		}*/

		if (bordercolor != "") {
			elm.style.borderColor = bordercolor;
			elm.style.borderStyle = elm.style.borderStyle == "" ? "solid" : elm.style.borderStyle;
			elm.style.borderWidth = border == "" ? "1px" : border;
		} else
			elm.style.borderColor = '';

		elm.style.backgroundColor = bgcolor;
		elm.style.height = getCSSSize(height);

		inst.addVisual();

		// Fix for stange MSIE align bug
		//elm.outerHTML = elm.outerHTML;

		inst.nodeChanged();
		inst.execCommand('mceEndUndoLevel');

		// Repaint if dimensions changed
		if (formObj.width.value != orgTableWidth || formObj.height.value != orgTableHeight)
			inst.execCommand('mceRepaint');

		tinyMCEPopup.close();
		return true;
	}

	// Create new table
	html += '<table';

	html += makeAttrib('id', id);
	html += makeAttrib('border', border);
	html += makeAttrib('cellpadding', cellpadding);
	html += makeAttrib('cellspacing', cellspacing);

	if (width && inst.settings.inline_styles) {
		if (style)
			style += '; ';

		style += 'width: ' + width;
	} else
		html += makeAttrib('width', width);

/*	if (height) {
		if (style)
			style += '; ';

		style += 'height: ' + height;
	}*/

	//html += makeAttrib('height', height);
	//html += makeAttrib('bordercolor', bordercolor);
	//html += makeAttrib('bgcolor', bgcolor);
	html += makeAttrib('align', align);
	html += makeAttrib('frame', frame);
	html += makeAttrib('rules', rules);
	html += makeAttrib('class', className);
	html += makeAttrib('style', style);
	html += makeAttrib('summary', summary);
	html += makeAttrib('dir', dir);
	html += makeAttrib('lang', lang);
	html += '>';

	if (caption) {
		if (!tinymce.isIE)
			html += '<caption><br mce_bogus="1"/></caption>';
		else
			html += '<caption></caption>';
	}

	for (var y=0; y<rows; y++) {
		html += "<tr>";

		for (var x=0; x<cols; x++) {
			if (!tinymce.isIE)
				html += '<td><br mce_bogus="1"/></td>';
			else
				html += '<td></td>';
		}

		html += "</tr>";
	}

	html += "</table>";

	inst.execCommand('mceBeginUndoLevel');
	inst.execCommand('mceInsertContent', false, html);
	inst.addVisual();
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

	var cols = 2, rows = 2, border = tinyMCEPopup.getParam('table_default_border', '0'), cellpadding = tinyMCEPopup.getParam('table_default_cellpadding', ''), cellspacing = tinyMCEPopup.getParam('table_default_cellspacing', '');
	var align = "", width = "", height = "", bordercolor = "", bgcolor = "", className = "";
	var id = "", summary = "", style = "", dir = "", lang = "", background = "", bgcolor = "", bordercolor = "", rules, frame;
	var inst = tinyMCEPopup.editor, dom = inst.dom;
	var formObj = document.forms[0];
	var elm = dom.getParent(inst.selection.getNode(), "table");

	action = tinyMCEPopup.getWindowArg('action');

	if (!action)
		action = elm ? "update" : "insert";

	if (elm && action != "insert") {
		var rowsAr = elm.rows;
		var cols = 0;
		for (var i=0; i<rowsAr.length; i++)
			if (rowsAr[i].cells.length > cols)
				cols = rowsAr[i].cells.length;

		cols = cols;
		rows = rowsAr.length;

		st = dom.parseStyle(dom.getAttrib(elm, "style"));
		border = trimSize(getStyle(elm, 'border', 'borderWidth'));
		cellpadding = dom.getAttrib(elm, 'cellpadding', "");
		cellspacing = dom.getAttrib(elm, 'cellspacing', "");
		width = trimSize(getStyle(elm, 'width', 'width'));
		height = trimSize(getStyle(elm, 'height', 'height'));
		bordercolor = convertRGBToHex(getStyle(elm, 'bordercolor', 'borderLeftColor'));
		bgcolor = convertRGBToHex(getStyle(elm, 'bgcolor', 'backgroundColor'));
		align = dom.getAttrib(elm, 'align', align);
		frame = dom.getAttrib(elm, 'frame');
		rules = dom.getAttrib(elm, 'rules');
		className = tinymce.trim(dom.getAttrib(elm, 'class').replace(/mceItem.+/g, ''));
		id = dom.getAttrib(elm, 'id');
		summary = dom.getAttrib(elm, 'summary');
		style = dom.serializeStyle(st);
		dir = dom.getAttrib(elm, 'dir');
		lang = dom.getAttrib(elm, 'lang');
		background = getStyle(elm, 'background', 'backgroundImage').replace(new RegExp("url\\('?([^']*)'?\\)", 'gi'), "$1");
		formObj.caption.checked = elm.getElementsByTagName('caption').length > 0;

		orgTableWidth = width;
		orgTableHeight = height;

		action = "update";
		formObj.insert.value = inst.getLang('update');
	}

	addClassesToList('class', "table_styles");
	TinyMCE_EditableSelects.init();

	// Update form
	selectByValue(formObj, 'align', align);
	selectByValue(formObj, 'frame', frame);
	selectByValue(formObj, 'rules', rules);
	selectByValue(formObj, 'class', className, true, true);
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
	var st = dom.parseStyle(formObj.style.value);

/*	var width = formObj.width.value;
	if (width != "")
		st['width'] = tinyMCEPopup.getParam("inline_styles") ? getCSSSize(width) : "";
	else
		st['width'] = "";*/

	var height = formObj.height.value;
	if (height != "")
		st['height'] = getCSSSize(height);
	else
		st['height'] = "";

	formObj.style.value = dom.serializeStyle(st);
}

function changedBackgroundImage() {
	var formObj = document.forms[0];
	var st = dom.parseStyle(formObj.style.value);

	st['background-image'] = "url('" + formObj.backgroundimage.value + "')";

	formObj.style.value = dom.serializeStyle(st);
}

function changedBorder() {
	var formObj = document.forms[0];
	var st = dom.parseStyle(formObj.style.value);

	// Update border width if the element has a color
	if (formObj.border.value != "" && formObj.bordercolor.value != "")
		st['border-width'] = formObj.border.value + "px";

	formObj.style.value = dom.serializeStyle(st);
}

function changedColor() {
	var formObj = document.forms[0];
	var st = dom.parseStyle(formObj.style.value);

	st['background-color'] = formObj.bgcolor.value;

	if (formObj.bordercolor.value != "") {
		st['border-color'] = formObj.bordercolor.value;

		// Add border-width if it's missing
		if (!st['border-width'])
			st['border-width'] = formObj.border.value == "" ? "1px" : formObj.border.value + "px";
	}

	formObj.style.value = dom.serializeStyle(st);
}

function changedStyle() {
	var formObj = document.forms[0];
	var st = dom.parseStyle(formObj.style.value);

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

tinyMCEPopup.onInit.add(init);
