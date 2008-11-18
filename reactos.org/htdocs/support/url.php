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
 *	URL LOG SCRIPT
 */

	require_once("connect.db.php");


	$RSDB_url_t="";
	//$RSDB_url_o="";
	$RSDB_url_u="";
	$RSDB_url_a="";
	$RSDB_url_i="";
	$RSDB_url_s="";
	$RSDB_referrer="";
	$RSDB_usragent="";
	
	if (array_key_exists("t", $_GET)) $RSDB_url_t=htmlspecialchars($_GET["t"]);
	//if (array_key_exists("o", $_GET)) $RSDB_url_o=htmlspecialchars($_GET["o"]);
	if (array_key_exists("u", $_GET)) $RSDB_url_u=htmlspecialchars($_GET["u"]);
	if (array_key_exists("a", $_GET)) $RSDB_url_a=htmlspecialchars($_GET["a"]);
	if (array_key_exists("i", $_GET)) $RSDB_url_i=htmlspecialchars($_GET["i"]);
	if (array_key_exists("s", $_GET)) $RSDB_url_s=htmlspecialchars($_GET["s"]);
	
	$RSDB_url_i = base64_decode($RSDB_url_i);
	
	
	if ($RSDB_url_u != $RSDB_referrer) {
		$RSDB_url_s = "BAD".$RSDB_url_s;
	}
	if ($RSDB_referrer == "" && $RSDB_url_u != "") {
		$RSDB_referrer = $RSDB_url_u;
	}
	
	if (array_key_exists('HTTP_REFERER', $_SERVER)) $RSDB_referrer=htmlspecialchars($_SERVER['HTTP_REFERER']);
	if (array_key_exists('HTTP_USER_AGENT', $_SERVER)) $RSDB_usragent=htmlspecialchars($_SERVER['HTTP_USER_AGENT']);
	
	if(isset($_COOKIE['roscms_usrset_lang'])) {
		$roscms_usrsetting_lang=$_COOKIE["roscms_usrset_lang"];
	}
	else {
		$roscms_usrsetting_lang="";
	}


	$report_submit="INSERT INTO `rsdb_urls` ( `url_id` , `url_t` , `url_u` , `url_a` , `url_i` , `url_s` , `url_lang` , `url_browser` ) 
					VALUES ('', '".mysql_real_escape_string($RSDB_url_t)."', '".mysql_real_escape_string($RSDB_referrer)."', '".mysql_real_escape_string($RSDB_url_a)."', '".mysql_real_escape_string($RSDB_url_i)."', '".mysql_real_escape_string($RSDB_url_s)."', '".mysql_real_escape_string($roscms_usrsetting_lang)."', '".mysql_real_escape_string($RSDB_usragent)."' );";
	$db_report_submit=mysql_query($report_submit);


?>
