<?php

    /*
    ReactOS Paste Service
    Copyright (C) 2006  Klemens Friedl <frik85@reactos.org>

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

function create_header() {

global $ros_paste_SET_path;
global $ros_paste_SET_path_ex;


?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html lang="EN">
<head>
	<base href='<?php echo $ros_paste_SET_path; ?>' />

	<title>ReactOS Paste Service - Copy &amp; Paste</title>
	<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
	<meta http-equiv="Pragma" content="no-cache" />
	<meta name="Description" content="ReactOS Paste Service" />
	<meta name="Keywords" content="ReactOS, paste service" />
	<meta name="Copyright" content="ReactOS Foundation" />
	<meta name="Generator" content="Klemens Friedl" />
	<meta name="Content-language" content="EN" />
	<meta name="Robots" content="index,follow" />
	<link rel="SHORTCUT ICON" href="../favicon.ico" />
	<link href="style.css" type="text/css" rel="stylesheet" />



	<script>
	<!--
		function clk(url,ct,sg) {
			if(document.images) {
				var u="";
				
				if (url) {
					u="&url="+escape(url).replace(/\+/g,"%2B");
				}
				
								new Image().src="url.php?t=" + escape(ct) + "&u=" + u + "&a=Mg==" + "&i=ODUuMjM2LjIzMy40NA==" + "&s" + sg;

			}
			return true;
		}
	-->
	</script>
</head>
<body>
<div id="top">
  <div id="topMenu"> 
    <!-- 
       Use <p> to align things for links/lynx, then in the css make it
	   margin: 0; and use text-align: left/right/etc;.
   -->
    <p align="center"> 
		<a href="http://www.reactos.org/?page=index">Home</a> <font color="#ffffff">|</font> 
		<a href="http://www.reactos.org/?page=about">Info</a> <font color="#ffffff">|</font> 
		<a href="http://www.reactos.org/?page=community">Community</a> <font color="#ffffff">|</font> 
		<a href="http://www.reactos.org/?page=dev">Development</a> <font color="#ffffff">|</font> 
		<a href="http://www.reactos.org/roscms/?page=user">myReactOS</a> </p>
 </div>
</div>



<!-- Start of Navigation Bar -->



<table border="0" width="100%" cellpadding="0" cellspacing="0">
  <tr valign="top"> 
    <td width="147" id="leftNav"> <div class="navTitle">Navigation</div>
      <ol>
        
      <li><a href="http://www.reactos.org/?page=index">Home</a></li>
        <li><a href="http://www.reactos.org/?page=about">Info</a></li>
        <li><a href="http://www.reactos.org/?page=community">Community</a></li>
        <li><a href="http://www.reactos.org/?page=dev">Development</a></li>
        <li><a href="http://www.reactos.org/roscms/?page=user">myReactOS</a></li>
      </ol></div>
      <p></p>
	  <div class="navTitle">Paste Service </div>
		<ol>
		<li><a href="<?php echo $ros_paste_SET_path; ?>">Paste Your Content</a></li>
		<li><a href="<?php echo $ros_paste_SET_path_ex; ?>conditions/">Conditions</a></li>
		<li><a href="<?php echo $ros_paste_SET_path_ex; ?>help/">Help &amp; FAQ</a></li>
		<li><a href="<?php echo $ros_paste_SET_path_ex; ?>recent/">Recent Pastes</a></li>
		</ol>
		</div>
		<p></p>
		
<!-- Google AdSense - start -->
<script type="text/javascript"><!--
google_ad_client = "pub-8424466656027272";
google_ad_width = 120;
google_ad_height = 600;
google_ad_format = "120x600_as";
google_ad_type = "text";
google_ad_channel ="6475218909";
google_color_border = "5984C3";
google_color_bg = "EEEEEE";
google_color_link = "000000";
google_color_text = "000000";
google_color_url = "006090";
//--></script>
<script type="text/javascript"
  src="http://pagead2.googlesyndication.com/pagead/show_ads.js">
</script>
<!-- Google AdSense - end -->
    </td>
	  
<!-- End of Navigation Bar -->
	  
	  <td id="content">
<?php
	}
?>