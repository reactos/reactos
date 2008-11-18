/**
 * $Id: editor_plugin_src.js 42 2006-08-08 14:32:24Z spocke $
 *
 * @author Moxiecode
 * @copyright Copyright © 2004-2007, Moxiecode Systems AB, All rights reserved.
 */

/* Import plugin specific language pack */
tinyMCE.importPluginLanguagePack('visualchars');

var TinyMCE_VisualCharsPlugin = {
	getInfo : function() {
		return {
			longname : 'Visual characters',
			author : 'Moxiecode Systems AB',
			authorurl : 'http://tinymce.moxiecode.com',
			infourl : 'http://wiki.moxiecode.com/index.php/TinyMCE:Plugins/visualchars',
			version : tinyMCE.majorVersion + "." + tinyMCE.minorVersion
		};
	},

	initInstance : function(inst) {
		inst.visualChars = {
			state : false
		};
	},

	getControlHTML : function(cn) {
		switch (cn) {
			case "visualchars":
				return tinyMCE.getButtonHTML(cn, 'lang_visualchars_desc', '{$pluginurl}/images/visualchars.gif', 'mceVisualChars', false);
		}

		return "";
	},

	execCommand : function(editor_id, element, command, user_interface, value) {
		var inst = tinyMCE.getInstanceById(editor_id);

		switch (command) {
			case "mceVisualChars":
				this._toggleVisualChars(editor_id, inst);
				return true;
		}

		return false;
	},

	cleanup : function(type, content, inst) {
		if (type == "insert_to_editor_dom" || type == "get_from_editor_dom") {
			inst.visualChars.state = true;
			this._toggleVisualChars(inst.editorId, inst);
		}

		return content;
	},

	// Private plugin internal methods

	_toggleVisualChars : function(editor_id, inst) {
		var nl, i, h, d = inst.getDoc(), b = inst.getBody(), nv, s = inst.selection, bo;

		inst.visualChars.state = !inst.visualChars.state;

		bo = s.getBookmark(true);

		tinyMCE.switchClass(editor_id + '_visualchars', inst.visualChars.state ? 'mceButtonSelected' : 'mceButtonNormal');

		if (inst.visualChars.state) {
			nl = tinyMCE.selectNodes(b, function(n) {return n.nodeType == 3 && n.nodeValue && n.nodeValue.indexOf('\u00a0') != -1;});

			for (i=0; i<nl.length; i++) {
				nv = nl[i].nodeValue;
				nv = nv.replace(/(\u00a0+)/g, '<span class="mceItemHiddenVisualChar">$1</span>');
				nv = nv.replace(/\u00a0/g, '\u00b7');
				tinyMCE.setOuterHTML(nl[i], nv, d);
			}
		} else {
			nl = tinyMCE.selectNodes(b, function(n) {return n.nodeType == 1 && n.nodeName == 'SPAN' && n.className == 'mceItemHiddenVisualChar';});

			for (i=0; i<nl.length; i++)
				tinyMCE.setOuterHTML(nl[i], nl[i].innerHTML.replace(/(&middot;|\u00b7)/g, '&nbsp;'), d);
		}

		//s.moveToBookmark(bo);
	}
};

tinyMCE.addPlugin("visualchars", TinyMCE_VisualCharsPlugin);
