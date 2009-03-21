/**
 * $Id: editor_plugin_src.js 201 2007-02-12 15:56:56Z spocke $
 *
 * @author Moxiecode
 * @copyright Copyright © 2004-2008, Moxiecode Systems AB, All rights reserved.
 */

(function() {
	tinymce.create('tinymce.plugins.VisualChars', {
		init : function(ed, url) {
			var t = this;

			t.editor = ed;

			// Register commands
			ed.addCommand('mceVisualChars', t._toggleVisualChars, t);

			// Register buttons
			ed.addButton('visualchars', {title : 'visualchars.desc', cmd : 'mceVisualChars'});

			ed.onBeforeGetContent.add(function(ed, o) {
				if (t.state) {
					t.state = true;
					t._toggleVisualChars();
				}
			});
		},

		getInfo : function() {
			return {
				longname : 'Visual characters',
				author : 'Moxiecode Systems AB',
				authorurl : 'http://tinymce.moxiecode.com',
				infourl : 'http://wiki.moxiecode.com/index.php/TinyMCE:Plugins/visualchars',
				version : tinymce.majorVersion + "." + tinymce.minorVersion
			};
		},

		// Private methods

		_toggleVisualChars : function() {
			var t = this, ed = t.editor, nl, i, h, d = ed.getDoc(), b = ed.getBody(), nv, s = ed.selection, bo;

			t.state = !t.state;
			ed.controlManager.setActive('visualchars', t.state);

			if (t.state) {
				nl = [];
				tinymce.walk(b, function(n) {
					if (n.nodeType == 3 && n.nodeValue && n.nodeValue.indexOf('\u00a0') != -1)
						nl.push(n);
				}, 'childNodes');

				for (i=0; i<nl.length; i++) {
					nv = nl[i].nodeValue;
					nv = nv.replace(/(\u00a0+)/g, '<span class="mceItemHidden mceVisualNbsp">$1</span>');
					nv = nv.replace(/\u00a0/g, '\u00b7');
					ed.dom.setOuterHTML(nl[i], nv, d);
				}
			} else {
				nl = tinymce.grep(ed.dom.select('span', b), function(n) {
					return ed.dom.hasClass(n, 'mceVisualNbsp');
				});

				for (i=0; i<nl.length; i++)
					ed.dom.setOuterHTML(nl[i], nl[i].innerHTML.replace(/(&middot;|\u00b7)/g, '&nbsp;'), d);
			}
		}
	});

	// Register plugin
	tinymce.PluginManager.add('visualchars', tinymce.plugins.VisualChars);
})();