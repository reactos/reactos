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
								FROM `rsdb_item_comp` 
								WHERE `comp_visible` = '1'
								AND `comp_id` = " . $RSDB_SET_item . "
								ORDER BY `comp_name` ASC") ;
	
	$result_page = mysql_fetch_array($query_page);		
	
if ($result_page['comp_id']) {
	echo "<h2>".$result_page['comp_name'] ." [". "ReactOS ".@show_osversion($result_page['comp_osversion']) ."]</h2>"; 
	
	include('inc/tree/tree_item_menubar.php');
	
	
	
?>
<p align="center">The following pictures are owned by whoever posted them. We are not responsible for them in any way. </p>
<p align="center"><strong><a href="<?php echo $RSDB_intern_link_submit_comp_screenshot; ?>add">Submit Screenshot</a></strong> (image file)</p>
<?php

// Voting - update DB
if ($RSDB_SET_vote != "" && $RSDB_SET_vote2 != "") {
	db_update_stars_vote($RSDB_SET_vote, $RSDB_SET_vote2, "rsdb_object_media", "media");
}
	
if ($RSDB_SET_entry == "" || $RSDB_SET_entry == 0) {

?>

<table width="100%"  border="0" cellpadding="3" cellspacing="1">
<?php
	$roscms_TEMP_counter = 0;
	
	$query_screenshots = mysql_query("SELECT * 
										FROM `rsdb_object_media` 
										WHERE `media_groupid` = ". mysql_escape_string($result_page['comp_media']) ."  
										AND (( media_useful_vote_value / media_useful_vote_user) > 2 OR  media_useful_vote_user < 5)
										ORDER BY `media_order` ASC") ;
	while($result_screenshots= mysql_fetch_array($query_screenshots)) {
		$roscms_TEMP_counter++;
		if ($roscms_TEMP_counter == 1) {
			echo "<tr>";
		}
		echo '<td width="33%" valign="top">';

		echo '<p align="center"><br /><a href="'.$RSDB_intern_link_item_item2.'screens&amp;entry='.$result_screenshots["media_id"].'"><img src="media/files/'.$result_screenshots["media_filetype"].'/'.urlencode($result_screenshots["media_thumbnail"]).'" width="250" height="188" border="0" alt="';
		echo 'Description: '.htmlentities($result_screenshots["media_description"])."\nUser: ".usrfunc_GetUsername($result_screenshots["media_user_id"])."\nDate: ".$result_screenshots["media_date"]."\n\n".htmlentities($result_screenshots["media_exif"]);
		echo '"></a><br /><i>'.htmlentities($result_screenshots["media_description"]).'</i><br />';
		echo '<br /><font size="1">';
			  
			  	$RSDB_TEMP_voting_history = strchr($result_screenshots['media_useful_vote_user_history'],("|".$RSDB_intern_user_id."="));
				if ($RSDB_TEMP_voting_history == false) {
					echo "Rate this screenshot: ";
					if ($result_screenshots['media_useful_vote_user'] > $RSDB_setting_stars_threshold) {
						echo draw_stars_vote($result_screenshots['media_useful_vote_value'], $result_screenshots['media_useful_vote_user'], 5, "", ($RSDB_intern_link_item_item2_vote.$result_screenshots['media_id']."&amp;vote2="));
					}
					else {
						echo draw_stars_vote(0, 0, 5, "", ($RSDB_intern_link_item_item2_vote.$result_screenshots['media_id']."&amp;vote2="));
					}
				}
				else {
					echo "Rating: ";
					echo draw_stars($result_screenshots['media_useful_vote_value'], $result_screenshots['media_useful_vote_user'], 5, "");
				}
				
		echo '</font><br /><br /></p>';

		echo "</td>";
		if ($roscms_TEMP_counter == 3) {
			echo "</tr>";
			$roscms_TEMP_counter = 0;
		}
	}
	
	if ($roscms_TEMP_counter == 1) {
		echo '<td width="33%" valign="top">&nbsp;</td>';
		echo '<td width="33%" valign="top">&nbsp;</td></tr>';
	}
	if ($roscms_TEMP_counter == 2) {
		echo '<td width="33%" valign="top">&nbsp;</td></tr>';
	}

echo "</table>";

}
else {
	// Show one picture in max resolution:
	$query_screenshots = mysql_query("SELECT * 
										FROM `rsdb_object_media` 
										WHERE `media_id` = ". mysql_escape_string($RSDB_SET_entry) ."  
										LIMIT 1 ;") ;
	$result_screenshots= mysql_fetch_array($query_screenshots);
	echo '<p align="center"><b><a href="'.$RSDB_intern_link_item_item2.'screens">Show all screenshots</a></b></p>';
	echo '<h5>'.htmlentities($result_screenshots["media_description"]).'&nbsp;</h5>';
	echo '<p><img src="media/files/'.$result_screenshots["media_filetype"].'/'.urlencode($result_screenshots["media_file"]).'" border="0" alt="';
	echo ''.$result_screenshots["media_description"].'"></a></p>';
	echo '<p><b>Description:</b> '.htmlentities($result_screenshots["media_description"]).'<br />';
	echo '<b>User:</b> '.usrfunc_GetUsername($result_screenshots["media_user_id"]).'<br />';
	echo '<b>Date:</b> '.$result_screenshots["media_date"].'</p>';
	echo '<p><b>EXIF-Data:</b><br />'.nl2br($result_screenshots["media_exif"]).'</p>';
	echo '<p align="center"><b><a href="'.$RSDB_intern_link_item_item2.'screens">Show all screenshots</a></b></p>';
}
}
?>
