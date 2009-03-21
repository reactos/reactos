/**
 * $Id: editor_plugin_src.js 920 2008-09-09 14:05:33Z spocke $
 *
 * @author Moxiecode
 * @copyright Copyright © 2004-2008, Moxiecode Systems AB, All rights reserved.
 */

(function() {
	tinymce.create('tinymce.plugins.FullPagePlugin', {
		init : function(ed, url) {
			var t = this;

			t.editor = ed;

			// Register commands
			ed.addCommand('mceFullPageProperties', function() {
				ed.windowManager.open({
					file : url + '/fullpage.htm',
					width : 430 + parseInt(ed.getLang('fullpage.delta_width', 0)),
					height : 495 + parseInt(ed.getLang('fullpage.delta_height', 0)),
					inline : 1
				}, {
					plugin_url : url,
					head_html : t.head
				});
			});

			// Register buttons
			ed.addButton('fullpage', {title : 'fullpage.desc', cmd : 'mceFullPageProperties'});

			ed.onBeforeSetContent.add(t._setContent, t);
			ed.onSetContent.add(t._setBodyAttribs, t);
			ed.onGetContent.add(t._getContent, t);
		},

		getInfo : function() {
			return {
				longname : 'Fullpage',
				author : 'Moxiecode Systems AB',
				authorurl : 'http://tinymce.moxiecode.com',
				infourl : 'http://wiki.moxiecode.com/index.php/TinyMCE:Plugins/fullpage',
				version : tinymce.majorVersion + "." + tinymce.minorVersion
			};
		},

		// Private plugin internal methods

		_setBodyAttribs : function(ed, o) {
			var bdattr, i, len, kv, k, v, t, attr = this.head.match(/body(.*?)>/i);

			if (attr && attr[1]) {
				bdattr = attr[1].match(/\s*(\w+\s*=\s*".*?"|\w+\s*=\s*'.*?'|\w+\s*=\s*\w+|\w+)\s*/g);

				if (bdattr) {
					for(i = 0, len = bdattr.length; i < len; i++) {
						kv = bdattr[i].split('=');
						k = kv[0].replace(/\s/,'');
						v = kv[1];

						if (v) {
							v = v.replace(/^\s+/,'').replace(/\s+$/,'');
							t = v.match(/^["'](.*)["']$/);

							if (t)
								v = t[1];
						} else
							v = k;

						ed.dom.setAttrib(ed.getBody(), 'style', v);
					}
				}
			}
		},

		_createSerializer : function() {
			return new tinymce.dom.Serializer({
				dom : this.editor.dom,
				apply_source_formatting : true
			});
		},

		_setContent : function(ed, o) {
			var t = this, sp, ep, c = o.content, v, st = '';

			// Parse out head, body and footer
			c = c.replace(/<(\/?)BODY/gi, '<$1body');
			sp = c.indexOf('<body');

			if (sp != -1) {
				sp = c.indexOf('>', sp);
				t.head = c.substring(0, sp + 1);

				ep = c.indexOf('</body', sp);
				if (ep == -1)
					ep = c.indexOf('</body', ep);

				o.content = c.substring(sp + 1, ep);
				t.foot = c.substring(ep);

				function low(s) {
					return s.replace(/<\/?[A-Z]+/g, function(a) {
						return a.toLowerCase();
					})
				};

				t.head = low(t.head);
				t.foot = low(t.foot);
			} else {
				t.head = '';
				if (ed.getParam('fullpage_default_xml_pi'))
					t.head += '<?xml version="1.0" encoding="' + ed.getParam('fullpage_default_encoding', 'ISO-8859-1') + '" ?>\n';

				t.head += ed.getParam('fullpage_default_doctype', '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">');
				t.head += '\n<html>\n<head>\n<title>' + ed.getParam('fullpage_default_title', 'Untitled document') + '</title>\n';

				if (v = ed.getParam('fullpage_default_encoding'))
					t.head += '<meta http-equiv="Content-Type" content="' + v + '" />\n';

				if (v = ed.getParam('fullpage_default_font_family'))
					st += 'font-family: ' + v + ';';

				if (v = ed.getParam('fullpage_default_font_size'))
					st += 'font-size: ' + v + ';';

				if (v = ed.getParam('fullpage_default_text_color'))
					st += 'color: ' + v + ';';

				t.head += '</head>\n<body' + (st ? ' style="' + st + '"' : '') + '>\n';
				t.foot = '\n</body>\n</html>';
			}
		},

		_getContent : function(ed, o) {
			var t = this;

			o.content = tinymce.trim(t.head) + '\n' + tinymce.trim(o.content) + '\n' + tinymce.trim(t.foot);
		}
	});

	// Register plugin
	tinymce.PluginManager.add('fullpage', tinymce.plugins.FullPagePlugin);
})();