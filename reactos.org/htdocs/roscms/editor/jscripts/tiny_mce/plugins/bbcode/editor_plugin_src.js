var TinyMCE_BBCodePlugin = {
	getInfo : function() {
		return {
			longname : 'BBCode Plugin',
			author : 'Moxiecode Systems AB',
			authorurl : 'http://tinymce.moxiecode.com',
			infourl : 'http://wiki.moxiecode.com/index.php/TinyMCE:Plugins/bbcode',
			version : tinyMCE.majorVersion + "." + tinyMCE.minorVersion
		};
	},

	cleanup : function(type, content) {
		var dialect = tinyMCE.getParam('bbcode_dialect', 'punbb').toLowerCase();

		switch (type) {
			case "insert_to_editor":
				content = this['_' + dialect + '_bbcode2html'](content);
				break;

			case "get_from_editor":
				content = this['_' + dialect + '_html2bbcode'](content);
				break;
		}

		return content;
	},

	// Private methods

	// HTML -> BBCode in PunBB dialect
	_punbb_html2bbcode : function(s) {
		s = tinyMCE.trim(s);

		function rep(re, str) {
			s = s.replace(re, str);
		};

		// example: <strong> to [b]
		rep(/<a href=\"(.*?)\".*?>(.*?)<\/a>/gi,"[url]$1[/url]");
		rep(/<font.*?color=\"(.*?)\".*?class=\"codeStyle\".*?>(.*?)<\/font>/gi,"[code][color=$1]$2[/color][/code]");
		rep(/<font.*?color=\"(.*?)\".*?class=\"quoteStyle\".*?>(.*?)<\/font>/gi,"[quote][color=$1]$2[/color][/quote]");
		rep(/<font.*?class=\"codeStyle\".*?color=\"(.*?)\".*?>(.*?)<\/font>/gi,"[code][color=$1]$2[/color][/code]");
		rep(/<font.*?class=\"quoteStyle\".*?color=\"(.*?)\".*?>(.*?)<\/font>/gi,"[quote][color=$1]$2[/color][/quote]");
		rep(/<font.*?color=\"(.*?)\".*?>(.*?)<\/font>/gi,"[color=$1]$2[/color]");
		rep(/<font>(.*?)<\/font>/gi,"$1");
		rep(/<img.*?src=\"(.*?)\".*?\/>/gi,"[img]$1[/img]");
		rep(/<span class=\"codeStyle\">(.*?)<\/span>/gi,"[code]$1[/code]");
		rep(/<span class=\"quoteStyle\">(.*?)<\/span>/gi,"[quote]$1[/quote]");
		rep(/<strong class=\"codeStyle\">(.*?)<\/strong>/gi,"[code][b]$1[/b][/code]");
		rep(/<strong class=\"quoteStyle\">(.*?)<\/strong>/gi,"[quote][b]$1[/b][/quote]");
		rep(/<em class=\"codeStyle\">(.*?)<\/em>/gi,"[code][i]$1[/i][/code]");
		rep(/<em class=\"quoteStyle\">(.*?)<\/em>/gi,"[quote][i]$1[/i][/quote]");
		rep(/<u class=\"codeStyle\">(.*?)<\/u>/gi,"[code][u]$1[/u][/code]");
		rep(/<u class=\"quoteStyle\">(.*?)<\/u>/gi,"[quote][u]$1[/u][/quote]");
		rep(/<\/(strong|b)>/gi,"[/b]");
		rep(/<(strong|b)>/gi,"[b]");
		rep(/<\/(em|i)>/gi,"[/i]");
		rep(/<(em|i)>/gi,"[i]");
		rep(/<\/u>/gi,"[/u]");
		rep(/<u>/gi,"[u]");
		rep(/<br \/>/gi,"\n");
		rep(/<br\/>/gi,"\n");
		rep(/<br>/gi,"\n");
		rep(/<p>/gi,"");
		rep(/<\/p>/gi,"\n");
		rep(/&nbsp;/gi," ");
		rep(/&quot;/gi,"\"");
		rep(/&lt;/gi,"<");
		rep(/&gt;/gi,">");
		rep(/&amp;/gi,"&");
		rep(/&undefined;/gi,"'"); // quickfix

		return s; 
	},

	// BBCode -> HTML from PunBB dialect
	_punbb_bbcode2html : function(s) {
		s = tinyMCE.trim(s);

		function rep(re, str) {
			s = s.replace(re, str);
		};

		// example: [b] to <strong>
		rep(/\n/gi,"<br />");
		rep(/\[b\]/gi,"<strong>");
		rep(/\[\/b\]/gi,"</strong>");
		rep(/\[i\]/gi,"<em>");
		rep(/\[\/i\]/gi,"</em>");
		rep(/\[u\]/gi,"<u>");
		rep(/\[\/u\]/gi,"</u>");
		rep(/\[url\](.*?)\[\/url\]/gi,"<a href=\"$1\">$1</a>");
		rep(/\[img\](.*?)\[\/img\]/gi,"<img src=\"$1\" />");
		rep(/\[color=(.*?)\](.*?)\[\/color\]/gi,"<font color=\"$1\">$2</font>");
		rep(/\[code\](.*?)\[\/code\]/gi,"<span class=\"codeStyle\">$1</span>&nbsp;");
		rep(/\[quote.*?\](.*?)\[\/quote\]/gi,"<span class=\"quoteStyle\">$1</span>&nbsp;");

		return s; 
	}
};

tinyMCE.addPlugin("bbcode", TinyMCE_BBCodePlugin);
