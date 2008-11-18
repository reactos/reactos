<?php
    /*
    ROST - ReactOS Support Database
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
 *	ReactOS Support Database System - ROST
 *	
 *	(c) by Klemens Friedl <frik85>
 *	
 *	2005 - 2006 
 */


	// To prevent hacking activity:
	if ( !defined('ROST') )
	{
		die(" ");
	}



function create_structure()
{
	include('rost_config.php');
	include('rost_vars.php');
	
	
	global $ROST_SET_view;
	global $ROST_SET_page;

	global $roscms_intern_account_level;
	global $roscms_intern_login_check;
	global $roscms_intern_login_check_username;
	global $roscms_intern_account_group;
	global $roscms_intern_usrgrp_sadmin;
	global $roscms_intern_usrgrp_admin;
	global $roscms_intern_usrgrp_dev;
	global $roscms_intern_usrgrp_team;
	global $roscms_intern_usrgrp_trans;
	global $roscms_intern_usrgrp_user;


	global $ROST_langres;
	

	global $ROST_USER_name;
	global $ROST_intern_user_id;

?>


<table border="0" width="100%" cellpadding="0" cellspacing="0">
  <tr valign="top"> 
    <td width="147" id="leftNav">
<?php /* <div class="navTitle"><?php echo $ROST_langres['Navigation']; ?></div>
      <ol>  
      <li><a href="<?php echo $ROST_intern_path_server; ?>?page=index"><?php echo $ROST_langres['Home']; ?></a></li>
        <li><a href="<?php echo $ROST_intern_path_server; ?>?page=about"><?php echo $ROST_langres['Info']; ?></a></li>
        <li><a href="<?php echo $ROST_intern_path_server; ?>?page=community"><?php echo $ROST_langres['Community']; ?></a></li>
        <li><a href="<?php echo $ROST_intern_path_server; ?>?page=dev"><?php echo $ROST_langres['Dev']; ?></a></li>
        <li><a href="<?php echo $ROST_intern_path_server."roscms/"; ?>?page=user"><?php echo $ROST_langres['myReactOS']; ?></a></li>
      </ol></div>
      <p></p> */ ?>
	  <div class="navTitle">Translate</div>
		<ol>
		  <li><a href="<?php echo $ROST_intern_link_section; ?>home">Overview</a></li>
		  <li><a href="<?php echo $ROST_intern_link_section; ?>all">Translations</a></li>
	<?php
		if ( $ROST_SET_sec == "all" || check_langcode($ROST_SET_sec) ) {
			// main language:
			echo '<li><a href="'. $ROST_setting_default_language_short .'">&nbsp;-&nbsp;'. $ROST_setting_default_language_name .'</a></li>';
			
			// major languages:
			$query_languages = SQL_query("SELECT * 
												FROM `languages` 
												WHERE `lang_active` = '1'
												AND `lang_quantifier` = '2'
												ORDER BY `lang_code` ASC ;");
			while ($result_languages = SQL_loop_array($query_languages)) {
				$temp_short_language_name = split("[ ]", $result_languages['lang_name']);
				echo '<li><a href="'. $ROST_intern_link_section.$result_languages['lang_code'] .'">&nbsp;-&nbsp;'. $temp_short_language_name[0] .'</a></li>';
			}
		}
	?>
		  <li><a href="<?php echo $ROST_intern_link_section; ?>help">Help &amp; FAQ</a></li>
		  <li><a href="<?php echo $ROST_intern_link_section; ?>admin">Admin Interface</a></li>
<?php
		/*
			if ( $ROST_SET_sec == "admin" || $ROST_SET_sec == "import" || $ROST_SET_sec == "export") { 
				echo '<li><a href="'. $ROST_intern_link_section .'import">&nbsp;-&nbsp;Import</a></li>';
				echo '<li><a href="'. $ROST_intern_link_section .'export">&nbsp;-&nbsp;Export</a></li>';
			}
		*/
?>
		</ol>
		</div>
		<p></p>
<?php	
	include("inc/member_bar.php");
?>
	  </td>
	  
<!-- End of Navigation Bar -->
	  
	  <td id="content">
	  
<?php
} // End of function create_structure
?>
