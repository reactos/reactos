tinyMCEPopup.requireLangPack();

var SearchReplaceDialog = {
	init : function(ed) {
		var f = document.forms[0], m = tinyMCEPopup.getWindowArg("mode");

		this.switchMode(m);

		f[m + '_panel_searchstring'].value = tinyMCEPopup.getWindowArg("search_string");

		// Focus input field
		f[m + '_panel_searchstring'].focus();
	},

	switchMode : function(m) {
		var f, lm = this.lastMode;

		if (lm != m) {
			f = document.forms[0];

			if (lm) {
				f[m + '_panel_searchstring'].value = f[lm + '_panel_searchstring'].value;
				f[m + '_panel_backwardsu'].checked = f[lm + '_panel_backwardsu'].checked;
				f[m + '_panel_backwardsd'].checked = f[lm + '_panel_backwardsd'].checked;
				f[m + '_panel_casesensitivebox'].checked = f[lm + '_panel_casesensitivebox'].checked;
			}

			mcTabs.displayTab(m + '_tab',  m + '_panel');
			document.getElementById("replaceBtn").style.display = (m == "replace") ? "inline" : "none";
			document.getElementById("replaceAllBtn").style.display = (m == "replace") ? "inline" : "none";
			this.lastMode = m;
		}
	},

	searchNext : function(a) {
		var ed = tinyMCEPopup.editor, se = ed.selection, r = se.getRng(), f, m = this.lastMode, s, b, fl = 0, w = ed.getWin(), wm = ed.windowManager, fo = 0;

		// Get input
		f = document.forms[0];
		s = f[m + '_panel_searchstring'].value;
		b = f[m + '_panel_backwardsu'].checked;
		ca = f[m + '_panel_casesensitivebox'].checked;
		rs = f['replace_panel_replacestring'].value;

		if (s == '')
			return;

		function fix() {
			// Correct Firefox graphics glitches
			r = se.getRng().cloneRange();
			ed.getDoc().execCommand('SelectAll', false, null);
			se.setRng(r);
		};

		function replace() {
			if (tinymce.isIE)
				ed.selection.getRng().duplicate().pasteHTML(rs); // Needs to be duplicated due to selection bug in IE
			else
				ed.getDoc().execCommand('InsertHTML', false, rs);
		};

		// IE flags
		if (ca)
			fl = fl | 4;

		switch (a) {
			case 'all':
				// Move caret to beginning of text
				ed.execCommand('SelectAll');
				ed.selection.collapse(true);

				if (tinymce.isIE) {
					while (r.findText(s, b ? -1 : 1, fl)) {
						r.scrollIntoView();
						r.select();
						replace();
						fo = 1;
					}

					tinyMCEPopup.storeSelection();
				} else {
					while (w.find(s, ca, b, false, false, false, false)) {
						replace();
						fo = 1;
					}
				}

				if (fo)
					tinyMCEPopup.alert(ed.getLang('searchreplace_dlg.allreplaced'));
				else
					tinyMCEPopup.alert(ed.getLang('searchreplace_dlg.notfound'));

				return;

			case 'current':
				if (!ed.selection.isCollapsed())
					replace();

				break;
		}

		se.collapse(b);
		r = se.getRng();

		// Whats the point
		if (!s)
			return;

		if (tinymce.isIE) {
			if (r.findText(s, b ? -1 : 1, fl)) {
				r.scrollIntoView();
				r.select();
			} else
				tinyMCEPopup.alert(ed.getLang('searchreplace_dlg.notfound'));

			tinyMCEPopup.storeSelection();
		} else {
			if (!w.find(s, ca, b, false, false, false, false))
				tinyMCEPopup.alert(ed.getLang('searchreplace_dlg.notfound'));
			else
				fix();
		}
	}
};

tinyMCEPopup.onInit.add(SearchReplaceDialog.init, SearchReplaceDialog);
