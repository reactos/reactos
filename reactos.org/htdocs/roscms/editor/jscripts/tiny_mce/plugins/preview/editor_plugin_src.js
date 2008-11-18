/**
 * $Id: editor_plugin_src.js 201 2007-02-12 15:56:56Z spocke $
 *
 * @author Moxiecode
 * @copyright Copyright © 2004-2007, Moxiecode Systems AB, All rights reserved.
 */

/* Import plugin specific language pack */
tinyMCE.importPluginLanguagePack('preview');

var TinyMCE_PreviewPlugin = {
	getInfo : function() {
		return {
			longname : 'Preview',
			author : 'Moxiecode Systems AB',
			authorurl : 'http://tinymce.moxiecode.com',
			infourl : 'http://wiki.moxiecode.com/index.php/TinyMCE:Plugins/preview',
			version : tinyMCE.majorVersion + "." + tinyMCE.minorVersion
		};
	},

	/**
	 * Returns the HTML contents of the preview control.
	 */
	getControlHTML : function(cn) {
		switch (cn) {
			case "preview":
				return tinyMCE.getButtonHTML(cn, 'lang_preview_desc', '{$pluginurl}/images/preview.gif', 'mcePreview');
		}

		return "";
	},

	/**
	 * Executes the mcePreview command.
	 */
	execCommand : function(editor_id, element, command, user_interface, value) {
		// Handle commands
		switch (command) {
			case "mcePreview":
				var previewPage = tinyMCE.getParam("plugin_preview_pageurl", null);
				var previewWidth = tinyMCE.getParam("plugin_preview_width", "550");
				var previewHeight = tinyMCE.getParam("plugin_preview_height", "600");

				// Use a custom preview page
				if (previewPage) {
					var template = new Array();

					template['file'] = previewPage;
					template['width'] = previewWidth;
					template['height'] = previewHeight;

					tinyMCE.openWindow(template, {editor_id : editor_id, resizable : "yes", scrollbars : "yes", inline : "yes", content : tinyMCE.getContent(), content_css : tinyMCE.getParam("content_css")});
				} else {
					var win = window.open("", "mcePreview", "menubar=no,toolbar=no,scrollbars=yes,resizable=yes,left=20,top=20,width=" + previewWidth + ",height="  + previewHeight);
					var html = "", i;
					var c = tinyMCE.getContent();
					var pos = c.indexOf('<body'), pos2, css = tinyMCE.getParam("content_css").split(',');

					if (pos != -1) {
						pos = c.indexOf('>', pos);
						pos2 = c.lastIndexOf('</body>');
						c = c.substring(pos + 1, pos2);
					}

					html += tinyMCE.getParam('doctype');
					html += '<html xmlns="http://www.w3.org/1999/xhtml">';
					html += '<head>';
					html += '<title>' + tinyMCE.getLang('lang_preview_desc') + '</title>';
					html += '<base href="' + tinyMCE.settings['base_href'] + '" />';
					html += '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />';

					for (i=0; i<css.length; i++)
						html += '<link href="' + css[i] + '" rel="stylesheet" type="text/css" />';

					html += '<script type="text/javascript">';
					html += 'window.opener.TinyMCE_PreviewPlugin._setDoc(document);';
					html += 'window.opener.TinyMCE_PreviewPlugin._setWin(window);';
					html += 'writeFlash = window.opener.TinyMCE_PreviewPlugin._writeFlash;';
					html += 'writeShockWave = window.opener.TinyMCE_PreviewPlugin._writeShockWave;';
					html += 'writeQuickTime = window.opener.TinyMCE_PreviewPlugin._writeQuickTime;';
					html += 'writeRealMedia = window.opener.TinyMCE_PreviewPlugin._writeRealMedia;';
					html += 'writeWindowsMedia = window.opener.TinyMCE_PreviewPlugin._writeWindowsMedia;';
					html += 'writeEmbed = window.opener.TinyMCE_PreviewPlugin._writeEmbed;';
					html += '</script>';
					html += '</head>';
					html += '<body dir="' + tinyMCE.getParam("directionality") + '" onload="window.opener.TinyMCE_PreviewPlugin._onLoad();">';
					html += c;
					html += '</body>';
					html += '</html>';

					win.document.write(html);
					win.document.close();
				}

				return true;
		}

		return false;
	},

	_setDoc : function(d) {
		TinyMCE_PreviewPlugin._doc = d;
		d._embeds = new Array();
	},

	_setWin : function(d) {
		TinyMCE_PreviewPlugin._win = d;
	},

	_onLoad : function() {
		var nl, i, el = new Array(), d = TinyMCE_PreviewPlugin._doc, sv, ne;

		nl = d.getElementsByTagName("script");
		for (i=0; i<nl.length; i++) {
			sv = tinyMCE.isMSIE ? nl[i].innerHTML : nl[i].firstChild.nodeValue;

			if (new RegExp('write(Flash|ShockWave|WindowsMedia|QuickTime|RealMedia)\\(.*', 'g').test(sv))
				el[el.length] = nl[i];
		}

		for (i=0; i<el.length; i++) {
			ne = d.createElement("div");
			ne.innerHTML = d._embeds[i];
			el[i].parentNode.insertBefore(ne.firstChild, el[i]);
		}
	},

	_writeFlash : function(p) {
		p.src = tinyMCE.convertRelativeToAbsoluteURL(tinyMCE.settings['base_href'], p.src);
		TinyMCE_PreviewPlugin._writeEmbed(
			'D27CDB6E-AE6D-11cf-96B8-444553540000',
			'http://download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab#version=6,0,40,0',
			'application/x-shockwave-flash',
			p
		);
	},

	_writeShockWave : function(p) {
		p.src = tinyMCE.convertRelativeToAbsoluteURL(tinyMCE.settings['base_href'], p.src);
		TinyMCE_PreviewPlugin._writeEmbed(
			'166B1BCA-3F9C-11CF-8075-444553540000',
			'http://download.macromedia.com/pub/shockwave/cabs/director/sw.cab#version=8,5,1,0',
			'application/x-director',
			p
		);
	},

	_writeQuickTime : function(p) {
		p.src = tinyMCE.convertRelativeToAbsoluteURL(tinyMCE.settings['base_href'], p.src);
		TinyMCE_PreviewPlugin._writeEmbed(
			'02BF25D5-8C17-4B23-BC80-D3488ABDDC6B',
			'http://www.apple.com/qtactivex/qtplugin.cab#version=6,0,2,0',
			'video/quicktime',
			p
		);
	},

	_writeRealMedia : function(p) {
		p.src = tinyMCE.convertRelativeToAbsoluteURL(tinyMCE.settings['base_href'], p.src);
		TinyMCE_PreviewPlugin._writeEmbed(
			'CFCDAA03-8BE4-11cf-B84B-0020AFBBCCFA',
			'http://download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab#version=6,0,40,0',
			'audio/x-pn-realaudio-plugin',
			p
		);
	},

	_writeWindowsMedia : function(p) {
		p.src = tinyMCE.convertRelativeToAbsoluteURL(tinyMCE.settings['base_href'], p.src);
		p.url = p.src;
		TinyMCE_PreviewPlugin._writeEmbed(
			'6BF52A52-394A-11D3-B153-00C04F79FAA6',
			'http://activex.microsoft.com/activex/controls/mplayer/en/nsmp2inf.cab#Version=5,1,52,701',
			'application/x-mplayer2',
			p
		);
	},

	_writeEmbed : function(cls, cb, mt, p) {
		var h = '', n, d = TinyMCE_PreviewPlugin._doc, ne, c;

		h += '<object classid="clsid:' + cls + '" codebase="' + cb + '"';
		h += typeof(p.id) != "undefined" ? 'id="' + p.id + '"' : '';
		h += typeof(p.name) != "undefined" ? 'name="' + p.name + '"' : '';
		h += typeof(p.width) != "undefined" ? 'width="' + p.width + '"' : '';
		h += typeof(p.height) != "undefined" ? 'height="' + p.height + '"' : '';
		h += typeof(p.align) != "undefined" ? 'align="' + p.align + '"' : '';
		h += '>';

		for (n in p)
			h += '<param name="' + n + '" value="' + p[n] + '">';

		h += '<embed type="' + mt + '"';

		for (n in p)
			h += n + '="' + p[n] + '" ';

		h += '></embed></object>';

		d._embeds[d._embeds.length] = h;
	}
};

tinyMCE.addPlugin("preview", TinyMCE_PreviewPlugin);
