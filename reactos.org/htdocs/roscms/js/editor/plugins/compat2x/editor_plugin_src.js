/**
 * $Id: editor_plugin_src.js 264 2007-04-26 20:53:09Z spocke $
 *
 * @author Moxiecode
 * @copyright Copyright © 2004-2008, Moxiecode Systems AB, All rights reserved.
 */

(function() {
	var DOM = tinymce.DOM, Event = tinymce.dom.Event, each = tinymce.each, is = tinymce.is;

	tinymce.create('tinymce.plugins.Compat2x', {
		getInfo : function() {
			return {
				longname : 'Compat2x',
				author : 'Moxiecode Systems AB',
				authorurl : 'http://tinymce.moxiecode.com',
				infourl : 'http://wiki.moxiecode.com/index.php/TinyMCE:Plugins/compat2x',
				version : tinyMCE.majorVersion + "." + tinyMCE.minorVersion
			};
		}
	});

	(function() {
		// Extend tinyMCE/EditorManager
		tinymce.extend(tinyMCE, {
			addToLang : function(p, l) {
				each(l, function(v, k) {
					tinyMCE.i18n[(tinyMCE.settings.language || 'en') + '.' + (p ? p + '_' : '') + k] = v;
				});
			},

			getInstanceById : function(n) {
				return this.get(n);
			}
		});
	})();

	(function() {
		var EditorManager = tinymce.EditorManager;

		tinyMCE.instances = {};
		tinyMCE.plugins = {};
		tinymce.PluginManager.onAdd.add(function(pm, n, p) {
			tinyMCE.plugins[n] = p;
		});

		tinyMCE.majorVersion = tinymce.majorVersion;
		tinyMCE.minorVersion = tinymce.minorVersion;
		tinyMCE.releaseDate = tinymce.releaseDate;
		tinyMCE.baseURL = tinymce.baseURL;
		tinyMCE.isIE = tinyMCE.isMSIE = tinymce.isIE || tinymce.isOpera;
		tinyMCE.isMSIE5 = tinymce.isIE;
		tinyMCE.isMSIE5_0 = tinymce.isIE;
		tinyMCE.isMSIE7 = tinymce.isIE;
		tinyMCE.isGecko = tinymce.isGecko;
		tinyMCE.isSafari = tinymce.isWebKit;
		tinyMCE.isOpera = tinymce.isOpera;
		tinyMCE.isMac = false;
		tinyMCE.isNS7 = false;
		tinyMCE.isNS71 = false;
		tinyMCE.compat = true;

		// Extend tinyMCE class
		TinyMCE_Engine = tinyMCE;
		tinymce.extend(tinyMCE, {
			getParam : function(n, dv) {
				return this.activeEditor.getParam(n, dv);
			},

			addEvent : function(e, na, f, sc) {
				tinymce.dom.Event.add(e, na, f, sc || this);
			},

			getControlHTML : function(n) {
				return EditorManager.activeEditor.controlManager.createControl(n);
			},

			loadCSS : function(u) {
				tinymce.DOM.loadCSS(u);
			},

			importCSS : function(doc, u) {
				if (doc == document)
					this.loadCSS(u);
				else
					new tinymce.dom.DOMUtils(doc).loadCSS(u);
			},

			log : function() {
				console.debug.apply(console, arguments);
			},

			getLang : function(n, dv) {
				var v = EditorManager.activeEditor.getLang(n.replace(/^lang_/g, ''), dv);

				// Is number
				if (/^[0-9\-.]+$/g.test(v))
					return parseInt(v);

				return v;
			},

			isInstance : function(o) {
				return o != null && typeof(o) == "object" && o.execCommand;
			},

			triggerNodeChange : function() {
				EditorManager.activeEditor.nodeChanged();
			},

			regexpReplace : function(in_str, reg_exp, replace_str, opts) {
				var re;

				if (in_str == null)
					return in_str;

				if (typeof(opts) == "undefined")
					opts = 'g';

				re = new RegExp(reg_exp, opts);

				return in_str.replace(re, replace_str);
			},

			trim : function(s) {
				return tinymce.trim(s);
			},

			xmlEncode : function(s) {
				return tinymce.DOM.encode(s);
			},

			explode : function(s, d) {
				var o = [];

				tinymce.each(s.split(d), function(v) {
					if (v != '')
						o.push(v);
				});

				return o;
			},

			switchClass : function(id, cls) {
				var b;

				if (/^mceButton/.test(cls)) {
					b = EditorManager.activeEditor.controlManager.get(id);

					if (!b)
						return;

					switch (cls) {
						case "mceButtonNormal":
							b.setDisabled(false);
							b.setActive(false);
							return;

						case "mceButtonDisabled":
							b.setDisabled(true);
							return;

						case "mceButtonSelected":
							b.setActive(true);
							b.setDisabled(false);
							return;
					}
				}
			},

			addCSSClass : function(e, n, b) {
				return tinymce.DOM.addClass(e, n, b);
			},

			hasCSSClass : function(e, n) {
				return tinymce.DOM.hasClass(e, n);
			},

			removeCSSClass : function(e, n) {
				return tinymce.DOM.removeClass(e, n);
			},

			getCSSClasses : function() {
				var cl = EditorManager.activeEditor.dom.getClasses(), o = [];

				each(cl, function(c) {
					o.push(c['class']);
				});

				return o;
			},

			setWindowArg : function(n, v) {
				EditorManager.activeEditor.windowManager.params[n] = v;
			},

			getWindowArg : function(n, dv) {
				var wm = EditorManager.activeEditor.windowManager, v;

				v = wm.getParam(n);
				if (v === '')
					return '';

				return v || wm.getFeature(n) || dv;
			},

			getParentNode : function(n, f) {
				return this._getDOM().getParent(n, f);
			},

			selectElements : function(n, na, f) {
				var i, a = [], nl, x;

				for (x=0, na = na.split(','); x<na.length; x++)
					for (i=0, nl = n.getElementsByTagName(na[x]); i<nl.length; i++)
						(!f || f(nl[i])) && a.push(nl[i]);

				return a;
			},

			getNodeTree : function(n, na, t, nn) {
				return this.selectNodes(n, function(n) {
					return (!t || n.nodeType == t) && (!nn || n.nodeName == nn);
				}, na ? na : []);
			},

			getAttrib : function(e, n, dv) {
				return this._getDOM().getAttrib(e, n, dv);
			},

			setAttrib : function(e, n, v) {
				return this._getDOM().setAttrib(e, n, v);
			},

			getElementsByAttributeValue : function(n, e, a, v) {
				var i, nl = n.getElementsByTagName(e), o = [];

				for (i=0; i<nl.length; i++) {
					if (tinyMCE.getAttrib(nl[i], a).indexOf(v) != -1)
						o[o.length] = nl[i];
				}

				return o;
			},

			selectNodes : function(n, f, a) {
				var i;

				if (!a)
					a = [];

				if (f(n))
					a[a.length] = n;

				if (n.hasChildNodes()) {
					for (i=0; i<n.childNodes.length; i++)
						tinyMCE.selectNodes(n.childNodes[i], f, a);
				}

				return a;
			},

			getContent : function() {
				return EditorManager.activeEditor.getContent();
			},

			getParentElement : function(n, na, f) {
				if (na)
					na = new RegExp('^(' + na.toUpperCase().replace(/,/g, '|') + ')$', 'g');

				return this._getDOM().getParent(n, function(n) {
					return n.nodeType == 1 && (!na || na.test(n.nodeName)) && (!f || f(n));
				}, this.activeEditor.getBody());
			},

			importPluginLanguagePack : function(n) {
				tinymce.PluginManager.requireLangPack(n);
			},

			getButtonHTML : function(cn, lang, img, c, u, v) {
				var ed = EditorManager.activeEditor;

				img = img.replace(/\{\$pluginurl\}/g, tinyMCE.pluginURL);
				img = img.replace(/\{\$themeurl\}/g, tinyMCE.themeURL);
				lang = lang.replace(/^lang_/g, '');

				return ed.controlManager.createButton(cn, {
					title : lang,
					command : c,
					ui : u,
					value : v,
					scope : this,
					'class' : 'compat',
					image : img
				});
			},

			addSelectAccessibility : function(e, s, w) {
				// Add event handlers 
				if (!s._isAccessible) {
					s.onkeydown = tinyMCE.accessibleEventHandler;
					s.onblur = tinyMCE.accessibleEventHandler;
					s._isAccessible = true;
					s._win = w;
				}

				return false;
			},

			accessibleEventHandler : function(e) {
				var elm, win = this._win;

				e = tinymce.isIE ? win.event : e;
				elm = tinymce.isIE ? e.srcElement : e.target;

				// Unpiggyback onchange on blur
				if (e.type == "blur") {
					if (elm.oldonchange) {
						elm.onchange = elm.oldonchange;
						elm.oldonchange = null;
					}

					return true;
				}

				// Piggyback onchange
				if (elm.nodeName == "SELECT" && !elm.oldonchange) {
					elm.oldonchange = elm.onchange;
					elm.onchange = null;
				}

				// Execute onchange and remove piggyback
				if (e.keyCode == 13 || e.keyCode == 32) {
					elm.onchange = elm.oldonchange;
					elm.onchange();
					elm.oldonchange = null;

					tinyMCE.cancelEvent(e);
					return false;
				}

				return true;
			},

			cancelEvent : function(e) {
				return tinymce.dom.Event.cancel(e);
			},

			handleVisualAid : function(e) {
				EditorManager.activeEditor.addVisual(e);
			},

			getAbsPosition : function(n, r) {
				return tinymce.DOM.getPos(n, r);
			},

			cleanupEventStr : function(s) {
				s = "" + s;
				s = s.replace('function anonymous()\n{\n', '');
				s = s.replace('\n}', '');
				s = s.replace(/^return true;/gi, ''); // Remove event blocker

				return s;
			},

			getVisualAidClass : function(s) {
				// TODO: Implement
				return s;
			},

			parseStyle : function(s) {
				return this._getDOM().parseStyle(s);
			},

			serializeStyle : function(s) {
				return this._getDOM().serializeStyle(s);
			},

			openWindow : function(tpl, args) {
				var ed = EditorManager.activeEditor, o = {}, n;

				// Convert name/value array to object
				for (n in tpl)
					o[n] = tpl[n];

				tpl = o;

				args = args || {};
				tpl.url = new tinymce.util.URI(tinymce.ThemeManager.themeURLs[ed.settings.theme]).toAbsolute(tpl.file);
				tpl.inline = tpl.inline || args.inline;

				ed.windowManager.open(tpl, args);
			},

			closeWindow : function(win) {
				EditorManager.activeEditor.windowManager.close(win);
			},

			getOuterHTML : function(e) {
				return tinymce.DOM.getOuterHTML(e);
			},

			setOuterHTML : function(e, h, d) {
				return tinymce.DOM.setOuterHTML(e, h, d);
			},

			hasPlugin : function(n) {
				return tinymce.PluginManager.get(n) != null;
			},

			_setEventsEnabled : function() {
				// Ignore it!!
			},

			addPlugin : function(pn, f) {
				var t = this;

				function PluginWrapper(ed) {
					tinyMCE.selectedInstance = ed;

					ed.onInit.add(function() {
						t.settings = ed.settings;
						t.settings['base_href'] = tinyMCE.documentBasePath;
						tinyMCE.settings = t.settings;
						tinyMCE.documentBasePath = ed.documentBasePath;
						//ed.formElement = DOM.get(ed.id);

						if (f.initInstance)
							f.initInstance(ed);

						ed.contentDocument = ed.getDoc();
						ed.contentWindow = ed.getWin();
						ed.undoRedo = ed.undoManager;
						ed.startContent = ed.getContent({format : 'raw'});

						tinyMCE.instances[ed.id] = ed;
						tinyMCE.loadedFiles = [];
					});

					ed.onActivate.add(function() {
						tinyMCE.settings = ed.settings;
						tinyMCE.selectedInstance = ed;
					});

				/*	if (f.removeInstance) {
						ed.onDestroy.add(function() {
							return f.removeInstance(ed.id);
						});
					}*/

					if (f.handleNodeChange) {
						ed.onNodeChange.add(function(ed, cm, n) {
							f.handleNodeChange(ed.id, n, 0, 0, false, !ed.selection.isCollapsed());
						});
					}

					if (f.onChange) {
						ed.onChange.add(function(ed, n) {
							return f.onChange(ed);
						});
					}

					if (f.cleanup) {
						ed.onGetContent.add(function() {
							//f.cleanup(type, content, inst);
						});
					}

					this.getInfo = function() {
						return f.getInfo();
					};

					this.createControl = function(n) {
						tinyMCE.pluginURL = tinymce.baseURL + '/plugins/' + pn;
						tinyMCE.themeURL = tinymce.baseURL + '/themes/' + tinyMCE.activeEditor.settings.theme;

						if (f.getControlHTML)
							return f.getControlHTML(n);

						return null;
					};

					this.execCommand = function(cmd, ui, val) {
						if (f.execCommand)
							return f.execCommand(ed.id, ed.getBody(), cmd, ui, val);

						return false;
					};
				};

				tinymce.PluginManager.add(pn, PluginWrapper);
			},

			_getDOM : function() {
				return tinyMCE.activeEditor ? tinyMCE.activeEditor.dom : tinymce.DOM;
			},

			convertRelativeToAbsoluteURL : function(b, u) {
				return new tinymce.util.URI(b).toAbsolute(u);
			},

			convertAbsoluteURLToRelativeURL : function(b, u) {
				return new tinymce.util.URI(b).toRelative(u);
			}
		});

		// Extend Editor class
		tinymce.extend(tinymce.Editor.prototype, {
			getFocusElement : function() {
				return this.selection.getNode();
			},

			getData : function(n) {
				if (!this.data)
					this.data = [];

				if (!this.data[n])
					this.data[n] = [];

				return this.data[n];
			},

			hasPlugin : function(n) {
				return this.plugins[n] != null;
			},

			getContainerWin : function() {
				return window;
			},

			getHTML : function(raw) {
				return this.getContent({ format : raw ? 'raw' : 'html'});
			},

			setHTML : function(h) {
				this.setContent(h);
			},

			getSel : function() {
				return this.selection.getSel();
			},

			getRng : function() {
				return this.selection.getRng();
			},

			isHidden : function() {
				var s;

				if (!tinymce.isGecko)
					return false;

				s = this.getSel();

				// Weird, wheres that cursor selection?
				return (!s || !s.rangeCount || s.rangeCount == 0);
			},

			translate : function(s) {
				var c = this.settings.language, o;

				if (!s)
					return s;

				o = tinymce.EditorManager.i18n[c + '.' + s] || s.replace(/{\#([^}]+)\}/g, function(a, b) {
					return tinymce.EditorManager.i18n[c + '.' + b] || '{#' + b + '}';
				});

				o = o.replace(/{\$lang_([^}]+)\}/g, function(a, b) {
					return tinymce.EditorManager.i18n[c + '.' + b] || '{$lang_' + b + '}';
				});

				return o;
			},

			repaint : function() {
				this.execCommand('mceRepaint');
			}
		});

		// Extend selection
		tinymce.extend(tinymce.dom.Selection.prototype, {
			getSelectedText : function() {
				return this.getContent({format : 'text'});
			},

			getSelectedHTML : function() {
				return this.getContent({format : 'html'});
			},

			getFocusElement : function() {
				return this.getNode();
			},

			selectNode : function(node, collapse, select_text_node, to_start) {
				var t = this;

				t.select(node, select_text_node || 0);

				if (!is(collapse))
					collapse = true;

				if (collapse) {
					if (!is(to_start))
						to_start = true;

					t.collapse(to_start);
				}
			}
		});
	}).call(this);

	// Register plugin
	tinymce.PluginManager.add('compat2x', tinymce.plugins.Compat2x);
})();

