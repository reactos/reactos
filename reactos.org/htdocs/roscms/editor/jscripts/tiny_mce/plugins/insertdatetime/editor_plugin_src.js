/**
 * $Id: editor_plugin_src.js 201 2007-02-12 15:56:56Z spocke $
 *
 * @author Moxiecode
 * @copyright Copyright © 2004-2007, Moxiecode Systems AB, All rights reserved.
 */

/* Import plugin specific language pack */
tinyMCE.importPluginLanguagePack('insertdatetime');

var TinyMCE_InsertDateTimePlugin = {
	getInfo : function() {
		return {
			longname : 'Insert date/time',
			author : 'Moxiecode Systems AB',
			authorurl : 'http://tinymce.moxiecode.com',
			infourl : 'http://wiki.moxiecode.com/index.php/TinyMCE:Plugins/insertdatetime',
			version : tinyMCE.majorVersion + "." + tinyMCE.minorVersion
		};
	},

	/**
	 * Returns the HTML contents of the insertdate, inserttime controls.
	 */
	getControlHTML : function(cn) {
		switch (cn) {
			case "insertdate":
				return tinyMCE.getButtonHTML(cn, 'lang_insertdate_desc', '{$pluginurl}/images/insertdate.gif', 'mceInsertDate');

			case "inserttime":
				return tinyMCE.getButtonHTML(cn, 'lang_inserttime_desc', '{$pluginurl}/images/inserttime.gif', 'mceInsertTime');
		}

		return "";
	},

	/**
	 * Executes the mceInsertDate command.
	 */
	execCommand : function(editor_id, element, command, user_interface, value) {
		/* Adds zeros infront of value */
		function addZeros(value, len) {
			value = "" + value;

			if (value.length < len) {
				for (var i=0; i<(len-value.length); i++)
					value = "0" + value;
			}

			return value;
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
			fmt = fmt.replace("%B", "" + tinyMCE.getLang("lang_inserttime_months_long")[d.getMonth()]);
			fmt = fmt.replace("%b", "" + tinyMCE.getLang("lang_inserttime_months_short")[d.getMonth()]);
			fmt = fmt.replace("%A", "" + tinyMCE.getLang("lang_inserttime_day_long")[d.getDay()]);
			fmt = fmt.replace("%a", "" + tinyMCE.getLang("lang_inserttime_day_short")[d.getDay()]);
			fmt = fmt.replace("%%", "%");

			return fmt;
		}

		// Handle commands
		switch (command) {
			case "mceInsertDate":
				tinyMCE.execInstanceCommand(editor_id, 'mceInsertContent', false, getDateTime(new Date(), tinyMCE.getParam("plugin_insertdate_dateFormat", tinyMCE.getLang('lang_insertdate_def_fmt'))));
				return true;

			case "mceInsertTime":
				tinyMCE.execInstanceCommand(editor_id, 'mceInsertContent', false, getDateTime(new Date(), tinyMCE.getParam("plugin_insertdate_timeFormat", tinyMCE.getLang('lang_inserttime_def_fmt'))));
				return true;
		}

		// Pass to next handler in chain
		return false;
	}
};

tinyMCE.addPlugin("insertdatetime", TinyMCE_InsertDateTimePlugin);
