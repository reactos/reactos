//

var defaultDocTypes = 
	'XHTML 1.0 Transitional=<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">,' +
	'XHTML 1.0 Frameset=<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Frameset//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd">,' +
	'XHTML 1.0 Strict=<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">,' +
	'XHTML 1.1=<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">">,' +
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

var addMenuLayer = new MCLayer("addmenu");
var lastElementType = null;
var topDoc;

function init() {
	var f = document.forms['fullpage'];
	var i, p, doctypes, encodings, mediaTypes, fonts;
	var inst = tinyMCE.getInstanceById(tinyMCE.getWindowArg('editor_id'));

	// Setup doctype select box
	doctypes = tinyMCE.getParam("fullpage_doctypes", defaultDocTypes).split(',');
	for (i=0; i<doctypes.length; i++) {
		p = doctypes[i].split('=');

		if (p.length > 1)
			addSelectValue(f, 'doctypes', p[0], p[1]);
	}

	// Setup fonts select box
	fonts = tinyMCE.getParam("fullpage_fonts", defaultFontNames).split(';');
	for (i=0; i<fonts.length; i++) {
		p = fonts[i].split('=');

		if (p.length > 1)
			addSelectValue(f, 'fontface', p[0], p[1]);
	}

	// Setup fontsize select box
	fonts = tinyMCE.getParam("fullpage_fontsizes", defaultFontSizes).split(',');
	for (i=0; i<fonts.length; i++)
		addSelectValue(f, 'fontsize', fonts[i], fonts[i]);

	// Setup mediatype select boxs
	mediaTypes = tinyMCE.getParam("fullpage_media_types", defaultMediaTypes).split(',');
	for (i=0; i<mediaTypes.length; i++) {
		p = mediaTypes[i].split('=');

		if (p.length > 1) {
			addSelectValue(f, 'element_style_media', p[0], p[1]);
			addSelectValue(f, 'element_link_media', p[0], p[1]);
		}
	}

	// Setup encodings select box
	encodings = tinyMCE.getParam("fullpage_encodings", defaultEncodings).split(',');
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

	// Create iframe
	var iframe = document.createElement('iframe');

	iframe.id = 'tempFrame';
	iframe.style.display = 'none';
	iframe.src = tinyMCE.baseURL + "/plugins/fullpage/blank.htm";

	document.body.appendChild(iframe);

	tinyMCEPopup.resizeToInnerSize();
}

function setupIframe(doc) {
	var inst = tinyMCE.getInstanceById(tinyMCE.getWindowArg('editor_id'));
	var hc = inst.fullpageTopContent;
	var f = document.forms[0];
	var xmlVer, xmlEnc, docType;
	var nodes, i, x, name, value, tmp, l;

	// Keep it from not loading/executing stuff
	hc = hc.replace(/<script>/gi, '<script type="text/javascript">');
	hc = hc.replace(/\ssrc=/gi, " mce_src=");
	hc = hc.replace(/\shref=/gi, " mce_href=");
	hc = hc.replace(/\stype=/gi, " mce_type=");
	hc = hc.replace(/<script/gi, '<script type="text/unknown" ');

	// Add end to make it DOM parseable
	hc += '</body></html>';

	topDoc = doc;
	doc.open();
	doc.write(hc);
	doc.close();

	// ------- Setup options for genral tab

	// Parse xml and doctype
	xmlVer = getReItem(/<\?\s*?xml.*?version\s*?=\s*?"(.*?)".*?\?>/gi, hc, 1);
	xmlEnc = getReItem(/<\?\s*?xml.*?encoding\s*?=\s*?"(.*?)".*?\?>/gi, hc, 1);
	docType = getReItem(/<\!DOCTYPE.*?>/gi, hc, 0);
	f.langcode.value = getReItem(/lang="(.*?)"/gi, hc, 1);

	// Get title
	f.metatitle.value = tinyMCE.entityDecode(getReItem(/<title>(.*?)<\/title>/gi, hc, 1));

	// Check for meta encoding
	nodes = doc.getElementsByTagName("meta");
	for (i=0; i<nodes.length; i++) {
		name = tinyMCE.getAttrib(nodes[i], 'name');
		value = tinyMCE.getAttrib(nodes[i], 'content');
		httpEquiv = tinyMCE.getAttrib(nodes[i], 'httpEquiv');

		switch (name.toLowerCase()) {
			case "keywords":
				f.metakeywords.value = value;
				break;

			case "description":
				f.metadescription.value = value;
				break;

			case "author":
				f.metaauthor.value = value;
				break;

			case "copyright":
				f.metacopyright.value = value;
				break;

			case "robots":
				selectByValue(f, 'metarobots', value, true, true);
				break;
		}

		switch (httpEquiv.toLowerCase()) {
			case "content-type":
				tmp = getReItem(/charset\s*=\s*(.*)\s*/gi, value, 1);

				// Override XML encoding
				if (tmp != "")
					xmlEnc = tmp;

				break;
		}
	}

	selectByValue(f, 'doctypes', docType, true, true);
	selectByValue(f, 'docencoding', xmlEnc, true, true);
	selectByValue(f, 'langdir', tinyMCE.getAttrib(doc.body, 'dir'), true, true);

	if (xmlVer != '')
		f.xml_pi.checked = true;

	// ------- Setup options for appearance tab

	// Get primary stylesheet
	nodes = doc.getElementsByTagName("link");
	for (i=0; i<nodes.length; i++) {
		l = nodes[i];
		tmp = tinyMCE.getAttrib(l, 'media');

		if (tinyMCE.getAttrib(l, 'mce_type') == "text/css" && (tmp == "" || tmp == "screen" || tmp == "all") && tinyMCE.getAttrib(l, 'rel') == "stylesheet") {
			f.stylesheet.value = tinyMCE.getAttrib(l, 'mce_href');
			break;
		}
	}

	// Get from style elements
	nodes = doc.getElementsByTagName("style");
	for (i=0; i<nodes.length; i++) {
		tmp = parseStyleElement(nodes[i]);

		for (x=0; x<tmp.length; x++) {
		//	if (tmp[x].rule.indexOf('a:hover') != -1 && tmp[x].data['color'])
		//		f.hover_color.value = tmp[x].data['color'];

			if (tmp[x].rule.indexOf('a:visited') != -1 && tmp[x].data['color'])
				f.visited_color.value = tmp[x].data['color'];

			if (tmp[x].rule.indexOf('a:link') != -1 && tmp[x].data['color'])
				f.link_color.value = tmp[x].data['color'];

			if (tmp[x].rule.indexOf('a:active') != -1 && tmp[x].data['color'])
				f.active_color.value = tmp[x].data['color'];
		}
	}

	// Get from body attribs

/*	f.leftmargin.value = tinyMCE.getAttrib(doc.body, "leftmargin");
	f.rightmargin.value = tinyMCE.getAttrib(doc.body, "rightmargin");
	f.topmargin.value = tinyMCE.getAttrib(doc.body, "topmargin");
	f.bottommargin.value = tinyMCE.getAttrib(doc.body, "bottommargin");*/
	f.textcolor.value = convertRGBToHex(tinyMCE.getAttrib(doc.body, "text"));
	f.active_color.value = convertRGBToHex(tinyMCE.getAttrib(doc.body, "alink"));
	f.link_color.value = convertRGBToHex(tinyMCE.getAttrib(doc.body, "link"));
	f.visited_color.value = convertRGBToHex(tinyMCE.getAttrib(doc.body, "vlink"));
	f.bgcolor.value = convertRGBToHex(tinyMCE.getAttrib(doc.body, "bgcolor"));
	f.bgimage.value = convertRGBToHex(tinyMCE.getAttrib(doc.body, "background"));

	// Get from style info
	var style = tinyMCE.parseStyle(tinyMCE.getAttrib(doc.body, 'style'));

	if (style['font-family'])
		selectByValue(f, 'fontface', style['font-family'], true, true);
	else
		selectByValue(f, 'fontface', tinyMCE.getParam("fullpage_default_fontface", ""), true, true);

	if (style['font-size'])
		selectByValue(f, 'fontsize', style['font-size'], true, true);
	else
		selectByValue(f, 'fontsize', tinyMCE.getParam("fullpage_default_fontsize", ""), true, true);

	if (style['color'])
		f.textcolor.value = convertRGBToHex(style['color']);

	if (style['background-image'])
		f.bgimage.value = style['background-image'].replace(new RegExp("url\\('?([^']*)'?\\)", 'gi'), "$1");

	if (style['background-color'])
		f.bgcolor.value = convertRGBToHex(style['background-color']);

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

	f.style.value = tinyMCE.serializeStyle(style);

	updateColor('textcolor_pick', 'textcolor');
	updateColor('bgcolor_pick', 'bgcolor');
	updateColor('visited_color_pick', 'visited_color');
	updateColor('active_color_pick', 'active_color');
	updateColor('link_color_pick', 'link_color');
	//updateColor('hover_color_pick', 'hover_color');
}

function updateAction() {
	var inst = tinyMCE.getInstanceById(tinyMCE.getWindowArg('editor_id'));
	var f = document.forms[0];
	var nl, i, h, v, s, head, html, l, tmp, addlink = true;

	head = topDoc.getElementsByTagName('head')[0];

	// Fix scripts without a type
	nl = topDoc.getElementsByTagName('script');
	for (i=0; i<nl.length; i++) {
		if (tinyMCE.getAttrib(nl[i], 'mce_type') == '')
			nl[i].setAttribute('mce_type', 'text/javascript');
	}

	// Get primary stylesheet
	nl = topDoc.getElementsByTagName("link");
	for (i=0; i<nl.length; i++) {
		l = nl[i];

		tmp = tinyMCE.getAttrib(l, 'media');

		if (tinyMCE.getAttrib(l, 'mce_type') == "text/css" && (tmp == "" || tmp == "screen" || tmp == "all") && tinyMCE.getAttrib(l, 'rel') == "stylesheet") {
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
		l = topDoc.createElement('link');

		l.setAttribute('mce_type', 'text/css');
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

	topDoc.body.dir = getSelectValue(f, 'langdir');
	topDoc.body.style.cssText = f.style.value;

	topDoc.body.setAttribute('vLink', f.visited_color.value);
	topDoc.body.setAttribute('link', f.link_color.value);
	topDoc.body.setAttribute('text', f.textcolor.value);
	topDoc.body.setAttribute('aLink', f.active_color.value);

	topDoc.body.style.fontFamily = getSelectValue(f, 'fontface');
	topDoc.body.style.fontSize = getSelectValue(f, 'fontsize');
	topDoc.body.style.backgroundColor = f.bgcolor.value;

	if (f.leftmargin.value != '')
		topDoc.body.style.marginLeft = f.leftmargin.value + 'px';

	if (f.rightmargin.value != '')
		topDoc.body.style.marginRight = f.rightmargin.value + 'px';

	if (f.bottommargin.value != '')
		topDoc.body.style.marginBottom = f.bottommargin.value + 'px';

	if (f.topmargin.value != '')
		topDoc.body.style.marginTop = f.topmargin.value + 'px';

	html = topDoc.getElementsByTagName('html')[0];
	html.setAttribute('lang', f.langcode.value);
	html.setAttribute('xml:lang', f.langcode.value);

	if (f.bgimage.value != '')
		topDoc.body.style.backgroundImage = "url('" + f.bgimage.value + "')";
	else
		topDoc.body.style.backgroundImage = '';

	inst.cleanup.addRuleStr('-title,meta[http-equiv|name|content],base[href|target],link[href|rel|type|title|media],style[type],script[type|language|src],html[lang|xml:lang|xmlns],body[style|dir|vlink|link|text|alink],head');

	h = inst.cleanup.serializeNodeAsHTML(topDoc.documentElement);

	h = h.substring(0, h.lastIndexOf('</body>'));

	if (h.indexOf('<title>') == -1)
		h = h.replace(/<head.*?>/, '$&\n' + '<title>' + inst.cleanup.xmlEncode(f.metatitle.value) + '</title>');
	else
		h = h.replace(/<title>(.*?)<\/title>/, '<title>' + inst.cleanup.xmlEncode(f.metatitle.value) + '</title>');

	if ((v = getSelectValue(f, 'doctypes')) != '')
		h = v + '\n' + h;

	if (f.xml_pi.checked) {
		s = '<?xml version="1.0"';

		if ((v = getSelectValue(f, 'docencoding')) != '')
			s += ' encoding="' + v + '"';

		s += '?>\n';
		h = s + h;
	}

	inst.fullpageTopContent = h;

	tinyMCEPopup.execCommand('mceFullPageUpdate', false, '');
	tinyMCEPopup.close();
}

function setMeta(he, k, v) {
	var nl, i, m;

	nl = he.getElementsByTagName('meta');
	for (i=0; i<nl.length; i++) {
		if (k == 'Content-Type' && tinyMCE.getAttrib(nl[i], 'http-equiv') == k) {
			if (v == '')
				nl[i].parentNode.removeChild(nl[i]);
			else
				nl[i].setAttribute('content', "text/html; charset=" + v);

			return;
		}

		if (tinyMCE.getAttrib(nl[i], 'name') == k) {
			if (v == '')
				nl[i].parentNode.removeChild(nl[i]);
			else
				nl[i].setAttribute('content', v);
			return;
		}
	}

	if (v == '')
		return;

	m = topDoc.createElement('meta');

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

	r = new Array();
	p = v.split(/{|}/);

	for (i=0; i<p.length; i+=2) {
		if (p[i] != "")
			r[r.length] = {rule : tinyMCE.trim(p[i]), data : tinyMCE.parseStyle(p[i+1])};
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

function getReItem(r, s, i) {
	var c = r.exec(s);

	if (c && c.length > i)
		return c[i];

	return '';
}

function changedStyleField(field) {
	//alert(field.id);
}

function showAddMenu() {
	var re = document.getElementById('addbutton');

	addMenuLayer.moveRelativeTo(re, 'tr');
	if (addMenuLayer.isMSIE)
		addMenuLayer.moveBy(2, 0);

	addMenuLayer.show();
	addMenuLayer.setAutoHide(true, hideAddMenu);
	addMenuLayer.addCSSClass(re, 'selected');
}

function hideAddMenu(l, e, mx, my) {
	var re = document.getElementById('addbutton');
	addMenuLayer.removeCSSClass(re, 'selected');
}

function addHeadElm(type) {
	var le = document.getElementById('headlist');
	var re = document.getElementById('addbutton');
	var te = document.getElementById(type + '_element');

	if (lastElementType)
		lastElementType.style.display = 'none';

	te.style.display = 'block';

	lastElementType = te;

	addMenuLayer.hide();
	addMenuLayer.removeCSSClass(re, 'selected');

	document.getElementById(type + '_updateelement').value = tinyMCE.getLang('lang_insert', 'Insert', true);

	le.size = 10;
}

function updateHeadElm(item) {
	var type = item.substring(0, item.indexOf('_'));
	var le = document.getElementById('headlist');
	var re = document.getElementById('addbutton');
	var te = document.getElementById(type + '_element');

	if (lastElementType)
		lastElementType.style.display = 'none';

	te.style.display = 'block';

	lastElementType = te;

	addMenuLayer.hide();
	addMenuLayer.removeCSSClass(re, 'selected');

	document.getElementById(type + '_updateelement').value = tinyMCE.getLang('lang_update', 'Update', true);

	le.size = 10;
}

function cancelElementUpdate() {
	var le = document.getElementById('headlist');

	if (lastElementType)
		lastElementType.style.display = 'none';

	le.size = 26;
}