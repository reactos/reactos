// FullPage Plugin for HTMLArea-3.0
// Implementation by Mihai Bazon.  Sponsored by http://thycotic.com
//
// htmlArea v3.0 - Copyright (c) 2002 interactivetools.com, inc.
// This notice MUST stay intact for use (see license.txt).
//
// A free WYSIWYG editor replacement for <textarea> fields.
// For full source code and docs, visit http://www.interactivetools.com/
//
// Version 3.0 developed by Mihai Bazon for InteractiveTools.
//   http://dynarch.com/mishoo
//
// $Id: full-page.js,v 1.2 2005/01/11 15:00:57 garvinhicking Exp $

function FullPage(editor) {
	this.editor = editor;

	var cfg = editor.config;
	cfg.fullPage = true;
	var tt = FullPage.I18N;
	var self = this;

	cfg.registerButton("FP-docprop", tt["Document properties"], editor.imgURL("docprop.gif", "FullPage"), false,
			   function(editor, id) {
				   self.buttonPress(editor, id);
			   });

	// add a new line in the toolbar
	cfg.toolbar[0].splice(0, 0, "separator");
	cfg.toolbar[0].splice(0, 0, "FP-docprop");
};

FullPage._pluginInfo = {
	name          : "FullPage",
	version       : "1.0",
	developer     : "Mihai Bazon",
	developer_url : "http://dynarch.com/mishoo/",
	c_owner       : "Mihai Bazon",
	sponsor       : "Thycotic Software Ltd.",
	sponsor_url   : "http://thycotic.com",
	license       : "htmlArea"
};

FullPage.prototype.buttonPress = function(editor, id) {
	var self = this;
	switch (id) {
	    case "FP-docprop":
		var doc = editor._doc;
		var links = doc.getElementsByTagName("link");
		var style1 = '';
		var style2 = '';
		var charset = '';
		for (var i = links.length; --i >= 0;) {
			var link = links[i];
			if (/stylesheet/i.test(link.rel)) {
				if (/alternate/i.test(link.rel))
					style2 = link.href;
				else
					style1 = link.href;
			}
		}
		var metas = doc.getElementsByTagName("meta");
		for (var i = metas.length; --i >= 0;) {
			var meta = metas[i];
			if (/content-type/i.test(meta.httpEquiv)) {
				r = /^text\/html; *charset=(.*)$/i.exec(meta.content);
				charset = r[1];
			}
		}
		var title = doc.getElementsByTagName("title")[0];
		title = title ? title.innerHTML : '';
		var init = {
			f_doctype      : editor.doctype,
			f_title        : title,
			f_body_bgcolor : HTMLArea._colorToRgb(doc.body.style.backgroundColor),
			f_body_fgcolor : HTMLArea._colorToRgb(doc.body.style.color),
			f_base_style   : style1,
			f_alt_style    : style2,
			f_charset      : charset,
			editor         : editor
		};
		editor._popupDialog("plugin://FullPage/docprop", function(params) {
			self.setDocProp(params);
		}, init);
		break;
	}
};

FullPage.prototype.setDocProp = function(params) {
	var txt = "";
	var doc = this.editor._doc;
	var head = doc.getElementsByTagName("head")[0];
	var links = doc.getElementsByTagName("link");
	var metas = doc.getElementsByTagName("meta");
	var style1 = null;
	var style2 = null;
	var charset = null;
	var charset_meta = null;
	for (var i = links.length; --i >= 0;) {
		var link = links[i];
		if (/stylesheet/i.test(link.rel)) {
			if (/alternate/i.test(link.rel))
				style2 = link;
			else
				style1 = link;
		}
	}
	for (var i = metas.length; --i >= 0;) {
		var meta = metas[i];
		if (/content-type/i.test(meta.httpEquiv)) {
			r = /^text\/html; *charset=(.*)$/i.exec(meta.content);
			charset = r[1];
			charset_meta = meta;
		}
	}
	function createLink(alt) {
		var link = doc.createElement("link");
		link.rel = alt ? "alternate stylesheet" : "stylesheet";
		head.appendChild(link);
		return link;
	};
	function createMeta(name, content) {
		var meta = doc.createElement("meta");
		meta.httpEquiv = name;
		meta.content = content;
		head.appendChild(meta);
		return meta;
	};

	if (!style1 && params.f_base_style)
		style1 = createLink(false);
	if (params.f_base_style)
		style1.href = params.f_base_style;
	else if (style1)
		head.removeChild(style1);

	if (!style2 && params.f_alt_style)
		style2 = createLink(true);
	if (params.f_alt_style)
		style2.href = params.f_alt_style;
	else if (style2)
		head.removeChild(style2);

	if (charset_meta) {
		head.removeChild(charset_meta);
		charset_meta = null;
	}
	if (!charset_meta && params.f_charset)
		charset_meta = createMeta("Content-Type", "text/html; charset="+params.f_charset);

  	for (var i in params) {
		var val = params[i];
		switch (i) {
		    case "f_title":
			var title = doc.getElementsByTagName("title")[0];
			if (!title) {
				title = doc.createElement("title");
				head.appendChild(title);
			} else while (node = title.lastChild)
				title.removeChild(node);
			if (!HTMLArea.is_ie)
				title.appendChild(doc.createTextNode(val));
			else
				doc.title = val;
			break;
		    case "f_doctype":
			this.editor.setDoctype(val);
			break;
		    case "f_body_bgcolor":
			doc.body.style.backgroundColor = val;
			break;
		    case "f_body_fgcolor":
			doc.body.style.color = val;
			break;
		}
	}
};
