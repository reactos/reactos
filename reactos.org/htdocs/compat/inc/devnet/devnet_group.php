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



	$query_page = mysql_query("SELECT * 
								FROM `rsdb_groups` 
								WHERE `grpentr_visible` = '1'
								AND `grpentr_id` = " . $RSDB_SET_group . "
								" . $RSDB_intern_code_db_rsdb_groups . "
								ORDER BY `grpentr_name` ASC") ;
	
	$result_page = mysql_fetch_array($query_page);
	
	if ($result_page['grpentr_type'] == "default") {

?>

<style type="text/css">
<!--
/* tab colors */
.tab                { background-color : #ffffff; }
.tab_s              { background-color : #5984C3; }
.tab_u              { background-color : #A0B7C9; }

/* tab link colors */
a.tabLink           { text-decoration : none; }
a.tabLink:link      { text-decoration : none; }
a.tabLink:visited   { text-decoration : none; }
a.tabLink:hover     { text-decoration : underline; }
a.tabLink:active    { text-decoration : underline; }

/* tab link size */
p.tabLink_s         { color: navy; font-size : 10pt; font-weight : bold; padding : 0 8px 1px 2px; margin : 0; }
p.tabLink_u         { color: black; font-size : 10pt; padding : 0 8px 1px 2px; margin : 0; }

/* text styles */
.strike 	       { text-decoration: line-through; }
.bold              { font-weight: bold; }
.newstitle         { font-weight: bold; color: purple; }
.title_group       { font-size: 16px; font-weight: bold; color: #5984C3; text-decoration: none; }
.bluetitle:visited { color: #323fa2; text-decoration: none; }

.Stil1 {font-size: xx-small}
.Stil2 {font-size: x-small}
.Stil3 {color: #FFFFFF}
.Stil4 {font-size: xx-small; color: #FFFFFF; }

-->
</style>

	<table align="center" border="0" cellpadding="0" cellspacing="0" width="100%">
        <tr align="left" valign="top">
          <!-- title -->
          <td valign="bottom" width="100%">
            <table border="0" cellpadding="0" cellspacing="0" width="100%">
                <tr>
                  <td class="title_group" nowrap="nowrap">&nbsp;</td>
                </tr>
                <tr valign="bottom">
                  <td class="tab_s"><img src="images/white_pixel.gif" alt="" height="1" width="1"></td>
                </tr>
          </table></td>
          <!-- start tab -->
          <td nowrap="nowrap">
            <table border="0" cellpadding="0" cellspacing="0">
                <tr align="left" valign="top">
                  <td width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr align="left" valign="top">
                  <td class="tab_s" width="1"><img src="images/white_pixel.gif" alt="" height="4" width="1"></td>
                  <td width="4"><img src="images/tab_corner_active.gif" alt="" height="4" width="4"></td>
                  <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="middle">
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="4"></td>
                  <td nowrap="nowrap"><p class="tabLink_s"><a href="<?php echo $RSDB_intern_link_group_group2; ?>overview" class="tabLink">Overview</a></p></td>
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="bottom">
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab" width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
          </table></td>
          <!-- end tab -->

          <!-- fill the remaining space -->
          <td valign="bottom" width="10">
            <table border="0" cellpadding="0" cellspacing="0" width="100%">
                <tr valign="bottom">
                  <td class="tab_s"><img src="images/white_pixel.gif" alt="" height="1" width="10"></td>
                </tr>
          </table></td>
        </tr>
</table>

	<h2><?php echo $result_page['grpentr_name']; ?></h2>
	<p><font size="3" face="Arial, Helvetica, sans-serif"><?php echo $result_page['grpentr_description']; ?></font></p>
	<p>&nbsp;</p>
	
<?php
	// Count the devnet entries
	$query_count_devnet=mysql_query("SELECT COUNT('devnet_id') FROM rsdb_item_devnet WHERE devnet_groupid = '". $result_page["grpentr_id"] ."' ;");	
	$result_count_devnet = mysql_fetch_row($query_count_devnet);
	
	if ($result_count_devnet[0]) {
		echo "<ul>";
		$RSDB_intern_sortby_SQL_a_query = "SELECT * 
								FROM `rsdb_item_devnet` 
								WHERE `devnet_groupid` = " . $RSDB_SET_group . "
								AND `devnet_visible` = '1'
								ORDER BY `devnet_order` ASC ;";
		$query_sortby_a = mysql_query($RSDB_intern_sortby_SQL_a_query) ;
		while($result_sortby_a = mysql_fetch_array($query_sortby_a)) {
			echo '<li><a href="'.$RSDB_intern_link_item.$result_sortby_a['devnet_id'].'">'.$result_sortby_a['devnet_name'].'</a></li>';
		}
		echo "</ul>";
	}
} // end if {$result_page['grpentr_type'] == "default"}
elseif ($result_page['grpentr_type'] == "article") {
	$query_articlefile = mysql_query("SELECT * 
										FROM `rsdb_item_devnet` 
										WHERE `devnet_groupid` = " . $RSDB_SET_group . "
										AND `devnet_visible` = '1'
										ORDER BY `devnet_order` ASC ;");
	$result_articlefile = mysql_fetch_array($query_articlefile);
	echo "<h2>".$result_articlefile['devnet_name']."</h2>";
	echo "<p><i>".$result_articlefile['devnet_description']."</i></p>";
	echo $result_articlefile['devnet_text'];
}

?>
	