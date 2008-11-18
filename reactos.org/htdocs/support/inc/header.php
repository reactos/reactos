<?php
    /*
    RSDB - ReactOS Support Database
    Copyright (C) 2005-2006  Klemens Friedl <frik85@reactos.org>

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

/*
 *	ReactOS Support Database System - RSDB
 *	
 *	(c) by Klemens Friedl <frik85>
 *	
 *	2005 - 2006 
 */


	// To prevent hacking activity:
	if ( !defined('RSDB') )
	{
		die(" ");
	}


	
	// This page was generated in ...
	$roscms_gentime = microtime(); 
	$roscms_gentime = explode(' ',$roscms_gentime); 
	$roscms_gentime = $roscms_gentime[1] + $roscms_gentime[0]; 
	$roscms_pg_start = $roscms_gentime; 

function create_head($page_title, $logo, $RSDB_langres)
{
	include('rsdb_setting.php');

?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html lang="<?php echo $RSDB_langres['lang_code']; ?>">
<head>
	<title>ReactOS <?php echo $page_title ; ?></title>
	<base href="<?php echo $RSDB_intern_path_server.$RSDB_intern_path; ?>" />
	<meta http-equiv="Content-Type" content="text/html; charset=<?php echo $RSDB_langres['charset']; ?>" />
	<meta http-equiv="Pragma" content="no-cache" />
	<meta name="Description" content="The ReactOS Compatibility Database has stored a lot of information about application and driver compatibility with ReactOS." />
	<meta name="Keywords" content="ReactOS, Compatibility, application, driver, RSDB, support, database, DB, test, report, rating, Ros32, Ros64, Win32, Win64, W32, W64, Wine, vendor, release, version" />
	<meta name="Copyright" content="ReactOS Foundation" />
	<meta name="Generator" content="RSDB" />
	<meta name="Content-language" content="<?php echo $RSDB_langres['lang_code']; ?>" />
	<meta name="Robots" content="index,follow" />
	<link rel="SHORTCUT ICON" href="../favicon.ico" />
	<link href="<?php echo $RSDB_intern_path_server.$RSDB_intern_path; ?>style.css" type="text/css" rel="stylesheet" />
	<script src="<?php echo $RSDB_intern_path_server.$RSDB_intern_path; ?>smoothscroll.js" language="javascript"></script>
	<script src="<?php echo $RSDB_intern_path_server.$RSDB_intern_path; ?>search.js" language="javascript"></script>
	

	<script>
	<!--
		function clk(url,ct,sg) {
			if(document.images) {
				var u="";
				
				if (url) {
					u="&url="+escape(url).replace(/\+/g,"%2B");
				}
				
				<?php
					$RSDB_i="";
					if (array_key_exists('REMOTE_ADDR', $_SERVER)) $RSDB_i=htmlspecialchars($_SERVER['REMOTE_ADDR']);
				?>
				new Image().src="url.php?t=" + escape(ct) + "&u=" + u + "&a=<?php echo base64_encode($RSDB_intern_user_id);?>" + "&i=<?php echo base64_encode($RSDB_i);?>" + "&s" + sg;

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
		<a href="<?php echo $RSDB_intern_path_server; ?>?page=index"><?php echo $RSDB_langres['Home']; ?></a> <font color="#ffffff">|</font> 
		<a href="<?php echo $RSDB_intern_path_server; ?>?page=about"><?php echo $RSDB_langres['Info']; ?></a> <font color="#ffffff">|</font> 
		<a href="<?php echo $RSDB_intern_path_server; ?>?page=community"><?php echo $RSDB_langres['Community']; ?></a> <font color="#ffffff">|</font> 
		<a href="<?php echo $RSDB_intern_path_server; ?>?page=dev"><?php echo $RSDB_langres['Dev']; ?></a> <font color="#ffffff">|</font> 
		<a href="<?php echo $RSDB_intern_path_server."roscms/"; ?>?page=user"><?php echo $RSDB_langres['myReactOS']; ?></a> </p>
 </div>
</div>


<?php /*<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html><head><title>ReactOS Package Manager - Online</title>

<link rel="stylesheet" type="text/css" href="reactos.basic.css"><!-- reactos.css -->
<META content="ReactOS Package Manager - CMS System" name=GENERATOR>
<META content="Klemens Friedl" name=AUTHOR>
</head>  
<body>*/


//require("./inc/inc_account_check.php");

?>

<!-- Start of Navigation Bar -->

<?php
} // End of function create_head
?>
