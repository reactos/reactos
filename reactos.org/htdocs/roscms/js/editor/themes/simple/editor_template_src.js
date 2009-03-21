/**
 * $Id: editor_template_src.js 920 2008-09-09 14:05:33Z spocke $
 *
 * This file is meant to showcase how to create a simple theme. The advanced
 * theme is more suitable for production use.
 *
 * @author Moxiecode
 * @copyright Copyright © 2004-2008, Moxiecode Systems AB, All rights reserved.
 */

(function() {
	var DOM = tinymce.DOM;

	// Tell it to load theme specific language pack(s)
	tinymce.ThemeManager.requireLangPack('simple');

	tinymce.create('tinymce.themes.SimpleTheme', {
		init : function(ed, url) {
			var t = this, states = ['Bold', 'Italic', 'Underline', 'Strikethrough', 'InsertUnorderedList', 'InsertOrderedList'], s = ed.settings;

			t.editor = ed;

			ed.onInit.add(function() {
				ed.onNodeChange.add(function(ed, cm) {
					tinymce.each(states, function(c) {
						cm.get(c.toLowerCase()).setActive(ed.queryCommandState(c));
					});
				});

				ed.dom.loadCSS(url + "/skins/" + s.skin + "/content.css");
			});

			DOM.loadCSS((s.editor_css ? ed.documentBaseURI.toAbsolute(s.editor_css) : '') || url + "/skins/" + s.skin + "/ui.css");
		},

		renderUI : function(o) {
			var t = this, n = o.targetNode, ic, tb, ed = t.editor, cf = ed.controlManager, sc;

			n = DOM.insertAfter(DOM.create('span', {id : ed.id + '_container', 'class' : 'mceEditor ' + ed.settings.skin + 'SimpleSkin'}), n);
			n = sc = DOM.add(n, 'table', {cellPadding : 0, cellSpacing : 0, 'class' : 'mceLayout'});
			n = tb = DOM.add(n, 'tbody');

			// Create iframe container
			n = DOM.add(tb, 'tr');
			n = ic = DOM.add(DOM.add(n, 'td'), 'div', {'class' : 'mceIframeContainer'});

			// Create toolbar container
			n = DOM.add(DOM.add(tb, 'tr', {'class' : 'last'}), 'td', {'class' : 'mceToolbar mceLast', align : 'center'});

			// Create toolbar
			tb = t.toolbar = cf.createToolbar("tools1");
			tb.add(cf.createButton('bold', {title : 'simple.bold_desc', cmd : 'Bold'}));
			tb.add(cf.createButton('italic', {title : 'simple.italic_desc', cmd : 'Italic'}));
			tb.add(cf.createButton('underline', {title : 'simple.underline_desc', cmd : 'Underline'}));
			tb.add(cf.createButton('strikethrough', {title : 'simple.striketrough_desc', cmd : 'Strikethrough'}));
			tb.add(cf.createSeparator());
			tb.add(cf.createButton('undo', {title : 'simple.undo_desc', cmd : 'Undo'}));
			tb.add(cf.createButton('redo', {title : 'simple.redo_desc', cmd : 'Redo'}));
			tb.add(cf.createSeparator());
			tb.add(cf.createButton('cleanup', {title : 'simple.cleanup_desc', cmd : 'mceCleanup'}));
			tb.add(cf.createSeparator());
			tb.add(cf.createButton('insertunorderedlist', {title : 'simple.bullist_desc', cmd : 'InsertUnorderedList'}));
			tb.add(cf.createButton('insertorderedlist', {title : 'simple.numlist_desc', cmd : 'InsertOrderedList'}));
			tb.renderTo(n);

			return {
				iframeContainer : ic,
				editorContainer : ed.id + '_container',
				sizeContainer : sc,
				deltaHeight : -20
			};
		},

		getInfo : function() {
			return {
				longname : 'Simple theme',
				author : 'Moxiecode Systems AB',
				authorurl : 'http://tinymce.moxiecode.com',
				version : tinymce.majorVersion + "." + tinymce.minorVersion
			}
		}
	});

	tinymce.ThemeManager.add('simple', tinymce.themes.SimpleTheme);
})();