 /**
 * $Id: editor_plugin_src.js 42 2006-08-08 14:32:24Z spocke $
 *
 * @author Moxiecode - based on work by Andrew Tetlaw
 * @copyright Copyright © 2004-2006, Moxiecode Systems AB, All rights reserved.
 */

function preinit() {
	// Initialize
	tinyMCE.setWindowArg('mce_windowresize', false);
}

function init() {
	tinyMCEPopup.resizeToInnerSize();
	var inst = tinyMCE.getInstanceById(tinyMCE.getWindowArg('editor_id'));
	var elm = inst.getFocusElement();

	var f = document.forms[0];
	
	var onclick = tinyMCE.cleanupEventStr(tinyMCE.getAttrib(elm, 'onclick'));

	setFormValue('title', tinyMCE.getAttrib(elm, 'title'));
	setFormValue('id', tinyMCE.getAttrib(elm, 'id'));
	setFormValue('style', tinyMCE.serializeStyle(tinyMCE.parseStyle(tinyMCE.getAttrib(elm, "style"))));
	setFormValue('dir', tinyMCE.getAttrib(elm, 'dir'));
	setFormValue('lang', tinyMCE.getAttrib(elm, 'lang'));
	setFormValue('tabindex', tinyMCE.getAttrib(elm, 'tabindex', typeof(elm.tabindex) != "undefined" ? elm.tabindex : ""));
	setFormValue('accesskey', tinyMCE.getAttrib(elm, 'accesskey', typeof(elm.accesskey) != "undefined" ? elm.accesskey : ""));
	setFormValue('onfocus', tinyMCE.cleanupEventStr(tinyMCE.getAttrib(elm, 'onfocus')));
	setFormValue('onblur', tinyMCE.cleanupEventStr(tinyMCE.getAttrib(elm, 'onblur')));
	setFormValue('onclick', onclick);
	setFormValue('ondblclick', tinyMCE.cleanupEventStr(tinyMCE.getAttrib(elm, 'ondblclick')));
	setFormValue('onmousedown', tinyMCE.cleanupEventStr(tinyMCE.getAttrib(elm, 'onmousedown')));
	setFormValue('onmouseup', tinyMCE.cleanupEventStr(tinyMCE.getAttrib(elm, 'onmouseup')));
	setFormValue('onmouseover', tinyMCE.cleanupEventStr(tinyMCE.getAttrib(elm, 'onmouseover')));
	setFormValue('onmousemove', tinyMCE.cleanupEventStr(tinyMCE.getAttrib(elm, 'onmousemove')));
	setFormValue('onmouseout', tinyMCE.cleanupEventStr(tinyMCE.getAttrib(elm, 'onmouseout')));
	setFormValue('onkeypress', tinyMCE.cleanupEventStr(tinyMCE.getAttrib(elm, 'onkeypress')));
	setFormValue('onkeydown', tinyMCE.cleanupEventStr(tinyMCE.getAttrib(elm, 'onkeydown')));
	setFormValue('onkeyup', tinyMCE.cleanupEventStr(tinyMCE.getAttrib(elm, 'onkeyup')));
	
	className = tinyMCE.getVisualAidClass(tinyMCE.getAttrib(elm, 'class'), false);
		
	addClassesToList('classlist', 'advlink_styles');
	selectByValue(f, 'classlist', className, true);
	
	TinyMCE_EditableSelects.init();
}

function setFormValue(name, value) {
	if(value && document.forms[0].elements[name]){
		document.forms[0].elements[name].value = value;
	}
}

function insertAction() {
	var inst = tinyMCE.getInstanceById(tinyMCE.getWindowArg('editor_id'));
	var elm = inst.getFocusElement();

	tinyMCEPopup.execCommand("mceBeginUndoLevel");	
	tinyMCEPopup.restoreSelection();
	
	setAllAttribs(elm);
	
	tinyMCE.handleVisualAid(inst.getBody(), true, inst.visualAid, inst);
	tinyMCE._setEventsEnabled(inst.getBody(), false);
	tinyMCEPopup.execCommand("mceEndUndoLevel");
	tinyMCEPopup.close();
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
		tinyMCE.setAttrib(elm, attrib.toLowerCase(), value);

		if (attrib == "style")
			attrib = "style.cssText";

		if (attrib.substring(0, 2) == 'on')
			value = 'return true;' + value;

		if (attrib == "class")
			attrib = "className";

		eval('elm.' + attrib + "=value;");
	} else
		elm.removeAttribute(attrib);
}

function setAllAttribs(elm) {
	var f = document.forms[0];

	setAttrib(elm, 'title');
	setAttrib(elm, 'id');
	setAttrib(elm, 'style');
	setAttrib(elm, 'class', getSelectValue(f, 'classlist'));
	setAttrib(elm, 'dir');
	setAttrib(elm, 'lang');
	setAttrib(elm, 'tabindex');
	setAttrib(elm, 'accesskey');
	setAttrib(elm, 'onfocus');
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
	setAttrib(elm, 'onkeyup');

	// Refresh in old MSIE
	if (tinyMCE.isMSIE5)
		elm.outerHTML = elm.outerHTML;
}

function insertAttribute() {
	tinyMCEPopup.close();
}