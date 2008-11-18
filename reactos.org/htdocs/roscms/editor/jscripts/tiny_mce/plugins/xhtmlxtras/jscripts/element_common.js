 /**
 * $Id: editor_plugin_src.js 42 2006-08-08 14:32:24Z spocke $
 *
 * @author Moxiecode - based on work by Andrew Tetlaw
 * @copyright Copyright © 2004-2007, Moxiecode Systems AB, All rights reserved.
 */

function initCommonAttributes(elm) {
	var formObj = document.forms[0];

	// Setup form data for common element attributes
	setFormValue('title', tinyMCE.getAttrib(elm, 'title'));
	setFormValue('id', tinyMCE.getAttrib(elm, 'id'));
	selectByValue(formObj, 'class', tinyMCE.getAttrib(elm, 'class'), true);
	setFormValue('style', tinyMCE.getAttrib(elm, 'style'));
	selectByValue(formObj, 'dir', tinyMCE.getAttrib(elm, 'dir'));
	setFormValue('lang', tinyMCE.getAttrib(elm, 'lang'));
	setFormValue('onfocus', tinyMCE.getAttrib(elm, 'onfocus'));
	setFormValue('onblur', tinyMCE.getAttrib(elm, 'onblur'));
	setFormValue('onclick', tinyMCE.getAttrib(elm, 'onclick'));
	setFormValue('ondblclick', tinyMCE.getAttrib(elm, 'ondblclick'));
	setFormValue('onmousedown', tinyMCE.getAttrib(elm, 'onmousedown'));
	setFormValue('onmouseup', tinyMCE.getAttrib(elm, 'onmouseup'));
	setFormValue('onmouseover', tinyMCE.getAttrib(elm, 'onmouseover'));
	setFormValue('onmousemove', tinyMCE.getAttrib(elm, 'onmousemove'));
	setFormValue('onmouseout', tinyMCE.getAttrib(elm, 'onmouseout'));
	setFormValue('onkeypress', tinyMCE.getAttrib(elm, 'onkeypress'));
	setFormValue('onkeydown', tinyMCE.getAttrib(elm, 'onkeydown'));
	setFormValue('onkeyup', tinyMCE.getAttrib(elm, 'onkeyup'));
}

function setFormValue(name, value) {
	if(document.forms[0].elements[name]) document.forms[0].elements[name].value = value;
}

function insertDateTime(id) {
	document.getElementById(id).value = getDateTime(new Date(), "%Y-%m-%dT%H:%M:%S");
}

function getDateTime(d, fmt) {
	fmt = fmt.replace("%D", "%m/%d/%y");
	fmt = fmt.replace("%r", "%I:%M:%S %p");
	fmt = fmt.replace("%Y", "" + d.getFullYear());
	fmt = fmt.replace("%y", "" + d.getYear());
	fmt = fmt.replace("%m", addZeros(d.getMonth()+1, 2));
	fmt = fmt.replace("%d", addZeros(d.getDate(), 2));
	fmt = fmt.replace("%H", "" + addZeros(d.getHours(), 2));
	fmt = fmt.replace("%M", "" + addZeros(d.getMinutes(), 2));
	fmt = fmt.replace("%S", "" + addZeros(d.getSeconds(), 2));
	fmt = fmt.replace("%I", "" + ((d.getHours() + 11) % 12 + 1));
	fmt = fmt.replace("%p", "" + (d.getHours() < 12 ? "AM" : "PM"));
	fmt = fmt.replace("%%", "%");

	return fmt;
}

function addZeros(value, len) {
	var i;

	value = "" + value;

	if (value.length < len) {
		for (i=0; i<(len-value.length); i++)
			value = "0" + value;
	}

	return value;
}

function selectByValue(form_obj, field_name, value, add_custom, ignore_case) {
	if (!form_obj || !form_obj.elements[field_name])
		return;

	var sel = form_obj.elements[field_name];

	var found = false;
	for (var i=0; i<sel.options.length; i++) {
		var option = sel.options[i];

		if (option.value == value || (ignore_case && option.value.toLowerCase() == value.toLowerCase())) {
			option.selected = true;
			found = true;
		} else
			option.selected = false;
	}

	if (!found && add_custom && value != '') {
		var option = new Option('Value: ' + value, value);
		option.selected = true;
		sel.options[sel.options.length] = option;
	}

	return found;
}

function setAttrib(elm, attrib, value) {
	var formObj = document.forms[0];
	var valueElm = formObj.elements[attrib.toLowerCase()];

	if (typeof(value) == "undefined" || value == null) {
		value = "";

		if (valueElm)
			value = valueElm.value;
	}

	if (value != "") {
		if (attrib == "style")
			attrib = "style.cssText";

		if (attrib.substring(0, 2) == 'on')
			value = 'return true;' + value;

		if (attrib == "class") {
			tinyMCE.addCSSClass(elm, value);
			return;
		}

		elm.setAttribute(attrib.toLowerCase(), value);
	} else
		elm.removeAttribute(attrib);
}

function setAllCommonAttribs(elm) {
	setAttrib(elm, 'title');
	setAttrib(elm, 'id');
	setAttrib(elm, 'class');
	setAttrib(elm, 'style');
	setAttrib(elm, 'dir');
	setAttrib(elm, 'lang');
	/*setAttrib(elm, 'onfocus');
	setAttrib(elm, 'onblur');
	setAttrib(elm, 'onclick');
	setAttrib(elm, 'ondblclick');
	setAttrib(elm, 'onmousedown');
	setAttrib(elm, 'onmouseup');
	setAttrib(elm, 'onmouseover');
	setAttrib(elm, 'onmousemove');
	setAttrib(elm, 'onmouseout');
	setAttrib(elm, 'onkeypress');
	setAttrib(elm, 'onkeydown');
	setAttrib(elm, 'onkeyup');*/
}

SXE = {
	currentAction : "insert",
	inst : tinyMCE.getInstanceById(tinyMCE.getWindowArg('editor_id')),
	updateElement : null
}

SXE.focusElement = SXE.inst.getFocusElement();

SXE.initElementDialog = function(element_name) {
	addClassesToList('class', 'xhtmlxtras_styles');
	TinyMCE_EditableSelects.init();

	element_name = element_name.toLowerCase();
	var elm = tinyMCE.getParentElement(SXE.focusElement, element_name);
	if (elm != null && elm.nodeName == element_name.toUpperCase()) {
		SXE.currentAction = "update";
	}

	if (SXE.currentAction == "update") {
		initCommonAttributes(elm);
		SXE.updateElement = elm;
	}

	document.forms[0].insert.value = tinyMCE.getLang('lang_' + SXE.currentAction, 'Insert', true); 
}

SXE.insertElement = function(element_name) {
	var elm = tinyMCE.getParentElement(SXE.focusElement, element_name), h, tagName;

	tinyMCEPopup.execCommand('mceBeginUndoLevel');
	if (elm == null) {
		var s = SXE.inst.selection.getSelectedHTML();
		if(s.length > 0) {
			tagName = element_name;

			if (tinyMCE.isIE && !tinyMCE.isOpera && element_name.indexOf('html:') == 0)
				element_name = element_name.substring(5).toLowerCase();

			h = '<' + tagName + ' id="#sxe_temp_' + element_name + '#">' + s + '</' + tagName + '>';

			tinyMCEPopup.execCommand('mceInsertContent', false, h);

			var elementArray = tinyMCE.getElementsByAttributeValue(SXE.inst.getBody(), element_name, 'id', '#sxe_temp_' + element_name + '#');
			for (var i=0; i<elementArray.length; i++) {
				var elm = elementArray[i];

				elm.id = '';
				elm.setAttribute('id', '');
				elm.removeAttribute('id');

				setAllCommonAttribs(elm);
			}
		}
	} else {
		setAllCommonAttribs(elm);
	}
	tinyMCE.triggerNodeChange();
	tinyMCEPopup.execCommand('mceEndUndoLevel');
}

SXE.removeElement = function(element_name){
	element_name = element_name.toLowerCase();
	elm = tinyMCE.getParentElement(SXE.focusElement, element_name);
	if(elm && elm.nodeName == element_name.toUpperCase()){
		tinyMCEPopup.execCommand('mceBeginUndoLevel');
		tinyMCE.execCommand('mceRemoveNode', false, elm);
		tinyMCE.triggerNodeChange();
		tinyMCEPopup.execCommand('mceEndUndoLevel');
	}
}

SXE.showRemoveButton = function() {
		document.getElementById("remove").style.display = 'block';
}

SXE.containsClass = function(elm,cl) {
	return (elm.className.indexOf(cl) > -1) ? true : false;
}

SXE.removeClass = function(elm,cl) {
	if(elm.className == null || elm.className == "" || !SXE.containsClass(elm,cl)) {
		return true;
	}
	var classNames = elm.className.split(" ");
	var newClassNames = "";
	for (var x = 0, cnl = classNames.length; x < cnl; x++) {
		if (classNames[x] != cl) {
			newClassNames += (classNames[x] + " ");
		}
	}
	elm.className = newClassNames.substring(0,newClassNames.length-1); //removes extra space at the end
}

SXE.addClass = function(elm,cl) {
	if(!SXE.containsClass(elm,cl)) elm.className ? elm.className += " " + cl : elm.className = cl;
	return true;
}