tinyMCEPopup.requireLangPack();

var doc;

var defaultDocTypes = 
	'XHTML 1.0 Transitional=<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">,' +
	'XHTML 1.0 Frameset=<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Frameset//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd">,' +
	'XHTML 1.0 Strict=<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">,' +
	'XHTML 1.1=<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">,' +
	'HTML 4.01 Transitional=<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">,' +
	'HTML 4.01 Strict=<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">,' +
	'HTML 4.01 Frameset=<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Frameset//EN" "http://www.w3.org/TR/html4/frameset.dtd">';

var defaultEncodings = 
	'Western european (iso-8859-1)=iso-8859-1,' +
	'Central European (iso-8859-2)=iso-8859-2,' +
	'Unicode (UTF-8)=utf-8,' +
	'Chinese traditional (Big5)=big5,' +
	'Cyrillic (iso-8859-5)=iso-8859-5,' +
	'Japanese (iso-2022-jp)=iso-2022-jp,' +
	'Greek (iso-8859-7)=iso-8859-7,' +
	'Korean (iso-2022-kr)=iso-2022-kr,' +
	'ASCII (us-ascii)=us-ascii';

var defaultMediaTypes = 
	'all=all,' +
	'screen=screen,' +
	'print=print,' +
	'tty=tty,' +
	'tv=tv,' +
	'projection=projection,' +
	'handheld=handheld,' +
	'braille=braille,' +
	'aural=aural';

var defaultFontNames = 'Arial=arial,helvetica,sans-serif;Courier New=courier new,courier,monospace;Georgia=georgia,times new roman,times,serif;Tahoma=tahoma,arial,helvetica,sans-serif;Times New Roman=times new roman,times,serif;Verdana=verdana,arial,helvetica,sans-serif;Impact=impact;WingDings=wingdings';
var defaultFontSizes = '10px,11px,12px,13px,14px,15px,16px';

function init() {
	var f = document.forms['fullpage'], el = f.elements, e, i, p, doctypes, encodings, mediaTypes, fonts, ed = tinyMCEPopup.editor, dom = tinyMCEPopup.dom, style;

	// Setup doctype select box
	doctypes = ed.getParam("fullpage_doctypes", defaultDocTypes).split(',');
	for (i=0; i<doctypes.length; i++) {
		p = doctypes[i].split('=');

		if (p.length > 1)
			addSelectValue(f, 'doctypes', p[0], p[1]);
	}

	// Setup fonts select box
	fonts = ed.getParam("fullpage_fonts", defaultFontNames).split(';');
	for (i=0; i<fonts.length; i++) {
		p = fonts[i].split('=');

		if (p.length > 1)
			addSelectValue(f, 'fontface', p[0], p[1]);
	}

	// Setup fontsize select box
	fonts = ed.getParam("fullpage_fontsizes", defaultFontSizes).split(',');
	for (i=0; i<fonts.length; i++)
		addSelectValue(f, 'fontsize', fonts[i], fonts[i]);

	// Setup mediatype select boxs
	mediaTypes = ed.getParam("fullpage_media_types", defaultMediaTypes).split(',');
	for (i=0; i<mediaTypes.length; i++) {
		p = mediaTypes[i].split('=');

		if (p.length > 1) {
			addSelectValue(f, 'element_style_media', p[0], p[1]);
			addSelectValue(f, 'element_link_media', p[0], p[1]);
		}
	}

	// Setup encodings select box
	encodings = ed.getParam("fullpage_encodings", defaultEncodings).split(',');
	for (i=0; i<encodings.length; i++) {
		p = encodings[i].split('=');

		if (p.length > 1) {
			addSelectValue(f, 'docencoding', p[0], p[1]);
			addSelectValue(f, 'element_script_charset', p[0], p[1]);
			addSelectValue(f, 'element_link_charset', p[0], p[1]);
		}
	}

	document.getElementById('bgcolor_pickcontainer').innerHTML = getColorPickerHTML('bgcolor_pick','bgcolor');
	document.getElementById('link_color_pickcontainer').innerHTML = getColorPickerHTML('link_color_pick','link_color');
	//document.getElementById('hover_color_pickcontainer').innerHTML = getColorPickerHTML('hover_color_pick','hover_color');
	document.getElementById('visited_color_pickcontainer').innerHTML = getColorPickerHTML('visited_color_pick','visited_color');
	document.getElementById('active_color_pickcontainer').innerHTML = getColorPickerHTML('active_color_pick','active_color');
	document.getElementById('textcolor_pickcontainer').innerHTML = getColorPickerHTML('textcolor_pick','textcolor');
	document.getElementById('stylesheet_browsercontainer').innerHTML = getBrowserHTML('stylesheetbrowser','stylesheet','file','fullpage');
	document.getElementById('link_href_pickcontainer').innerHTML = getBrowserHTML('link_href_browser','element_link_href','file','fullpage');
	document.getElementById('script_src_pickcontainer').innerHTML = getBrowserHTML('script_src_browser','element_script_src','file','fullpage');
	document.getElementById('bgimage_pickcontainer').innerHTML = getBrowserHTML('bgimage_browser','bgimage','image','fullpage');

	// Resize some elements
	if (isVisible('stylesheetbrowser'))
		document.getElementById('stylesheet').style.width = '220px';

	if (isVisible('link_href_browser'))
		document.getElementById('element_link_href').style.width = '230px';

	if (isVisible('bgimage_browser'))
		document.getElementById('bgimage').style.width = '210px';

	// Add iframe
	dom.add(document.body, 'iframe', {id : 'documentIframe', src : 'javascript:""', style : {display : 'none'}});
	doc = dom.get('documentIframe').contentWindow.document;
	h = tinyMCEPopup.getWindowArg('head_html');

	// Preprocess the HTML disable scripts and urls
	h = h.replace(/<script>/gi, '<script type="text/javascript">');
	h = h.replace(/type=([\"\'])?/gi, 'type=$1-mce-');
	h = h.replace(/(src=|href=)/g, 'mce_$1');

	// Write in the content in the iframe
	doc.write(h + '</body></html>');
	doc.close();

	// Parse xml and doctype
	xmlVer = getReItem(/<\?\s*?xml.*?version\s*?=\s*?"(.*?)".*?\?>/gi, h, 1);
	xmlEnc = getReItem(/<\?\s*?xml.*?encoding\s*?=\s*?"(.*?)".*?\?>/gi, h, 1);
	docType = getReItem(/<\!DOCTYPE.*?>/gi, h, 0);
	f.langcode.value = getReItem(/lang="(.*?)"/gi, h, 1);

	// Parse title
	if (e = doc.getElementsByTagName('title')[0])
		el.metatitle.value = e.textContent || e.text;

	// Parse meta
	tinymce.each(doc.getElementsByTagName('meta'), function(n) {
		var na = (n.getAttribute('name', 2) || '').toLowerCase(), va = n.getAttribute('content', 2), eq = n.getAttribute('httpEquiv', 2) || '';

		e = el['meta' + na];

		if (na == 'robots') {
			selectByValue(f, 'metarobots', tinymce.trim(va), true, true);
			return;
		}

		switch (eq.toLowerCase()) {
			case "content-type":
				tmp = getReItem(/charset\s*=\s*(.*)\s*/gi, va, 1);

				// Override XML encoding
				if (tmp != "")
					xmlEnc = tmp;

				return;
		}

		if (e)
			e.value = va;
	});

	selectByValue(f, 'doctypes', docType, true, true);
	selectByValue(f, 'docencoding', xmlEnc, true, true);
	selectByValue(f, 'langdir', doc.body.getAttribute('dir', 2) || '', true, true);

	if (xmlVer != '')
		el.xml_pi.checked = true;

	// Parse appearance

	// Parse primary stylesheet
	tinymce.each(doc.getElementsByTagName("link"), function(l) {
		var m = l.getAttribute('media', 2) || '', t = l.getAttribute('type', 2) || '';

		if (t == "-mce-text/css" && (m == "" || m == "screen" || m == "all") && (l.getAttribute('rel', 2) || '') == "stylesheet") {
			f.stylesheet.value = l.getAttribute('mce_href', 2) || '';
			return false;
		}
	});

	// Get from style elements
	tinymce.each(doc.getElementsByTagName("style"), function(st) {
		var tmp = parseStyleElement(st);

		for (x=0; x<tmp.length; x++) {
			if (tmp[x].rule.indexOf('a:visited') != -1 && tmp[x].data['color'])
				f.visited_color.value = tmp[x].data['color'];

			if (tmp[x].rule.indexOf('a:link') != -1 && tmp[x].data['color'])
				f.link_color.value = tmp[x].data['color'];

			if (tmp[x].rule.indexOf('a:active') != -1 && tmp[x].data['color'])
				f.active_color.value = tmp[x].data['color'];
		}
	});

	f.textcolor.value = tinyMCEPopup.dom.getAttrib(doc.body, "text");
	f.active_color.value = tinyMCEPopup.dom.getAttrib(doc.body, "alink");
	f.link_color.value = tinyMCEPopup.dom.getAttrib(doc.body, "link");
	f.visited_color.value = tinyMCEPopup.dom.getAttrib(doc.body, "vlink");
	f.bgcolor.value = tinyMCEPopup.dom.getAttrib(doc.body, "bgcolor");
	f.bgimage.value = tinyMCEPopup.dom.getAttrib(doc.body, "background");

	// Get from style info
	style = tinyMCEPopup.dom.parseStyle(tinyMCEPopup.dom.getAttrib(doc.body, 'style'));

	if (style['font-family'])
		selectByValue(f, 'fontface', style['font-family'], true, true);
	else
		selectByValue(f, 'fontface', ed.getParam("fullpage_default_fontface", ""), true, true);

	if (style['font-size'])
		selectByValue(f, 'fontsize', style['font-size'], true, true);
	else
		selectByValue(f, 'fontsize', ed.getParam("fullpage_default_fontsize", ""), true, true);

	if (style['color'])
		f.textcolor.value = convertRGBToHex(style['color']);

	if (style['background-image'])
		f.bgimage.value = style['background-image'].replace(new RegExp("url\\('?([^']*)'?\\)", 'gi'), "$1");

	if (style['background-color'])
		f.bgcolor.value = style['background-color'];

	if (style['margin']) {
		tmp = style['margin'].replace(/[^0-9 ]/g, '');
		tmp = tmp.split(/ +/);
		f.topmargin.value = tmp.length > 0 ? tmp[0] : '';
		f.rightmargin.value = tmp.length > 1 ? tmp[1] : tmp[0];
		f.bottommargin.value = tmp.length > 2 ? tmp[2] : tmp[0];
		f.leftmargin.value = tmp.length > 3 ? tmp[3] : tmp[0];
	}

	if (style['margin-left'])
		f.leftmargin.value = style['margin-left'].replace(/[^0-9]/g, '');

	if (style['margin-right'])
		f.rightmargin.value = style['margin-right'].replace(/[^0-9]/g, '');

	if (style['margin-top'])
		f.topmargin.value = style['margin-top'].replace(/[^0-9]/g, '');

	if (style['margin-bottom'])
		f.bottommargin.value = style['margin-bottom'].replace(/[^0-9]/g, '');

	f.style.value = tinyMCEPopup.dom.serializeStyle(style);

	// Update colors
	updateColor('textcolor_pick', 'textcolor');
	updateColor('bgcolor_pick', 'bgcolor');
	updateColor('visited_color_pick', 'visited_color');
	updateColor('active_color_pick', 'active_color');
	updateColor('link_color_pick', 'link_color');
}

function getReItem(r, s, i) {
	var c = r.exec(s);

	if (c && c.length > i)
		return c[i];

	return '';
}

function updateAction() {
	var f = document.forms[0], nl, i, h, v, s, head, html, l, tmp, addlink = true, ser;

	head = doc.getElementsByTagName('head')[0];

	// Fix scripts without a type
	nl = doc.getElementsByTagName('script');
	for (i=0; i<nl.length; i++) {
		if (tinyMCEPopup.dom.getAttrib(nl[i], 'mce_type') == '')
			nl[i].setAttribute('mce_type', 'text/javascript');
	}

	// Get primary stylesheet
	nl = doc.getElementsByTagName("link");
	for (i=0; i<nl.length; i++) {
		l = nl[i];

		tmp = tinyMCEPopup.dom.getAttrib(l, 'media');

		if (tinyMCEPopup.dom.getAttrib(l, 'mce_type') == "text/css" && (tmp == "" || tmp == "screen" || tmp == "all") && tinyMCEPopup.dom.getAttrib(l, 'rel') == "stylesheet") {
			addlink = false;

			if (f.stylesheet.value == '')
				l.parentNode.removeChild(l);
			else
				l.setAttribute('mce_href', f.stylesheet.value);

			break;
		}
	}

	// Add new link
	if (f.stylesheet.value != '') {
		l = doc.createElement('link');

		l.setAttribute('type', 'text/css');
		l.setAttribute('mce_href', f.stylesheet.value);
		l.setAttribute('rel', 'stylesheet');

		head.appendChild(l);
	}

	setMeta(head, 'keywords', f.metakeywords.value);
	setMeta(head, 'description', f.metadescription.value);
	setMeta(head, 'author', f.metaauthor.value);
	setMeta(head, 'copyright', f.metacopyright.value);
	setMeta(head, 'robots', getSelectValue(f, 'metarobots'));
	setMeta(head, 'Content-Type', getSelectValue(f, 'docencoding'));

	doc.body.dir = getSelectValue(f, 'langdir');
	doc.body.style.cssText = f.style.value;

	doc.body.setAttribute('vLink', f.visited_color.value);
	doc.body.setAttribute('link', f.link_color.value);
	doc.body.setAttribute('text', f.textcolor.value);
	doc.body.setAttribute('aLink', f.active_color.value);

	doc.body.style.fontFamily = getSelectValue(f, 'fontface');
	doc.body.style.fontSize = getSelectValue(f, 'fontsize');
	doc.body.style.backgroundColor = f.bgcolor.value;

	if (f.leftmargin.value != '')
		doc.body.style.marginLeft = f.leftmargin.value + 'px';

	if (f.rightmargin.value != '')
		doc.body.style.marginRight = f.rightmargin.value + 'px';

	if (f.bottommargin.value != '')
		doc.body.style.marginBottom = f.bottommargin.value + 'px';

	if (f.topmargin.value != '')
		doc.body.style.marginTop = f.topmargin.value + 'px';

	html = doc.getElementsByTagName('html')[0];
	html.setAttribute('lang', f.langcode.value);
	html.setAttribute('xml:lang', f.langcode.value);

	if (f.bgimage.value != '')
		doc.body.style.backgroundImage = "url('" + f.bgimage.value + "')";
	else
		doc.body.style.backgroundImage = '';

	ser = tinyMCEPopup.editor.plugins.fullpage._createSerializer();
	ser.setRules('-title,meta[http-equiv|name|content],base[href|target],link[href|rel|type|title|media],style[type],script[type|language|src],html[lang|xml::lang|xmlns],body[style|dir|vlink|link|text|alink],head');

	h = ser.serialize(doc.documentElement);
	h = h.substring(0, h.lastIndexOf('</body>'));

	if (h.indexOf('<title>') == -1)
		h = h.replace(/<head.*?>/, '$&\n' + '<title>' + tinyMCEPopup.dom.encode(f.metatitle.value) + '</title>');
	else
		h = h.replace(/<title>(.*?)<\/title>/, '<title>' + tinyMCEPopup.dom.encode(f.metatitle.value) + '</title>');

	if ((v = getSelectValue(f, 'doctypes')) != '')
		h = v + '\n' + h;

	if (f.xml_pi.checked) {
		s = '<?xml version="1.0"';

		if ((v = getSelectValue(f, 'docencoding')) != '')
			s += ' encoding="' + v + '"';

		s += '?>\n';
		h = s + h;
	}

	h = h.replace(/type=\"\-mce\-/gi, 'type="');

	tinyMCEPopup.editor.plugins.fullpage.head = h;
	tinyMCEPopup.editor.plugins.fullpage._setBodyAttribs(tinyMCEPopup.editor, {});
	tinyMCEPopup.close();
}

function changedStyleField(field) {
}

function setMeta(he, k, v) {
	var nl, i, m;

	nl = he.getElementsByTagName('meta');
	for (i=0; i<nl.length; i++) {
		if (k == 'Content-Type' && tinyMCEPopup.dom.getAttrib(nl[i], 'http-equiv') == k) {
			if (v == '')
				nl[i].parentNode.removeChild(nl[i]);
			else
				nl[i].setAttribute('content', "text/html; charset=" + v);

			return;
		}

		if (tinyMCEPopup.dom.getAttrib(nl[i], 'name') == k) {
			if (v == '')
				nl[i].parentNode.removeChild(nl[i]);
			else
				nl[i].setAttribute('content', v);
			return;
		}
	}

	if (v == '')
		return;

	m = doc.createElement('meta');

	if (k == 'Content-Type')
		m.httpEquiv = k;
	else
		m.setAttribute('name', k);

	m.setAttribute('content', v);
	he.appendChild(m);
}

function parseStyleElement(e) {
	var v = e.innerHTML;
	var p, i, r;

	v = v.replace(/<!--/gi, '');
	v = v.replace(/-->/gi, '');
	v = v.replace(/[\n\r]/gi, '');
	v = v.replace(/\s+/gi, ' ');

	r = [];
	p = v.split(/{|}/);

	for (i=0; i<p.length; i+=2) {
		if (p[i] != "")
			r[r.length] = {rule : tinymce.trim(p[i]), data : tinyMCEPopup.dom.parseStyle(p[i+1])};
	}

	return r;
}

function serializeStyleElement(d) {
	var i, s, st;

	s = '<!--\n';

	for (i=0; i<d.length; i++) {
		s += d[i].rule + ' {\n';

		st = tinyMCE.serializeStyle(d[i].data);

		if (st != '')
			st += ';';

		s += st.replace(/;/g, ';\n');
		s += '}\n';

		if (i != d.length - 1)
			s += '\n';
	}

	s += '\n-->';

	return s;
}

tinyMCEPopup.onInit.add(init);
