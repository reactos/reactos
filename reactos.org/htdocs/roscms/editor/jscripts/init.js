		tinyMCE.init({
			mode : "none",
			theme : "advanced",
			editor_selector : "mceEditor",
			plugins : "table,advhr,advimage,advlink,emotions,iespell,insertdatetime,zoom,searchreplace,print,contextmenu,paste,directionality",
			theme_advanced_buttons1_add_before : "code, separator, newdocument, separator",
			theme_advanced_buttons1_add : "fontselect,fontsizeselect",
			theme_advanced_buttons2_add : "separator,insertdate,inserttime,zoom,separator,forecolor,backcolor",
			theme_advanced_buttons2_add_before: "cut,copy,paste,pastetext,pasteword,separator,search,replace,separator",
			theme_advanced_buttons3_add_before : "tablecontrols,separator",
			theme_advanced_buttons3_add : "emotions,iespell,media,advhr,separator,print,separator,ltr,rtl",
			theme_advanced_disable : "help, code",
			theme_advanced_toolbar_location : "top",
			theme_advanced_toolbar_align : "left",
			theme_advanced_statusbar_location : "bottom",
			/*content_css : "example_word.css",*/
			plugi2n_insertdate_dateFormat : "%Y-%m-%d",
			plugi2n_insertdate_timeFormat : "%H:%M:%S",
			external_link_list_url : "example_link_list.js",
			external_image_list_url : "example_image_list.js",
			media_external_list_url : "example_media_list.js",
			file_browser_callback : "fileBrowserCallBack",		
			valid_elements : "" +
				"+a[id|style|rel|rev|charset|hreflang|dir|lang|tabindex|accesskey|type|name|href|target|title|class|onfocus|onblur|onclick|" + 
					"ondblclick|onmousedown|onmouseup|onmouseover|onmousemove|onmouseout|onkeypress|onkeydown|onkeyup]," + 
				"-strong/-b[class|style]," + 
				"-em/-i[class|style]," + 
				"-strike[class|style]," + 
				"-u[class|style]," + 
				"#p[id|style|dir|class|align]," + 
				"-ol[class|style]," + 
				"-ul[class|style]," + 
				"-li[class|style]," + 
				 "br," + 
				 "img[id|dir|lang|longdesc|usemap|style|class|src|onmouseover|onmouseout|border|alt=|title|hspace|vspace|width|height|align]," + 
				"-sub[style|class]," + 
				"-sup[style|class]," + 
				"-blockquote[dir|style]," + 
				"-table[border=0|cellspacing|cellpadding|width|height|class|align|summary|style|dir|id|lang|bgcolor|background|bordercolor]," + 
				"-tr[id|lang|dir|class|rowspan|width|height|align|valign|style|bgcolor|background|bordercolor]," + 
				 "tbody[id|class]," + 
				 "thead[id|class]," + 
				 "tfoot[id|class]," + 
				"-td[id|lang|dir|class|colspan|rowspan|width|height|align|valign|style|bgcolor|background|bordercolor|scope]," + 
				"-th[id|lang|dir|class|colspan|rowspan|width|height|align|valign|style|scope]," + 
				 "caption[id|lang|dir|class|style]," + 
				"-div[id|dir|class|align|style]," + 
				"-span[style|class|align]," + 
				"-pre[class|align|style]," + 
				 "address[class|align|style]," + 
				"-h1[id|style|dir|class|align]," + 
				"-h2[id|style|dir|class|align]," + 
				"-h3[id|style|dir|class|align]," + 
				"-h4[id|style|dir|class|align]," + 
				"-h5[id|style|dir|class|align]," + 
				"-h6[id|style|dir|class|align]," + 
				 "hr[class|style]," + 
				"-font[face|size|style|id|class|dir|color]," + 
				 "dd[id|class|title|style|dir|lang]," + 
				 "dl[id|class|title|style|dir|lang]," + 
				 "dt[id|class|title|style|dir|lang]",		
			paste_use_dialog : false,
			theme_advanced_resizing : true,
			theme_advanced_resize_horizontal : false,
			theme_advanced_link_targets : "_something=My somthing;_something2=My somthing2;_something3=My somthing3;",
			paste_auto_cleanup_on_paste : true,
			paste_convert_headers_to_strong : false,
			paste_strip_class_attributes : "all",
			paste_remove_spans : false,
			paste_remove_styles : false,	
			apply_source_formatting : true 
		});
