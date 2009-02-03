    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005-2008  Klemens Friedl <frik85@reactos.org>
                  2005       Ge van Geldorp <gvg@reactos.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    */

// search filter standard text
clearSearchFilter();

var nres=0;
var hlRows=true;
document.getElementById('tablist').style.display = 'block';
document.getElementById('frametable').style.display = 'block';
document.getElementById('frameedit').style.display = 'none';

// open standard left menu
loadMenu('smenutab8');

if (roscms_get_edit != "") {
  document.getElementById('frametable').style.display = 'none';
  document.getElementById('frameedit').style.display = 'block';
  loadEditor('all', roscms_get_edit);
}

// load user filter
document.getElementById('labtitel2c').innerHTML = '<div align="right"><img src="images/ajax_loading.gif" alt="loading ..." style="width:13px; height:13px;" /></div>';
makeRequest('?page=backend&type=text&subtype=ufs&d_val=load', 'ufs', 'labtitel2c', 'html', 'GET', '');

// load user labels
document.getElementById('labtitel3c').innerHTML = '<div align="right"><img src="images/ajax_loading.gif" alt="loading ..." style="width:13px; height:13px;" /></div>';
makeRequest('?page=backend&type=text&subtype=ut', 'ut', 'labtitel3c', 'html', 'GET', '');

if (readCookie('labtitel1') == 0) TabOpenCloseEx('labtitel1');
if (readCookie('labtitel2') == 0) TabOpenCloseEx('labtitel2');
if (readCookie('labtitel3') == 0) TabOpenCloseEx('labtitel3');

roscms_page_load_finished = true;

// window unload blocker
if (exitmsg !== '') {
  window.onbeforeunload = exitmsg;
}
else {
  window.onbeforeunload = false;
}

// initialise tinyMCE
tinyMCE.init({
  mode : "textareas",
  theme : "advanced",
  editor_selector : "mceEditor",
  plugins : "table,advhr,advimage,advlink,iespell,insertdatetime,zoom,searchreplace,print,contextmenu,paste,directionality",
  theme_advanced_buttons1 : "newdocument,|,bold,italic,underline,strikethrough,|,justifyleft,justifycenter,justifyright,justifyfull,styleselect,formatselect,fontselect,fontsizeselect",
  theme_advanced_buttons2 : "cut,copy,paste,pastetext,pasteword,|,search,replace,|,bullist,numlist,|,outdent,indent,blockquote,|,undo,redo,|,link,unlink,anchor,image,cleanup,|,insertdate,inserttime,preview,|,forecolor,backcolor",
  theme_advanced_buttons3 : "tablecontrols,|,hr,removeformat,visualaid,|,sub,sup,|,charmap,iespell,advhr,|,print,|,fullscreen",
  theme_advanced_toolbar_location : "top",
  theme_advanced_toolbar_align : "left",
  theme_advanced_statusbar_location : "bottom",
  theme_advanced_resizing : false,

  plugi2n_insertdate_dateFormat : "%Y-%m-%d",
  plugi2n_insertdate_timeFormat : "%H:%M:%S",
  external_link_list_url : "example_link_list.js",
  external_image_list_url : "example_image_list.js",
  media_external_list_url : "example_media_list.js",
  file_browser_callback : "fileBrowserCallBack",

  paste_use_dialog : false,
  paste_auto_cleanup_on_paste : true,
  paste_convert_headers_to_strong : false,
  paste_strip_class_attributes : "all",
  paste_remove_spans : false,
  paste_remove_styles : false,
  apply_source_formatting : true 
});