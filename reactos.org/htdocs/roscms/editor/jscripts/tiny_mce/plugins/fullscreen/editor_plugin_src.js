/**
 * $Id: editor_plugin_src.js 201 2007-02-12 15:56:56Z spocke $
 *
 * @author Moxiecode
 * @copyright Copyright © 2004-2007, Moxiecode Systems AB, All rights reserved.
 */

/* Import plugin specific language pack */
tinyMCE.importPluginLanguagePack('fullscreen');

var TinyMCE_FullScreenPlugin = {
	getInfo : function() {
		return {
			longname : 'Fullscreen',
			author : 'Moxiecode Systems AB',
			authorurl : 'http://tinymce.moxiecode.com',
			infourl : 'http://wiki.moxiecode.com/index.php/TinyMCE:Plugins/fullscreen',
			version : tinyMCE.majorVersion + "." + tinyMCE.minorVersion
		};
	},

	initInstance : function(inst) {
		if (!tinyMCE.settings['fullscreen_skip_plugin_css'])
			tinyMCE.importCSS(inst.getContainerWin().document, tinyMCE.baseURL + "/plugins/fullscreen/css/page.css");
	},

	getControlHTML : function(cn) {
		switch (cn) {
			case "fullscreen":
				return tinyMCE.getButtonHTML(cn, 'lang_fullscreen_desc', '{$pluginurl}/images/fullscreen.gif', 'mceFullScreen');
		}

		return "";
	},

	execCommand : function(editor_id, element, command, user_interface, value) {
		var inst;

		// Handle commands
		switch (command) {
			case "mceFullScreen":
				inst = tinyMCE.getInstanceById(editor_id);

				if (tinyMCE.getParam('fullscreen_new_window'))
					this._toggleFullscreenWin(inst);
				else
					this._toggleFullscreen(inst);

				return true;
		}

		// Pass to next handler in chain
		return false;
	},

	_toggleFullscreenWin : function(inst) {
		if (tinyMCE.getParam('fullscreen_is_enabled')) {
			// In fullscreen mode
			window.opener.tinyMCE.execInstanceCommand(tinyMCE.getParam('fullscreen_editor_id'), 'mceSetContent', false, tinyMCE.getContent(inst.editorId));
			top.close();
		} else {
			tinyMCE.setWindowArg('editor_id', inst.editorId);

			var win = window.open(tinyMCE.baseURL + "/plugins/fullscreen/fullscreen.htm", "mceFullScreenPopup", "fullscreen=yes,menubar=no,toolbar=no,scrollbars=no,resizable=yes,left=0,top=0,width=" + screen.availWidth + ",height=" + screen.availHeight);
			try { win.resizeTo(screen.availWidth, screen.availHeight); } catch (e) {}
		}
	},

	_toggleFullscreen : function(inst) {
		var ds = inst.getData('fullscreen'), editorContainer, tableElm, iframe, vp, cw, cd, re, w, h, si, blo, delta = 0, cell, row, fcml, bcml;

		cw = inst.getContainerWin();
		cd = cw.document;
		editorContainer = cd.getElementById(inst.editorId + '_parent');
		tableElm = editorContainer.firstChild;
		iframe = inst.iframeElement;
		re = cd.getElementById(inst.editorId + '_resize');
		blo = document.getElementById('mce_fullscreen_blocker');
		fcm = new TinyMCE_Layer(inst.editorId + '_fcMenu');
		fcml = new TinyMCE_Layer(inst.editorId + '_fcMenu');
		bcml = new TinyMCE_Layer(inst.editorId + '_bcMenu');

		if (fcml.exists() && fcml.isVisible()) {
			tinyMCE.switchClass(inst.editorId + '_forecolor', 'mceMenuButton');
			fcml.hide();
		}

		if (bcml.exists() && bcml.isVisible()) {
			tinyMCE.switchClass(inst.editorId + '_backcolor', 'mceMenuButton');
			bcml.hide();
		}

		if (!ds.enabled) {
			// Handle External Toolbar
			if (inst.toolbarElement) {
				delta += inst.toolbarElement.offsetHeight;

				cell = tableElm.tBodies[0].insertRow(0).insertCell(-1);
				cell.className = 'mceToolbarTop';
				cell.nowrap = true;

				ds.oldToolbarParent = inst.toolbarElement.parentNode;
				ds.toolbarHolder = document.createTextNode('...');

				cell.appendChild(ds.oldToolbarParent.replaceChild(ds.toolbarHolder, inst.toolbarElement));
			}

			ds.parents = [];

			vp = tinyMCE.getViewPort(cw);
			ds.scrollX = vp.left;
			ds.scrollY = vp.top;

			// Opera has a bug restoring scrollbars
			if (!tinyMCE.isOpera)
				tinyMCE.addCSSClass(cd.body, 'mceFullscreen');

			tinyMCE.getParentNode(tableElm.parentNode, function (n) {
				if (n.nodeName == 'BODY')
					return true;

				if (n.nodeType == 1)
					tinyMCE.addCSSClass(n, 'mceFullscreenPos');

				return false;
			});

			if (re)
				re.style.display = 'none';

			vp = tinyMCE.getViewPort(cw);

			ds.oldWidth = iframe.style.width ? iframe.style.width : iframe.offsetWidth;
			ds.oldHeight = iframe.style.height ? iframe.style.height : iframe.offsetHeight;
			ds.oldTWidth = tableElm.style.width ? tableElm.style.width : tableElm.offsetWidth;
			ds.oldTHeight = tableElm.style.height ? tableElm.style.height : tableElm.offsetHeight;

			// Handle % width
			if (ds.oldWidth && ds.oldWidth.indexOf)
				ds.oldTWidth = ds.oldWidth.indexOf('%') != -1 ? ds.oldWidth : ds.oldTWidth;

			if (!blo && tinyMCE.isRealIE) {
				blo = tinyMCE.createTag(document, 'iframe', {id : 'mce_fullscreen_blocker', src : 'about:blank', frameBorder : 0, width : vp.width, height : vp.height, style : 'display: block; position: absolute; left: 0; top: 0; z-index: 999; margin: 0; padding: 0;'});
				document.body.appendChild(blo);
			}

			tableElm.style.position = 'absolute';
			tableElm.style.zIndex = 1000;
			tableElm.style.left = tableElm.style.top = '0';

			tableElm.style.width = vp.width + 'px';
			tableElm.style.height = vp.height + 'px';

			if (tinyMCE.isRealIE) {
				iframe.style.width = vp.width + 'px';
				iframe.style.height = vp.height + 'px';

				// Calc new width/height based on overflow
				w = iframe.parentNode.clientWidth - (tableElm.offsetWidth - vp.width);
				h = iframe.parentNode.clientHeight - (tableElm.offsetHeight - vp.height);
			} else {
				w = iframe.parentNode.clientWidth;
				h = iframe.parentNode.clientHeight;
			}

			iframe.style.width = w + "px";
			iframe.style.height = (h+delta) + "px";

			tinyMCE.switchClass(inst.editorId + '_fullscreen', 'mceButtonSelected');
			ds.enabled = true;

			inst.useCSS = false;
		} else {
			// Handle External Toolbar
			if (inst.toolbarElement) {
				row = inst.toolbarElement.parentNode.parentNode;

				row.parentNode.removeChild(row);

				ds.oldToolbarParent.replaceChild(inst.toolbarElement, ds.toolbarHolder);

				ds.oldToolbarParent = null;
				ds.toolbarHolder = null;
			}

			if (blo)
				blo.parentNode.removeChild(blo);

			si = 0;
			tinyMCE.getParentNode(tableElm.parentNode, function (n) {
				if (n.nodeName == 'BODY')
					return true;

				if (n.nodeType == 1)
					tinyMCE.removeCSSClass(n, 'mceFullscreenPos');
			});

			if (re && tinyMCE.getParam("theme_advanced_resizing", false))
				re.style.display = 'block';

			tableElm.style.position = 'static';
			tableElm.style.zIndex = '';
			tableElm.style.width = '';
			tableElm.style.height = '';

			tableElm.style.width = ds.oldTWidth ? ds.oldTWidth : '';
			tableElm.style.height = ds.oldTHeight ? ds.oldTHeight : '';

			iframe.style.width = ds.oldWidth ? ds.oldWidth : '';
			iframe.style.height = ds.oldHeight ? ds.oldHeight : '';

			tinyMCE.switchClass(inst.editorId + '_fullscreen', 'mceButtonNormal');
			ds.enabled = false;

			tinyMCE.removeCSSClass(cd.body, 'mceFullscreen');
			cw.scrollTo(ds.scrollX, ds.scrollY);

			inst.useCSS = false;
		}
	},

	handleNodeChange : function(editor_id, node, undo_index, undo_levels, visual_aid, any_selection) {
		if (tinyMCE.getParam('fullscreen_is_enabled'))
			tinyMCE.switchClass(editor_id + '_fullscreen', 'mceButtonSelected');

		return true;
	}
};

tinyMCE.addPlugin("fullscreen", TinyMCE_FullScreenPlugin);
