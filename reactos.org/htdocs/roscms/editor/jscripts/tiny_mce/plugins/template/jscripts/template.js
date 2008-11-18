// Import external list url javascript
var url = tinyMCE.getParam("template_external_list_url");
if (url != null) {
	// Fix relative
	if (url.charAt(0) != '/' && url.indexOf('://') == -1)
		url = tinyMCE.documentBasePath + "/" + url;

	document.write('<sc'+'ript language="javascript" type="text/javascript" src="' + url + '"></sc'+'ript>');
}

var TPU = { //Template Popup Utils
	currentTemplateHTML : null,
	templates : [],
	inst : tinyMCE.getInstanceById(tinyMCE.getWindowArg('editor_id')),
	plugin : tinyMCE.getWindowArg('pluginObj'),
	data : tinyMCE.selectedInstance.getData('template'),

 	init : function() {
 		document.forms[0].insert.value = tinyMCE.getLang('lang_' + this.data.currentAction, 'Insert', true); 
		TPU.loadTemplatePaths();

		if (this.data.currentAction == "update")
			document.getElementById('warning').innerHTML = tinyMCE.getLang('lang_template_warning');

		this.resizeInputs();
	},

 	loadTemplatePaths : function() {
		var tsrc, sel, x, u;

 		tsrc = tinyMCE.getParam("template_templates", false);
 		sel = document.getElementById('tpath');

		// Setup external template list
		if (!tsrc && typeof(tinyMCETemplateList) != 'undefined') {
			for (x=0, tsrc = []; x<tinyMCETemplateList.length; x++)
				tsrc.push({title : tinyMCETemplateList[x][0], src : tinyMCETemplateList[x][1], description : tinyMCETemplateList[x][2]});
		}

		for (x=0; x<tsrc.length; x++) {
			u = tsrc[x].src;

			// Force absolute
			if (u.indexOf('://') == -1 && u.indexOf('/') != 0)
				u = tinyMCE.documentBasePath + "/" + u;

			tsrc[x].src = u;
		}

		TPU.templates = tsrc;

		for (x = 0; x < tsrc.length; x++)
			sel.options[sel.options.length] = new Option(tsrc[x].title, tsrc[x].src);
	},

 	selectTemplate : function(o) {
		var x, d = window.frames['templatesrc'].document;

		this.currentTemplateHTML = this.plugin._replaceValues(this.getFileContents(o.value));

		// Force complete document
		if (!/<body/gi.test(this.currentTemplateHTML)) {
			this.currentTemplateHTML = '<html xmlns="http://www.w3.org/1999/xhtml">' + 
				'<head>' + 
					'<title>blank_page</title>' + 
					'<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />' + 
				'</head>' + 
				'<body>' + 
				this.currentTemplateHTML + 
				'</body>' + 
				'</html>';
		}

		// Write HTML to preview iframe
		d.body.innerHTML = this.currentTemplateHTML;

		// Display description
 		for (x = 0; x < TPU.templates.length; x++) {
			if (TPU.templates[x].src == o.value) {
				document.getElementById('tmpldesc').innerHTML = TPU.templates[x].description;
				break;
			}
		}
 	},

 	insertTemplate : function() {
		var sel, opt;

		sel = document.getElementById('tpath');
		opt = sel.options[sel.selectedIndex];

		// Is it a template or snippet
		if (TPU.currentTemplateHTML.indexOf('mceTmpl'))
			tinyMCEPopup.execCommand('mceTemplate', false, {title : opt.text, tsrc : opt.value, body : TPU.currentTemplateHTML});
		else
			tinyMCEPopup.execCommand('mceInsertContent', false, TPU.currentTemplateHTML);

		tinyMCEPopup.close();
	},

	getFileContents : function(u) {
		var x, d, t = 'text/plain';

		function g(s) {
			x = 0;

			try {
				x = new ActiveXObject(s);
			} catch (s) {
			}

			return x;
		};

		x = window.ActiveXObject ? g('Msxml2.XMLHTTP') || g('Microsoft.XMLHTTP') : new XMLHttpRequest();

		// Synchronous AJAX load file
		x.overrideMimeType && x.overrideMimeType(t);
		x.open("GET", u, false);
		x.send(null);

		return x.responseText;
	},

	resizeInputs : function() {
		var wHeight, wWidth, elm;

		if (!self.innerWidth) {
			wHeight = document.body.clientHeight - 160;
			wWidth = document.body.clientWidth - 40;
		} else {
			wHeight = self.innerHeight - 160;
			wWidth = self.innerWidth - 40;
		}

		elm = document.getElementById('templatesrc');

		if (elm) {
			elm.style.height = Math.abs(wHeight) + 'px';
			elm.style.width  = Math.abs(wWidth - 5) + 'px';
		}
	}
};
