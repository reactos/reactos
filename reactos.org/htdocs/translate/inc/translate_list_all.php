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
	if ( !defined('ROST') )
	{
		die(" ");
	}

?>
<h1>Translate ReactOS</h1> 
<h2>Translate ReactOS</h2> 
<p>All languages </p>
<table width="100%" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3">
    <td width="25%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Application</strong></font></div></td>
    <td width="15%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Latest change </strong></font></div></td>
    <td width="30%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Up-to-date</strong></font></div></td>
    <td width="30%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>O<font color="#FFFFFF">utdated</font> </strong></font></div></td>
  </tr>
  <?php

	$farbe1="#E2E2E2";
	$farbe2="#EEEEEE";
	$zaehler="0";
	
	$query_content = mysql_query("SELECT * 
									FROM `apps` 
									WHERE `app_enabled` = 1
									ORDER BY `app_name` ASC ;") ;	
									
	while($result_content = mysql_fetch_array($query_content)) { 
?>
  <tr>
    <td valign="middle" bgcolor="<?php
								$zaehler++;
								if ($zaehler == "1") {
									echo $farbe1;
									$farbe = $farbe1;
								}
								elseif ($zaehler == "2") {
									$zaehler="0";
									echo $farbe2;
									$farbe = $farbe2;
								}
							 ?>"><div align="left"><font size="2" face="Verdana, Arial, Helvetica, sans-serif">&nbsp;<a href="<?php echo $ROST_intern_path_ex ."all/". $result_content['app_name']; ?>"><b><?php echo $result_content['app_name']; ?></b></a><b></b></font></div></td>
    <td valign="middle" bgcolor="<?php echo $farbe; ?>"><div align="center"><font size="2" face="Verdana, Arial, Helvetica, sans-serif"><?php 
	
		$query_content_orginal = mysql_query("SELECT * 
									FROM translations 
									WHERE app_id = '". mysql_real_escape_string($result_content['app_id']) ."'
									AND xml_lang = '". mysql_real_escape_string($ROST_setting_default_language) ."'
									ORDER BY xml_rev DESC ;") ;	
		$result_content_orginal = mysql_fetch_array($query_content_orginal);

		echo $result_content_orginal['trans_datetime'];
	
	
	 ?></font></div></td>
    <td valign="middle" bgcolor="<?php echo $farbe; ?>"><div align="left"><font size="2" face="Verdana, Arial, Helvetica, sans-serif">&nbsp;English</font></div></td>
    <td valign="middle" bgcolor="<?php echo $farbe; ?>" ><div align="left"><font size="2" face="Verdana, Arial, Helvetica, sans-serif">&nbsp;German, Russian, French</font></div></td>
  </tr>
  <?php	

	}	// end while
?>
</table>
<p>&nbsp;</p>
