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
	<table width="100%" border="0" cellpadding="1" cellspacing="5">
      <tr>
        <td width="40%" valign="top">

          <h3>Description</h3>
          <table width="100%" border="0" cellpadding="1" cellspacing="1">
          <tr bgcolor="#5984C3">
            <td width="100" bgcolor="#5984C3"><div align="left"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>&nbsp;Name</strong></font></div></td>
            <td width="300" bgcolor="#E2E2E2">
              <div align="left"><font size="3" face="Arial, Helvetica, sans-serif"><?php echo $result_page['grpentr_name']; ?></font></div></td>
          </tr>
          <tr>
            <td valign="top" bgcolor="#5984C3" >
              <div align="left"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>&nbsp;Description</strong></font></div></td>
            <td valign="top" bgcolor="#EEEEEE">
              <div align="left"><font size="3" face="Arial, Helvetica, sans-serif"><?php echo $result_page['grpentr_description']; ?></font><font size="2" face="Arial, Helvetica, sans-serif"> </font></div></td>
          </tr>
          <tr>
            <td valign="top" bgcolor="#5984C3" ><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>&nbsp;Vendor</strong></font></td>
            <td valign="top" bgcolor="#E2E2E2">&nbsp;</td>
          </tr>
          <tr>
            <td valign="top" bgcolor="#5984C3" ><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>&nbsp;URL</strong></font></td>
            <td valign="top" bgcolor="#EEEEEE"><a href="http://www.reactos.org">test.com</a></td>
          </tr>
          <tr>
            <td valign="top" bgcolor="#5984C3" ><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>&nbsp;Award</strong></font></td>
            <td valign="top" bgcolor="#E2E2E2"><img src="media/icons/awards/gold_32.gif" width="32" height="32"></td>
          </tr>
          <tr>
            <td valign="top" bgcolor="#5984C3" ><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>&nbsp;Votes</strong></font></td>
            <td valign="top" bgcolor="#EEEEEE">17</td>
          </tr>
          <tr>
            <td valign="top" bgcolor="#5984C3" >&nbsp;</td>
            <td valign="top" bgcolor="#E2E2E2">&nbsp;</td>
          </tr>
        </table>        </td>
        <td width="10%" align="center" valign="top"></td>
        <td width="40%" valign="top">
        <h3 align="right">Screenshot</h3>
        <p align="center"><img src="media/screenshots/<?php
		
			if ($result_page['grpentr_pic'] != "") {
				echo $result_page['grpentr_pic'];
		 	}
			else {
				echo "media_default.jpg";
			}
		 
		 ?>" width="250" height="188"></p></td>
      </tr>
    </table>
    <p>&nbsp;</p>
	
<?php
	// Count the media entries
	$query_count_media=mysql_query("SELECT COUNT('media_id') FROM rsdb_item_media WHERE media_groupid = '". $result_page["grpentr_id"] ."' ;");	
	$result_count_media = mysql_fetch_row($query_count_media);
	
	if ($result_count_media[0]) {

?>	
	<h3>Versions</h3>
    <table width="100%" border="0" >
      <tr>
        <td><div align="center"><span class="Stil2"><a name="ver"></a>Sort by:
                  <?php

				if ($RSDB_SET_sort == "item") {
					echo "<b>Item version</b> | <a href='".$RSDB_intern_link_group_sort."ros#ver'>ReactOS version</a>";
					
					$RSDB_intern_sortby_SQL_a_query = "SELECT * 
														FROM `rsdb_item_media` 
														WHERE `media_groupid` = " . $RSDB_SET_group . "
														ORDER BY `media_appversion` DESC ;";
					
					
					$RSDB_intern_sortby_headline_field = "media_appversion";
					$RSDB_intern_sortby_linkname_field = "media_rosversion";
				}
				if ($RSDB_SET_sort == "ros") {
					echo "<a href='".$RSDB_intern_link_group_sort."item#ver'>Item version</a> | <b>ReactOS version</b>";
					
					$RSDB_intern_sortby_SQL_a_query = "SELECT * 
														FROM `rsdb_item_media` 
														WHERE `media_groupid` = " . $RSDB_SET_group . "
														ORDER BY `media_rosversion` DESC ;";
					
					
					$RSDB_intern_sortby_headline_field = "media_rosversion";
					$RSDB_intern_sortby_linkname_field = "media_appversion";
				}
				

		 ?>
        </span></div></td>
      </tr>
      <?php
	// Table
	$RSDB_intern_TEMP_version_saved_a = "";
	$query_sortby_a = mysql_query($RSDB_intern_sortby_SQL_a_query) ;
	while($result_sortby_a = mysql_fetch_array($query_sortby_a)) {
		if ($result_sortby_a[$RSDB_intern_sortby_headline_field] != $RSDB_intern_TEMP_version_saved_a) {
		
				if ($RSDB_SET_sort == "item") {
							$RSDB_intern_sortby_SQL_b_query = "SELECT * 
														FROM `rsdb_item_media` 
														WHERE `media_groupid` = " . $RSDB_SET_group . "
														AND `media_appversion` = '" . $result_sortby_a['media_appversion'] . "'
														ORDER BY `media_rosversion` DESC ;";
				}
				if ($RSDB_SET_sort == "ros") {
							$RSDB_intern_sortby_SQL_b_query = "SELECT * 
														FROM `rsdb_item_media` 
														WHERE `media_groupid` = " . $RSDB_SET_group . "
														AND `media_rosversion` = '" . $result_sortby_a['media_rosversion'] . "'
														ORDER BY `media_appversion` DESC ;";
														//echo $RSDB_intern_sortby_SQL_b_query;
				}
		
?>
      <tr>
        <td>
          <table width="100%">
            <tr bgcolor="#5984C3">
              <td colspan="5">
                <table width="100%" border="0" cellpadding="0" cellspacing="0">
                  <tr>
                    <td width="40%"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>&nbsp;<?php
									
						if ($RSDB_SET_sort == "item") {
							echo $result_sortby_a['media_name']." [";
							echo $result_sortby_a[$RSDB_intern_sortby_headline_field]."]"; 
						}
						else {
							echo $result_sortby_a[$RSDB_intern_sortby_headline_field]; 
						}
						
					 ?></strong></font></td>
                    <td width="20%"><div align="center" class="Stil4">Medal</div></td>
                    <td width="13%"><div align="center" class="Stil4">Function</div></td>
                    <td width="13%"><div align="center" class="Stil4">Install</div></td>
                    <td width="14%"><div align="center" class="Stil4">User Rating</div></td>
                  </tr>
                </table>
            </tr>
            <?php  
			// Table line
			$farbe1="#E2E2E2";
			$farbe2="#EEEEEE";
			$zaehler="0";
			$query_sortby_b = mysql_query($RSDB_intern_sortby_SQL_b_query) ;
			while($result_sortby_b = mysql_fetch_array($query_sortby_b)) { 
?>
            <tr bgcolor="<?php
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
								 ?>">
              <td width="40%" bgcolor="<?php echo $farbe; ?>">&nbsp;<a href="<?php echo $RSDB_intern_link_item.$result_sortby_b['media_id']; ?>"><?php

					if ($RSDB_SET_sort == "ros") {
						echo $result_sortby_b['media_name']." [";
						echo $result_sortby_b[$RSDB_intern_sortby_linkname_field]."]";
					}
					else {
						echo $result_sortby_b[$RSDB_intern_sortby_linkname_field];
					}				
				 ?></a></td>
              <td width="20%" bgcolor="<?php echo $farbe; ?>"><img src="media/icons/awards/fail.gif" width="16" height="16"> <span class="Stil1">Known not to work</span></td>
              <td width="13%" bgcolor="<?php echo $farbe; ?>"><img src="media/icons/info/1.gif" width="16" height="16"> <span class="Stil1">work great </span></td>
              <td width="13%" bgcolor="<?php echo $farbe; ?>"><img src="media/icons/info/2.gif" width="16" height="16"> <span class="Stil1">major bugs </span></td>
              <td width="14%" bgcolor="<?php echo $farbe; ?>"><img src="media/icons/stars/star.gif" width="16" height="16"><img src="media/icons/stars/star.gif" width="16" height="16"><img src="media/icons/stars/star.gif" width="16" height="16"><img src="media/icons/stars/star.gif" width="16" height="16"><img src="media/icons/stars/star.gif" width="16" height="16"></td>
            </tr>
            <?php
			}
?>
          </table>
      <tr>
        <td>
          <?php
			$RSDB_intern_TEMP_version_saved_a = $result_sortby_a[$RSDB_intern_sortby_headline_field];
		}
	}
?>
        </td>
      </tr>
    </table>
    <p>&nbsp;</p>
<?php
	}
?>

<?php  
	// Query Bundles
	$query_bundlelist = mysql_query("SELECT * FROM rsdb_group_bundles WHERE bundle_groupid = '". $result_page["grpentr_id"] ."' ;") ;
	while($result_bundlelist = mysql_fetch_array($query_bundlelist)) {
		// Count the bundle entries for the current bundle
		$query_count_bundle=mysql_query("SELECT COUNT('bundle_id') FROM rsdb_group_bundles WHERE bundle_id = '". $result_bundlelist["bundle_id"] ."' ;");	
		$result_count_bundle = mysql_fetch_row($query_count_bundle);
		
		if ($result_count_bundle[0]) {
?>
	<h3>Bundle</h3>
<?php  
			$farbe1="#E2E2E2";
			$farbe2="#EEEEEE";
			$zaehler="0";
			echo "<table width='100%'  border='0'>";
			echo "  <tr bgcolor='#5984C3'>";
			echo "	<td width='30%'><div align='center'><font color='#FFFFFF' face='Arial, Helvetica, sans-serif'><b>Name</b></font></div></td>";
			echo "	<td width='70%'><div align='center'><font color='#FFFFFF' face='Arial, Helvetica, sans-serif'><b>Description</b></font></div></td>";
			echo "  </tr>";
			$query_bundlelist_groupitem = mysql_query("SELECT * FROM rsdb_group_bundles WHERE bundle_id = '". $result_bundlelist["bundle_id"] ."' ORDER BY `bundle_groupid` ASC ;") ;
			while($result_bundlelist_groupitem = mysql_fetch_array($query_bundlelist_groupitem)) {
				$query_bundlelist_item = mysql_query("SELECT * FROM rsdb_groups WHERE grpentr_id = '". $result_bundlelist_groupitem["bundle_groupid"] ."' ORDER BY `grpentr_name` ASC ;") ;
				$result_bundlelist_item = mysql_fetch_array($query_bundlelist_item);
				echo "  <tr bgcolor='";
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
				echo "'>";
				echo "	<td width='30%'>&nbsp;<a href='". $RSDB_intern_link_group.$result_bundlelist_item['grpentr_id'] ."'>". $result_bundlelist_item["grpentr_name"] ."</td>";
				echo "	<td width='70%'>&nbsp;". $result_bundlelist_item["grpentr_description"] ."</td>";
				echo "  </tr>";
			}
			echo "</table>";
		}
?>
    <p>&nbsp;</p>
<?php
	}
?>



<?php
	} // end if {$result_page['grpentr_type'] == "default"}
?>