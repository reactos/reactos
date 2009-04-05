<?php
    /*
    CompatDB - ReactOS Compatability Database
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


/**
 * class Star
 * 
 */
class Star
{



  /**
   * @FILLME
   *
   * @access public
   */
  private static function draw($number, $user, $mask, $votetext, $votelink, $iconsize) { // intern function, don't call this function directly
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
} // end of member function icon



  /**
   * @FILLME
   *
   * @access public
   */
  public static function drawNormal($number, $user, $mask, $votetext) { // draw the stars; "vote"/"test" text is optional
	return self::draw($number, $user, $mask, $votetext, "", "default");
  } // end of member function icon



  /**
   * @FILLME
   *
   * @access public
   */
  public static function drawSmall($number, $user, $mask, $votetext) { // draw the stars; "vote"/"test" text is optional
	return self::draw($number, $user, $mask, $votetext, "", "small");
  } // end of member function icon



  /**
   * @FILLME
   *
   * @access public
   */
  public static function drawVoteable($number, $user, $mask, $votetext, $votelink) { // for voting only
	return self::draw($number, $user, $mask, $votetext, $votelink, "default");
  
  } // end of member function icon



  /**
   * @FILLME
   *
   * @access public
   */
  public static function addVote($entry_id, $star_nr, $tblname, $fieldname) { // for voting only
	global $RSDB_intern_user_id;

	if ($entry_id != "" && $star_nr != "" && ($star_nr >= 1 && $star_nr <= 5)) {
    $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM ".$tblname." WHERE ".$fieldname."_id = :entry_id");
    $stmt->bindParam('entry_id',$entry_id,PDO::PARAM_STR);
    $stmt->execute();
		$result_star_vote1=$stmt->fetchOnce(PDO::FETCH_ASSOC);

		$RSDB_TEMP_voting_history1 = strchr($result_star_vote1[$fieldname.'_useful_vote_user_history'],("|".$RSDB_intern_user_id."="));
		
		if ($RSDB_TEMP_voting_history1 == false) {
			$RSDB_TEMP_voting_history2 = $result_star_vote1[$fieldname.'_useful_vote_user_history']."|". $RSDB_intern_user_id ."=". $star_nr;
      $stmt=CDBConnection::getInstance()->prepare("UPDATE ".$tblname." SET ".$fieldname."_useful_vote_value = ".$fieldname."_useful_vote_value+:star_nr, ".$fieldname."_useful_vote_user = ".$fieldname."_useful_vote_user+1, ".$fieldname."_useful_vote_user_history = :history WHERE ".$fieldname."_id = :entry_id");
      $stmt->bindParam('star_nr',$star_nr,PDO::PARAM_INT);
      $stmt->bindParam('history',$RSDB_TEMP_voting_history2,PDO::PARAM_STR);
      $stmt->bindParam('entry_id',$entry_id,PDO::PARAM_STR);
      $stmt->execute();

			Message::show("<b>Your rating/vote has been casted!</b>");
			echo "<br />";
			
			// Stats update:
      $stmt=CDBConnection::getInstance()->prepare("UPDATE rsdb_stats SET stat_s_icvotes = (stat_s_icvotes + 1), stat_s_votes = (stat_s_votes + 1) WHERE stat_date = :date");
      $stmt->bindValue('date',date('Y-m-d'),PDO::PARAM_STR);
      $stmt->execute();
		}
		else {
			Message::show("<b>You have already rated/voted this entry!</b>");
			echo "<br />";
		}
	}
	else {
		Message::show("<b>Invalid rating/vote!</b>");
		echo "<br />";
	}
  
  } // end of member function icon



  /**
   * @FILLME
   *
   * @access public
   */
  public static function thresholdForum($RSDB_TEMP_msgid, $RSDB_TEMP_threshold, $RSDB_TEMP_threshold_new, $RSDB_TEMP_tablename, $RSDB_TEMP_fieldname) {
	global $RSDB_SET_item;
	global $RSDB_TEMP_order;
	global $RSDB_intern_link_item_item2_forum_msg;
	global $RSDB_SET_fstyle;
	global $RSDB_intern_user_id;
	global $RSDB_intern_link_item_item2_vote;
	global $RSDB_TEMP_counter_threshold;
	
	if ($RSDB_TEMP_threshold_new == true) {
		$RSDB_TEMP_counter_threshold = 0;
	}

  $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp_".$RSDB_TEMP_tablename." WHERE ".$RSDB_TEMP_fieldname."_visible = '1' AND ".$RSDB_TEMP_fieldname."_comp_id = :item_id AND ".$RSDB_TEMP_fieldname."_parent = :parent ORDER BY ".$RSDB_TEMP_fieldname."_date ".$RSDB_TEMP_order."");
  $stmt->bindParam('item_id',$RSDB_SET_item,PDO::PARAM_STR);
  $stmt->bindParam('parent',$RSDB_TEMP_msgid,PDO::PARAM_STR);
  $stmt->execute();

	while($result_fmsgreports = $stmt->fetch(PDO::FETCH_ASSOC)) {
		
		$number = $result_fmsgreports[$RSDB_TEMP_fieldname.'_useful_vote_value'];
		$user = $result_fmsgreports[$RSDB_TEMP_fieldname.'_useful_vote_user'];
		$result_stars = @($number / $user);
		$result_round_stars = round($result_stars, 0);

		if ($result_round_stars >= $RSDB_TEMP_threshold) {
			$RSDB_TEMP_counter_threshold++;
		}
		calc_threshold_stars_forum($result_fmsgreports[$RSDB_TEMP_fieldname.'_id'], $RSDB_TEMP_threshold, "", $RSDB_TEMP_tablename, $RSDB_TEMP_fieldname);
	}
	return $RSDB_TEMP_counter_threshold;
  
  } // end of member function icon



  /**
   * @FILLME
   *
   * @access public
   */
  public static function thresholdTests($RSDB_TEMP_msgid, $RSDB_TEMP_threshold, $RSDB_TEMP_threshold_new, $RSDB_TEMP_tablename, $RSDB_TEMP_fieldname) {
	global $RSDB_SET_item;
	global $RSDB_TEMP_order;
	global $RSDB_intern_link_item_item2_forum_msg;
	global $RSDB_SET_fstyle;
	global $RSDB_intern_user_id;
	global $RSDB_intern_link_item_item2_vote;
	global $RSDB_TEMP_counter_threshold;
	
	if ($RSDB_TEMP_threshold_new == true) {
		$RSDB_TEMP_counter_threshold = 0;
	}

  $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp_".$RSDB_TEMP_tablename." WHERE ".$RSDB_TEMP_fieldname."_visible = '1' AND ".$RSDB_TEMP_fieldname."_comp_id = :item_id ORDER BY ".$RSDB_TEMP_fieldname."_date " . $RSDB_TEMP_order . "");
  $stmt->bindParam('item_id',$RSDB_SET_item,PDO::PARAM_STR);
  $stmt->execute();

	while($result_fmsgreports = $stmt->fetch(PDO::FETCH_ASSOC)) {
		
		$number = $result_fmsgreports[$RSDB_TEMP_fieldname.'_useful_vote_value'];
		$user = $result_fmsgreports[$RSDB_TEMP_fieldname.'_useful_vote_user'];
		$result_stars = @($number / $user);
		$result_round_stars = round($result_stars, 0);

		if ($result_round_stars >= $RSDB_TEMP_threshold) {
			$RSDB_TEMP_counter_threshold++;
		}
		
	}
	return $RSDB_TEMP_counter_threshold;
  
  } // end of member function icon



} // end of Award
?>
