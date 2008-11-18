/**
 * $Id: editor_plugin_src.js 162 2007-01-03 16:16:52Z spocke $
 *
 * @author Moxiecode
 * @copyright Copyright © 2004-2007, Moxiecode Systems AB, All rights reserved.
 */

tinyMCE.importPluginLanguagePack('template');

var TinyMCE_TemplatePlugin = {
	getInfo : function() {
		return {
			longname : 'Template plugin',
			author : 'Moxiecode Systems AB',
			authorurl : 'http://www.moxiecode.com',
			infourl : 'http://wiki.moxiecode.com/index.php/TinyMCE:Plugins/template',
			version : tinyMCE.majorVersion + "." + tinyMCE.minorVersion
		};
	},

	initInstance : function(inst) {
		var cdate, mdate, content, x = 0, key, value, rvals, ds = inst.getData('template');

		// ensure the required elements and sttributes are added
		//inst.cleanup.addRuleStr('*[' + TinyMCE_TemplatePlugin.TMPL_DATE_SRC_ATTR + '],div[title,tsrc]');

		//setup template content functions
		// creation date and modified date classes
		cdate = tinyMCE.getParam("template_cdate_classes", '').split(/\s+/);
		mdate = tinyMCE.getParam("template_mdate_classes", '').split(/\s+/);

		// classes that specify where selected content should go
		content = tinyMCE.getParam("template_selected_content_classes", '').split(/\s+/);

		for (x = 0; x < cdate.length; x++)
			TinyMCE_TemplatePlugin.functions[cdate[x]] = TinyMCE_TemplatePlugin.functions['cdate'];	

		for (x = 0; x < mdate.length; x++)
			TinyMCE_TemplatePlugin.functions[mdate[x]] = TinyMCE_TemplatePlugin.functions['mdate'];

		for (x = 0; x < content.length; x++)
			TinyMCE_TemplatePlugin.functions[content[x]] = TinyMCE_TemplatePlugin.functions['selectedContent'];

		// special template functions for replacing template content
		rvals = tinyMCE.getParam("template_replace_values", false);
		for (key in rvals) {
			value = rvals[key];

			if (typeof value == "function")
				TinyMCE_TemplatePlugin.functions[key] = value;
			else
				TinyMCE_TemplatePlugin.functions[key] = TinyMCE_TemplatePlugin.functions['generateReplacer'](value);
		}

		// Setup replace_items
		rvals = tinyMCE.getParam("template_replace_values", false);
		ds.replace_items = {};

		for (key in rvals)
			ds.replace_items[key] = rvals[key];

		inst.addShortcut('ctrl', 't', 'lang_template_desc', 'mceTemplate');

		// Setup data storage
		ds.currentAction = "insert";
		ds.currentTmplNode = null;
	},

	getControlHTML : function(cn) {
		switch (cn) {
			case "template":
				return tinyMCE.getButtonHTML(cn, 'lang_template_desc', '{$pluginurl}/images/template.gif', 'mceTemplate', true);
		}

		return "";
	},

	execCommand : function(editor_id, element, command, user_interface, value) {
		var nodeArray, current, newTmpl, x, inst = tinyMCE.getInstanceById(editor_id), ds = inst.getData('template'), telm;

		switch (command) {
			case "mceTemplate":
				if (user_interface) {
					// called from toolbar button - show the popup
					tinyMCE.openWindow({
						file : '../../plugins/template/template.htm', // Relative to theme
						width : tinyMCE.getParam('template_popup_width', 750),
						height : tinyMCE.getParam('template_popup_height', 600)
					}, {editor_id : editor_id, resizable : "yes", scrollbars : "no", pluginObj : TinyMCE_TemplatePlugin});
				} else {
					// internal command do the template stuff

					// get the returned HTML string from the pop-up and appened it to a DIV element
					telm = TinyMCE_TemplatePlugin._convertToNode(value.body);

					// Find template body
					nodeArray = tinyMCE.selectElements(telm, 'div', function(n) {
						return tinyMCE.hasCSSClass(n, TinyMCE_TemplatePlugin.TMPL);
					});

					telm = nodeArray.length > 0 ? nodeArray[0] : null;
					nodeArray = [];

					if (ds.currentAction == "insert") {
						//insert new template after applying all the template content functions

						// Is it a template or snippet
						if (telm) {
							tinyMCE.execCommand('mceBeginUndoLevel');
							ds.currentAction = "insert-new";
							TinyMCE_TemplatePlugin._insertTemplate(editor_id, telm, value.title, value.tsrc, true);
							ds.currentAction == "insert";
							tinyMCE.execCommand('mceEndUndoLevel');
							tinyMCE.execInstanceCommand(editor_id, 'mceCleanup', false);
						} else
							tinyMCE.execCommand('mceInsertContent', false, this._replaceValues(value.body));
					} else {
						// First collect the selected template in the editor
						nodeArray = TinyMCE_TemplatePlugin._collectTemplateElements(ds.currentTmplNode);
						current = [];
						newTmpl = [];
						tinyMCE.getNodeTree(telm, newTmpl);

						for (x=0; x<nodeArray.length; x++)
							tinyMCE.getNodeTree(nodeArray[x], current);

						/** 
						 * inner function used in the loop below.
						 * compares the supplied HTML element to the new template to:
						 * - find a match with the new template and copy the element's content over
						 * - find no match and indicate content will be lost
						 */
						var _test = function(elm) {
							var replaced = true;

							if (elm.className) {
								var names = elm.className.split(/\s+/), c, n;

								for (c = 0; c<names.length; c++) {
									if (names[c].match(/^mce/i))
										continue; // ignore all internal class names

									for (n=0; n<newTmpl.length; n++){
										replaced = false;

										if (newTmpl[n].className && newTmpl[n].className.match(new RegExp(names[c], "gi"))) {
											newTmpl[n].innerHTML = elm.innerHTML;
											//if(tinyMCE.getAttrib(elm,TinyMCE_TemplatePlugin.TMPL_DATE_SRC_ATTR,"") != "") {
											//	tinyMCE.setAttrib(newTmpl[n], TinyMCE_TemplatePlugin.TMPL_DATE_SRC_ATTR, tinyMCE.getAttrib(elm,TinyMCE_TemplatePlugin.TMPL_DATE_SRC_ATTR)); 
											//}
											replaced = true;
											break;
										}

									}
								}
							}
	
							return replaced;
						};

						// comparison loop - first mis-match alerts user for confirmation.
						var cont = true;
						var asked = false;

						for (x = 0; x < current.length; x++) {
							if(!_test(current[x])) {
								cont = (asked || confirm("The new template has less elements than the currently selected content.\nIf you proceed you will loose content.\nAre you sure you want to proceed?", "Proceed?"));
								asked = true;

								if (!cont)
									break;
							}
						};

						// apply replacement if allowed to
						if (cont) {
							tinyMCE.execCommand('mceBeginUndoLevel');
							TinyMCE_TemplatePlugin._replaceTemplateContent(current[0], editor_id, telm, value.title, value.tsrc);
							tinyMCE.execCommand('mceEndUndoLevel');
							tinyMCE.execInstanceCommand(editor_id, 'mceCleanup', false);
						}
					}

					tinyMCE.triggerNodeChange(true);
				}

				return true;
		}

		return false;
	},

	handleNodeChange : function(editor_id, node, undo_index, undo_levels, visual_aid, any_selection) {
		var inst = tinyMCE.getInstanceById(editor_id), ds = inst.getData('template');

		if (tinyMCE.hasCSSClass(node, TinyMCE_TemplatePlugin.TMPL_ELEMENT) || tinyMCE.hasCSSClass(node.parentNode, TinyMCE_TemplatePlugin.TMPL_ELEMENT)) {
			tinyMCE.switchClass(editor_id + '_template', 'mceButtonSelected');
			ds.currentAction = "update";
			ds.currentTmplNode = node;

			return true;
		}

		ds.currentAction = "insert";
		ds.currentTmplNode = null;
		tinyMCE.switchClass(editor_id + '_template', 'mceButtonNormal');

		return false;
	},

	cleanup : function(type, content, inst) {
		var nodes = [];

		switch (type) {
			case "get_from_editor":
				// replace the opening wrapper div tag with a HTML comment
				content = content.replace(
					new RegExp('<div class="' + TinyMCE_TemplatePlugin.TMPL + '">', 'gi'),
					'<!-- ' + TinyMCE_TemplatePlugin.TMPL_BEGINS + ' -->'
				);

				// delete any empty template wrappers
				content = content.replace(
					new RegExp('<div class="' + TinyMCE_TemplatePlugin.TMPL + '">(\s|&nbsp;|&#160;)?(<!-- ' + TinyMCE_TemplatePlugin.TMPL_ENDS + ' -->|\s)?</div>', 'gi'),
					''
				);

				// replace the closing wrapper tag
				content = content.replace(
					new RegExp('<!-- ' + TinyMCE_TemplatePlugin.TMPL_ENDS + ' --></div>', 'gi'),
					'<!-- ' + TinyMCE_TemplatePlugin.TMPL_ENDS + ' -->'
				);

				break;

			case "insert_to_editor":
				// replace HTML comment with DIV wrapper
				content = content.replace(
					new RegExp('<!-- ' + TinyMCE_TemplatePlugin.TMPL_BEGINS + ' -->', 'gi'),
					'<div class="' + TinyMCE_TemplatePlugin.TMPL + '">'
				);

				content = content.replace(
					new RegExp('<!-- ' + TinyMCE_TemplatePlugin.TMPL_ENDS + ' -->', 'gi'),
					'<!-- ' + TinyMCE_TemplatePlugin.TMPL_ENDS + ' --></div>'
				);

				break;

			case "get_from_editor_dom":
				// apply template content replacement functions
				nodes = tinyMCE.selectNodes(content, function(n) {
						return tinyMCE.hasCSSClass(n, TinyMCE_TemplatePlugin.TMPL_ELEMENT);
					}
				);

				TinyMCE_TemplatePlugin._applyFunctions(nodes, type);

				break;

			case "insert_to_editor_dom":
				// apply template content replacement functions
				nodes = tinyMCE.selectNodes(content, function(n) {
						return tinyMCE.hasCSSClass(n, TinyMCE_TemplatePlugin.TMPL_ELEMENT);
					}
				);

				TinyMCE_TemplatePlugin._applyFunctions(nodes, type);

				break;
		}

		return content;
	},

	// Private plugin internal methods

	/**
	 * Creates a HTML DIV element and sets the innerHTML to equal the temlate innerHTML so that the template can be manipulated as DOM nodes.
	 *
	 * @param {string} Template innerHTML
	 * @return a HTML Element
	 * @type HTMLElement
	 */
	_convertToNode : function(html) {
		var elm = document.createElement('div');

		elm.innerHTML = html;

		return elm;
	},

	/**
	 * pass an array of template html elements and they will have the template class name added and any template functions applied
	 *
	 * @param {array} template HTML elements
	 * @return array of template HTML elements
	 * @type array
	 */
	_prepareTemplateContent : function(elms) {
		var x, n, nodes = [];

		if (!elms)
			return {};

		if (!elms.length)
			elms = [elms];

		for (x = 0; x<elms.length; x++)
			tinyMCE.getNodeTree(elms[x], nodes, 1);

		for (n = 0; n<nodes.length; n++) {
			tinyMCE.addCSSClass(nodes[n], TinyMCE_TemplatePlugin.TMPL_ELEMENT);
			TinyMCE_TemplatePlugin._applyFunctions(nodes[n], TinyMCE_TemplatePlugin.TMPL_TEMPLATE_EVENT);
		}

		return elms;
	},

	_replaceValues : function(s) {
		var t = this, ds = tinyMCE.selectedInstance.getData('template');

		return s.replace(/\{\$([^\}]+)\}/g, function(a, b) {
			var it = ds.replace_items[b];

			if (it) {
				// Only supports text for now
				if (typeof(it) != 'function')
					return it;
			}

			return b;
		});
	},

	/**
	 * Applies any special functions to the template elements
	 *
	 * @param {array} template HTML elements
	 * @return array of template HTML elements
	 * @type array
	 */
	_applyFunctions : function(elms, editor_event) {
		var x, elm, names, c, f;

		if (!elms)
			return {};

		if (!elms.length)
			elms = [elms];

		for(x = 0; x < elms.length; x++) {
			elm = elms[x];

			if (elm.className){
				names = elm.className.split(/\s+/);

				for (c = 0; c < names.length; c++){
					if (names[c] == TinyMCE_TemplatePlugin.TMPL_ELEMENT)
						continue;

					f = (TinyMCE_TemplatePlugin.functions[names[c]] ? TinyMCE_TemplatePlugin.functions[names[c]] : TinyMCE_TemplatePlugin.functions['blank']);
					f(elm, editor_event);
				}
			}
		}

		return elms;
	},

	/**
	 * Given one node reference this function will collect all the nodes of the template to which it belongs.
	 * It does this by finding the parent template wrapper DIV and returning all child nodes.
	 *
	 * @param {HTMLElement} a HTMLElement which is part of a template
	 * @return array of template HTML elements
	 * @type array
	 */
	_collectTemplateElements : function(node) {
		var nodeArray = [], p;

		p = tinyMCE.getParentElement(node, 'DIV', function(n) {
			return tinyMCE.hasCSSClass(n, TinyMCE_TemplatePlugin.TMPL);
		});

		if (p)
			tinyMCE.getNodeTree(p, nodeArray);

		return nodeArray;
	},

	/**
	 * Simply calls TinyMCE_TemplatePlugin._deleteTemplateContent and then TinyMCE_TemplatePlugin._insertTemplate
	 *
	 * @param {HTMLElement} currently selected template node in editor
	 * @param {string} id of editor instance
	 * @param {HTMLElement} template contents as a HTMLElement (the parent DIV wrapper)
	 * @param {string} title of template (unused as yet)
	 * @param {string} source URI of the template file (unused as yet)
	 * @return array of template HTML elements
	 * @type array
	 */
	_replaceTemplateContent : function(currentNode, editor_id, newTemplate, title, tsrc) {
		TinyMCE_TemplatePlugin._deleteTemplateContent(currentNode);
		TinyMCE_TemplatePlugin._insertTemplate(editor_id, newTemplate, title, tsrc, false);
	},

	/**
	 * Deletes a template from the editor content
	 * Finds the parent DIV wrapper and deletes it and all children
	 * @param {HTMLElement} currently selected template node in editor
	 */
	_deleteTemplateContent : function(node) {
		var p = tinyMCE.getParentElement(node, 'DIV', function(n) {
			return tinyMCE.hasCSSClass(n, TinyMCE_TemplatePlugin.TMPL);
		});

		if (p)
			p.parentNode.removeChild(p, true);
	},

	/**
	 *  Inserts a template into the specified editor
	 *
	 * @param {string} id of editor instance
	 * @param {HTMLElement} template contents as a HTMLElement (the parent DIV wrapper)
	 * @param {string} title of template (unused as yet)
	 * @param {string} source URI of the template file (unused as yet)
	 */
	_insertTemplate : function(editor_id, elm, title, tsrc, incComments) {
		var html;

		TinyMCE_TemplatePlugin._prepareTemplateContent(elm);

		html = '<div class="' + TinyMCE_TemplatePlugin.TMPL + '">';
		html += elm.innerHTML;
		html += '<!-- ' + TinyMCE_TemplatePlugin.TMPL_ENDS + ' --></div>';

		tinyMCE.execInstanceCommand(editor_id, 'mceInsertContent', false, html);
	},

	/**
	 * template functions - functions for modifying template content
	 */
	functions : { 
		blank : function(elm, editor_event) {},

		cdate : function(elm, editor_event) {
			var d, dsrc;

			if (editor_event != TinyMCE_TemplatePlugin.TMPL_TEMPLATE_EVENT)
				return;

			d = new Date();
			// find out if the creation date was previously stored
			dsrc = elm.innerHTML.match(new RegExp("<!-- " + TinyMCE_TemplatePlugin.TMPL_DATE_SRC_ATTR + ":(.*)  -->", "gi"));

			if (dsrc)
				d = new Date(RegExp.$1);

			elm.innerHTML = TinyMCE_TemplatePlugin._getDateTime(d, tinyMCE.getParam("template_cdate_format", tinyMCE.getLang("lang_template_def_date_format")));
			//now we have to store the date value in a format easily read again, in case a future template change changes the date format...
			elm.innerHTML += "<!-- " + TinyMCE_TemplatePlugin.TMPL_DATE_SRC_ATTR + ":" + d.toUTCString() + "  -->";
		},

		mdate : function(elm, editor_event) {
			var d = new Date();
			elm.innerHTML = TinyMCE_TemplatePlugin._getDateTime(d, tinyMCE.getParam("template_mdate_format", tinyMCE.getLang("lang_template_def_date_format")));
		},

		/**
		 * This will insert the currently selected editor content into the template element.
		 * It only does this if the template inserted is a new one and if the element does not have the special class.
		 * The special class name prevents this from happening more than once.
		 */
		selectedContent : function(elm, editor_event) {
			var ds = tinyMCE.selectedInstance.getData('template');
	
			if (editor_event != TinyMCE_TemplatePlugin.TMPL_TEMPLATE_EVENT)
				return;

			if (ds.currentAction == "insert-new" && !tinyMCE.hasCSSClass(elm, TinyMCE_TemplatePlugin.TMPL_SEL_HTML_DONE)) {
				elm.innerHTML = tinyMCE.selectedInstance.selection.getSelectedHTML();
				tinyMCE.addCSSClass(elm, TinyMCE_TemplatePlugin.TMPL_SEL_HTML_DONE);
			}
		},

		/**
		 * When the plugin is initialised this generates the functions that insert configured strings into template elements.
		 */
		generateReplacer : function(s) {
			return function(elm, editor_event) {elm.innerHTML = "" + s;};
		}
	},

	/**
	 * formats a date according to the format string - straight from the 'insert date/time' plugin
	 *
	 * @param {Date} date object
	 * @param {string} format string
	 * @return formatted date
	 * @type string
	 */
	_getDateTime : function(d,fmt) {
			if (!fmt)
				return "";

			function addZeros(value, len) {
				var i;

				value = "" + value;

				if (value.length < len) {
					for (i=0; i<(len-value.length); i++)
						value = "0" + value;
				}

				return value;
			}

			fmt = fmt.replace("%D", "%m/%d/%y");
			fmt = fmt.replace("%r", "%I:%M:%S %p");
			fmt = fmt.replace("%Y", "" + d.getFullYear());
			fmt = fmt.replace("%y", "" + d.getYear());
			fmt = fmt.replace("%m", addZeros(d.getMonth()+1, 2));
			fmt = fmt.replace("%d", addZeros(d.getDate(), 2));
			fmt = fmt.replace("%H", "" + addZeros(d.getHours(), 2));
			fmt = fmt.replace("%M", "" + addZeros(d.getMinutes(), 2));
			fmt = fmt.replace("%S", "" + addZeros(d.getSeconds(), 2));
			fmt = fmt.replace("%I", "" + ((d.getHours() + 11) % 12 + 1));
			fmt = fmt.replace("%p", "" + (d.getHours() < 12 ? "AM" : "PM"));
			fmt = fmt.replace("%B", "" + tinyMCE.getLang("lang_template_months_long")[d.getMonth()]);
			fmt = fmt.replace("%b", "" + tinyMCE.getLang("lang_template_months_short")[d.getMonth()]);
			fmt = fmt.replace("%A", "" + tinyMCE.getLang("lang_template_day_long")[d.getDay()]);
			fmt = fmt.replace("%a", "" + tinyMCE.getLang("lang_template_day_short")[d.getDay()]);
			fmt = fmt.replace("%%", "%");

			return fmt;
	},

	TMPL_ELEMENT : 'mceTmplElm',
	TMPL : 'mceTmpl',
	TMPL_BEGINS : 'mceTmplBegins',
	TMPL_SEL_HTML_DONE : 'mceSelHTMLDone',
	TMPL_ENDS : 'mceTmplEnds',
	TMPL_DATE_SRC_ATTR : 'mcetmpldtesrc',
	TMPL_TEMPLATE_EVENT : 'prepare_template'
};

tinyMCE.addPlugin("template", TinyMCE_TemplatePlugin);
