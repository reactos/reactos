var AdvHRDialog = {
	init : function(ed) {
		var dom = ed.dom, f = document.forms[0], n = ed.selection.getNode(), w;

		w = dom.getAttrib(n, 'width');
		f.width.value = w ? parseInt(w) : (dom.getStyle('width') || '');
		f.size.value = dom.getAttrib(n, 'size') || parseInt(dom.getStyle('height')) || '';
		f.noshade.checked = !!dom.getAttrib(n, 'noshade') || !!dom.getStyle('border-width');
		selectByValue(f, 'width2', w.indexOf('%') != -1 ? '%' : 'px');
	},

	update : function() {
		var ed = tinyMCEPopup.editor, h, f = document.forms[0], st = '';

		h = '<hr';

		if (f.size.value) {
			h += ' size="' + f.size.value + '"';
			st += ' height:' + f.size.value + 'px;';
		}

		if (f.width.value) {
			h += ' width="' + f.width.value + (f.width2.value == '%' ? '%' : '') + '"';
			st += ' width:' + f.width.value + (f.width2.value == '%' ? '%' : 'px') + ';';
		}

		if (f.noshade.checked) {
			h += ' noshade="noshade"';
			st += ' border-width: 1px; border-style: solid; border-color: #CCCCCC; color: #ffffff;';
		}

		if (ed.settings.inline_styles)
			h += ' style="' + tinymce.trim(st) + '"';

		h += ' />';

		ed.execCommand("mceInsertContent", false, h);
		tinyMCEPopup.close();
	}
};

tinyMCEPopup.requireLangPack();
tinyMCEPopup.onInit.add(AdvHRDialog.init, AdvHRDialog);
