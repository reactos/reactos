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



function create_structure($page )
{
	include('rsdb_setting.php');
	include('rsdb_config.php');
	
	global $RSDB_SET_view;
	global $RSDB_SET_page;

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
	global $RSDB_langres;
	
	global $RSDB_langres;
?>


<table border="0" width="100%" cellpadding="0" cellspacing="0">
  <tr valign="top"> 
    <td width="147" id="leftNav"> <div class="navTitle"><?php echo $RSDB_langres['Navigation']; ?></div>
      <ol>
        
      <li><a href="<?php echo $RSDB_intern_path_server; ?>?page=index"><?php echo $RSDB_langres['Home']; ?></a></li>
        <li><a href="<?php echo $RSDB_intern_path_server; ?>?page=about"><?php echo $RSDB_langres['Info']; ?></a></li>
        <li><a href="<?php echo $RSDB_intern_path_server; ?>?page=community"><?php echo $RSDB_langres['Community']; ?></a></li>
        <li><a href="<?php echo $RSDB_intern_path_server; ?>?page=dev"><?php echo $RSDB_langres['Dev']; ?></a></li>
        <li><a href="<?php echo $RSDB_intern_path_server."roscms/"; ?>?page=user"><?php echo $RSDB_langres['myReactOS']; ?></a></li>
      </ol></div>
      <p></p>
<?php 
	if ($RSDB_SET_view == "comp") {
	?>
	  <div class="navTitle"><?php 
									echo "Compatibility";
						  ?></div>
		<ol>
		  <li><a href="<?php echo $RSDB_intern_link_view_EX."home".$RSDB_URI_slash; ?>">Overview</a></li>
		  <li><a href="<?php echo $RSDB_intern_link_category_EX."0".$RSDB_URI_slash; ?>">Browse Database</a></li>
	<?php
		if ( $RSDB_SET_sec == "category" || $RSDB_SET_sec == "name" || $RSDB_SET_sec == "vendor" || $RSDB_SET_sec == "rank") {
	?>
			  <li><a href="<?php echo $RSDB_intern_link_category_EX."0".$RSDB_URI_slash; ?>">&nbsp;- By Category</a></li>
			  <li><a href="<?php echo $RSDB_intern_link_name_letter_EX."all".$RSDB_URI_slash; ?>">&nbsp;- By Name</a></li>
		<?php
			if ($RSDB_SET_view == "comp") {
		?>
			  <li><a href="<?php echo $RSDB_intern_link_vendor_letter_EX."all".$RSDB_URI_slash; ?>">&nbsp;- By Vendor</a></li>
		<?php
			}
			if ($RSDB_SET_view == "comp") {
		?>
			  <li><a href="<?php echo $RSDB_intern_link_rank_EX."rank".$RSDB_URI_slash; ?>">&nbsp;- By Rank</a></li>
		<?php
			}
		}
	?>
		  <li id="noscriptsearchbar" style="display: block"><a href="<?php echo $RSDB_intern_link_view_EX."search".$RSDB_URI_slash; ?>">Search</a></li>
		  <li><a href="<?php echo $RSDB_intern_link_view_EX."submit".$RSDB_URI_slash; ?>">Submit Application</a></li>
		  <li><a href="<?php echo $RSDB_intern_link_view_EX."stats".$RSDB_URI_slash; ?>">Database Statistics</a></li>
		  <li><a href="<?php echo $RSDB_intern_link_view_EX."help".$RSDB_URI_slash; ?>">Help &amp; FAQ</a></li>
		</ol>
		</div>
		<p></p>
<?php
	}
?>
      <div class="navTitle">Search</div>
      <ol>
        <li> 
			<div id="ajaxsearchbar" align="center" style="display: none">
				<div align="center"><label for="searchbar" accesskey="s"></label><input name="searchbar" type="text" id="searchbar" tabindex="0" onkeyup="loadItemList(this.value,'bar','comp','ajaxloadbar','sresultbar')" size="17" maxlength="50" style="font-family: Verdana; font-size: x-small; font-style: normal;" /> </div>
				<div id="sresultbar" style="display: none" align="left"></div>
			</div>
			<div align="center"><img id="ajaxloadbar" src="images/ajax_loading.gif" style="display: none"></div>
		</li>
      </ol>
	  <p></p>
	  
	 <div class="navTitle">Quick Links</div>
      <ol>
      <li><a href="<?php echo $RSDB_intern_path_server; ?>forum/">Support Forum</a></li>
        <li><a href="<?php echo $RSDB_intern_path_server; ?>?page=community_irc">Chat Channels</a></li>
        <li><a href="<?php echo $RSDB_intern_path_server; ?>?page=community_mailinglists">Mailing Lists</a></li>
        <li><a href="<?php echo $RSDB_intern_path_server; ?>wiki/">ReactOS Wiki</a></li>
      </ol></div>
      <p></p>

	  
	  <script type="text/javascript" language="javascript">
	  <!--
			document.getElementById('ajaxsearchbar').style.display = "block";
			document.getElementById('noscriptsearchbar').style.display = "none";
	  -->
	  </script>
<?php	
	//include("inc/comp/sub/search_bar.php");
	include("inc/member_bar.php");
?>
<div class="navTitle">Language</div>   
      <ol>
        <li> 
          <div align="center"> 
		  <?php 
		    $rpm_page="";
		  	$rpm_lang="";
			if (array_key_exists("page", $_GET)) $rpm_page=htmlspecialchars($_GET["page"]);
		  	if (array_key_exists("lang", $_GET)) $rpm_lang=htmlspecialchars($_GET["lang"]);
//			echo $rpm_lang;
			if ($rpm_lang == '' && isset($_COOKIE['roscms_usrset_lang'])) {
				$rpm_lang = $_COOKIE['roscms_usrset_lang'];
				if (substr($rpm_lang, -1) == '/') {
					$rpm_lang = substr($rpm_lang, strlen($rpm_lang) - 1);
				}
				$rpm_lang = check_lang($rpm_lang);
			}
//			echo $rpm_lang;
		  ?>
            <select id="select" size="1" name="select" class="selectbox" style="width:140px" onchange="window.open(this.options[this.selectedIndex].value,'_main')">
			<optgroup label="current language"> 
			  <?php 
			$query_roscms_lang = mysql_query("SELECT * 
														FROM `rsdb_languages` 
														WHERE `lang_id` = '". mysql_real_escape_string($rpm_lang) ."' ;");
			$result_roscms_lang = mysql_fetch_array($query_roscms_lang);
			
			echo '<option value="#">'.$result_roscms_lang[1].'</option>';
              ?>
              </optgroup>
			  <optgroup label="most popular">
              <option value="<?php echo $RSDB_intern_link_language; ?>en">English</option>
              <option value="<?php echo $RSDB_intern_link_language; ?>de">Deutsch (German)</option>
              <option value="<?php echo $RSDB_intern_link_language; ?>fr">Français (French)</option>
              <option value="<?php echo $RSDB_intern_link_language; ?>ru">Russian (Russian)</option>
              </optgroup>
            </select>
          </div>
        </li>
      </ol>
      <p></p>
      <div class="navTitle">Latest Update</div>
      <ol>
        <li> 
          <div align="center"><?php echo date("Y-m-d"); $zeit = localtime(time() , 1); echo " " . sprintf("%02d",$zeit['tm_hour']).":".sprintf("%02d",$zeit['tm_min']); ?></div>
        </li>
      </ol></div>
	  <p></p>
      </td>
	  
<!-- End of Navigation Bar -->
	  
	  <td id="content">
	  
<?php
} // End of function create_structure
?>
