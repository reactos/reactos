/**
 * $Id: editor_plugin_src.js 42 2006-08-08 14:32:24Z spocke $
 *
 * @author Moxiecode
 * @copyright Copyright © 2004-2007, Moxiecode Systems AB, All rights reserved.
 */

tinyMCE.importPluginLanguagePack('devkit');

var TinyMCE_DevKitPlugin = {
	_logFilter : '\\[(importCSS|execCommand|execInstanceCommand|debug)\\]',
	_logPadding : '',
	_startTime : null,
	_benchMark : false,
	_winLoaded : false,
	_isDebugEvents : false,

	getInfo : function() {
		return {
			longname : 'Development Kit',
			author : 'Moxiecode Systems AB',
			authorurl : 'http://tinymce.moxiecode.com',
			infourl : 'http://wiki.moxiecode.com/index.php/TinyMCE:Plugins/devkit',
			version : tinyMCE.majorVersion + "." + tinyMCE.minorVersion
		};
	},

	initInstance : function(inst) {
		this._setup();
	},

	_setup : function() {
		if (this._loaded)
			return;

		this._loaded = true;

		// Register a document reference for more easy access in the FF DOM inspector
		document.___TinyMCE = tinyMCE;

		// Setup devkit by settings
		this._logFilter = tinyMCE.getParam('devkit_log_filter', this._logFilter);
		this._benchMark = tinyMCE.getParam('devkit_bench_mark', false);

		var ifr = document.createElement('iframe');

		ifr.setAttribute("id", "devkit");
		ifr.setAttribute("frameBorder", "0");
		ifr.setAttribute("src", tinyMCE.baseURL + '/plugins/devkit/devkit.htm');

		document.body.appendChild(ifr);

		// Workaround for strange IE reload bug
		//if (tinyMCE.isRealIE)
		//	document.getElementById('devkit').outerHTML = document.getElementById('devkit').outerHTML;

		tinyMCE.importCSS(document, tinyMCE.baseURL + '/plugins/devkit/css/devkit_ui.css');
	},

	_start : function() {
		this._logPadding += '\u00a0';

		return new Date().getTime();
	},

	_end : function(st) {
		if (this._logPadding.length > 0)
			this._logPadding = this._logPadding.substring(0, this._logPadding.length - 1);

		if (this._benchMark)
			this._log("benchmark", "Execution time: " + (new Date().getTime() - st));
	},

	_log : function(t) {
		var m, a, i, e = document.getElementById('devkit'), now = new Date().getTime();

		if (!this._startTime)
			this._startTime = now;

		m = (this._logPadding.length > 1 ? this._logPadding : '') + '[' + (now - this._startTime) + '] [' + t + '] ';

		a = this._log.arguments;
		for (i=1; i<a.length; i++) {
			if (typeof(a[i]) == 'undefined')
				continue;

			if (i > 1)
				m += ', ';

			m += a[i];
		}

		if (!new RegExp(this._logFilter, 'gi').test(m)) {
			if (this._logPadding.length > 0)
				this._logPadding = this._logPadding.substring(0, this._logPadding.length - 1);

			return;
		}

		if (!this._winLoaded)
			tinyMCE.log[tinyMCE.log.length] = m;
		else
			e.contentWindow.debug(m);
	},

	_debugEvents : function(s) {
		var i, ld, inst, n, ev = ['CheckboxStateChange','DOMAttrModified','DOMMenuItemActive',
				'DOMMenuItemInactive','DOMMouseScroll','DOMNodeInserted','DOMNodeRemoved',
				'RadioStateChange','blur','broadcast','change','click','close','command',
				'commandupdate','contextmenu','dblclick','dragdrop','dragenter','dragexit',
				'draggesture','dragover','focus','input','keydown','keypress','keyup','load',
				'mousedown','mouseout','mouseover','mouseup','overflow','overflowchanged','popuphidden',
				'popuphiding','popupshowing','popupshown','select','syncfrompreference','synctopreference',
				'underflow','unload','abort','activate','afterprint','afterupdate','beforeactivate',
				'beforecopy','beforecut','beforedeactivate','beforeeditfocus','beforepaste','beforeprint',
				'beforeunload','beforeupdate','bounce','cellchange','controlselect','copy','cut',
				'dataavailable','datasetchanged','datasetcomplete','deactivate','dragend','dragleave',
				'dragstart','drop','error','errorupdate','filterchange','finish','focusin','focusout',
				'help','layoutcomplete','losecapture','mouseenter','mouseleave','mousewheel',
				'move','moveend','movestart','paste','propertychange','readystatechange','reset','resize',
				'resizeend','resizestart','rowenter','rowexit','rowsdelete','rowsinserted','scroll',
				'selectionchange','selectstart','start','stop','submit'];
		// mousemove

		if (TinyMCE_DevKitPlugin._isDebugEvents == s)
			return;

		TinyMCE_DevKitPlugin._isDebugEvents = s;

		for (n in tinyMCE.instances) {
			inst = tinyMCE.instances[n];

			if (!tinyMCE.isInstance(inst) || inst.getDoc() == ld)
				continue;

			ld = inst.getDoc();

			for (i=0; i<ev.length; i++) {
				if (s)
					tinyMCE.addEvent(ld, ev[i], TinyMCE_DevKitPlugin._debugEvent);
				else
					tinyMCE.removeEvent(ld, ev[i], TinyMCE_DevKitPlugin._debugEvent);
			}
		}
	},

	_debugEvent : function(e) {
		var t;

		e = e ? e : tinyMCE.selectedInstance.getWin().event;
		t = e.srcElement ? e.srcElement : e.target;

		tinyMCE.debug(e.type, t ? t.nodeName : '');
	},

	_serialize : function(o) {
		var i, v, s = TinyMCE_DevKitPlugin._serialize;

		if (o == null)
			return 'null';

		switch (typeof o) {
			case 'string':
				v = '\bb\tt\nn\ff\rr\""\'\'\\\\';

				return '"' + o.replace(new RegExp('([\u0080-\uFFFF\\x00-\\x1f\\"])', 'g'), function(a, b) {
					i = v.indexOf(b);

					if (i+1)
						return '\\' + v.charAt(i + 1);

					a = b.charCodeAt().toString(16);

					return '\\u' + '0000'.substring(a.length) + a;
				}) + '"';

			case 'object':
				if (o instanceof Array) {
					for (i=0, v = '['; i<o.length; i++)
						v += (i > 0 ? ',' : '') + s(o[i]);

					return v + ']';
				}

				v = '{';

				for (i in o)
					v += typeof o[i] != 'function' ? (v.length > 1 ? ',"' : '"') + i + '":' + s(o[i]) : '';

				return v + '}';
		}

		return '' + o;
	}
};

// Patch and piggy back functions
tinyMCE.__debug = tinyMCE.debug;
tinyMCE.debug = function() {
	var a, i, m = '', now = new Date().getTime(), start = TinyMCE_DevKitPlugin._startTime;

	if (!start)
		TinyMCE_DevKitPlugin._startTime = start = now;

	a = this.debug.arguments;
	for (i=0; i<a.length; i++) {
		if (typeof(a[i]) == 'undefined')
			continue;

		if (i > 0)
			m += ', ';

		m += a[i];
	}

	TinyMCE_DevKitPlugin._log('debug', m);
};

tinyMCE.dump = function(o) {
	tinyMCE.debug(TinyMCE_DevKitPlugin._serialize(o));
};

tinyMCE.sleep = function(t) {
	var s = new Date().getTime(), b;

	while (new Date().getTime() - s < t) b=1;
};

tinyMCE.__execCommand = tinyMCE.execCommand;
tinyMCE.execCommand = function(command, user_interface, value) {
	var r, st, dk = TinyMCE_DevKitPlugin;

	st = dk._start();
	dk._log('execCommand', command, user_interface, value);
	r = tinyMCE.__execCommand(command, user_interface, value);
	dk._end(st);

	return r;
};

tinyMCE.__execInstanceCommand = tinyMCE.execInstanceCommand;
tinyMCE.execInstanceCommand = function(editor_id, command, user_interface, value, focus) {
	var r, st, dk = TinyMCE_DevKitPlugin;

	st = dk._start();
	dk._log('execInstanceCommand', editor_id, command, user_interface, value);
	r = tinyMCE.__execInstanceCommand(editor_id, command, user_interface, value);
	dk._end(st);

	return r;
};

TinyMCE_Engine.prototype.__handleEvent = TinyMCE_Engine.prototype.handleEvent;
TinyMCE_Engine.prototype.handleEvent = function(e) {
	var r, st, dk = TinyMCE_DevKitPlugin;

	st = dk._start();
	dk._log('handleEvent', e.type);
	r = tinyMCE.__handleEvent(e);
	dk._end(st);

	return r;
};

tinyMCE.__importCSS = tinyMCE.importCSS;
tinyMCE.importCSS = function(doc, css) {
	var r, st, dk = TinyMCE_DevKitPlugin;

	st = dk._start();
	dk._log('importCSS', doc, css);
	r = tinyMCE.__importCSS(doc, css);
	dk._end(st);

	return r;
};

tinyMCE.__triggerNodeChange = tinyMCE.triggerNodeChange;
tinyMCE.triggerNodeChange = function(focus, setup_content) {
	var r, st, dk = TinyMCE_DevKitPlugin;

	st = dk._start();
	dk._log('triggerNodeChange', focus, setup_content);
	r = tinyMCE.__triggerNodeChange(focus, setup_content);
	dk._end(st);

	return r;
};

tinyMCE.__dispatchCallback = tinyMCE.dispatchCallback;
tinyMCE.dispatchCallback = function(i, p, n) {
	var r, st, dk = TinyMCE_DevKitPlugin;

	st = dk._start();
	dk._log('dispatchCallback', i, p, n);
	r = tinyMCE.__dispatchCallback(i, p, n);
	dk._end(st);

	return r;
};

tinyMCE.__executeCallback = tinyMCE.executeCallback;
tinyMCE.executeCallback = function(i, p, n) {
	var r, st, dk = TinyMCE_DevKitPlugin;

	st = dk._start();
	dk._log('executeCallback', i, p, n);
	r = tinyMCE.__executeCallback(i, p, n);
	dk._end(st);

	return r;
};

tinyMCE.__execCommandCallback = tinyMCE.execCommandCallback;
tinyMCE.execCommandCallback = function(i, p, n) {
	var r, st, dk = TinyMCE_DevKitPlugin;

	st = dk._start();
	dk._log('execCommandCallback', i, p, n);
	r = tinyMCE.__execCommandCallback(i, p, n);
	dk._end(st);

	return r;
};

tinyMCE.addPlugin("devkit", TinyMCE_DevKitPlugin);
