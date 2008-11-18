/**
 * $Id: editor_plugin_src.js 201 2007-02-12 15:56:56Z spocke $
 *
 * @author Moxiecode
 * @copyright Copyright © 2004-2007, Moxiecode Systems AB, All rights reserved.
 */

/* Import plugin specific language pack */
tinyMCE.importPluginLanguagePack('media');

var TinyMCE_MediaPlugin = {
	getInfo : function() {
		return {
			longname : 'Media',
			author : 'Moxiecode Systems AB',
			authorurl : 'http://tinymce.moxiecode.com',
			infourl : 'http://wiki.moxiecode.com/index.php/TinyMCE:Plugins/media',
			version : tinyMCE.majorVersion + "." + tinyMCE.minorVersion
		};
	},

	initInstance : function(inst) {
		// Warn if user has flash plugin and media plugin at the same time
		if (inst.hasPlugin('flash') && !tinyMCE.flashWarn) {
			alert('Flash plugin is deprecated and should not be used together with the media plugin.');
			tinyMCE.flashWarn = true;
		}

		if (!tinyMCE.settings['media_skip_plugin_css'])
			tinyMCE.importCSS(inst.getDoc(), tinyMCE.baseURL + "/plugins/media/css/content.css");
	},

	getControlHTML : function(cn) {
		switch (cn) {
			case "media":
				return tinyMCE.getButtonHTML(cn, 'lang_media_desc', '{$pluginurl}/images/media.gif', 'mceMedia');
		}

		return "";
	},

	execCommand : function(editor_id, element, command, user_interface, value) {
		// Handle commands
		switch (command) {
			case "mceMedia":
				tinyMCE.openWindow({
						file : '../../plugins/media/media.htm',
						width : 430 + tinyMCE.getLang('lang_media_delta_width', 0),
						height : 470 + tinyMCE.getLang('lang_media_delta_height', 0)
					}, {
						editor_id : editor_id,
						inline : "yes"
				});

				return true;
	   }

	   // Pass to next handler in chain
	   return false;
	},

	cleanup : function(type, content, inst) {
		var nl, img, i, ne, d, s, ci;

		switch (type) {
			case "insert_to_editor":
				img = tinyMCE.getParam("theme_href") + '/images/spacer.gif';
				content = content.replace(/<script[^>]*>\s*write(Flash|ShockWave|WindowsMedia|QuickTime|RealMedia)\(\{([^\)]*)\}\);\s*<\/script>/gi, '<img class="mceItem$1" title="$2" src="' + img + '" />');
				content = content.replace(/<object([^>]*)>/gi, '<div class="mceItemObject" $1>');
				content = content.replace(/<embed([^>]*)>/gi, '<div class="mceItemObjectEmbed" $1>');
				content = content.replace(/<\/(object|embed)([^>]*)>/gi, '</div>');
				content = content.replace(/<param([^>]*)>/gi, '<div $1 class="mceItemParam"></div>');
				content = content.replace(new RegExp('\\/ class="mceItemParam"><\\/div>', 'gi'), 'class="mceItemParam"></div>');
				break;

			case "insert_to_editor_dom":
				d = inst.getDoc();
				nl = content.getElementsByTagName("img");
				for (i=0; i<nl.length; i++) {
					if (/mceItem(Flash|ShockWave|WindowsMedia|QuickTime|RealMedia)/.test(nl[i].className)) {
						nl[i].width = nl[i].title.replace(/.*width:[^0-9]?([0-9]+)%?.*/g, '$1');
						nl[i].height = nl[i].title.replace(/.*height:[^0-9]?([0-9]+)%?.*/g, '$1');
						//nl[i].align = nl[i].title.replace(/.*align:([a-z]+).*/gi, '$1');
					}
				}

				nl = tinyMCE.selectElements(content, 'DIV', function (n) {return tinyMCE.hasCSSClass(n, 'mceItemObject');});
				for (i=0; i<nl.length; i++) {
					ci = tinyMCE.getAttrib(nl[i], "classid").toLowerCase().replace(/\s+/g, '');

					switch (ci) {
						case 'clsid:d27cdb6e-ae6d-11cf-96b8-444553540000':
							nl[i].parentNode.replaceChild(TinyMCE_MediaPlugin._createImg('mceItemFlash', d, nl[i]), nl[i]);
							break;

						case 'clsid:166b1bca-3f9c-11cf-8075-444553540000':
							nl[i].parentNode.replaceChild(TinyMCE_MediaPlugin._createImg('mceItemShockWave', d, nl[i]), nl[i]);
							break;

						case 'clsid:6bf52a52-394a-11d3-b153-00c04f79faa6':
							nl[i].parentNode.replaceChild(TinyMCE_MediaPlugin._createImg('mceItemWindowsMedia', d, nl[i]), nl[i]);
							break;

						case 'clsid:02bf25d5-8c17-4b23-bc80-d3488abddc6b':
							nl[i].parentNode.replaceChild(TinyMCE_MediaPlugin._createImg('mceItemQuickTime', d, nl[i]), nl[i]);
							break;

						case 'clsid:cfcdaa03-8be4-11cf-b84b-0020afbbccfa':
						case 'clsid:22d6f312-b0f6-11d0-94ab-0080c74c7e95':
						case 'clsid:05589fa1-c356-11ce-bf01-00aa0055595a':
							nl[i].parentNode.replaceChild(TinyMCE_MediaPlugin._createImg('mceItemRealMedia', d, nl[i]), nl[i]);
							break;
					}
				}

				// Handle embed (if any)
				nl = tinyMCE.selectNodes(content, function (n) {return n.className == 'mceItemObjectEmbed';});
				for (i=0; i<nl.length; i++) {
					switch (tinyMCE.getAttrib(nl[i], 'type')) {
						case 'application/x-shockwave-flash':
							TinyMCE_MediaPlugin._createImgFromEmbed(nl[i], d, 'mceItemFlash');
							break;

						case 'application/x-director':
							TinyMCE_MediaPlugin._createImgFromEmbed(nl[i], d, 'mceItemShockWave');
							break;

						case 'application/x-mplayer2':
							TinyMCE_MediaPlugin._createImgFromEmbed(nl[i], d, 'mceItemWindowsMedia');
							break;

						case 'video/quicktime':
							TinyMCE_MediaPlugin._createImgFromEmbed(nl[i], d, 'mceItemQuickTime');
							break;

						case 'audio/x-pn-realaudio-plugin':
							TinyMCE_MediaPlugin._createImgFromEmbed(nl[i], d, 'mceItemRealMedia');
							break;
					}
				}
				break;

			case "get_from_editor":
				var startPos = -1, endPos, attribs, chunkBefore, chunkAfter, embedHTML, at, pl, cb, mt, ex;

				while ((startPos = content.indexOf('<img', startPos+1)) != -1) {
					endPos = content.indexOf('/>', startPos);
					attribs = TinyMCE_MediaPlugin._parseAttributes(content.substring(startPos + 4, endPos));

					// Is not flash, skip it
					if (!/mceItem(Flash|ShockWave|WindowsMedia|QuickTime|RealMedia)/.test(attribs['class']))
						continue;

					endPos += 2;

					// Parse attributes
					at = attribs['title'];
					if (at) {
						at = at.replace(/&(#39|apos);/g, "'");
						at = at.replace(/&#quot;/g, '"');

						try {
							pl = eval('x={' + at + '};');
						} catch (ex) {
							pl = {};
						}
					}

					// Use object/embed
					if (!tinyMCE.getParam('media_use_script', false)) {
						switch (attribs['class']) {
							case 'mceItemFlash':
								ci = 'd27cdb6e-ae6d-11cf-96b8-444553540000';
								cb = 'http://download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab#version=6,0,40,0';
								mt = 'application/x-shockwave-flash';
								break;

							case 'mceItemShockWave':
								ci = '166B1BCA-3F9C-11CF-8075-444553540000';
								cb = 'http://download.macromedia.com/pub/shockwave/cabs/director/sw.cab#version=8,5,1,0';
								mt = 'application/x-director';
								break;

							case 'mceItemWindowsMedia':
								ci = tinyMCE.getParam('media_wmp6_compatible') ? '05589FA1-C356-11CE-BF01-00AA0055595A' : '6BF52A52-394A-11D3-B153-00C04F79FAA6';
								cb = 'http://activex.microsoft.com/activex/controls/mplayer/en/nsmp2inf.cab#Version=5,1,52,701';
								mt = 'application/x-mplayer2';
								break;

							case 'mceItemQuickTime':
								ci = '02BF25D5-8C17-4B23-BC80-D3488ABDDC6B';
								cb = 'http://www.apple.com/qtactivex/qtplugin.cab#version=6,0,2,0';
								mt = 'video/quicktime';
								break;

							case 'mceItemRealMedia':
								ci = 'CFCDAA03-8BE4-11cf-B84B-0020AFBBCCFA';
								cb = 'http://download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab#version=6,0,40,0';
								mt = 'audio/x-pn-realaudio-plugin';
								break;
						}

						// Force absolute URL
						if (!tinyMCE.getParam("relative_urls"))
							pl.src = tinyMCE.convertRelativeToAbsoluteURL(tinyMCE.settings['base_href'], pl.src);

						embedHTML = TinyMCE_MediaPlugin._getEmbed(ci, cb, mt, pl, attribs);
					} else {
						// Use script version
						switch (attribs['class']) {
							case 'mceItemFlash':
								s = 'writeFlash';
								break;

							case 'mceItemShockWave':
								s = 'writeShockWave';
								break;

							case 'mceItemWindowsMedia':
								s = 'writeWindowsMedia';
								break;

							case 'mceItemQuickTime':
								s = 'writeQuickTime';
								break;

							case 'mceItemRealMedia':
								s = 'writeRealMedia';
								break;
						}

						if (attribs.width)
							at = at.replace(/width:[^0-9]?[0-9]+%?[^0-9]?/g, "width:'" + attribs.width + "'");

						if (attribs.height)
							at = at.replace(/height:[^0-9]?[0-9]+%?[^0-9]?/g, "height:'" + attribs.height + "'");

						// Force absolute URL
						if (!tinyMCE.getParam("relative_urls")) {
							pl.src = tinyMCE.convertRelativeToAbsoluteURL(tinyMCE.settings['base_href'], pl.src);
							at = at.replace(new RegExp("src:'[^']*'", "g"), "src:'" + pl.src + "'");
						}

						embedHTML = '<script type="text/javascript">' + s + '({' + at + '});</script>';
					}

					// Insert embed/object chunk
					chunkBefore = content.substring(0, startPos);
					chunkAfter = content.substring(endPos);
					content = chunkBefore + embedHTML + chunkAfter;
				}
				break;
		}

		return content;
	},

	handleNodeChange : function(editor_id, node, undo_index, undo_levels, visual_aid, any_selection) {
		if (node == null)
			return;

		do {
			if (node.nodeName == "IMG" && /mceItem(Flash|ShockWave|WindowsMedia|QuickTime|RealMedia)/.test(tinyMCE.getAttrib(node, 'class'))) {
				tinyMCE.switchClass(editor_id + '_media', 'mceButtonSelected');
				return true;
			}
		} while ((node = node.parentNode));

		tinyMCE.switchClass(editor_id + '_media', 'mceButtonNormal');

		return true;
	},

	_createImgFromEmbed : function(n, d, cl) {
		var ne, at, i, ti = '', an;

		ne = d.createElement('img');
		ne.src = tinyMCE.getParam("theme_href") + '/images/spacer.gif';
		ne.width = tinyMCE.getAttrib(n, 'width');
		ne.height = tinyMCE.getAttrib(n, 'height');
		ne.className = cl;

		at = n.attributes;
		for (i=0; i<at.length; i++) {
			if (at[i].specified && at[i].nodeValue) {
				an = at[i].nodeName.toLowerCase();

				if (an == 'src')
					continue;

				if (an == 'mce_src')
					an = 'src';

				if (an.indexOf('mce_') == -1 && !new RegExp('^(class|type)$').test(an))
					ti += an.toLowerCase() + ':\'' + at[i].nodeValue + "',";
			}
		}

		ti = ti.length > 0 ? ti.substring(0, ti.length - 1) : ti;
		ne.title = ti;

		n.parentNode.replaceChild(ne, n);
	},

	_createImg : function(cl, d, n) {
		var i, nl, ti = "", an, av, al = new Array();

		ne = d.createElement('img');
		ne.src = tinyMCE.getParam("theme_href") + '/images/spacer.gif';
		ne.width = tinyMCE.getAttrib(n, 'width');
		ne.height = tinyMCE.getAttrib(n, 'height');
		ne.className = cl;

		al.id = tinyMCE.getAttrib(n, 'id');
		al.name = tinyMCE.getAttrib(n, 'name');
		al.width = tinyMCE.getAttrib(n, 'width');
		al.height = tinyMCE.getAttrib(n, 'height');
		al.bgcolor = tinyMCE.getAttrib(n, 'bgcolor');
		al.align = tinyMCE.getAttrib(n, 'align');
		al.class_name = tinyMCE.getAttrib(n, 'mce_class');

		nl = n.getElementsByTagName('div');
		for (i=0; i<nl.length; i++) {
			av = tinyMCE.getAttrib(nl[i], 'value');
			av = av.replace(new RegExp('\\\\', 'g'), '\\\\');
			av = av.replace(new RegExp('"', 'g'), '\\"');
			av = av.replace(new RegExp("'", 'g'), "\\'");
			an = tinyMCE.getAttrib(nl[i], 'name');
			al[an] = av;
		}

		if (al.movie) {
			al.src = al.movie;
			al.movie = null;
		}

		for (an in al) {
			if (al[an] != null && typeof(al[an]) != "function" && al[an] != '')
				ti += an.toLowerCase() + ':\'' + al[an] + "',";
		}

		ti = ti.length > 0 ? ti.substring(0, ti.length - 1) : ti;
		ne.title = ti;

		return ne;
	},

	_getEmbed : function(cls, cb, mt, p, at) {
		var h = '', n;

		p.width = at.width ? at.width : p.width;
		p.height = at.height ? at.height : p.height;

		h += '<object classid="clsid:' + cls + '" codebase="' + cb + '"';
		h += typeof(p.id) != "undefined" ? ' id="' + p.id + '"' : '';
		h += typeof(p.name) != "undefined" ? ' name="' + p.name + '"' : '';
		h += typeof(p.width) != "undefined" ? ' width="' + p.width + '"' : '';
		h += typeof(p.height) != "undefined" ? ' height="' + p.height + '"' : '';
		h += typeof(p.align) != "undefined" ? ' align="' + p.align + '"' : '';
		h += '>';

		for (n in p) {
			if (p[n] && typeof(p[n]) != "function") {
				h += '<param name="' + n + '" value="' + p[n] + '" />';

				// Add extra url parameter if it's an absolute URL on WMP
				if (n == 'src' && p[n].indexOf('://') != -1 && mt == 'application/x-mplayer2')
					h += '<param name="url" value="' + p[n] + '" />';
			}
		}

		h += '<embed type="' + mt + '"';

		for (n in p) {
			if (typeof(p[n]) == "function")
				continue;

			// Skip url parameter for embed tag on WMP
			if (!(n == 'url' && mt == 'application/x-mplayer2'))
				h += ' ' + n + '="' + p[n] + '"';
		}

		h += '></embed></object>';

		return h;
	},

	_parseAttributes : function(attribute_string) {
		var attributeName = "", endChr = '"';
		var attributeValue = "";
		var withInName;
		var withInValue;
		var attributes = new Array();
		var whiteSpaceRegExp = new RegExp('^[ \n\r\t]+', 'g');

		if (attribute_string == null || attribute_string.length < 2)
			return null;

		withInName = withInValue = false;

		for (var i=0; i<attribute_string.length; i++) {
			var chr = attribute_string.charAt(i);

			if ((chr == '"' || chr == "'") && !withInValue) {
				withInValue = true;
				endChr = chr;
			} else if (chr == endChr && withInValue) {
				withInValue = false;

				var pos = attributeName.lastIndexOf(' ');
				if (pos != -1)
					attributeName = attributeName.substring(pos+1);

				attributes[attributeName.toLowerCase()] = attributeValue.substring(1);

				attributeName = "";
				attributeValue = "";
			} else if (!whiteSpaceRegExp.test(chr) && !withInName && !withInValue)
				withInName = true;

			if (chr == '=' && withInName)
				withInName = false;

			if (withInName)
				attributeName += chr;

			if (withInValue)
				attributeValue += chr;
		}

		return attributes;
	}
};

tinyMCE.addPlugin("media", TinyMCE_MediaPlugin);
