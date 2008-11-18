/**
 * $Id: editor_plugin_src.js 201 2007-02-12 15:56:56Z spocke $
 *
 * @author Moxiecode
 * @copyright Copyright © 2004-2007, Moxiecode Systems AB, All rights reserved.
 */

/* Import plugin specific language pack */
tinyMCE.importPluginLanguagePack('iespell');

var TinyMCE_IESpellPlugin = {
	getInfo : function() {
		return {
			longname : 'IESpell (MSIE Only)',
			author : 'Moxiecode Systems AB',
			authorurl : 'http://tinymce.moxiecode.com',
			infourl : 'http://wiki.moxiecode.com/index.php/TinyMCE:Plugins/iespell',
			version : tinyMCE.majorVersion + "." + tinyMCE.minorVersion
		};
	},

	/**
	 * Returns the HTML contents of the iespell control.
	 */
	getControlHTML : function(cn) {
		// Is it the iespell control and is the brower MSIE.
		if (cn == "iespell" && (tinyMCE.isMSIE && !tinyMCE.isOpera))
			return tinyMCE.getButtonHTML(cn, 'lang_iespell_desc', '{$pluginurl}/images/iespell.gif', 'mceIESpell');

		return "";
	},

	/**
	 * Executes the mceIESpell command.
	 */
	execCommand : function(editor_id, element, command, user_interface, value) {
		// Handle ieSpellCommand
		if (command == "mceIESpell") {
			try {
				var ieSpell = new ActiveXObject("ieSpell.ieSpellExtension");
				ieSpell.CheckDocumentNode(tinyMCE.getInstanceById(editor_id).contentDocument.documentElement);
			} catch (e) {
				if (e.number == -2146827859) {
					if (confirm(tinyMCE.getLang("lang_iespell_download", "", true)))
						window.open('http://www.iespell.com/download.php', 'ieSpellDownload', '');
				} else
					alert("Error Loading ieSpell: Exception " + e.number);
			}

			return true;
		}

		// Pass to next handler in chain
		return false;
	}
};

tinyMCE.addPlugin("iespell", TinyMCE_IESpellPlugin);
