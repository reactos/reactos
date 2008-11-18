// Context Menu Plugin for HTMLArea-3.0
// Sponsored by www.americanbible.org
// Implementation by Mihai Bazon, http://dynarch.com/mishoo/
//
// (c) dynarch.com 2003.
// Distributed under the same terms as HTMLArea itself.
// This notice MUST stay intact for use (see license.txt).
//
// $Id: context-menu.js,v 1.2 2005/01/11 15:00:55 garvinhicking Exp $

HTMLArea.loadStyle("menu.css", "ContextMenu");

function ContextMenu(editor) {
	this.editor = editor;
};

ContextMenu._pluginInfo = {
	name          : "ContextMenu",
	version       : "1.0",
	developer     : "Mihai Bazon",
	developer_url : "http://dynarch.com/mishoo/",
	c_owner       : "dynarch.com",
	sponsor       : "American Bible Society",
	sponsor_url   : "http://www.americanbible.org",
	license       : "htmlArea"
};

ContextMenu.prototype.onGenerate = function() {
	var self = this;
	var doc = this.editordoc = this.editor._iframe.contentWindow.document;
	HTMLArea._addEvents(doc, ["contextmenu"],
			    function (event) {
				    return self.popupMenu(HTMLArea.is_ie ? self.editor._iframe.contentWindow.event : event);
			    });
	this.currentMenu = null;
};

ContextMenu.prototype.getContextMenu = function(target) {
	var self = this;
	var editor = this.editor;
	var config = editor.config;
	var menu = [];
	var tbo = this.editor.plugins.TableOperations;
	if (tbo) tbo = tbo.instance;
	var i18n = ContextMenu.I18N;

	var selection = editor.hasSelectedText();
	if (selection)
		menu.push([ i18n["Cut"], function() { editor.execCommand("cut"); }, null, config.btnList["cut"][1] ],
			  [ i18n["Copy"], function() { editor.execCommand("copy"); }, null, config.btnList["copy"][1] ]);
	menu.push([ i18n["Paste"], function() { editor.execCommand("paste"); }, null, config.btnList["paste"][1] ]);

	var currentTarget = target;
	var elmenus = [];

	var link = null;
	var table = null;
	var tr = null;
	var td = null;
	var img = null;

	function tableOperation(opcode) {
		tbo.buttonPress(editor, opcode);
	};

	function insertPara(after) {
		var el = currentTarget;
		var par = el.parentNode;
		var p = editor._doc.createElement("p");
		p.appendChild(editor._doc.createElement("br"));
		par.insertBefore(p, after ? el.nextSibling : el);
		var sel = editor._getSelection();
		var range = editor._createRange(sel);
		if (!HTMLArea.is_ie) {
			sel.removeAllRanges();
			range.selectNodeContents(p);
			range.collapse(true);
			sel.addRange(range);
		} else {
			range.moveToElementText(p);
			range.collapse(true);
			range.select();
		}
	};

	for (; target; target = target.parentNode) {
		var tag = target.tagName;
		if (!tag)
			continue;
		tag = tag.toLowerCase();
		switch (tag) {
		    case "img":
			img = target;
			elmenus.push(null,
				     [ i18n["Image Properties"],
				       function() {
					       editor._insertImage(img);
				       },
				       i18n["Show the image properties dialog"],
				       config.btnList["insertimage"][1] ]
				);
			break;
		    case "a":
			link = target;
			elmenus.push(null,
				     [ i18n["Modify Link"],
				       function() { editor.execCommand("createlink", true); },
				       i18n["Current URL is"] + ': ' + link.href,
				       config.btnList["createlink"][1] ],

				     [ i18n["Check Link"],
				       function() { window.open(link.href); },
				       i18n["Opens this link in a new window"] ],

				     [ i18n["Remove Link"],
				       function() {
					       if (confirm(i18n["Please confirm that you want to unlink this element."] + "\n" +
							   i18n["Link points to:"] + " " + link.href)) {
						       while (link.firstChild)
							       link.parentNode.insertBefore(link.firstChild, link);
						       link.parentNode.removeChild(link);
					       }
				       },
				       i18n["Unlink the current element"] ]
				);
			break;
		    case "td":
			td = target;
			if (!tbo) break;
			elmenus.push(null,
				     [ i18n["Cell Properties"],
				       function() { tableOperation("TO-cell-prop"); },
				       i18n["Show the Table Cell Properties dialog"],
				       config.btnList["TO-cell-prop"][1] ]
				);
			break;
		    case "tr":
			tr = target;
			if (!tbo) break;
			elmenus.push(null,
				     [ i18n["Row Properties"],
				       function() { tableOperation("TO-row-prop"); },
				       i18n["Show the Table Row Properties dialog"],
				       config.btnList["TO-row-prop"][1] ],

				     [ i18n["Insert Row Before"],
				       function() { tableOperation("TO-row-insert-above"); },
				       i18n["Insert a new row before the current one"],
				       config.btnList["TO-row-insert-above"][1] ],

				     [ i18n["Insert Row After"],
				       function() { tableOperation("TO-row-insert-under"); },
				       i18n["Insert a new row after the current one"],
				       config.btnList["TO-row-insert-under"][1] ],

				     [ i18n["Delete Row"],
				       function() { tableOperation("TO-row-delete"); },
				       i18n["Delete the current row"],
				       config.btnList["TO-row-delete"][1] ]
				);
			break;
		    case "table":
			table = target;
			if (!tbo) break;
			elmenus.push(null,
				     [ i18n["Table Properties"],
				       function() { tableOperation("TO-table-prop"); },
				       i18n["Show the Table Properties dialog"],
				       config.btnList["TO-table-prop"][1] ],

				     [ i18n["Insert Column Before"],
				       function() { tableOperation("TO-col-insert-before"); },
				       i18n["Insert a new column before the current one"],
				       config.btnList["TO-col-insert-before"][1] ],

				     [ i18n["Insert Column After"],
				       function() { tableOperation("TO-col-insert-after"); },
				       i18n["Insert a new column after the current one"],
				       config.btnList["TO-col-insert-after"][1] ],

				     [ i18n["Delete Column"],
				       function() { tableOperation("TO-col-delete"); },
				       i18n["Delete the current column"],
				       config.btnList["TO-col-delete"][1] ]
				);
			break;
		    case "body":
			elmenus.push(null,
				     [ i18n["Justify Left"],
				       function() { editor.execCommand("justifyleft"); }, null,
				       config.btnList["justifyleft"][1] ],
				     [ i18n["Justify Center"],
				       function() { editor.execCommand("justifycenter"); }, null,
				       config.btnList["justifycenter"][1] ],
				     [ i18n["Justify Right"],
				       function() { editor.execCommand("justifyright"); }, null,
				       config.btnList["justifyright"][1] ],
				     [ i18n["Justify Full"],
				       function() { editor.execCommand("justifyfull"); }, null,
				       config.btnList["justifyfull"][1] ]
				);
			break;
		}
	}

	if (selection && !link)
		menu.push(null, [ i18n["Make link"],
				  function() { editor.execCommand("createlink", true); },
				  i18n["Create a link"],
				  config.btnList["createlink"][1] ]);

	for (var i = 0; i < elmenus.length; ++i)
		menu.push(elmenus[i]);

	if (!/html|body/i.test(currentTarget.tagName))
		menu.push(null,
			  [ i18n["Remove the"] + " &lt;" + currentTarget.tagName + "&gt; " + i18n["Element"],
			    function() {
				    if (confirm(i18n["Please confirm that you want to remove this element:"] + " " +
						currentTarget.tagName)) {
					    var el = currentTarget;
					    var p = el.parentNode;
					    p.removeChild(el);
					    if (HTMLArea.is_gecko) {
						    if (p.tagName.toLowerCase() == "td" && !p.hasChildNodes())
							    p.appendChild(editor._doc.createElement("br"));
						    editor.forceRedraw();
						    editor.focusEditor();
						    editor.updateToolbar();
						    if (table) {
							    var save_collapse = table.style.borderCollapse;
							    table.style.borderCollapse = "collapse";
							    table.style.borderCollapse = "separate";
							    table.style.borderCollapse = save_collapse;
						    }
					    }
				    }
			    },
			    i18n["Remove this node from the document"] ],
			  [ i18n["Insert paragraph before"],
			    function() { insertPara(false); },
			    i18n["Insert a paragraph before the current node"] ],
			  [ i18n["Insert paragraph after"],
			    function() { insertPara(true); },
			    i18n["Insert a paragraph after the current node"] ]
			  );
	return menu;
};

ContextMenu.prototype.popupMenu = function(ev) {
	var self = this;
	var i18n = ContextMenu.I18N;
	if (this.currentMenu)
		this.currentMenu.parentNode.removeChild(this.currentMenu);
	function getPos(el) {
		var r = { x: el.offsetLeft, y: el.offsetTop };
		if (el.offsetParent) {
			var tmp = getPos(el.offsetParent);
			r.x += tmp.x;
			r.y += tmp.y;
		}
		return r;
	};
	function documentClick(ev) {
		ev || (ev = window.event);
		if (!self.currentMenu) {
			alert(i18n["How did you get here? (Please report!)"]);
			return false;
		}
		var el = HTMLArea.is_ie ? ev.srcElement : ev.target;
		for (; el != null && el != self.currentMenu; el = el.parentNode);
		if (el == null)
			self.closeMenu();
		//HTMLArea._stopEvent(ev);
		//return false;
	};
	var keys = [];
	function keyPress(ev) {
		ev || (ev = window.event);
		HTMLArea._stopEvent(ev);
		if (ev.keyCode == 27) {
			self.closeMenu();
			return false;
		}
		var key = String.fromCharCode(HTMLArea.is_ie ? ev.keyCode : ev.charCode).toLowerCase();
		for (var i = keys.length; --i >= 0;) {
			var k = keys[i];
			if (k[0].toLowerCase() == key)
				k[1].__msh.activate();
		}
	};
	self.closeMenu = function() {
		self.currentMenu.parentNode.removeChild(self.currentMenu);
		self.currentMenu = null;
		HTMLArea._removeEvent(document, "mousedown", documentClick);
		HTMLArea._removeEvent(self.editordoc, "mousedown", documentClick);
		if (keys.length > 0)
			HTMLArea._removeEvent(self.editordoc, "keypress", keyPress);
		if (HTMLArea.is_ie)
			self.iePopup.hide();
	};
	var target = HTMLArea.is_ie ? ev.srcElement : ev.target;
	var ifpos = getPos(self.editor._iframe);
	var x = ev.clientX + ifpos.x;
	var y = ev.clientY + ifpos.y;

	var div;
	var doc;
	if (!HTMLArea.is_ie) {
		doc = document;
	} else {
		// IE stinks
		var popup = this.iePopup = window.createPopup();
		doc = popup.document;
		doc.open();
		doc.write("<html><head><style type='text/css'>@import url(" + _editor_url + "plugins/ContextMenu/menu.css); html, body { padding: 0px; margin: 0px; overflow: hidden; border: 0px; }</style></head><body unselectable='yes'></body></html>");
		doc.close();
	}
	div = doc.createElement("div");
	if (HTMLArea.is_ie)
		div.unselectable = "on";
	div.oncontextmenu = function() { return false; };
	div.className = "htmlarea-context-menu";
	if (!HTMLArea.is_ie)
		div.style.left = div.style.top = "0px";
	doc.body.appendChild(div);

	var table = doc.createElement("table");
	div.appendChild(table);
	table.cellSpacing = 0;
	table.cellPadding = 0;
	var parent = doc.createElement("tbody");
	table.appendChild(parent);

	var options = this.getContextMenu(target);
	for (var i = 0; i < options.length; ++i) {
		var option = options[i];
		var item = doc.createElement("tr");
		parent.appendChild(item);
		if (HTMLArea.is_ie)
			item.unselectable = "on";
		else item.onmousedown = function(ev) {
			HTMLArea._stopEvent(ev);
			return false;
		};
		if (!option) {
			item.className = "separator";
			var td = doc.createElement("td");
			td.className = "icon";
			var IE_IS_A_FUCKING_SHIT = '>';
			if (HTMLArea.is_ie) {
				td.unselectable = "on";
				IE_IS_A_FUCKING_SHIT = " unselectable='on' style='height=1px'>&nbsp;";
			}
			td.innerHTML = "<div" + IE_IS_A_FUCKING_SHIT + "</div>";
			var td1 = td.cloneNode(true);
			td1.className = "label";
			item.appendChild(td);
			item.appendChild(td1);
		} else {
			var label = option[0];
			item.className = "item";
			item.__msh = {
				item: item,
				label: label,
				action: option[1],
				tooltip: option[2] || null,
				icon: option[3] || null,
				activate: function() {
					self.closeMenu();
					self.editor.focusEditor();
					this.action();
				}
			};
			label = label.replace(/_([a-zA-Z0-9])/, "<u>$1</u>");
			if (label != option[0])
				keys.push([ RegExp.$1, item ]);
			label = label.replace(/__/, "_");
			var td1 = doc.createElement("td");
			if (HTMLArea.is_ie)
				td1.unselectable = "on";
			item.appendChild(td1);
			td1.className = "icon";
			if (item.__msh.icon)
				td1.innerHTML = "<img align='middle' src='" + item.__msh.icon + "' />";
			var td2 = doc.createElement("td");
			if (HTMLArea.is_ie)
				td2.unselectable = "on";
			item.appendChild(td2);
			td2.className = "label";
			td2.innerHTML = label;
			item.onmouseover = function() {
				this.className += " hover";
				self.editor._statusBarTree.innerHTML = this.__msh.tooltip || '&nbsp;';
			};
			item.onmouseout = function() { this.className = "item"; };
			item.oncontextmenu = function(ev) {
				this.__msh.activate();
				if (!HTMLArea.is_ie)
					HTMLArea._stopEvent(ev);
				return false;
			};
			item.onmouseup = function(ev) {
				var timeStamp = (new Date()).getTime();
				if (timeStamp - self.timeStamp > 500)
					this.__msh.activate();
				if (!HTMLArea.is_ie)
					HTMLArea._stopEvent(ev);
				return false;
			};
			//if (typeof option[2] == "string")
			//item.title = option[2];
		}
	}

	if (!HTMLArea.is_ie) {
		var dx = x + div.offsetWidth - window.innerWidth + 4;
		var dy = y + div.offsetHeight - window.innerHeight + 4;
		if (dx > 0) x -= dx;
		if (dy > 0) y -= dy;
		div.style.left = x + "px";
		div.style.top = y + "px";
	} else {
		// determine the size (did I mention that IE stinks?)
		var foobar = document.createElement("div");
		foobar.className = "htmlarea-context-menu";
		foobar.innerHTML = div.innerHTML;
		document.body.appendChild(foobar);
		var w = foobar.offsetWidth;
		var h = foobar.offsetHeight;
		document.body.removeChild(foobar);
		this.iePopup.show(ev.screenX, ev.screenY, w, h);
	}

	this.currentMenu = div;
	this.timeStamp = (new Date()).getTime();

	HTMLArea._addEvent(document, "mousedown", documentClick);
	HTMLArea._addEvent(this.editordoc, "mousedown", documentClick);
	if (keys.length > 0)
		HTMLArea._addEvent(this.editordoc, "keypress", keyPress);

	HTMLArea._stopEvent(ev);
	return false;
};
