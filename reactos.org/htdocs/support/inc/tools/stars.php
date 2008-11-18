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



function draw_stars_intern($number, $user, $mask, $votetext, $votelink, $iconsize) { // intern function, don't call this function directly
	$draw_stars_result = '';
	
	
	/*
	if ($user < 7 && $user != 1) {
		$result_stars = 0;
	}
	else {
		$result_stars = @($number / $user);
	}
	*/
	
	$result_stars = @($number / $user);
		
	$result_round_stars = round($result_stars, 0);
	
	if ($iconsize == "default") {
		$iconsize_ext = "";
		$iconsize_height = "15";
		$iconsize_width = "15";
	}
	elseif ($iconsize == "small") {
		$iconsize_ext = "_small";
		$iconsize_height = "13";
		$iconsize_width = "13";
	}
	
	if ($result_round_stars <= $mask) {
		
		for ($goldstar=0; $goldstar < $result_round_stars; $goldstar++) {
			if ($votelink != "") {
				$draw_stars_result .= '<a href="'.$votelink.($goldstar+1).'">';
			}
			$draw_stars_result .= '<img src="media/icons/stars/star_gold'.$iconsize_ext.'.gif" width="'.$iconsize_width.'" height="'.$iconsize_height.'" ';
			if ($votelink != "") {
				if (($goldstar+1) == "1") {
					$draw_stars_result .= 'alt="Click here to vote '.($goldstar+1).' star!" ';
				}
				else {
					$draw_stars_result .= 'alt="Click here to vote '.($goldstar+1).' stars!" ';
				}
			}
			else {
				$draw_stars_result .= 'alt="*" ';
			}
			$draw_stars_result .= 'border="0" />';
			if ($votelink != "") {
				$draw_stars_result .= '</a>';
			}
		}
		
		for ($graystar=0; $graystar < ($mask-$result_round_stars); $graystar++) {
			if ($votelink != "") {
				$draw_stars_result .= '<a href="'.$votelink.($result_round_stars+$graystar+1).'">';
			}
			$draw_stars_result .= '<img src="media/icons/stars/star_gray'.$iconsize_ext.'.gif" width="'.$iconsize_width.'" height="'.$iconsize_height.'" ';
			if ($votelink != "") {
				if (($result_round_stars+$graystar+1) == "1") {
					$draw_stars_result .= 'alt="Click here to vote '.($result_round_stars+$graystar+1).' star!" ';
				}
				else {
					$draw_stars_result .= 'alt="Click here to vote '.($result_round_stars+$graystar+1).' stars!" ';
				}
			}
			else {
				$draw_stars_result .= 'alt="_" ';
			}
			$draw_stars_result .= 'border="0" />';
			if ($votelink != "") {
				$draw_stars_result .= '</a>';
			}
		}
		
		$draw_stars_result .= ' '.round($result_stars, 2).'/'.$mask;
		
		if ($votetext) {
			$draw_stars_result .= ' ('.$user.' '.$votetext.')';
		}
		/*if ($user < 7 && $user != "") {
			$draw_stars_result .= ' (rating is hidden)';
		}*/
	}
	else {
		$draw_stars_result .= $votetext.' data corrupted :-(';
	}
	
	return $draw_stars_result;
}



function draw_stars($number, $user, $mask, $votetext) { // draw the stars; "vote"/"test" text is optional
	return draw_stars_intern($number, $user, $mask, $votetext, "", "default");
}

function draw_stars_small($number, $user, $mask, $votetext) { // draw the stars; "vote"/"test" text is optional
	return draw_stars_intern($number, $user, $mask, $votetext, "", "small");
}

function draw_stars_vote($number, $user, $mask, $votetext, $votelink) { // for voting only
	return draw_stars_intern($number, $user, $mask, $votetext, $votelink, "default");
}




function db_update_stars_vote($entry_id, $star_nr, $tblname, $fieldname) { // for voting only
	global $RSDB_intern_user_id;

	if ($entry_id != "" && $star_nr != "" && ($star_nr >= 1 && $star_nr <= 5)) {
		$query_star_vote1=mysql_query("SELECT * 
							FROM `". mysql_escape_string($tblname) ."` 
							WHERE `".mysql_escape_string($fieldname)."_id` =". mysql_escape_string($entry_id) ." ;");
		$result_star_vote1=mysql_fetch_array($query_star_vote1);

		$RSDB_TEMP_voting_history1 = strchr($result_star_vote1[$fieldname.'_useful_vote_user_history'],("|".$RSDB_intern_user_id."="));
		
		if ($RSDB_TEMP_voting_history1 == false) {
			$RSDB_TEMP_voting_history2 = $result_star_vote1[$fieldname.'_useful_vote_user_history']."|". mysql_escape_string($RSDB_intern_user_id) ."=". mysql_escape_string($star_nr);
			$query_star_vote2="UPDATE `". mysql_escape_string($tblname) ."` 
							SET `".mysql_escape_string($fieldname)."_useful_vote_value` = ".mysql_escape_string($fieldname)."_useful_vote_value+". mysql_escape_string($star_nr) .", 
							`".mysql_escape_string($fieldname)."_useful_vote_user` = ".mysql_escape_string($fieldname)."_useful_vote_user+1,   
							`".mysql_escape_string($fieldname)."_useful_vote_user_history` = '". mysql_escape_string($RSDB_TEMP_voting_history2) ."' 
							WHERE `".mysql_escape_string($fieldname)."_id` =". mysql_escape_string($entry_id) ." LIMIT 1 ;";
							//echo $query_star_vote2;
			$result_star_vote2=mysql_query($query_star_vote2);
			msg_bar("<b>Your rating/vote has been casted!</b>");
			echo "<br />";
			
			// Stats update:
			$update_stats_entry = "UPDATE `rsdb_stats` SET
									`stat_s_icvotes` = (stat_s_icvotes + 1) 
									`stat_s_votes` = (stat_s_votes + 1) 
									WHERE `stat_date` = '". date("Y-m-d") ."' LIMIT 1 ;";
			mysql_query($update_stats_entry);
		}
		else {
			msg_bar("<b>You have already rated/voted this entry!</b>");
			echo "<br />";
		}
	}
	else {
		msg_bar("<b>Invalid rating/vote!</b>");
		echo "<br />";
	}
}


function calc_threshold_stars_forum($RSDB_TEMP_msgid, $RSDB_TEMP_threshold, $RSDB_TEMP_threshold_new, $RSDB_TEMP_tablename, $RSDB_TEMP_fieldname) {
	global $RSDB_SET_item;
	global $RSDB_TEMP_order;
	global $RSDB_intern_link_item_item2_forum_msg;
	global $RSDB_SET_fstyle;
	global $RSDB_intern_user_id;
	global $RSDB_intern_link_item_item2_vote;
	global $RSDB_TEMP_counter_threshold;
	global $RSDB_intern_code_view_shortname;
	
	if ($RSDB_TEMP_threshold_new == true) {
		$RSDB_TEMP_counter_threshold = 0;
	}

	$query_fmsgreports = mysql_query("SELECT * 
					FROM `rsdb_item_comp_".mysql_escape_string($RSDB_TEMP_tablename)."` 
					WHERE `".mysql_escape_string($RSDB_TEMP_fieldname)."_visible` = '1'
					AND `".mysql_escape_string($RSDB_TEMP_fieldname)."_".mysql_escape_string($RSDB_intern_code_view_shortname)."_id` = " . mysql_escape_string($RSDB_SET_item) . "
					AND `".mysql_escape_string($RSDB_TEMP_fieldname)."_parent` = " . mysql_escape_string($RSDB_TEMP_msgid) . "
					ORDER BY `".mysql_escape_string($RSDB_TEMP_fieldname)."_date` " . mysql_escape_string($RSDB_TEMP_order) . " ;") ;

	while($result_fmsgreports = mysql_fetch_array($query_fmsgreports)) {
		
		$number = $result_fmsgreports[$RSDB_TEMP_fieldname.'_useful_vote_value'];
		$user = $result_fmsgreports[$RSDB_TEMP_fieldname.'_useful_vote_user'];
		$result_stars = @($number / $user);
		$result_round_stars = round($result_stars, 0);

//		echo "<p>".draw_stars($number, $user, 5, "").$result_fmsgreports['fmsg_subject']."</p>";

		if ($result_round_stars >= $RSDB_TEMP_threshold) {
			$RSDB_TEMP_counter_threshold++;
		}
		
//		echo "<br />";
		calc_threshold_stars_forum($result_fmsgreports[$RSDB_TEMP_fieldname.'_id'], $RSDB_TEMP_threshold, "", $RSDB_TEMP_tablename, $RSDB_TEMP_fieldname);
	}
	return $RSDB_TEMP_counter_threshold;
}

function calc_threshold_stars_test($RSDB_TEMP_msgid, $RSDB_TEMP_threshold, $RSDB_TEMP_threshold_new, $RSDB_TEMP_tablename, $RSDB_TEMP_fieldname) {
	global $RSDB_SET_item;
	global $RSDB_TEMP_order;
	global $RSDB_intern_link_item_item2_forum_msg;
	global $RSDB_SET_fstyle;
	global $RSDB_intern_user_id;
	global $RSDB_intern_link_item_item2_vote;
	global $RSDB_TEMP_counter_threshold;
	global $RSDB_intern_code_view_shortname;
	
	if ($RSDB_TEMP_threshold_new == true) {
		$RSDB_TEMP_counter_threshold = 0;
	}

	$query_fmsgreports = mysql_query("SELECT * 
					FROM `rsdb_item_comp_".mysql_escape_string($RSDB_TEMP_tablename)."` 
					WHERE `".mysql_escape_string($RSDB_TEMP_fieldname)."_visible` = '1'
					AND `".mysql_escape_string($RSDB_TEMP_fieldname)."_".mysql_escape_string($RSDB_intern_code_view_shortname)."_id` = " . mysql_escape_string($RSDB_SET_item) . "
					ORDER BY `".mysql_escape_string($RSDB_TEMP_fieldname)."_date` " . mysql_escape_string($RSDB_TEMP_order) . " ;") ;

	while($result_fmsgreports = mysql_fetch_array($query_fmsgreports)) {
		
		$number = $result_fmsgreports[$RSDB_TEMP_fieldname.'_useful_vote_value'];
		$user = $result_fmsgreports[$RSDB_TEMP_fieldname.'_useful_vote_user'];
		$result_stars = @($number / $user);
		$result_round_stars = round($result_stars, 0);

//		echo "<p>".draw_stars($number, $user, 5, "").$result_fmsgreports['fmsg_subject']."</p>";

		if ($result_round_stars >= $RSDB_TEMP_threshold) {
			$RSDB_TEMP_counter_threshold++;
		}
		
	}
	return $RSDB_TEMP_counter_threshold;
}

?>