var devkit = parent.tinyMCE.plugins['devkit'], logEnabled = true, flip = false, book = null;

function init() {
	var log, i, f = document.forms[0];

	devkit._winLoaded = true;

	log = tinyMCE.log;

	for (i=0; i<log.length; i++)
		debug(log[i]);

	f.logfilter.value = devkit._logFilter;
}

function changeFilter(f) {
	devkit._logFilter = f;
}

function toggleLog(s) {
	logEnabled = s;
}

function toggleFlip() {
	document.getElementById('flipbtn').src = flip ? 'images/flip_down.gif' : 'images/flip_up.gif';

	if (flip)
		parent.document.getElementById('devkit').className = 'devkitup';
	else
		parent.document.getElementById('devkit').className = 'devkitdown';

	flip = !flip;
}

function debug(s) {
	var d, l, n;

	if (!logEnabled || !new RegExp(devkit._logFilter, 'gi').test(s))
		return;

	d = document;
	l = d.getElementById('log');
	n = d.createElement('span');

	n.innerHTML = tinyMCE.xmlEncode(s);

	l.appendChild(n);
	l.scrollTop = l.scrollHeight;
}

function renderInfo() {
	var se = document.getElementById('info'), n, sn, inst, h = '', sel, rng, instCount = 0, rc;

	h += '<h2>Browser info:</h2>';

	h += '<table border="0" cellpadding="0" cellspacing="0" class="data">';
	h += addRenderInfo('navigator.userAgent', navigator.userAgent);
	h += addRenderInfo('navigator.appName', navigator.appName);
	h += addRenderInfo('navigator.platform', navigator.platform);
	h += addRenderInfo('navigator.language', navigator.language, 'bspec');
	h += addRenderInfo('navigator.browserLanguage', navigator.browserLanguage, 'bspec');
	h += addRenderInfo('navigator.systemLanguage', navigator.systemLanguage, 'bspec');
	h += addRenderInfo('navigator.userLanguage', navigator.userLanguage, 'bspec');
	h += addRenderInfo('opera.buildNumber("inconspicuous")', typeof(opera) != 'undefined' && opera.buildNumber ? opera.buildNumber('inconspicuous') : null, 'bspec');
	h += addRenderInfo('window.innerWidth', parent.window.innerWidth, 'bspec');
	h += addRenderInfo('window.innerHeight', parent.window.innerHeight, 'bspec');
	h += addRenderInfo('document.body.offsetWidth', parent.document.body.offsetWidth);
	h += addRenderInfo('document.body.offsetHeight', parent.document.body.offsetHeight);
	h += addRenderInfo('screen.width', screen.width);
	h += addRenderInfo('screen.height', screen.height);
	h += addRenderInfo('screen.availWidth', screen.availWidth);
	h += addRenderInfo('screen.availHeight', screen.availHeight);
	h += addRenderInfo('screen.colorDepth', screen.colorDepth);
	h += addRenderInfo('screen.pixelDepth', screen.pixelDepth, 'bspec');
	h += addRenderInfo('document.contentType', document.contentType, 'bspec');
	h += '</table>';

	h += '<h2>TinyMCE_Engine info:</h2>';

	h += '<table border="0" cellpadding="0" cellspacing="0" class="data">';
	h += addRenderInfo('baseURL', tinyMCE.baseURL);
	h += addRenderInfo('selectedInstance.editorId', tinyMCE.selectedInstance ? tinyMCE.selectedInstance.editorId : null);
	h += addRenderInfo('selectedElement.nodeName', tinyMCE.selectedElement ? tinyMCE.selectedElement.nodeName : null, 'dep');
	h += addRenderInfo('loadedFiles',tinyMCE.loadedFiles.join(','));
	h += addRenderInfo('isMSIE', tinyMCE.isMSIE);
	h += addRenderInfo('isMSIE5', tinyMCE.isMSIE5);
	h += addRenderInfo('isMSIE5_0', tinyMCE.isMSIE5_0);
	h += addRenderInfo('isMSIE7', tinyMCE.isMSIE7);
	h += addRenderInfo('isGecko', tinyMCE.isGecko);
	h += addRenderInfo('isSafari', tinyMCE.isSafari);
	h += addRenderInfo('isOpera', tinyMCE.isOpera);
	h += addRenderInfo('isMac', tinyMCE.isMac);
	h += addRenderInfo('isNS7', tinyMCE.isNS7);
	h += addRenderInfo('isNS71', tinyMCE.isNS71);
	h += addRenderInfo('idCounter', tinyMCE.idCounter);
	h += addRenderInfo('currentConfig', tinyMCE.currentConfig);
	h += addRenderInfo('majorVersion', tinyMCE.majorVersion);
	h += addRenderInfo('minorVersion', tinyMCE.minorVersion);
	h += addRenderInfo('releaseDate', tinyMCE.releaseDate);
	h += addRenderInfo('documentBasePath', tinyMCE.documentBasePath);
	h += addRenderInfo('documentURL', tinyMCE.documentURL);
	h += '</table>';

	for (n in tinyMCE.instances) {
		inst = tinyMCE.instances[n];

		if (!tinyMCE.isInstance(inst))
			continue;

		sel = inst.selection.getSel();
		rng = inst.selection.getRng();

		h += '<h2>TinyMCE_Control(' + (instCount++) + ') id: ' + inst.editorId + '</h2>';
		h += '<table border="0" cellpadding="0" cellspacing="0" class="data">';

		h += addRenderInfo('editorId', inst.editorId);
		h += addRenderInfo('visualAid', inst.visualAid);
		h += addRenderInfo('foreColor', inst.foreColor);
		h += addRenderInfo('backColor', inst.backColor);
		h += addRenderInfo('formTargetElementId', inst.formTargetElementId);
		h += addRenderInfo('formElement', inst.formElement ? inst.formElement.nodeName : null);
		h += addRenderInfo('oldTargetElement', inst.oldTargetElement ? inst.oldTargetElement.nodeName : null);
		h += addRenderInfo('linkElement', inst.linkElement ? inst.linkElement.nodeName : null, 'dep');
		h += addRenderInfo('imgElement', inst.imgElement ? inst.imgElement.nodeName : null, 'dep');
		h += addRenderInfo('selectedNode', inst.selectedNode ? inst.selectedNode.nodeName : null, 'dep');
		h += addRenderInfo('targetElement', inst.targetElement ? inst.targetElement.nodeName : null);
		h += addRenderInfo('getBody().nodeName', inst.getBody() ? inst.getBody().nodeName : null);
		h += addRenderInfo('getBody().getAttribute("id")', inst.getBody() ? inst.getBody().getAttribute("id") : null);
		h += addRenderInfo('getDoc().location', inst.getDoc() ? inst.getDoc().location : null);
		h += addRenderInfo('startContent', inst.startContent);
		h += addRenderInfo('isHidden()', inst.isHidden());
		h += addRenderInfo('isDirty()', inst.isDirty());
		h += addRenderInfo('undoRedo.undoLevels.length', inst.undoRedo.undoLevels.length);
		h += addRenderInfo('undoRedo.undoIndex', inst.undoRedo.undoIndex);
		h += addRenderInfo('selection.getSelectedHTML()', inst.selection.getSelectedHTML());
		h += addRenderInfo('selection.isCollapsed()', inst.selection.isCollapsed() || 'false');
		h += addRenderInfo('selection.getSelectedText()', inst.selection.getSelectedText());
		h += addRenderInfo('selection.getFocusElement().nodeName', inst.selection.getFocusElement().nodeName);
		h += addRenderInfo('selection.getFocusElement().outerHTML', tinyMCE.getOuterHTML(inst.selection.getFocusElement()));

		if ((tinyMCE.isGecko || tinyMCE.isOpera) && sel && rng) {
			h += addRenderInfo('selection.getSel().anchorNode.nodeName', sel.anchorNode ? sel.anchorNode.nodeName : null, 'bspec');
			h += addRenderInfo('selection.getSel().anchorOffset', sel.anchorOffset, 'bspec');
			h += addRenderInfo('selection.getSel().focusNode.nodeName', sel.focusNode ? sel.focusNode.nodeName : null, 'bspec');
			h += addRenderInfo('selection.getSel().focusOffset', sel.focusOffset, 'bspec');
			h += addRenderInfo('selection.getRng().startContainer.nodeName', rng.startContainer ? rng.startContainer.nodeName : null, 'bspec');
			h += addRenderInfo('selection.getRng().startOffset', rng.startOffset, 'bspec');
			h += addRenderInfo('selection.getRng().endContainer.nodeName', rng.endContainer ? rng.endContainer.nodeName : null, 'bspec');
			h += addRenderInfo('selection.getRng().endOffset', rng.endOffset, 'bspec');
		}

		if (typeof(rng.item) != 'undefined' || typeof(rng.htmlText) != 'undefined') {
			if (!rng.item) {
				h += addRenderInfo('selection.getSel().type', sel.type, 'bspec');
				h += addRenderInfo('selection.getRng().htmlText', rng.htmlText, 'bspec');
				h += addRenderInfo('selection.getRng().text', rng.text, 'bspec');
			} else
				h += addRenderInfo('selection.getRng().item(0).nodeName', rng.item(0).nodeName, 'bspec');
		}

		h += '</table>';
	}

	h += '<p>Fields marked in <strong class="bspec">gray</strong> is not cross browser and should be used with care.</p>';
	h += '<p>Fields marked <strong class="dep">red</strong> are marked deprecated and will be removed in the future.</p><br />';

	se.innerHTML = h;
}

function addRenderInfo(n, v, c) {
	return '<tr><td' + (c ? ' class="' + c + '"' : '')+ '>' + n + '</td><td><input type="text" value="' + tinyMCE.xmlEncode(v != null ? ('' + v).replace(/[\r\n]/g, '') : 'null') + '" /></td></tr>';
}

function renderSettings() {
	var se = document.getElementById('settings'), n, sn, inst, h = '', v;

	for (n in tinyMCE.instances) {
		inst = tinyMCE.instances[n];

		if (!tinyMCE.isInstance(inst))
			continue;

		h += '<h2>Instance id: ' + inst.editorId + '</h2>';
		h += '<table border="0" cellpadding="0" cellspacing="0" class="data">';

		for (sn in inst.settings) {
			v = inst.settings[sn];

			h += '<tr><td class="col1">' + tinyMCE.xmlEncode(sn) + '</td><td><input type="text" value="' + tinyMCE.xmlEncode(v) + '" /></td></tr>';
		}

		h += '</table>';
	}

	se.innerHTML = h;
}

function renderContent() {
	var se = document.getElementById('content'), n, inst, h = '';

	for (n in tinyMCE.instances) {
		inst = tinyMCE.instances[n];

		if (!tinyMCE.isInstance(inst))
			continue;

		h += '<h2>Instance id: ' + inst.editorId + '</h2>';

		h += '<h3>Start content - inst.startContent:</h3>';
		h += '<div>' + tinyMCE.xmlEncode(inst.startContent) + '</div>';

		h += '<h3>Raw content - inst.getBody().innerHTML or inst.getHTML(true):</h3>';
		h += '<div>' + tinyMCE.xmlEncode(inst.getHTML(true)) + '</div>';

		h += '<h3>Cleaned content - inst.getHTML():</h3>';
		h += '<div>' + tinyMCE.xmlEncode(inst.getHTML()) + '</div>';

		if (inst.serializedHTML) {
			h += '<h3>Serialized HTML content - inst.serializedHTML:</h3>';
			h += '<div>' + tinyMCE.xmlEncode(inst.serializedHTML) + '</div>';
		}
	}

	se.innerHTML = h;
}

function renderCommandStates() {
	var se = document.getElementById('command_states'), n, inst, h = '', v, ex;
	var cmds = new Array('2D-Position','AbsolutePosition','BackColor','BlockDirLTR','BlockDirRTL','Bold','BrowseMode','Copy','CreateBookmark','CreateLink','Cut','Delete','DirLTR','DirRTL','EditMode','enableInlineTableEditing','enableObjectResizing','FontName','FontSize','ForeColor','FormatBlock','Indent','InsertButton','InsertFieldset','InsertHorizontalRule','InsertIFrame','InsertImage','InsertInputButton','InsertInputCheckbox','InsertInputFileUpload','InsertInputHidden','InsertInputImage','InsertInputPassword','InsertInputRadio','InsertInputReset','InsertInputSubmit','InsertInputText','InsertMarquee','InsertOrderedList','InsertParagraph','InsertSelectDropdown','InsertSelectListbox','InsertTextArea','InsertUnorderedList','Italic','JustifyCenter','JustifyFull','JustifyLeft','JustifyNone','JustifyRight','LiveResize','MultipleSelection','Open','Outdent','OverWrite','Paste','PlayImage','Redo','Refresh','RemoveFormat','SaveAs','SelectAll','SizeToControl','SizeToControlHeight','SizeToControlWidth','Stop','StopImage','StrikeThrough','styleWithCSS','Subscript','Superscript','UnBookmark','Underline','Undo','Unlink','Unselect'), i;

	for (n in tinyMCE.instances) {
		inst = tinyMCE.instances[n];

		if (!tinyMCE.isInstance(inst))
			continue;

		h += '<h2>Instance id: ' + inst.editorId + '</h2>';
		h += '<table border="0" cellpadding="0" cellspacing="0" class="data">';

		for (i=0; i<cmds.length; i++) {
			v = null;

			try {
				v = tinyMCE.isGecko || inst.getDoc().queryCommandSupported(cmds[i]);
				v = v ? inst.queryCommandState(cmds[i]) : 'Not supported';
			} catch (ex) {
				v = 'Not supported';
			}

			h += '<tr><td><input type="text" value="' + tinyMCE.xmlEncode(cmds[i]) + '" /></td><td><input type="text" value="' + tinyMCE.xmlEncode(v) + '" /></td></tr>';
		}

		h += '</table>';
	}

	se.innerHTML = h;
}

function renderUndoRedo() {
	var se = document.getElementById('undo_redo'), inst, n, h = '', i, le, id, d, ur;
	var f = document.forms[0];	

	if (tinyMCE.undoLevels) {
		le = tinyMCE.undoLevels;

		h += '<h2>Global undo/redo</h2>';
		h += '<table border="0" cellpadding="0" cellspacing="0" width="50%" class="data">';
		h += '<tr><td>undoLevels.length</td><td>' + le.length + '</td></tr>';
		h += '<tr><td>undoIndex</td><td>' + tinyMCE.undoIndex + '</td></tr>';
		h += '</table>';

		for (i=0; i<le.length; i++)
			h += '<h3>Level: ' + i + ', Instance: ' + (le[i] ? le[i].editorId : 'null') + '</h3>';
	}

	for (n in tinyMCE.instances) {
		inst = tinyMCE.instances[n];

		if (!tinyMCE.isInstance(inst))
			continue;

		ur = inst.undoRedo;
		le = ur.undoLevels;

		h += '<hr /><h2>Instance id: ' + inst.editorId + '</h2>';
		h += '<table border="0" cellpadding="0" cellspacing="0" width="50%" class="data">';
		h += '<tr><td>undoLevels.length</td><td>' + le.length + '</td></tr>';
		h += '<tr><td>undoIndex</td><td>' + ur.undoIndex + '</td></tr>';
		h += '<tr><td>typingUndoIndex</td><td>' + ur.typingUndoIndex + '</td></tr>';
		h += '<tr><td>undoRedo</td><td>' + ur.undoRedo + '</td></tr>';
		h += '</table>';

		for (i=0; i<le.length; i++) {
			h += '<h3>Level: ' + i + (!le[i].bookmark ? "" : " [bookmark]") + '</h3>';
			h += '<div class="undodata">' + tinyMCE.xmlEncode(le[i].content) + '</div>';

			if (i > 0 && f.undo_diff.checked) {
				d = diff_main(i > 0 ? le[i-1].content.replace(/[\r\n]+/g, '') : null, le[i].content.replace(/[\r\n]+/g, ''), false);
				diff_cleanup_semantic(d);
				h += '<h3>Diff ' + (i-1) + ',' + i + '</h3><div class="undodata">' + diff_prettyhtml(d) + '</div>';
			}
		}
	}

	se.innerHTML = h;
}

function clearLog() {
	document.getElementById('log').innerHTML = '';
	devkit._startTime = null;
}

function cancelAction() {
	parent.document.getElementById('devkit').style.display = 'none';
}

function toggleDebugEvents(s) {
	devkit._debugEvents(s);
}

function storeSelection() {
	book = tinyMCE.selectedInstance.selection.getBookmark();

	return false;
}

function restoreSelection() {
	tinyMCE.selectedInstance.selection.moveToBookmark(book);

	return false;
}