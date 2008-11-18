/** This file is derived from PopupDiv, developed by Mihai Bazon for
 * SamWare.net.  Modifications were needed to make it usable in HTMLArea.
 * HTMLArea is a free WYSIWYG online HTML editor from InteractiveTools.com.
 *
 * This file does not function standalone.  It is dependent of global functions
 * defined in HTMLArea-3.0 (htmlarea.js).
 *
 * Please see file htmlarea.js for further details.
 **/

var is_ie = ( (navigator.userAgent.toLowerCase().indexOf("msie") != -1) &&
	      (navigator.userAgent.toLowerCase().indexOf("opera") == -1) );
var is_compat = (document.compatMode == "BackCompat");

function PopupDiv(editor, titleText, handler, initFunction) {
	var self = this;

	this.editor = editor;
	this.doc = editor._mdoc;
	this.handler = handler;

	var el = this.doc.createElement("div");
	el.className = "content";

	var popup = this.doc.createElement("div");
	popup.className = "dialog popupdiv";
	this.element = popup;
	var s = popup.style;
	s.position = "absolute";
	s.left = "0px";
	s.top = "0px";

	var title = this.doc.createElement("div");
	title.className = "title";
	this.title = title;
	popup.appendChild(title);

	HTMLArea._addEvent(title, "mousedown", function(ev) {
		self._dragStart(is_ie ? window.event : ev);
	});

	var button = this.doc.createElement("div");
	button.className = "button";
	title.appendChild(button);
	button.innerHTML = "&#x00d7;";
	title.appendChild(this.doc.createTextNode(titleText));
	this.titleText = titleText;

	button.onmouseover = function() {
		this.className += " button-hilite";
	};
	button.onmouseout = function() {
		this.className = this.className.replace(/\s*button-hilite\s*/g, " ");
	};
	button.onclick = function() {
		this.className = this.className.replace(/\s*button-hilite\s*/g, " ");
		self.close();
	};

	popup.appendChild(el);
	this.content = el;

	this.doc.body.appendChild(popup);

	this.dragging = false;
	this.onShow = null;
	this.onClose = null;
	this.modal = false;

	initFunction(this);
};

PopupDiv.currentPopup = null;

PopupDiv.prototype.showAtElement = function(el, mode) {
	this.defaultSize();
	var pos, ew, eh;
	var popup = this.element;
	popup.style.display = "block";
	var w = popup.offsetWidth;
	var h = popup.offsetHeight;
	popup.style.display = "none";
	if (el != window) {
		pos = PopupDiv.getAbsolutePos(el);
		ew = el.offsetWidth;
		eh = el.offsetHeight;
	} else {
		pos = {x:0, y:0};
		var size = PopupDiv.getWindowSize();
		ew = size.x;
		eh = size.y;
	}
	var FX = false, FY = false;
	if (mode.indexOf("l") != -1) {
		pos.x -= w;
		FX = true;
	}
	if (mode.indexOf("r") != -1) {
		pos.x += ew;
		FX = true;
	}
	if (mode.indexOf("t") != -1) {
		pos.y -= h;
		FY = true;
	}
	if (mode.indexOf("b") != -1) {
		pos.y += eh;
		FY = true;
	}
	if (mode.indexOf("c") != -1) {
		FX || (pos.x += Math.round((ew - w) / 2));
		FY || (pos.y += Math.round((eh - h) / 2));
	}
	this.showAt(pos.x, pos.y);
};

PopupDiv.prototype.defaultSize = function() {
	var s = this.element.style;
	var cs = this.element.currentStyle;
	var addX = (is_ie && is_compat) ? (parseInt(cs.borderLeftWidth) +
					   parseInt(cs.borderRightWidth) +
					   parseInt(cs.paddingLeft) +
					   parseInt(cs.paddingRight)) : 0;
	var addY = (is_ie && is_compat) ? (parseInt(cs.borderTopWidth) +
					   parseInt(cs.borderBottomWidth) +
					   parseInt(cs.paddingTop) +
					   parseInt(cs.paddingBottom)) : 0;
	s.display = "block";
	s.width = (this.content.offsetWidth + addX) + "px";
	s.height = (this.content.offsetHeight + this.title.offsetHeight) + "px";
	s.display = "none";
};

PopupDiv.prototype.showAt = function(x, y) {
	this.defaultSize();
	var s = this.element.style;
	s.display = "block";
	s.left = x + "px";
	s.top = y + "px";
	this.hideShowCovered();

	PopupDiv.currentPopup = this;
	HTMLArea._addEvents(this.doc.body, ["mousedown", "click"], PopupDiv.checkPopup);
	HTMLArea._addEvents(this.editor._doc.body, ["mousedown", "click"], PopupDiv.checkPopup);
	if (is_ie && this.modal) {
		this.doc.body.setCapture(false);
		this.doc.body.onlosecapture = function() {
			(PopupDiv.currentPopup) && (this.doc.body.setCapture(false));
		};
	}
	window.event && HTMLArea._stopEvent(window.event);

	if (typeof this.onShow == "function") {
		this.onShow();
	} else if (typeof this.onShow == "string") {
		eval(this.onShow);
	}

	var field = this.element.getElementsByTagName("input")[0];
	if (!field) {
		field = this.element.getElementsByTagName("select")[0];
	}
	if (!field) {
		field = this.element.getElementsByTagName("textarea")[0];
	}
	if (field) {
		field.focus();
	}
};

PopupDiv.prototype.close = function() {
	this.element.style.display = "none";
	PopupDiv.currentPopup = null;
	this.hideShowCovered();
	HTMLArea._removeEvents(this.doc.body, ["mousedown", "click"], PopupDiv.checkPopup);
	HTMLArea._removeEvents(this.editor._doc.body, ["mousedown", "click"], PopupDiv.checkPopup);
	is_ie && this.modal && this.doc.body.releaseCapture();
	if (typeof this.onClose == "function") {
		this.onClose();
	} else if (typeof this.onClose == "string") {
		eval(this.onClose);
	}
	this.element.parentNode.removeChild(this.element);
};

PopupDiv.prototype.getForm = function() {
	var forms = this.content.getElementsByTagName("form");
	return (forms.length > 0) ? forms[0] : null;
};

PopupDiv.prototype.callHandler = function() {
	var tags = ["input", "textarea", "select"];
	var params = new Object();
	for (var ti = tags.length; --ti >= 0;) {
		var tag = tags[ti];
		var els = this.content.getElementsByTagName(tag);
		for (var j = 0; j < els.length; ++j) {
			var el = els[j];
			params[el.name] = el.value;
		}
	}
	this.handler(this, params);
	return false;
};

PopupDiv.getAbsolutePos = function(el) {
	var r = { x: el.offsetLeft, y: el.offsetTop };
	if (el.offsetParent) {
		var tmp = PopupDiv.getAbsolutePos(el.offsetParent);
		r.x += tmp.x;
		r.y += tmp.y;
	}
	return r;
};

PopupDiv.getWindowSize = function() {
	if (window.innerHeight) {
		return { y: window.innerHeight, x: window.innerWidth };
	}
	if (this.doc.body.clientHeight) {
		return { y: this.doc.body.clientHeight, x: this.doc.body.clientWidth };
	}
	return { y: this.doc.documentElement.clientHeight, x: this.doc.documentElement.clientWidth };
};

PopupDiv.prototype.hideShowCovered = function () {
	var self = this;
	function isContained(el) {
		while (el) {
			if (el == self.element) {
				return true;
			}
			el = el.parentNode;
		}
		return false;
	};
	var tags = new Array("applet", "select");
	var el = this.element;

	var p = PopupDiv.getAbsolutePos(el);
	var EX1 = p.x;
	var EX2 = el.offsetWidth + EX1;
	var EY1 = p.y;
	var EY2 = el.offsetHeight + EY1;

	if (el.style.display == "none") {
		EX1 = EX2 = EY1 = EY2 = 0;
	}

	for (var k = tags.length; k > 0; ) {
		var ar = this.doc.getElementsByTagName(tags[--k]);
		var cc = null;

		for (var i = ar.length; i > 0;) {
			cc = ar[--i];
			if (isContained(cc)) {
				cc.style.visibility = "visible";
				continue;
			}

			p = PopupDiv.getAbsolutePos(cc);
			var CX1 = p.x;
			var CX2 = cc.offsetWidth + CX1;
			var CY1 = p.y;
			var CY2 = cc.offsetHeight + CY1;

			if ((CX1 > EX2) || (CX2 < EX1) || (CY1 > EY2) || (CY2 < EY1)) {
				cc.style.visibility = "visible";
			} else {
				cc.style.visibility = "hidden";
			}
		}
	}
};

PopupDiv.prototype._dragStart = function (ev) {
	if (this.dragging) {
		return false;
	}
	this.dragging = true;
	PopupDiv.currentPopup = this;
	var posX = ev.clientX;
	var posY = ev.clientY;
	if (is_ie) {
		posY += this.doc.body.scrollTop;
		posX += this.doc.body.scrollLeft;
	} else {
		posY += window.scrollY;
		posX += window.scrollX;
	}
	var st = this.element.style;
	this.xOffs = posX - parseInt(st.left);
	this.yOffs = posY - parseInt(st.top);
	HTMLArea._addEvent(this.doc, "mousemove", PopupDiv.dragIt);
	HTMLArea._addEvent(this.doc, "mouseover", HTMLArea._stopEvent);
	HTMLArea._addEvent(this.doc, "mouseup", PopupDiv.dragEnd);
	HTMLArea._stopEvent(ev);
};

PopupDiv.dragIt = function (ev) {
	var popup = PopupDiv.currentPopup;
	if (!(popup && popup.dragging)) {
		return false;
	}
	is_ie && (ev = window.event);
	var posX = ev.clientX;
	var posY = ev.clientY;
	if (is_ie) {
		posY += this.doc.body.scrollTop;
		posX += this.doc.body.scrollLeft;
	} else {
		posY += window.scrollY;
		posX += window.scrollX;
	}
	popup.hideShowCovered();
	var st = popup.element.style;
	st.left = (posX - popup.xOffs) + "px";
	st.top = (posY - popup.yOffs) + "px";
	HTMLArea._stopEvent(ev);
};

PopupDiv.dragEnd = function () {
	var popup = PopupDiv.currentPopup;
	if (!popup) {
		return false;
	}
	popup.dragging = false;
	HTMLArea._removeEvent(popup.doc, "mouseup", PopupDiv.dragEnd);
	HTMLArea._removeEvent(popup.doc, "mouseover", HTMLArea._stopEvent);
	HTMLArea._removeEvent(popup.doc, "mousemove", PopupDiv.dragIt);
	popup.hideShowCovered();
};

PopupDiv.checkPopup = function (ev) {
	is_ie && (ev = window.event);
	var el = is_ie ? ev.srcElement : ev.target;
	var cp = PopupDiv.currentPopup;
	for (; (el != null) && (el != cp.element); el = el.parentNode);
	if (el == null) {
		cp.modal || ev.type == "mouseover" || cp.close();
		HTMLArea._stopEvent(ev);
	}
};

PopupDiv.prototype.addButtons = function() {
	var self = this;
	var div = this.doc.createElement("div");
	this.content.appendChild(div);
	div.className = "buttons";
	for (var i = 0; i < arguments.length; ++i) {
		var btn = arguments[i];
		var button = this.doc.createElement("button");
		div.appendChild(button);
		button.innerHTML = HTMLArea.I18N.buttons[btn];
		switch (btn) {
		    case "ok":
			button.onclick = function() {
				self.callHandler();
				self.close();
			};
			break;
		    case "cancel":
			button.onclick = function() {
				self.close();
			};
			break;
		}
	}
};
