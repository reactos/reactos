/**
 * $Id: editor_plugin_src.js 201 2007-02-12 15:56:56Z spocke $
 *
 * @author Moxiecode
 * @copyright Copyright © 2004-2007, Moxiecode Systems AB, All rights reserved.
 */

/* Import plugin specific language pack */
tinyMCE.importPluginLanguagePack('style');

var TinyMCE_StylePlugin = {
	getInfo : function() {
		return {
			longname : 'Style',
			author : 'Moxiecode Systems AB',
			authorurl : 'http://tinymce.moxiecode.com',
			infourl : 'http://wiki.moxiecode.com/index.php/TinyMCE:Plugins/style',
			version : tinyMCE.majorVersion + "." + tinyMCE.minorVersion
		};
	},

	getControlHTML : function(cn) {
		switch (cn) {
			case "styleprops":
				return tinyMCE.getButtonHTML(cn, 'lang_style_styleinfo_desc', '{$pluginurl}/images/styleprops.gif', 'mceStyleProps', true);
		}

		return "";
	},

	execCommand : function(editor_id, element, command, user_interface, value) {
		var e, inst;

		// Handle commands
		switch (command) {
			case "mceStyleProps":
				TinyMCE_StylePlugin._styleProps();
				return true;

			case "mceSetElementStyle":
				inst = tinyMCE.getInstanceById(editor_id);
				e = inst.selection.getFocusElement();

				if (e) {
					e.style.cssText = value;
					inst.repaint();
				}

				return true;
		}

		// Pass to next handler in chain
		return false;
	},

	handleNodeChange : function(editor_id, node, undo_index, undo_levels, visual_aid, any_selection) {
		if (node.nodeName == 'BODY')
			tinyMCE.switchClass(editor_id + '_styleprops', 'mceButtonDisabled');
		else
			tinyMCE.switchClass(editor_id + '_styleprops', 'mceButtonNormal');
	},

	// Private plugin specific methods

	_styleProps : function() {
		var e = tinyMCE.selectedInstance.selection.getFocusElement();

		if (!e || e.nodeName == 'BODY')
			return;

		tinyMCE.openWindow({
			file : '../../plugins/style/props.htm',
			width : 480 + tinyMCE.getLang('lang_style_props_delta_width', 0),
			height : 320 + tinyMCE.getLang('lang_style_props_delta_height', 0)
		}, {
			editor_id : tinyMCE.selectedInstance.editorId,
			inline : "yes",
			style_text : e.style.cssText
		});
	}
};

tinyMCE.addPlugin("style", TinyMCE_StylePlugin);
