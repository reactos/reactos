<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>

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

	// To prevent hacking activity:
	if ( !defined('ROSCMS_SYSTEM') )
	{
		die("Hacking attempt");
	}

	global $RosCMS_GET_debug;

	global $roscms_intern_webserver_roscms;
	global $roscms_intern_script_name;
	global $roscms_security_level;
	
	global $roscms_intern_account_id;
	global $roscms_intern_login_check_username;
	global $roscms_standard_language;

	global $h_a;
	global $h_a2;
	
	global $RosCMS_GET_d_use;
	global $RosCMS_GET_d_flag;
	global $RosCMS_GET_d_id;
	global $RosCMS_GET_d_r_id;
	
	global $RosCMS_GET_d_name;
	global $RosCMS_GET_d_type;
	global $RosCMS_GET_d_r_lang;
	global $RosCMS_GET_d_template;
	
	global $RosCMS_GET_d_value;
	global $RosCMS_GET_d_value2;
	global $RosCMS_GET_d_value3;
	global $RosCMS_GET_d_value4;
	global $RosCMS_GET_d_value5;
	global $RosCMS_GET_d_value6;
	global $RosCMS_GET_d_value7;
	
	global $RosCMS_GET_d_arch;
	
	
	if ($RosCMS_GET_d_use == "mef") {
		require("inc/data_edit_tag.php");
		// Prevent caching:
		header("Content-type: text/html");
		header("Expires: Sun, 28 Jul 1996 05:00:00 GMT");    // Date in the past
		header("Last-Modified: " . gmdate("D, d M Y H:i:s") . " GMT"); 
															 // always modified
		header("Cache-Control: no-store, no-cache, must-revalidate");  // HTTP/1.1
		header("Cache-Control: post-check=0, pre-check=0", false);
		header("Pragma: no-cache");                          // HTTP/1.0

		//echo "<p>".$RosCMS_GET_d_flag.", ".$RosCMS_GET_d_id.", ".$RosCMS_GET_d_r_id."</p>";
	
	
		switch ($RosCMS_GET_d_flag) {
			default:
				//echo "<p>asasas: ".$RosCMS_GET_d_r_id." | ".strpos($RosCMS_GET_d_r_id, "tr")."</p>";
				
				if (strpos($RosCMS_GET_d_r_id, "tr") === false) {
					// normal (contains NO "tr")
					show_edit_data();
					show_edit_data_tag();
					show_edit_data2();
					show_edit_data_form(true, true);
				}
				else {
					// translation mode (contains "tr")					
					$temp_rev_id = substr($RosCMS_GET_d_r_id, 2);
					
				
					if ($RosCMS_GET_debug) echo "<p>rev-id: ".$temp_rev_id." | ".is_numeric($temp_rev_id)."</p>";

					$query_get_rev = mysql_query("SELECT data_id, rev_language    
															FROM data_revision 
															WHERE rev_id = ".mysql_real_escape_string($temp_rev_id)."
															LIMIT 1;");
					$result_get_rev = mysql_fetch_array($query_get_rev);
	
					if ($RosCMS_GET_debug) echo "<p>data-id: ".$result_get_rev['data_id']."</p>";
					
					if ($RosCMS_GET_debug) echo "<p>cur_revid: ".$temp_rev_id."; cur_dataid: ".$result_get_rev['data_id']." [".$result_get_rev['rev_language']."]</p>";
					
					if (roscms_security_check_kind ($result_get_rev['data_id'], "trans")) {
						//echo "<p>security check passed</p>";
					
						if (move_to_archive($result_get_rev['data_id'], $temp_rev_id, 1 /* copy mode */)) {
							//echo "<p>copy process passed</p>";
							$query_get_new_rev = mysql_query("SELECT *    
																FROM data_revision 
																WHERE data_id = ".mysql_real_escape_string($result_get_rev['data_id'])."
																AND rev_usrid = ".mysql_real_escape_string($roscms_intern_account_id)."
																AND rev_version = 0
																AND rev_language = '".mysql_real_escape_string($RosCMS_GET_d_r_lang)."' 
																AND rev_date = '".mysql_real_escape_string(date("Y-m-d"))."' 
																ORDER BY rev_id DESC
																LIMIT 1;");
							$result_get_new_rev = mysql_fetch_array($query_get_new_rev);
							
							$RosCMS_GET_d_id = $result_get_new_rev['data_id'];
							$RosCMS_GET_d_r_id = $result_get_new_rev['rev_id'];
							$RosCMS_GET_d_r_lang = $result_get_new_rev['rev_language'];
							
							if ($RosCMS_GET_debug) echo "<p>NEW revid: ".$RosCMS_GET_d_r_id."; dataid: ".$RosCMS_GET_d_id." [".$RosCMS_GET_d_r_lang."]</p>"; 
	
							show_edit_data();
							show_edit_data_tag();
							show_edit_data2();
							show_edit_data_form(true, true);
						}
						else {
							die("Translation not successful, due entry-copy problem. If this happens more than once or twice please contact the website admin.");
						}
					}
					else {
						echo "You have not enough rights to translate this entry.";
					}
				}
				break;
			case "newentry":  // create entry - interface
				switch ($RosCMS_GET_d_value) {
					default:
					case 'single':
						newentryselect(0);
						break;
					case 'dynamic':
						newentryselect(1);
						break;
					case 'template':
						newentryselect(2);
						break;
				}
				break;
			case "newentry2": // single entry - submit
				//echo "<p>asas</p>";
				echo newentryadd2(true, "draft", "", false);
				break;
			case "newentry3": // page & content - submit
				$temp_d_type = $RosCMS_GET_d_type;
				$temp_d_r_lang = $RosCMS_GET_d_r_lang;
				
				$RosCMS_GET_d_type = "page";
				$RosCMS_GET_d_r_lang = $roscms_standard_language;
				echo newentryadd2(false, "stable", $RosCMS_GET_d_template, false);
				
				echo "<hr />dazwischen: ".$temp_d_type."###".$temp_d_r_lang."#";
				
				$RosCMS_GET_d_type = $temp_d_type;
				$RosCMS_GET_d_r_lang = $temp_d_r_lang;	
				echo newentryadd2(true);
				break;
			case "newentry4": // dynamic entry - submit
				echo newentryadd2(true, "draft", "", true);
				break;
			case "alterfields":
				show_edit_data_tag(1);
				break;
			case "alterfields2":
				alterentryfields();
				show_edit_data();
				show_edit_data_tag();
				show_edit_data2();
				show_edit_data_form(true, true);
				break;
			case "alterentry":
				alterentry();
				show_edit_data();
				show_edit_data_tag();
				show_edit_data2();
				show_edit_data_form(true, true);
				break;
			case "showentry":
				show_edit_data_tag(4);
				break;
			case "altersecurity":
				altersecurityfields();
				show_edit_data();
				show_edit_data_tag();
				show_edit_data2();
				show_edit_data_form(true, true);
				break;
			case "showsecurity":
				show_edit_data_tag(3);
				break;
			case "showhistory":
				show_edit_data_tag(2);
				break;
			case "showtag":
				show_edit_data_tag();
				break;
			case "addtag":
				tag_add($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_GET_d_value, $RosCMS_GET_d_value2, $RosCMS_GET_d_value3);
				show_edit_data_tag();
				break;
			case "deltag":
				if ($roscms_security_level > 1 || $RosCMS_GET_d_value2 == $roscms_intern_account_id) {
					tag_delete($RosCMS_GET_d_value, $RosCMS_GET_d_value2);
				}
				show_edit_data_tag();
				break;
			case "changetag":
				tag_delete($RosCMS_GET_d_value4, $RosCMS_GET_d_value3);
				tag_add($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_GET_d_value /* name */, $RosCMS_GET_d_value2 /* value */, $RosCMS_GET_d_value3 /* usrid */);
				echo getTagId($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $roscms_intern_account_id /* usrid */, $RosCMS_GET_d_value /* name */);
				break;
			case "changetag2":
				//echo "<p>getTagId:".getTagId($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_GET_d_value3, $RosCMS_GET_d_value)."</p>";
				tag_delete(getTagId($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_GET_d_value3 /* usrid */, $RosCMS_GET_d_value /* name */), $RosCMS_GET_d_value3 /* usrid */);
				tag_add($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_GET_d_value /* name */, $RosCMS_GET_d_value2 /* value */, $RosCMS_GET_d_value3 /* usrid */);
				echo getTagId($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_GET_d_value3 /* usrid */, $RosCMS_GET_d_value /* name */);
				break;
			case "changetag3":
				$query_edit_mef_tag_max_tn = mysql_query("SELECT tn_id 
															FROM data_tag_name 
															WHERE tn_name = '".mysql_real_escape_string($RosCMS_GET_d_value)."'
															LIMIT 1;");
				$result_edit_mef_tag_max_tn = mysql_fetch_array($query_edit_mef_tag_max_tn);
				
				$query_edit_mef_tag_max = mysql_query("SELECT tag_id  
														FROM data_tag 
														WHERE data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."'
														AND data_rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."'
														AND tag_name_id = '".mysql_real_escape_string($result_edit_mef_tag_max_tn['tn_id'])."' ;");
				while($result_edit_mef_tag_max = mysql_fetch_array($query_edit_mef_tag_max)) {
					tag_delete(getTagId($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_GET_d_value3 /* usrid */, $RosCMS_GET_d_value /* name */), $RosCMS_GET_d_value3 /* usrid */);
				}
				tag_add($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_GET_d_value /* name */, $RosCMS_GET_d_value2 /* value */, $RosCMS_GET_d_value3 /* usrid */);
				echo getTagId($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_GET_d_value3 /* usrid */, $RosCMS_GET_d_value /* name */);
				break;
			case "diff": // compare two entries
				echo diff_entries($RosCMS_GET_d_value /* rev_id entry 1 */, $RosCMS_GET_d_value2 /* rev_id entry 2 */);
				break;
			case "diff2": // compare two entries; updates diff area
				echo diff_entries($RosCMS_GET_d_value /* rev_id entry 1 */, $RosCMS_GET_d_value2 /* rev_id entry 2 */);
				break;
			case "changetags":
				changetags($RosCMS_GET_d_value /* entry counter */, $RosCMS_GET_d_value2 /* entry rev_id's */, $RosCMS_GET_d_value3 /* entry flag */);
				break;
		}
	}
	
	
	function alterentry() {
		global $RosCMS_GET_d_id;
		global $RosCMS_GET_d_r_id;
		global $RosCMS_GET_d_arch;
		global $RosCMS_GET_d_value;
		global $RosCMS_GET_d_value2;
		global $RosCMS_GET_d_value3;
		global $RosCMS_GET_d_value4;
		global $RosCMS_GET_d_value5;
		global $RosCMS_GET_d_value6;
		global $RosCMS_GET_d_value7;
		global $h_a;
		global $h_a2;
	
		//echo "<p>alterentry()</p>";

		$query_sec_data = mysql_query("SELECT *  
										FROM data_revision".$h_a."
										WHERE rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."'
										AND data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."'
										LIMIT 1;");
		$result_sec_data = mysql_fetch_array($query_sec_data);

		// language
		if ($RosCMS_GET_d_value != "" && $RosCMS_GET_d_value != $result_sec_data['rev_language']) {
			// check if the choosen language do exist
			$query_sec_data_lang = mysql_query("SELECT COUNT(*)  
												FROM languages
												WHERE lang_id = '".mysql_real_escape_string($RosCMS_GET_d_value)."';");
			$result_sec_data_lang = mysql_fetch_row($query_sec_data_lang);
			
			if ($result_sec_data_lang[0] > 0) {
				$update_data_lang = mysql_query("UPDATE data_revision".$h_a." 
													SET rev_language = '".mysql_real_escape_string($RosCMS_GET_d_value)."' 
													WHERE rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."' 
													AND data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."' 
													LIMIT 1;");
				log_event_medium("entry language changed ".$result_sec_data['rev_language']." =&gt; ".$RosCMS_GET_d_value.log_prep_info($RosCMS_GET_d_id, $RosCMS_GET_d_r_id)."{alterentry}");
			}
		}
		
		// version-number
		if ($RosCMS_GET_d_value2 != "" && $RosCMS_GET_d_value2 != $result_sec_data['rev_version']) {
			// check for existing revisons with same number
			$query_sec_rev_number = mysql_query("SELECT COUNT(*)  
													FROM data_revision".$h_a."
													WHERE rev_version = '".mysql_real_escape_string($RosCMS_GET_d_value2)."'
													AND data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."' 
													AND rev_language = '".mysql_real_escape_string($result_sec_data['rev_language'])."';");
			$result_sec_rev_number = mysql_fetch_row($query_sec_rev_number);

			if ($result_sec_rev_number[0] <= 0) {
				//echo "<p>udapte version-number: ".$result_sec_data['rev_version']." =&gt; ".$RosCMS_GET_d_value2."</p>";
				$update_data_lang = mysql_query("UPDATE data_revision".$h_a." 
													SET rev_version = '".mysql_real_escape_string($RosCMS_GET_d_value2)."' 
													WHERE rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."' 
													AND data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."' 
													LIMIT 1;");
				log_event_medium("entry version-number changed: ".$result_sec_data['rev_version']." =&gt; ".$RosCMS_GET_d_value2.log_prep_info($RosCMS_GET_d_id, $RosCMS_GET_d_r_id)."{alterentry}");
			}
		}

		// user-name
		if ($RosCMS_GET_d_value3 != "") {
			// check for existing user-name
			$query_sec_username = mysql_query("SELECT user_id, user_name  
													FROM users 
													WHERE user_name = '".mysql_real_escape_string($RosCMS_GET_d_value3)."'
													LIMIT 1;");
			$result_sec_username = mysql_fetch_array($query_sec_username);

			if ($result_sec_username['user_id'] != "" && $result_sec_username['user_id'] > 1 && $result_sec_username['user_name'] != $result_sec_data['rev_usrid']) {
				$update_data_username = mysql_query("UPDATE data_revision".$h_a." 
														SET rev_usrid = '".mysql_real_escape_string($result_sec_username['user_id'])."' 
														WHERE rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."' 
														AND data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."' 
														LIMIT 1;");
				log_event_medium("entry user-name changed: ".$result_sec_data['rev_usrid']." =&gt; ".$result_sec_username['user_id']." (".$RosCMS_GET_d_value3.")".log_prep_info($RosCMS_GET_d_id, $RosCMS_GET_d_r_id)."{alterentry}");
			}
		}

		// date + time
		if ($RosCMS_GET_d_value4 != "" && strlen($RosCMS_GET_d_value4) == 10 && $RosCMS_GET_d_value5 != "" && strlen($RosCMS_GET_d_value5) == 8) {

			if ($result_sec_data['rev_date'] != $RosCMS_GET_d_value4 || $result_sec_data['rev_time'] != $RosCMS_GET_d_value5) {
				$tmp_date = explode("-", $RosCMS_GET_d_value4);
				$tmp_time = explode(":", $RosCMS_GET_d_value5);

				//echo "<p>count: ".count($tmp_date).", ".count($tmp_time)."</p>";
				
				// date and time checking, probably better with regex
				if (strlen($tmp_date[0]) == 4 && $tmp_date[0] > 1985 && $tmp_date[0] < 2085 && 
					strlen($tmp_date[1]) == 2 && $tmp_date[1] > 0 && $tmp_date[1] < 13 && 
					strlen($tmp_date[2]) == 2 && $tmp_date[2] > 0 && $tmp_date[2] < 32 && 
					strlen($tmp_time[0]) == 2 && $tmp_time[0] >= 0 && $tmp_time[0] < 25 && 
					strlen($tmp_time[1]) == 2 && $tmp_time[1] >= 0 && $tmp_time[1] < 60 && 
					strlen($tmp_time[2]) == 2 && $tmp_time[2] >= 0 && $tmp_time[2] < 60 && 	
					count($tmp_date) == 3 && 
					count($tmp_time) == 3) {
					
					//echo "<p>okay: ".$RosCMS_GET_d_value4.", ".$RosCMS_GET_d_value5."</p>";
							
					$update_data_username = mysql_query("UPDATE data_revision".$h_a." 
															SET rev_datetime = '".mysql_real_escape_string($RosCMS_GET_d_value4." ".$RosCMS_GET_d_value5)."', 
															rev_date = '".mysql_real_escape_string($RosCMS_GET_d_value4)."', 
															rev_time = '".mysql_real_escape_string($RosCMS_GET_d_value5)."' 
															WHERE rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."' 
															AND data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."' 
															LIMIT 1;");
					log_event_medium("entry date+time changed: ".$result_sec_data['rev_date']." ".$result_sec_data['rev_time']." =&gt; ".$RosCMS_GET_d_value4." ".$RosCMS_GET_d_value5.log_prep_info($RosCMS_GET_d_id, $RosCMS_GET_d_r_id)."{alterentry}");
				}
			}
		}

		// move entry to another data-id 
		if ($RosCMS_GET_d_value6 != "" && $RosCMS_GET_d_value7 != "") {

			$query_sec_data_entry = mysql_query("SELECT data_id, data_name, data_type   
											FROM data_".$h_a2."
											WHERE data_name = '".mysql_real_escape_string($RosCMS_GET_d_value6)."'
											AND data_type = '".mysql_real_escape_string($RosCMS_GET_d_value7)."'
											LIMIT 1;");
			$result_sec_data_entry = mysql_fetch_array($query_sec_data_entry);
			
			if ($result_sec_data_entry['data_id'] != "" && $result_sec_data_entry['data_id'] > 0 && $result_sec_data_entry['data_name'] != $RosCMS_GET_d_value6 && $result_sec_data_entry['data_type'] != $RosCMS_GET_d_value7) {
				//echo "<p>move entry: ".$RosCMS_GET_d_value6.", ".$RosCMS_GET_d_value7." [".$result_sec_data_entry['data_id']."]</p>";
				
				$update_data_username = mysql_query("UPDATE data_revision".$h_a." 
														SET data_id = '".mysql_real_escape_string($result_sec_data_entry['data_id'])."'
														WHERE rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."' 
														AND data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."' 
														LIMIT 1;");

				$query_sec_data_number = mysql_query("SELECT COUNT(*)  
														FROM data_revision".$h_a."
														WHERE data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."';");
				$result_sec_data_number = mysql_fetch_row($query_sec_data_number);


				log_event_medium("entry moved to another data-id: ".$RosCMS_GET_d_id." =&gt; ".$result_sec_data_entry['data_id'].log_prep_info($RosCMS_GET_d_id, $RosCMS_GET_d_r_id)."{alterentry}");


				if ($result_sec_data_number[0] <= 0) {
					//echo "<p>delete data-id: ".$RosCMS_GET_d_id."</p>";
					$delete_data_id = mysql_query("DELETE FROM data_".$h_a2." 
													WHERE data_id = ".mysql_real_escape_string($RosCMS_GET_d_id)." 
													LIMIT 1;"); 
					log_event_medium("unused data-id deleted: ".$RosCMS_GET_d_id.log_prep_info($RosCMS_GET_d_id, $RosCMS_GET_d_r_id)."{alterentry}");
				}
			}
		}
	}
	
	function altersecurityfields() {
		global $RosCMS_GET_d_id;
		global $RosCMS_GET_d_r_id;
		global $RosCMS_GET_d_arch;
		global $RosCMS_GET_d_value;
		global $RosCMS_GET_d_value2;
		global $RosCMS_GET_d_value3;
		global $RosCMS_GET_d_value4;
		global $h_a;
		global $h_a2;
		
		
		
		$query_sec_data = mysql_query("SELECT d.data_id, d.data_name, d.data_type, d.data_acl 
										FROM data_".$h_a2." d, data_revision".$h_a." r 
										WHERE r.rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."'
										AND r.data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."'
										AND d.data_id = r.data_id 
										LIMIT 1;");
		$result_sec_data = mysql_fetch_array($query_sec_data);
		
		if ($result_sec_data['data_name'] != "") {
			if ($RosCMS_GET_d_value != "" && $RosCMS_GET_d_value != $result_sec_data ['data_name']) {
				//echo "<p>update name</p>";
				$update_data_name = mysql_query("UPDATE data_".$h_a2." 
													SET data_name = '".mysql_real_escape_string($RosCMS_GET_d_value)."' 
													WHERE data_id = ".mysql_real_escape_string($RosCMS_GET_d_id)." 
													AND data_name = '".mysql_real_escape_string($result_sec_data['data_name'])."' 
													LIMIT 1;");
													
				log_event_medium("data-name changed: ".$result_sec_data['data_name']." =&gt; ".$RosCMS_GET_d_value.log_prep_info($RosCMS_GET_d_id, $RosCMS_GET_d_r_id)."{altersecurityfields}");
													
				if ($RosCMS_GET_d_value4 == "true") { 
					if ($RosCMS_GET_d_value2 != "" && $RosCMS_GET_d_value2 != $result_sec_data ['data_type']) {
						$tmp_data_type = $RosCMS_GET_d_value2;
					}
					else {
						$tmp_data_type = $result_sec_data ['data_type'];
					}
					
					switch ($tmp_data_type) {
						default:
							$tmp_syntax = "no";
							break;
						case 'page':
							$tmp_syntax = "no";
							break;
						case 'content':
							$tmp_syntax = "cont";
							break;
						case 'template':
							$tmp_syntax = "templ";
							break;
						case 'script':
							$tmp_syntax = "inc";
							break;					
						case 'system':
							$tmp_syntax = "sys";
							break;					
					}
									
					//echo "<p>update links</p><ul>";
					$query_sec_namelink = mysql_query("SELECT text_id, text_content 
													FROM data_text".$h_a." 
													ORDER BY text_id ASC;");
					while ($result_sec_namelink = mysql_fetch_array($query_sec_namelink)) {
						//echo "<li>".$result_sec_namelink['text_id'];
						//echo " [type: ".$tmp_data_type."; syntax: ".$tmp_syntax."] ";
						
						$tmp_content = $result_sec_namelink['text_content'];
						
						if ($tmp_syntax != "no") {
							// update import of the current data-type
							$tmp_content = str_replace("[#".$tmp_syntax."_".$result_sec_data['data_name']."]", "[#".$tmp_syntax."_".$RosCMS_GET_d_value."]", $tmp_content); 
							//echo " [#".$tmp_syntax."_".$result_sec_data['data_name']."] =&gt; [#".$tmp_syntax."_".$RosCMS_GET_d_value."] ";
						}
						
						if ($result_sec_data['data_type'] == "page") {
							// update links
							$tmp_content = str_replace("[#link_".$result_sec_data['data_name']."]", "[#link_".$RosCMS_GET_d_value."]", $tmp_content); 
							//echo " [#link_".$result_sec_data['data_name']."] =&gt; [#link_".$RosCMS_GET_d_value."] ";
						}
					
						$update_data_links = mysql_query("UPDATE data_text".$h_a." 
															SET text_content = '".mysql_real_escape_string($tmp_content)."' 
															WHERE text_id = ".mysql_real_escape_string($result_sec_namelink['text_id'])." 
															LIMIT 1;");

						//echo "</li>";
					}
					//echo "</ul>";
					
					log_event_medium("data-interlinks updated due data-name change".log_prep_info($RosCMS_GET_d_id, $RosCMS_GET_d_r_id)."{altersecurityfields}");
				}			
			}
			if ($RosCMS_GET_d_value2 != "" && $RosCMS_GET_d_value2 != $result_sec_data ['data_type']) {
				//echo "<p>update type</p>";
				$update_data_type = mysql_query("UPDATE data_".$h_a2." 
													SET data_type = '".mysql_real_escape_string($RosCMS_GET_d_value2)."' 
													WHERE data_id = ".mysql_real_escape_string($RosCMS_GET_d_id)." 
													AND data_type = '".mysql_real_escape_string($result_sec_data['data_type'])."' 
													LIMIT 1;");
				log_event_medium("data-type changed: ".$result_sec_data['data_type']." =&gt; ".$RosCMS_GET_d_value2.log_prep_info($RosCMS_GET_d_id, $RosCMS_GET_d_r_id)."{altersecurityfields}");
			}
			if ($RosCMS_GET_d_value3 != "" && $RosCMS_GET_d_value3 != $result_sec_data['data_acl']) {
				//echo "<p>update ACL</p>";
				$update_data_type = mysql_query("UPDATE data_".$h_a2." 
													SET data_acl = '".mysql_real_escape_string($RosCMS_GET_d_value3)."' 
													WHERE data_id = ".mysql_real_escape_string($RosCMS_GET_d_id)." 
													AND data_acl = '".mysql_real_escape_string($result_sec_data['data_acl'])."' 
													LIMIT 1;");
				log_event_medium("data-acl changed: ".$result_sec_data['data_acl']." =&gt; ".$RosCMS_GET_d_value3.log_prep_info($RosCMS_GET_d_id, $RosCMS_GET_d_r_id)."{altersecurityfields}");
			}
		}
	}
	
	function alterentryfields() {
		global $RosCMS_GET_d_id;
		global $RosCMS_GET_d_r_id;
		global $RosCMS_GET_d_arch;
		global $RosCMS_GET_d_value;
		global $RosCMS_GET_d_value2;
	
		if (!$RosCMS_GET_d_arch) { // only allowed in non-archive mode 
			$entry_stext = explode("|", $RosCMS_GET_d_value);
			$entry_text = explode("|", $RosCMS_GET_d_value2);
			
			// Short Text Fields
			foreach ($entry_stext as $key => $value) {
				$entry_stext_arr = explode("=", $value);
				
				$entry_stext_org = $entry_stext_arr[0];
				$entry_stext_str = $entry_stext_arr[1];
				$entry_stext_del = $entry_stext_arr[2];
				
				if ($entry_stext_org == $entry_stext_str && $entry_stext_del == "false") { // no changes
					// do nothing
					//echo "<p>do nothing: ".$entry_stext_str."</p>";
				}
				else {
					//echo "<p>* ".$key." => ".$value."</p>";
		
					$query_stext_field = mysql_query("SELECT COUNT(*)    
														FROM data_revision r, data_stext s 
														WHERE r.data_id = ".mysql_real_escape_string($RosCMS_GET_d_id)."
														AND r.rev_id = ".mysql_real_escape_string($RosCMS_GET_d_r_id)."
														AND s.data_rev_id = r.rev_id
														AND s.stext_name = '".mysql_real_escape_string($entry_stext_org)."' 
														LIMIT 1;");
					$result_stext_field = mysql_fetch_row($query_stext_field);
					
					if ($result_stext_field[0] <= 0) { // field doesn't exist
						if ($entry_stext_org == "new" && $entry_stext_del == "false" && $entry_stext_str != "") { // add new field
							//echo "<p>new: ".$entry_stext_str."</p>";
							$insert_stext_field = mysql_query("INSERT INTO data_stext ( stext_id , data_rev_id , stext_name , stext_content ) 
																VALUES (
																	NULL , 
																	'".mysql_real_escape_string($RosCMS_GET_d_r_id)."', 
																	'".mysql_real_escape_string($entry_stext_str)."', 
																	''
																);");
						}
					}
					else { // field exist
						if ($entry_stext_del == "true") { // delete field
							//echo "<p>delete: ".$entry_stext_org."</p>";
							$delete_stext_field = mysql_query("DELETE FROM data_stext 
																WHERE data_rev_id = ".mysql_real_escape_string($RosCMS_GET_d_r_id)." 
																AND stext_name = '".mysql_real_escape_string($entry_stext_org)."' 
																LIMIT 1;"); 
						}
						else if ($entry_stext_del == "false" && $entry_stext_str != "") { // update field-name
							//echo "<p>update: ".$entry_stext_str." (".$entry_stext_org.")</p>";
							$update_stext_field = mysql_query("UPDATE data_stext 
																SET stext_name = '".mysql_real_escape_string($entry_stext_str)."' 
																WHERE data_rev_id = ".mysql_real_escape_string($RosCMS_GET_d_r_id)." 
																AND stext_name = '".mysql_real_escape_string($entry_stext_org)."' 
																LIMIT 1;");
						}
					}
				}
			}
			
			//echo "\n\n<hr />\n\n";
			
			$key = "";
			$value = "";
			
			// Text Fields
			foreach ($entry_text as $key => $value) {
				$entry_text_arr = explode("=", $value);
				
				$entry_text_org = $entry_text_arr[0];
				$entry_text_str = $entry_text_arr[1];
				$entry_text_del = $entry_text_arr[2];
				
				if ($entry_text_org == $entry_text_str && $entry_text_del == "false") { // no changes
					// do nothing
					//echo "<p>do nothing: ".$entry_text_str."</p>";
				}
				else {
					//echo "<p>~ ".$key." => ".$value."</p>";
		
					$query_text_field = mysql_query("SELECT COUNT(*) 
														FROM data_revision r, data_text t 
														WHERE r.data_id = ".mysql_real_escape_string($RosCMS_GET_d_id)."
														AND r.rev_id = ".mysql_real_escape_string($RosCMS_GET_d_r_id)."
														AND t.data_rev_id = r.rev_id
														AND t.text_name = '".mysql_real_escape_string($entry_text_org)."' 
														LIMIT 1;"); 
					$result_text_field = mysql_fetch_row($query_text_field);
					
					if ($result_text_field[0] <= 0) { // field doesn't exist
						if ($entry_text_org == "new" && $entry_text_del == "false" && $entry_text_str != "") { // add new field
							//echo "<p>new: ".$entry_text_str."</p>";
							$insert_text_field = mysql_query("INSERT INTO data_text ( text_id , data_rev_id , text_name , text_content ) 
																VALUES (
																	NULL , 
																	'".mysql_real_escape_string($RosCMS_GET_d_r_id)."', 
																	'".mysql_real_escape_string($entry_text_str)."', ''
																);");
						}
					}
					else { // field exist
						if ($entry_text_del == "true") { // delete field
							//echo "<p>delete: ".$entry_text_org."</p>";
							$delete_text_field = mysql_query("DELETE FROM data_text 
																WHERE data_rev_id = ".mysql_real_escape_string($RosCMS_GET_d_r_id)." 
																AND text_name = '".mysql_real_escape_string($entry_text_org)."' 
																LIMIT 1;"); 
						}
						else if ($entry_text_del == "false" && $entry_text_str != "") { // update field-name
							//echo "<p>update: ".$entry_text_str." (".$entry_text_org.")</p>";
							$update_text_field = mysql_query("UPDATE data_text 
																SET text_name = '".mysql_real_escape_string($entry_text_str)."' 
																WHERE data_rev_id = ".mysql_real_escape_string($RosCMS_GET_d_r_id)." 
																AND text_name = '".mysql_real_escape_string($entry_text_org)."' 
																LIMIT 1 ;");
						}
					}
				}
			}
		}
	}
	
	function newentryselect($tmode = 0) {
		global $roscms_security_level;


		if ($roscms_security_level > 1) {
	?>
			<div id="frmadd" style="border-bottom: 1px solid #bbb; border-bottom-width: 1px; border-right: 1px solid #bbb; border-right-width: 1px; background: #FFFFFF none repeat scroll 0%;">
			<div style="margin:10px;">
	
			
			<div class="detailbody">
				<div class="detailmenubody" id="newentrymenu">
					<?php
						if ($tmode == 0) {
							echo '<b>Single Entry</b>';
						}
						else {
							echo '<span class="detailmenu" onclick="';
							echo "changecreateinterface('single')";
							echo '">Single Entry</span>';
						}
						
						echo "&nbsp;|&nbsp;";
						
						if ($tmode == 1) {
							echo '<b>Dynamic Entry</b>';
						}
						else {
							echo '<span class="detailmenu" onclick="';
							echo "changecreateinterface('dynamic')";
							echo '">Dynamic Entry</span>';
						}
	
						echo "&nbsp;|&nbsp;";
						
						if ($tmode == 2) {
							echo '<b>Page &amp; Content</b>';
						}
						else {
							echo '<span class="detailmenu" onclick="';
							echo "changecreateinterface('template')";
							echo '">Page &amp; Content</span>';
						}
						
					?>
				</div>
			</div>
	
		<?php
			if ($tmode == 0) {
		?>		
				<br /><div class="frmeditheadline">Name</div>
				<input type="text" id="txtaddentryname" name="txtaddentryname" /><br /><br />
				
				<br /><div class="frmeditheadline">Type</div>
				<select id="txtaddentrytype" name="txtaddentrytype">
					<option value="page">Page</option>
					<option value="content">Content</option>
					<option value="template">Template</option>
					<option value="script">Script</option>
					<option value="system">System</option>
				</select><br /><br />
		
				
				<br /><div class="frmeditheadline">Language</div>
				<select id="txtaddentrylang" name="txtaddentrylang">
				<?php
					$query_language = mysql_query("SELECT * 
													FROM languages
													WHERE lang_level > '0'
													ORDER BY lang_name ASC ;");
					while($result_language=mysql_fetch_array($query_language)) {
						echo '<option value="';
						echo $result_language['lang_id'];
						echo '">'.$result_language['lang_name'].'</option>';
						
					}
				?>
				</select>						
	
		<?php
			}
			else if ($tmode == 1) {
		?>		
				<br /><div class="frmeditheadline">Source</div>
				<select id="txtadddynsource" name="txtadddynsource">
					<option value="news_page">News</option>
					<option value="newsletter">Newsletter</option>
					<option value="interview">Interview</option>
				</select>
	
		<?php
			}
			else if ($tmode == 2) {
		?>		
				<br /><div class="frmeditheadline">Name</div>
				<input type="text" id="txtaddentryname3" name="txtaddentryname3" /><br />

				<br /><div class="frmeditheadline">Template</div>
				<select id="txtaddtemplate" name="txtaddtemplate">
				<?php
					echo '	<option value="none" selected="selected">no template</option>';
					$query_templates_stable = mysql_query("SELECT d.data_name, r.rev_id
															FROM data_revision r, data_ d 
															WHERE r.rev_version > 0 
															AND r.data_id = d.data_id 
															AND d.data_type = 'template' 
															ORDER BY d.data_name ASC;");
					while ($result_templates_stable = mysql_fetch_array($query_templates_stable)) {
						echo '	<option value="'. $result_templates_stable['data_name'] .'">'. $result_templates_stable['data_name'] .'</option>';
					}
				?>
				</select>		<?php
			}
		?>		
	
			<br /><br /><button type="button" onclick="createentry('<?php echo $tmode; ?>')">Create</button>
	
			</div>
			</div>
	<?php
		}
	}
		
	function newentryadd2($tmp_show_output = false, $tmp_entry_status = "draft", $tmp_layout_template = "", $tmp_dyncont = false) {
		global $RosCMS_GET_d_name;
		global $RosCMS_GET_d_type;
		global $RosCMS_GET_d_r_lang;

		global $RosCMS_GET_d_id;
		global $RosCMS_GET_d_r_id;
		global $roscms_intern_account_id;
		global $RosCMS_GET_debug;
				
		$query_mef_addnew_data = mysql_query("SELECT * 
												FROM data_ 
												WHERE data_name = '".mysql_real_escape_string($RosCMS_GET_d_name)."'
												AND data_type = '".mysql_real_escape_string($RosCMS_GET_d_type)."'
												LIMIT 1;");
		$result_mef_addnew_data = mysql_fetch_array($query_mef_addnew_data);
		
		if ($result_mef_addnew_data['data_id'] == "") {
			$insert_data_new = mysql_query("INSERT INTO data_ ( data_id , data_name , data_type ) 
											VALUES (
												NULL , 
												'".mysql_real_escape_string($RosCMS_GET_d_name)."', 
												'".mysql_real_escape_string($RosCMS_GET_d_type)."'
											);");
											
			$query_mef_addnew_data = "";
			$result_mef_addnew_data = "";
			$query_mef_addnew_data = mysql_query("SELECT * 
													FROM data_ 
													WHERE data_name = '".mysql_real_escape_string($RosCMS_GET_d_name)."'
													AND data_type = '".mysql_real_escape_string($RosCMS_GET_d_type)."'
													LIMIT 1;");
			$result_mef_addnew_data = mysql_fetch_array($query_mef_addnew_data);
		}
		
		$query_mef_addnew_data_rev = mysql_query("SELECT COUNT(*)
												FROM data_revision
												WHERE data_id = '".mysql_real_escape_string($result_mef_addnew_data['data_id'])."'
												AND rev_language = '".mysql_real_escape_string($RosCMS_GET_d_r_lang)."';");
		$result_mef_addnew_data_rev = mysql_fetch_row($query_mef_addnew_data_rev);
		
		if ($result_mef_addnew_data_rev[0] <= 0 || $tmp_dyncont == true) {
			// revision entry doesn't exist
			$insert_data_save = mysql_query("INSERT INTO data_revision ( rev_id , data_id , rev_version , rev_language , rev_usrid , rev_datetime , rev_date , rev_time ) 
												VALUES (
													NULL , 
													'".mysql_real_escape_string($result_mef_addnew_data['data_id'])."', 
													'0', 
													'".mysql_real_escape_string($RosCMS_GET_d_r_lang)."', 
													'".mysql_real_escape_string($roscms_intern_account_id)."', 
													NOW( ),
													CURDATE( ),
													CURTIME( )
												);");
			$query_data_save2 = mysql_query("SELECT rev_id
														FROM data_revision
														WHERE data_id = '".mysql_real_escape_string($result_mef_addnew_data['data_id'])."'
														AND rev_version = '0'
														AND rev_language = '".mysql_real_escape_string($RosCMS_GET_d_r_lang)."'
														AND rev_usrid = '".mysql_real_escape_string($roscms_intern_account_id)."'
														ORDER BY rev_datetime DESC;");
			$result_data_save2 = mysql_fetch_array($query_data_save2);
			
			
			if ($RosCMS_GET_d_type == "page") {
				if ($RosCMS_GET_debug) echo "<h3>Page</h3>";
				$temp_sql_type = ", (
														NULL , 
														'".mysql_real_escape_string($result_data_save2['rev_id'])."', 
														'".mysql_real_escape_string('comment')."', 
														'".mysql_real_escape_string('')."'
													),
													(
														NULL , 
														'".mysql_real_escape_string($result_data_save2['rev_id'])."', 
														'".mysql_real_escape_string('title')."', 
														'".mysql_real_escape_string(ucfirst($RosCMS_GET_d_name))."'
													)";
			}
			else if ($RosCMS_GET_d_type == "content" && $tmp_dyncont == true) {
				$temp_sql_type = ", (
														NULL , 
														'".mysql_real_escape_string($result_data_save2['rev_id'])."', 
														'".mysql_real_escape_string('title')."', 
														'".mysql_real_escape_string(ucfirst($RosCMS_GET_d_name))."'
													)";
			}
			else {
				$temp_sql_type = "";
			}
			
			$insert_data_save_stext = mysql_query("INSERT INTO data_stext ( stext_id , data_rev_id , stext_name , stext_content ) 
													VALUES 
													(
														NULL , 
														'".mysql_real_escape_string($result_data_save2['rev_id'])."', 
														'".mysql_real_escape_string('description')."', 
														'".mysql_real_escape_string('')."'
													)
													". $temp_sql_type ."
													;");
			
			if ($tmp_layout_template != "" && $tmp_layout_template != "no") {
				$temp_sql_content = " '[#templ_".mysql_real_escape_string($tmp_layout_template)."]' ";
			}
			else {
				$temp_sql_content = " '' ";
			}

													
			$insert_data_save_text = mysql_query("INSERT INTO data_text ( text_id , data_rev_id , text_name , text_content ) 
													VALUES (
														NULL , 
														'".mysql_real_escape_string($result_data_save2['rev_id'])."', 
														'".mysql_real_escape_string('content')."', 
													". $temp_sql_content ."
													);");
			//echo $result_data_save2['rev_id'];
			
			tag_add($result_mef_addnew_data['data_id'], $result_data_save2['rev_id'], "status", $tmp_entry_status, "-1");

			
			$RosCMS_GET_d_id = $result_mef_addnew_data['data_id'];
			$RosCMS_GET_d_r_id = $result_data_save2['rev_id'];

			
			if ($tmp_dyncont == true) { // add dynamic content number
				$tmp_number = 0;
				
				$query_mef_addnew_dynnumber = mysql_query("SELECT v.tv_value 
														FROM data_revision r, data_tag a, data_tag_name n, data_tag_value v 
														WHERE r.data_id  = '".mysql_real_escape_string($RosCMS_GET_d_id)."'
														AND r.rev_language = '".mysql_real_escape_string($RosCMS_GET_d_r_lang)."'
														AND r.rev_version > 0 
														AND r.data_id = a.data_id 
														AND r.rev_id = a.data_rev_id 
														AND (a.tag_usrid = '-1') 
														AND a.tag_name_id = n.tn_id  
														AND a.tag_value_id  = v.tv_id 
														AND n.tn_name = 'number' 
														ORDER BY v.tv_value ASC;");
				while ($result_mef_addnew_dynnumber = mysql_fetch_array($query_mef_addnew_dynnumber)) {
					//echo "<p>".$tmp_number." = ".$result_mef_addnew_dynnumber['tv_value']."</p>";
					if (($result_mef_addnew_dynnumber['tv_value'] * 1) > $tmp_number) {
						$tmp_number = ($result_mef_addnew_dynnumber['tv_value'] * 1);
						//echo "<p>=&gt; ".$tmp_number."</p>";
					}
				}
				
				$tmp_number++;
				//echo "<p>dyn-nbr: ".$tmp_number."</p>";
				
				$tmp_number_sort = "";
				for ($i = strlen($tmp_number); $i < 5; $i++) {
					$tmp_number_sort .= "0";
				}
				$tmp_number_sort .= $tmp_number;
				
				//echo "<p>strlen: ".strlen($tmp_number)."</p>";
				//echo "<p>number_sort: ".$tmp_number_sort."</p>";
				
				tag_add($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, "number", $tmp_number, "-1");
				tag_add($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, "number_sort", $tmp_number_sort, "-1");
				tag_add($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, "pub_date", date("Y-m-d"), "-1");
				tag_add($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, "pub_user", $roscms_intern_account_id, "-1");
				
				
				if ($RosCMS_GET_debug) echo "<p>add dynamic content number: ".$tmp_number."</p>";
			}
			
			if ($RosCMS_GET_d_type == "page") {
				tag_add($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, "extension", "html", "-1");
			}
			
			
			if ($tmp_show_output == true) {
				show_edit_data();
				show_edit_data_tag();
				show_edit_data2();
				show_edit_data_form(true, true);
			}
			
			if ($RosCMS_GET_debug) echo "<p>RosCMS_GET_d_template: ".$tmp_layout_template." ; show_output: ".$tmp_show_output."</p>";

			
		}
		else {
			if ($tmp_show_output == true) {
				// nothing todo, even revision entry is already there
				echo "<p>Entry already exist.</p>";
				newentryselect();
			}
		}
		
		//echo "<p>".$RosCMS_GET_d_name.", ".$RosCMS_GET_d_type.", ".$RosCMS_GET_d_r_lang."</p>";
		
		return "";
	}

	function diff_entries($diff1, $diff2) {
		global $roscms_security_level;
		
		if (substr($diff1, 0, 2) == "ar") {
			$h1_a = "_a";
			$h1_a2 = "a";
			$h1_a3 = "ar";
			$tmp_diff1 = substr($diff1, 2, strlen($diff1));
		}
		else {
			$h1_a = "";
			$h1_a2 = "";
			$h1_a3 = "";
			$tmp_diff1 = $diff1;
		}

		if (substr($diff2, 0, 2) == "ar") {
			$h2_a = "_a";
			$h2_a2 = "a";
			$h2_a3 = "ar";
			$tmp_diff2 = substr($diff2, 2, strlen($diff2));
		}
		else {
			$h2_a = "";
			$h2_a2 = "";
			$h2_a3 = "";
			$tmp_diff2 = $diff2;
		}
		
		//echo "<p>diff1: ".$tmp_diff1 ."</p>";
		//echo "<p>diff2: ".$tmp_diff2 ."</p>";


		// @TODO: add short text and optional long text additional entries
		
		$query_diff1_data = mysql_query("SELECT d.data_id, d.data_name, d.data_type, r.rev_id, r.rev_version, r.rev_language, r.rev_datetime, u.user_name, l.lang_name 
											FROM data_".$h1_a2." d, data_revision".$h1_a." r, users u, languages l 
											WHERE r.data_id = d.data_id 
											AND r.rev_id = '".mysql_real_escape_string($tmp_diff1)."'
											AND r.rev_usrid = u.user_id
											AND r.rev_language = l.lang_id 
											LIMIT 1;");
		$result_diff1_data = mysql_fetch_array($query_diff1_data);
		$result_diff1_data_text = mysql_query("SELECT t.text_name, t.text_content 
													FROM data_revision".$h1_a." r, data_text".$h1_a." t 
													WHERE r.rev_id = t.data_rev_id
													AND r.rev_id = '".mysql_real_escape_string($tmp_diff1)."'
													AND t.text_name = 'content' 
													ORDER BY text_name ASC;");
		$result_diff1_data_text = mysql_fetch_array($result_diff1_data_text);
		
	
		$query_diff2_data = mysql_query("SELECT d.data_id, d.data_name, d.data_type, r.rev_id, r.rev_version, r.rev_language, r.rev_datetime, u.user_name, l.lang_name 
											FROM data_".$h2_a2." d, data_revision".$h2_a." r, users u, languages l 
											WHERE r.data_id = d.data_id 
											AND r.rev_id = '".mysql_real_escape_string($tmp_diff2)."'
											AND r.rev_usrid = u.user_id
											AND r.rev_language = l.lang_id 
											LIMIT 1;");
		$result_diff2_data = mysql_fetch_array($query_diff2_data);
		$result_diff2_data_text = mysql_query("SELECT t.text_name, t.text_content 
													FROM data_revision".$h2_a." r, data_text".$h2_a." t 
													WHERE r.rev_id = t.data_rev_id
													AND r.rev_id = '".mysql_real_escape_string($tmp_diff2)."'
													AND t.text_name = 'content' 
													ORDER BY text_name ASC;");
		$result_diff2_data_text = mysql_fetch_array($result_diff2_data_text);
		?>
		<div style="display: block; border-bottom: 1px solid #bbb; border-bottom-width: 1px; border-right: 1px solid #bbb; border-right-width: 1px; background: #FFFFFF none repeat scroll 0%;"><div style="margin:10px;">
		
		<br />
		<div class="frmeditheadline">Compare</div>
		<?php
			if ($diff1 == $diff2) {
				echo "<p>Please select two different entries to display the differences!</p>";
			}
			else {
				echo "<br />";
			}
		?>
		<table width="100%" border="0">
		  <tr>
			<td>
			<div align="center">
			<select name="cbmdiff1" id="cbmdiff1" onchange="diffentries2(this.value, document.getElementById('cbmdiff2').value)">
				<?php
					diffcombobox($diff1, $result_diff1_data['data_id'], $result_diff1_data['data_name']);
				?>
			</select>
			</div>
			</td>
			<td width="50">
				<div align="center">
					<input type="submit" name="switchdiff" id="switchdiff" value="switch" onclick="diffentries2(document.getElementById('cbmdiff2').value, document.getElementById('cbmdiff1').value)" />
				</div>
			</td>
			<td><div align="center">
			<select name="cbmdiff2" id="cbmdiff2" onchange="diffentries2(document.getElementById('cbmdiff1').value, this.value)">
				<?php
					diffcombobox($diff2, $result_diff2_data['data_id'], $result_diff2_data['data_name']);
				?>
			</select>
			</div></td>
		  </tr>
  		 <tr><td>
		<ul style="font-size:9px;"><li>Type: <?php echo $result_diff1_data['data_type']; ?></li><li>Language: <?php echo $result_diff1_data['lang_name']; ?></li><li>User: <?php echo $result_diff1_data['user_name']; ?></li><?php if ($roscms_security_level > 1) { ?><li>ID: <?php echo $result_diff1_data['rev_id']; ?></li><?php } ?></ul>
		</td>
    	<td>&nbsp;</td>
		<td>
		<ul style="font-size:9px;"><li>Type: <?php echo $result_diff2_data['data_type']; ?></li><li>Language: <?php echo $result_diff2_data['lang_name']; ?></li><li>User: <?php echo $result_diff2_data['user_name']; ?></li><?php if ($roscms_security_level > 1) { ?><li>ID: <?php echo $result_diff2_data['rev_id']; ?></li><?php } ?></ul>
		</td></tr>
		</table>
		<div><pre id="frmeditdiff1" style="display: none;"><?php echo $result_diff1_data_text['text_content']; ?></pre></div>
		<div><pre id="frmeditdiff2" style="display: none; "><?php echo $result_diff2_data_text['text_content']; ?></pre></div>
		<div style="display: block; border-width: 1px; border-bottom: 1px solid #bbb;  border-right: 1px solid #bbb; border-top: 1px solid #e3e3e3; border-left: 1px solid #e3e3e3; background: #F2F2F2 none repeat scroll 0%;"><pre style="margin:10px; font-size:9px; font-family:Arial, Helvetica, sans-serif;" id="frmeditdiff">&nbsp;</pre>
		
		</div></div>
		<?php
	}
	
	function diffcombobox($temp_rev_id, $temp_d_id, $tempd_name) {
		$temp_cur_lang = "";
		global $h_a;
		global $h_a2;
		global $RosCMS_GET_d_arch;

		if ($RosCMS_GET_d_arch) {
			$h1_a = "_a";
			$h1_a2 = "a";
			$h1_a3 = "ar";
			$tmp_diff_revid = $temp_rev_id;
		}
		else if (substr($temp_rev_id, 0, 2) == "ar") {
			$h1_a = "_a";
			$h1_a2 = "a";
			$h1_a3 = "ar";
			$tmp_diff_revid = substr($temp_rev_id, 2, strlen($temp_rev_id));
		}
		else {
			$h1_a = "";
			$h1_a2 = "";
			$h1_a3 = "";
			$tmp_diff_revid = $temp_rev_id;
		}


		$query_get_data_name = mysql_query("SELECT data_name, data_type 
										FROM data_".$h1_a2." 
										WHERE data_id = '".mysql_real_escape_string($temp_d_id)."'
										LIMIT 1;");
		$result_get_data_name = mysql_fetch_array($query_get_data_name);

		if ($RosCMS_GET_d_arch) { // archive mode
			$query_get_data_name = mysql_query("SELECT data_name, data_type 
											FROM data_  
											WHERE data_name = '".mysql_real_escape_string($result_get_data_name['data_name'])."'
											AND data_type = '".mysql_real_escape_string($result_get_data_name['data_type'])."'
											LIMIT 1;");
			$result_get_data_name = mysql_fetch_array($query_get_data_name);
			
			$tmp_d_name = $result_get_data_name['data_name'];
			$tmp_d_type = $result_get_data_name['data_type'];
		}
		else {
			$tmp_d_name = $result_get_data_name['data_name'];
			$tmp_d_type = $result_get_data_name['data_type'];
		}
		
		//echo "<p>".$tmp_d_name."-".$tmp_d_type."</p>";
		
		
		for ($i = 0; $i < 2; $i++) { // first loop = normal, second lopp = archive
			if ($i == 1) {
				echo "<option value=\"\">&nbsp;</option>";
				echo "<option value=\"\">&nbsp;</option>";
				echo '<option value="" style="color: rgb(119, 119, 119);">&nbsp;&nbsp;&nbsp;----- Archive -----</option>';
				echo "<option value=\"\">&nbsp;</option>";
				$tmp_a = "_a";
				$tmp_a2 = "a";
				$tmp_a3 = "ar";				
			}
			else {
				$tmp_a = "";
				$tmp_a2 = "";
				$tmp_a3 = "";				
			}
			
			$temp_cur_lang = "";
			
			$query_diff1_cbm = mysql_query("SELECT d.data_id, d.data_name, r.rev_id, r.rev_language, r.rev_version, r.rev_date, u.user_name, l.lang_name 
											FROM data_".$tmp_a2." d, data_revision".$tmp_a." r, languages l, users u 
											WHERE d.data_name = '".mysql_real_escape_string($tmp_d_name)."'
											AND d.data_type = '".mysql_real_escape_string($tmp_d_type)."'
											AND r.data_id = d.data_id
											AND r.rev_language = l.lang_id 
											AND r.rev_version > 0 
											AND u.user_id = r.rev_usrid 
											ORDER BY l.lang_name  ASC, r.rev_datetime DESC;");
			while ($result_diff_cbm = mysql_fetch_array($query_diff1_cbm)) {
				if ($result_diff_cbm['rev_language'] != $temp_cur_lang) {
					if ($temp_cur_lang != "") {
						echo "</optgroup>";
					}
		
					$query_cur_lang = mysql_query("SELECT * 
													FROM languages 
													WHERE lang_id = '".mysql_real_escape_string($result_diff_cbm['rev_language'])."'
													LIMIT 1;");
					$result_cur_lang = mysql_fetch_array($query_cur_lang);
		
					echo "<optgroup label=\"".$result_diff_cbm['lang_name']."\">"; 
					
					$temp_cur_lang = $result_diff_cbm['rev_language'];
				}
				
				echo "<option value=\"".$tmp_a3.$result_diff_cbm['rev_id']."\"";
				if ($result_diff_cbm['rev_id'] == $tmp_diff_revid) {
					echo "selected=\"selected\"";
				}
				
				echo ">".$tempd_name;
				
				$temp_dynamic = getTagValue($result_diff_cbm['data_id'], $result_diff_cbm['rev_id'],  '-1', 'number');
				if ($temp_dynamic != "") {
					echo "_".$temp_dynamic;
				}
				
				echo " (".$result_diff_cbm['rev_date'].") - v. ".$result_diff_cbm['rev_version']."; ".$result_diff_cbm['user_name']."</option>";				
			}
			echo "</optgroup>";
		}
	}
	
	function changetags($entr_count, $entr_revid, $entr_flag) {
		global $roscms_intern_account_id;
		global $roscms_standard_language;
		global $RosCMS_GET_debug;
		global $roscms_security_level;
		global $h_a;
		global $h_a2;
		
		global $RosCMS_GET_d_value4;
		
		if ($RosCMS_GET_debug) echo "<p>COUNT: ".$entr_count."; IDs: ".$entr_revid."</p>";
		
		if ($entr_count > 0) {
			$entry_ids = split("-", $entr_revid);
			
			
			if ($entr_flag == "mp") {
				$entry_ids2 = split("_", $entry_ids[0]);
				if ($RosCMS_GET_debug) echo "===>".$entry_ids2[1];
			
				require_once("inc/data_export_page.php");
				$query_preview_page = mysql_query("SELECT d.data_name, d.data_id, r.rev_id, r.rev_language
													FROM data_revision r, data_ d
													WHERE r.rev_id = '".mysql_real_escape_string($entry_ids2[1])."'
													AND r.data_id = d.data_id 
													LIMIT 1;");
				$result_preview_page = mysql_fetch_array($query_preview_page);
				
				$temp_dynamic = getTagValue($result_preview_page['data_id'], $result_preview_page['rev_id'],  '-1', 'number');
				if ($RosCMS_GET_debug) echo "<h1>!! preview !!</h1>\n";
				echo generate_page($result_preview_page['data_name'], $result_preview_page['rev_language'], $temp_dynamic, "show");
			
			}
			else {
				if ($RosCMS_GET_debug) echo "<ul>";
				
				for ($i=0; $i < count($entry_ids); $i++) {
				
					$entry_ids2 = split("_", $entry_ids[$i]);
					if ($RosCMS_GET_debug) echo "<li>".$entry_ids2[0]." == ".$entry_ids2[1]."</li>";
					
					$query_rev_data = mysql_query("SELECT * 
													FROM data_revision r, data_ d
													WHERE r.rev_id = '".mysql_real_escape_string($entry_ids2[1])."' 
													AND r.data_id = d.data_id 
													LIMIT 1;");
					$result_rev_data = mysql_fetch_array($query_rev_data);

					if (($entr_flag == "ms" || $entr_flag == "mn") && $roscms_security_level > 1) {
						if (roscms_security_grp_member("transmaint")) {
	
							$query_account_lang = @mysql_query("SELECT user_language FROM users WHERE user_id = '".mysql_real_escape_string($roscms_intern_account_id)."'");
							$result_account_lang = @mysql_fetch_array($query_account_lang);
							
							if ($result_account_lang['user_language'] == "") {
								die("Set a valid language in your myReactOS account settings!");
							}
							else if ($result_account_lang['user_language'] != $result_rev_data['rev_language']) {
								echo "As Language Maintainer you can only mark entries of '".$result_account_lang['user_language']."' language as new!";
								continue;
							}
						}
					}
	
	
					$t_tagid = "";
					
					// 'as': add star
					// 'xs': remove star
					if ($entr_flag == "as" || $entr_flag == "xs") {
						$t_tagid = getTagId($result_rev_data['data_id'], $result_rev_data['rev_id'], $roscms_intern_account_id, "star");
					}
					// 'ms': mark as stable
					// 'mn': mark as new
					else if ($entr_flag == "ms" || $entr_flag == "mn") {
						$t_tagid = getTagId($result_rev_data['data_id'], $result_rev_data['rev_id'], "-1", "status");
					}
					
					if ($t_tagid != "") {
						tag_delete($t_tagid, $roscms_intern_account_id);
					}
					
					if ($entr_flag == "as") {
						tag_add($result_rev_data['data_id'], $result_rev_data['rev_id'], "star" /* name */, "on" /* value */, $roscms_intern_account_id /* usrid */);
					}
					// else (remove star) do nothing ;)

					// 'tg': add label
					if ($entr_flag == "tg") {
						//echo "<p>tag it: ".$RosCMS_GET_d_value4."</p>";
						tag_add($result_rev_data['data_id'], $result_rev_data['rev_id'], "tag" /* name */, $RosCMS_GET_d_value4 /* value */, $roscms_intern_account_id /* usrid */);
					}

					
					// 'mn': mark as new
					if ($entr_flag == "mn") {
						tag_add($result_rev_data['data_id'], $result_rev_data['rev_id'], "status" /* name */, "new" /* value */, "-1" /* usrid */);
					}
					
					
					// 'ms': mark as stable
					if ($entr_flag == "ms") {
						log_event_low("mark entry as stable: data-id ".$result_rev_data['data_id'].", rev-id ".$result_rev_data['rev_id'].log_prep_info($result_rev_data['data_id'], $result_rev_data['rev_id'])."{changetags}");


						tag_add($result_rev_data['data_id'], $result_rev_data['rev_id'], "status" /* name */, "stable" /* value */, "-1" /* usrid */);

						if ($result_rev_data['rev_version'] == 0) {

							$temp_dynamic = getTagValue($result_rev_data['data_id'], $result_rev_data['rev_id'],  '-1', 'number'); // get dynamic content number

							if ($RosCMS_GET_debug) echo "<p>dyn-cont-number: ".$temp_dynamic."</p>";			

							$roscms_sql_tags = "";
							$roscms_sql_tags2 = "";
							$roscms_sql_tags3 = "";

							if ($temp_dynamic != "") {
								$roscms_sql_tags .= ", data_tag a, data_tag_name n, data_tag_value v ";
								$roscms_sql_tags2 .= ", n.tn_name, v.tv_value ";
								$roscms_sql_tags3 .= " AND r.data_id = a.data_id 
														AND r.rev_id = a.data_rev_id 
														AND a.tag_usrid = '-1' 
														AND a.tag_name_id = n.tn_id  
														AND n.tn_name = 'number'  
														AND a.tag_value_id  = v.tv_id
														AND v.tv_value = '".mysql_real_escape_string($temp_dynamic)."' ";			
							}

							$query_revision_stable = mysql_query("SELECT * 
																	FROM data_revision r ".$roscms_sql_tags."
																	WHERE r.data_id = '".mysql_real_escape_string($result_rev_data['data_id'])."'
																	AND r.rev_version > 0
																	AND r.rev_language = '".mysql_real_escape_string($result_rev_data['rev_language'])."'
																	".$roscms_sql_tags3."
																	ORDER BY r.rev_version DESC, r.rev_id DESC 
																	LIMIT 1;");
							$result_revision_stable = mysql_fetch_array($query_revision_stable);

							$temp_version = 1;
							
							if ($result_revision_stable['rev_id'] != "") { // no stable entry exist, so skip move-process
								$temp_version = $result_revision_stable['rev_version'];
								$temp_version++;
								
								if ($RosCMS_GET_debug) echo "<p>### old v.: ".$result_revision_stable['rev_version']."; new v.: ".$temp_version."</p>";
								
								// delete old tag(s)
								$delete_old_tags = mysql_query("DELETE FROM data_tag WHERE data_rev_id = '".mysql_real_escape_string($result_rev_data['rev_id'])."' AND data_id = '".mysql_real_escape_string($result_rev_data['data_id'])."';");						
								
								// transfer 
								transfer_tags($result_revision_stable['data_id'], $result_revision_stable['rev_id'], $result_rev_data['data_id'], $result_rev_data['rev_id'], false);
								
								// move old revision to archive
								if (move_to_archive($result_revision_stable['data_id'], $result_revision_stable['rev_id'], 0)) {;
									if ($RosCMS_GET_debug) echo "<p>deleteRevision(".$result_revision_stable['rev_id'].");</p>";
									deleteRevision($result_revision_stable['rev_id']);
								}
								else {
									log_event_medium("move_to_archive() failed: data-id ".$result_revision_stable['data_id'].", rev-id ".$result_revision_stable['rev_id'].log_prep_info($result_revision_stable['data_id'], $result_revision_stable['rev_id'])."{changetags}");
									echo "Process not successful :S";
								}
							}

							// update the version number
							$update_rev_ver = mysql_query("UPDATE data_revision SET rev_version = '".mysql_real_escape_string($temp_version)."' WHERE rev_id = '".mysql_real_escape_string($result_rev_data['rev_id'])."' LIMIT 1;");						
							
							// generate related pages
							require_once("inc/data_export_page.php");
							
							if ($result_revision_stable['rev_language'] == "") {
								$tmp_lang = $roscms_standard_language;
							}
							else {
								$tmp_lang = $result_revision_stable['rev_language'];
							}
							
							$query_entry = mysql_query("SELECT data_id 
														FROM data_ 
														WHERE data_name = '".mysql_real_escape_string($result_rev_data['data_name'])."'
														AND data_type = 'page'
														LIMIT 1;");
							$result_entry = mysql_fetch_array($query_entry);	
													
							log_event_generate_low("+++++ [generate_page_output_update(".$result_rev_data['data_id'].", ".$tmp_lang.", ".$temp_dynamic.")]");
							
							if ($RosCMS_GET_debug) echo "<p>! generate_page_output_update(".$result_rev_data['data_id'].", ".$tmp_lang.", ".$temp_dynamic.")</p>";
							
							echo generate_page_output_update($result_entry['data_id'], $tmp_lang, $temp_dynamic);
							echo "Page generation process finished";
						}
						else {
							echo "Only 'new' entries can be made stable";
						}
					}

					// 'mn': mark as new
					if ($entr_flag == "mn") {
						$update_rev_ver = mysql_query("UPDATE data_revision SET rev_version = '0' WHERE rev_id = '".mysql_real_escape_string($result_rev_data['rev_id'])."' LIMIT 1;");
					}
	
					// 'va': move to archive
					if ($entr_flag == "va") {
						if ($RosCMS_GET_debug) echo "<p>move_to_archive(".$result_rev_data['data_id'].", ".$result_rev_data['rev_id'].", 0);</p>";
						move_to_archive($result_rev_data['data_id'], $result_rev_data['rev_id'], 0);
						if ($RosCMS_GET_debug) echo "<p>deleteRevision(".$result_rev_data['rev_id'].");</p>";
						deleteRevision($result_rev_data['rev_id']);
					}
					
					// 'xe': delete entries
					if ($entr_flag == "xe") {
						if ($result_rev_data['rev_usrid'] == $roscms_intern_account_id || $roscms_security_level > 1) {
							if ($roscms_security_level < 3) {
								move_to_archive($result_rev_data['data_id'], $result_rev_data['rev_id'], 0);
							}
							deleteRevision($result_rev_data['rev_id']);
						}
						else {
							echo "Not enough rights for delete process.";
						}
					}
				}
				if ($RosCMS_GET_debug) echo "</ul>";
			}
		}
	}
	
	function move_to_archive($d_id, $d_revid, $tm_mode) {
		global $roscms_intern_account_id;
		global $RosCMS_GET_d_r_lang;
		global $RosCMS_GET_debug;
				
		$d_id_org = $d_id;
		$d_revid_org = $d_revid;
		$d_name = "";
		
		if ($tm_mode == 0) { // move to archive
			$tmp_archive_sql = "_a";
			$tmp_archive_sql2 = "_";
			$tmp_archive = true;
		}
		else if ($tm_mode == 1) { // create copy
			$tmp_archive_sql = "";
			$tmp_archive_sql2 = "";
			$tmp_archive = false;
		}
		else {
			die("move_to_archive(???)");
		}
		
		if ($tm_mode == 1) {
			log_event_low("copy entire entry (e.g. translate): data-id ".$d_id.", rev-id ".$d_revid.log_prep_info($d_id, $d_revid)."{move_to_archive}");
		}
		else {
			log_event_medium("move entire entry to archive: data-id ".$d_id.", rev-id ".$d_revid.log_prep_info($d_id, $d_revid)."{move_to_archive}");
		}
		
		// data_
		$query_data = mysql_query("SELECT * 
									FROM data_  
									WHERE data_id = '".mysql_real_escape_string($d_id)."'
									LIMIT 1;");
		$result_data = mysql_fetch_array($query_data);
		
		if ($RosCMS_GET_debug) echo "<p>data_: ".$result_data['data_name']."</p>";
		
		if ($tmp_archive == true) {
			$query_data_a = mysql_query("SELECT *
										FROM data_a  
										WHERE data_name = '".mysql_real_escape_string($result_data['data_name'])."'
										AND data_type = '".mysql_real_escape_string($result_data['data_type'])."'
										ORDER BY data_id DESC
										LIMIT 1;");
			$result_data_a = mysql_fetch_array($query_data_a);
		
		
			if ($result_data_a['data_name'] == "") { // if not exist, copy "data_" entry to archive
				if ($RosCMS_GET_debug) echo "<p>insert: data_a</p>";
				
				$insert_data_a = mysql_query("INSERT INTO data_a ( data_id , data_name , data_type , data_acl ) 
												VALUES (
													NULL , 
													'".mysql_real_escape_string($result_data['data_name'])."', 
													'".mysql_real_escape_string($result_data['data_type'])."', 
													'".mysql_real_escape_string($result_data['data_acl'])."'
												);");
	
				$query_data_a2 = mysql_query("SELECT data_name, data_id 
											FROM data_a  
											WHERE data_name = '".mysql_real_escape_string($result_data['data_name'])."'
											AND data_type = '".mysql_real_escape_string($result_data['data_type'])."'
											ORDER BY data_id DESC
											LIMIT 1;");
				$result_data_a2 = mysql_fetch_array($query_data_a2);
				
				/*if ($result_data_a2['data_id'] != $d_id_org) {
					die("data-id is wrong");
				}*/
				$d_name = $result_data_a2['data_name'];
				$d_id = $result_data_a2['data_id'];
				
				if ($RosCMS_GET_debug) echo "<p>(1) data-name: ".$d_name.", data-id: ".$d_id."</p>";
			}
			else {
				/*if ($result_data_a['data_id'] != $d_id_org) {
					die("data-id is wrong");
				}*/
				$d_name = $result_data_a['data_name'];
				$d_id = $result_data_a['data_id'];
				
				if ($RosCMS_GET_debug) echo "<p>(2) data-name: ".$d_name.", data-id: ".$d_id."</p>";
			}
		}
		else {
			$d_name = $result_data['data_name'];
			$d_id = $result_data['data_id'];
			
			if ($RosCMS_GET_debug) echo "<p>(3) data-name: ".$d_name.", data-id: ".$d_id."</p>";
		}
		

		// data_revision
		$query_data_revsion = mysql_query("SELECT *  
												FROM data_revision  
												WHERE data_id = '".mysql_real_escape_string($d_id_org)."' 
												AND rev_id = '".mysql_real_escape_string($d_revid_org)."' 
												LIMIT 1;");
		$result_data_revsion = mysql_fetch_array($query_data_revsion);
	
		if ($tmp_archive == true) {
			$temp_version = $result_data_revsion['rev_version'];
			$temp_usrid = $result_data_revsion['rev_usrid'];
			$temp_usrlng = $result_data_revsion['rev_language'];
			$temp_datetime = $result_data_revsion['rev_datetime'];
			$temp_date = $result_data_revsion['rev_date'];
			$temp_time = $result_data_revsion['rev_time'];
			if ($RosCMS_GET_debug) echo "<p>move to archive: user-lang: ".$temp_usrlng."</p>";
		}
		else {
			$temp_version = 0;
			$temp_usrid = $roscms_intern_account_id;
			$temp_usrlng = $RosCMS_GET_d_r_lang;
			$temp_datetime = date("Y-m-d H:i:s");
			$temp_date = date("Y-m-d");
			$temp_time = date("H:i:s");
			if ($RosCMS_GET_debug) echo "<p>translate: user-lang: ".$temp_usrlng."</p>";
		}
	
		$insert_data_revsion_a = mysql_query("INSERT INTO data_revision".$tmp_archive_sql." ( rev_id , data_id , rev_version , rev_language , rev_usrid , rev_datetime , rev_date , rev_time ) 
										VALUES (
											NULL , 
											'".mysql_real_escape_string($d_id)."', 
											'".mysql_real_escape_string($temp_version)."', 
											'".mysql_real_escape_string($temp_usrlng)."', 
											'".mysql_real_escape_string($temp_usrid)."', 
											'".mysql_real_escape_string($temp_datetime)."', 
											'".mysql_real_escape_string($temp_date)."', 
											'".mysql_real_escape_string($temp_time)."' 
										);");

		$query_data_revsion_a = mysql_query("SELECT *  
												FROM data_revision".$tmp_archive_sql."  
												WHERE data_id = '".mysql_real_escape_string($d_id)."'
												AND rev_datetime = '".mysql_real_escape_string($temp_datetime)."'
												ORDER BY rev_id DESC 
												LIMIT 1;");
		$result_data_revsion_a = mysql_fetch_array($query_data_revsion_a);
	
		if ($result_data_revsion_a['rev_version'] == "") {
			die("copy-process of data_revision not successful");
		}
		
		$d_revid = $result_data_revsion_a['rev_id'];
		$d_dataid = $result_data_revsion_a['data_id'];
		
		if ($RosCMS_GET_debug) echo "<p>(!!) d_revid-name: ".$d_revid.", d_dataid: ".$d_dataid."</p>";

	
		// data_stext
		$query_data_stext = mysql_query("SELECT *  
											FROM data_stext  
											WHERE data_rev_id = '".mysql_real_escape_string($d_revid_org)."';");
		while($result_data_stext = mysql_fetch_array($query_data_stext)) {
			$insert_data_stext_a = mysql_query("INSERT INTO data_stext".$tmp_archive_sql." ( stext_id , data_rev_id , stext_name , stext_content ) 
												VALUES (
													NULL , 
													'".mysql_real_escape_string($d_revid)."', 
													'".mysql_real_escape_string($result_data_stext['stext_name'])."', 
													'".mysql_real_escape_string($result_data_stext['stext_content'])."'
												);");
		}


		// data_text
		$query_data_text = mysql_query("SELECT *  
											FROM data_text  
											WHERE data_rev_id = '".mysql_real_escape_string($d_revid_org)."';");
		while($result_data_text = mysql_fetch_array($query_data_text)) {
			$insert_data_text_a = mysql_query("INSERT INTO data_text".$tmp_archive_sql." ( text_id , data_rev_id , text_name , text_content ) 
												VALUES (
													NULL , 
													'".mysql_real_escape_string($d_revid)."', 
													'".mysql_real_escape_string($result_data_text['text_name'])."', 
													'".mysql_real_escape_string($result_data_text['text_content'])."'
												);");
		}


		// data_tag
		transfer_tags($d_id_org, $d_revid_org, $d_id, $d_revid, $tmp_archive);
		if ($RosCMS_GET_debug) echo "\n<p>transfer_tags(".$d_id_org.", ".$d_revid_org.", ".$d_id.", ".$d_revid.", ".$tmp_archive.")</p>";
		
		
		if ($tm_mode > 0) {
			if ($RosCMS_GET_debug) echo "\n<p>change status to draft</p>";
			delete_tag($d_dataid, $d_revid, "-1", "status");
			tag_add($d_dataid, $d_revid, "status", "draft", "-1");
		}
		
		return true;
	}
	
	function transfer_tags($d_id_org, $d_revid_org, $d_id, $d_revid, $archive) {
		global $RosCMS_GET_debug;

		if ($RosCMS_GET_debug) echo "\n<p>transfer_tags(".$d_id_org.", ".$d_revid_org.", ".$d_id.", ".$d_revid.", ".$archive.") {...}</p>";
	
		if ($archive == true) {
			if ($RosCMS_GET_debug) echo "\n<p>transfer_tags: archive mode</p>";
			$h_a = "_a";
		}
		else {
			if ($RosCMS_GET_debug) echo "\n<p>transfer_tags: normal mode</p>";
			$h_a = "";
		}
	
		// data_tag
		$query_data_tag = mysql_query("SELECT *  
										FROM data_tag 
										WHERE data_id = '".mysql_real_escape_string($d_id_org)."' 
										AND data_rev_id = '".mysql_real_escape_string($d_revid_org)."';");
		while($result_data_tag = mysql_fetch_array($query_data_tag)) {
			if ($RosCMS_GET_debug) echo "<p>=&gt; ".$result_data_tag['tag_id']." (".$result_data_tag['data_id'].", ".$result_data_tag['data_rev_id'].")</p>";
			
			// data_tag_name
			$query_data_tag_name = mysql_query("SELECT *  
													FROM data_tag_name 
													WHERE tn_id = '".mysql_real_escape_string($result_data_tag['tag_name_id'])."'
													LIMIT 1;");
			$result_data_tag_name = mysql_fetch_array($query_data_tag_name);
			if ($RosCMS_GET_debug) echo "<p>&nbsp;&nbsp;=&gt; name: ".$result_data_tag_name['tn_name']."</p>";
			
			$query_data_tag_name_a = mysql_query("SELECT *  
													FROM data_tag_name".$h_a."   
													WHERE tn_name = '".mysql_real_escape_string($result_data_tag_name['tn_name'])."'
													LIMIT 1;");
			$result_data_tag_name_a = mysql_fetch_array($query_data_tag_name_a);

			$temp_tn_id = $result_data_tag_name_a['tn_id'];

			if ($result_data_tag_name_a['tn_name'] == "") {
				$insert_data_tag_name_a = mysql_query("INSERT INTO data_tag_name".$h_a." ( tn_id , tn_name ) 
														VALUES (
															NULL , 
															'".mysql_real_escape_string($result_data_tag_name['tn_name'])."'
														);");
				$query_data_tag_name_a2 = mysql_query("SELECT *  
														FROM data_tag_name".$h_a."   
														WHERE tn_name = '".mysql_real_escape_string($result_data_tag_name['tn_name'])."'
														LIMIT 1;");
				$result_data_tag_name_a2 = mysql_fetch_array($query_data_tag_name_a2);

				$temp_tn_id = $result_data_tag_name_a2['tn_id'];
			}
			
			// data_tag_value
			$query_data_tag_value = mysql_query("SELECT *  
													FROM data_tag_value    
													WHERE tv_id = '".mysql_real_escape_string($result_data_tag['tag_value_id'])."'
													LIMIT 1;");
			$result_data_tag_value = mysql_fetch_array($query_data_tag_value);
			
			$query_data_tag_value_a = mysql_query("SELECT *  
													FROM data_tag_value".$h_a."   
													WHERE tv_value = '".mysql_real_escape_string($result_data_tag_value['tv_value'])."'
													LIMIT 1;");
			$result_data_tag_value_a = mysql_fetch_array($query_data_tag_value_a);
			
			$temp_tv_id = $result_data_tag_value_a['tv_id'];
			
			if ($result_data_tag_value_a['tv_value'] == "") {
				$insert_data_tag_value_a = mysql_query("INSERT INTO data_tag_value".$h_a." ( tv_id , tv_value ) 
														VALUES (
															NULL , 
															'".mysql_real_escape_string($result_data_tag_value['tv_value'])."'
														);");
				$query_data_tag_value_a2 = mysql_query("SELECT *  
														FROM data_tag_value".$h_a."   
														WHERE tv_value = '".mysql_real_escape_string($result_data_tag_value['tv_value'])."'
														LIMIT 1;");
				$result_data_tag_value_a2 = mysql_fetch_array($query_data_tag_value_a2);
				
				$temp_tv_id = $result_data_tag_value_a2['tv_id'];
			}

			$insert_data_tag_a = mysql_query("INSERT INTO data_tag".$h_a." ( tag_id , data_id , data_rev_id , tag_name_id , tag_value_id , tag_usrid ) 
												VALUES (
													NULL , 
													'".mysql_real_escape_string($d_id)."', 
													'".mysql_real_escape_string($d_revid)."', 
													'".mysql_real_escape_string($temp_tn_id)."', 
													'".mysql_real_escape_string($temp_tv_id)."', 
													'".mysql_real_escape_string($result_data_tag['tag_usrid'])."'
												);");
		}
	}
	
	function show_edit_data() {
		global $RosCMS_GET_d_id;
		global $RosCMS_GET_d_r_id;
		global $roscms_intern_account_id;
		global $RosCMS_GET_d_arch;
		global $h_a;
		global $h_a2;
		
		// Database Entry
		$query_edit_mef_data = mysql_query("SELECT d.data_id, d.data_name, d.data_type, r.rev_id, r.rev_version, r.rev_language, r.rev_datetime, u.user_name 
											FROM data_".$h_a2." d, data_revision".$h_a." r, users u
											WHERE r.data_id = d.data_id 
											AND r.rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."'
											AND r.data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."'
											AND r.rev_usrid = u.user_id
											LIMIT 1;");
		$result_edit_mef_data = mysql_fetch_array($query_edit_mef_data);
		?>
		<div style="padding-bottom: 3px;"><span class="frmeditheader">
		<span onclick="bchangestar(<?php echo $RosCMS_GET_d_id.",".$RosCMS_GET_d_r_id; ?>,'star','addtagn', '<?php echo $roscms_intern_account_id; ?>', 'editstar')" style="cursor: pointer;">
		<img id="editstar" class="<?php echo getTagId($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $roscms_intern_account_id, 'star'); ?>" src="images/star_<?php echo getTagValue($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $roscms_intern_account_id, 'star'); ?>_small.gif" alt="" style="width:13px; height:13px; border:0px;" /></span>
		&nbsp;
		<?php
			echo $result_edit_mef_data['data_name'];
			$temp_dynamic = getTagValue($RosCMS_GET_d_id, $RosCMS_GET_d_r_id,  '-1', 'number');
			if ($result_edit_mef_data['data_type'] == "content" && $temp_dynamic != "") {
				echo "_".$temp_dynamic;
				echo "<div id=\"entryeditdynnbr\" style=\"display:none;\">".$temp_dynamic."</div>";
			}
			else {
				echo "<div id=\"entryeditdynnbr\" style=\"display:none;\">no</div>";
			}
		?>
		</span> &nbsp; <span style="white-space: nowrap;">type: <span class="frmeditheader"><?php echo $result_edit_mef_data['data_type']; ?></span></span> &nbsp; 
		<span style="white-space: nowrap;">version: <span  id="mefrverid" class="frmeditheader"><?php echo $result_edit_mef_data['rev_version']; ?></span></span> &nbsp; 
		<span style="white-space: nowrap;">language: <span id="mefrlang" class="frmeditheader"><?php echo $result_edit_mef_data['rev_language']; ?></span></span> &nbsp; 
		<span style="white-space: nowrap;">user: <span id="mefrusrid" class="frmeditheader"><?php echo $result_edit_mef_data['user_name']; ?></span></span> &nbsp; 
		<?php 
			if ($RosCMS_GET_d_arch) {
		?>
			<span style="white-space: nowrap;">mode: <span id="mefrusrid" class="frmeditheader">archive</span></span> &nbsp; 
		<?php
			}
		?>
		<span id="frmedittags" class="frmeditbutton" onclick="TabOpenClose(this.id)" style="white-space: nowrap;"><img id="frmedittagsi" src="images/tab_closed.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Details</span></div>
		
		
		<div id="frmedittagsc" class="edittagbody" style="display: none;"><div class="edittagbody2" id="frmedittagsc2">
		<?php
	}
	
	function show_edit_data_tag($tmode = 0) {
		global $RosCMS_GET_d_id;
		global $RosCMS_GET_d_r_id;
		global $RosCMS_GET_d_flag;
		global $RosCMS_GET_d_arch;
		
		global $roscms_intern_account_id;
		global $roscms_security_level;
		global $RosCMS_GET_branch;
				
		global $h_a;
		global $h_a2;
	
	?>
		<div class="detailbody">
			<div class="detailmenubody">
				<?php
					if ($tmode == 0) {
						echo '<b>Metadata</b>';
					}
					else {
						echo '<span class="detailmenu" onclick="';
						echo "bshowtag(".$RosCMS_GET_d_id.",".$RosCMS_GET_d_r_id.",'a','b', '".$roscms_intern_account_id."')";
						echo '">Metadata</span>';
					}
					
					echo "&nbsp;|&nbsp;";
					
					if ($tmode == 2) {
						echo '<b>History</b>';
					}
					else {
						echo '<span class="detailmenu" onclick="';
						echo "bshowhistory(".$RosCMS_GET_d_id.",".$RosCMS_GET_d_r_id.",'a','b', '".$roscms_intern_account_id."')";
						echo '">History</span>';
					}
					
					if (roscms_security_check_kind($RosCMS_GET_d_id, "add")) { // allowed only for someone with "add" rights
						echo "&nbsp;|&nbsp;";
						
						if ($tmode == 1) {
							echo '<b>Fields</b>';
						}
						else {
							echo '<span class="detailmenu" onclick="';
							echo "balterfields(".$RosCMS_GET_d_id.",".$RosCMS_GET_d_r_id.", '".$roscms_intern_account_id."')";
							echo '">Fields</span>';
						}
					}

					if (roscms_security_check_kind($RosCMS_GET_d_id, "add")) { 
						echo "&nbsp;|&nbsp;";
						
						if ($tmode == 4) {
							echo '<b>Entry</b>';
						}
						else {
							echo '<span class="detailmenu" onclick="';
							echo "bshowentry(".$RosCMS_GET_d_id.",".$RosCMS_GET_d_r_id.", '".$roscms_intern_account_id."')";
							echo '">Entry</span>';
						}
					}
					
					if ($roscms_security_level == 3 && 
						(roscms_security_grp_member("ros_sadmin") || 
							(roscms_security_check_kind($RosCMS_GET_d_id, "add") && 
							roscms_security_grp_member("ros_admin")))) 
					{ // allowed only for related super administrators
						
						echo "&nbsp;|&nbsp;";
						
						if ($tmode == 3) {
							echo '<b>Security</b>';
						}
						else {
							echo '<span class="detailmenu" onclick="';
							echo "bshowsecurity(".$RosCMS_GET_d_id.",".$RosCMS_GET_d_r_id.", '".$roscms_intern_account_id."')";
							echo '">Security</span>';
						}
					}
				?>
			</div>
		</div>
	<?php
		if ($tmode == 0) { // metadata
			// Tags
			$t_counter_etagusr = "";
			
			if ($roscms_security_level > 1) {
				$tmpfilt = "a.tag_usrid = '-1' OR ";
			}
			else {
				$tmpfilt = "";
			}
			
			$query_edit_mef_data_tag = mysql_query("SELECT a.tag_id, a.tag_usrid, n.tn_name, v.tv_value 
													FROM data_".$h_a2." d, data_revision".$h_a." r, data_tag".$h_a." a, data_tag_name".$h_a." n, data_tag_value".$h_a." v
													WHERE (a.data_id = '0' OR (
																a.data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."'
																AND a.data_id = d.data_id
															)
														)
													AND (a.data_rev_id = '0' OR (
																a.data_rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."'
																AND a.data_rev_id = r.rev_id
															)
														)
													AND (".$tmpfilt."a.tag_usrid = '0' OR a.tag_usrid = '".mysql_real_escape_string($roscms_intern_account_id)."')
													AND a.tag_name_id = n.tn_id 
													AND a.tag_value_id  = v.tv_id 
													ORDER BY tag_usrid ASC, tn_name ASC;");
			while($result_edit_mef_data_tag = mysql_fetch_array($query_edit_mef_data_tag)) {
				if ($result_edit_mef_data_tag['tag_usrid'] != $t_counter_etagusr) {
					if ($result_edit_mef_data_tag['tag_usrid'] == "-1") {
						echo "<br /><div class=\"frmeditheadline\">System Metadata</div>";
					}
					else if ($result_edit_mef_data_tag['tag_usrid'] == "0") {
						echo "<br /><div class=\"frmeditheadline\">Labels</div>";
					}
					else if ($result_edit_mef_data_tag['tag_usrid'] == $roscms_intern_account_id) {
						echo "<br /><div class=\"frmeditheadline\">Private Labels</div>";
					}
				}
				$t_counter_etagusr = $result_edit_mef_data_tag['tag_usrid'];
				echo "<b>".$result_edit_mef_data_tag['tn_name'].":</b>&nbsp;".$result_edit_mef_data_tag['tv_value'];
				
				// show delete button
				if (($roscms_security_level > 1 && $result_edit_mef_data_tag['tag_usrid'] == "0") || // allow to delete label if SecLev > 1
					(roscms_security_check_kind($RosCMS_GET_d_id, "add") && $result_edit_mef_data_tag['tag_usrid'] == "-1") || // allow to delete sys metadata if user has the rights
					($result_edit_mef_data_tag['tag_usrid'] == $roscms_intern_account_id && $result_edit_mef_data_tag['tag_usrid'] > 0)) // allow someone to delete metadata his he set and the user-id > 0
				{
					
					echo "&nbsp;&nbsp;<span class=\"frmeditbutton\" onclick=\"bdeltag(".$RosCMS_GET_d_id.",".$RosCMS_GET_d_r_id.",'".$result_edit_mef_data_tag['tag_id']."', '".$roscms_intern_account_id."')\"><img src=\"images/remove.gif\" alt=\"\" style=\"width:11px; height:11px; border:0px;\" />&nbsp;Delete</span>";
				}

				echo "<br />";
			}
			?>
			<br /><br /><div class="frmeditheadline">Add Private Label</div>
			
			<label for="addtagn"><b>Tag:</b></label>&nbsp;
			<input type="text" id="addtagn" size="15" maxlength="100" value="" />&nbsp;<button type="button" onclick="<?php
				echo "baddtag(".$RosCMS_GET_d_id.",".$RosCMS_GET_d_r_id.",'tag','addtagn', '".$roscms_intern_account_id."')";
			?>">Add</button><br />			
			
			<?php 
				if ($roscms_security_level > 1) {
			?>
				<br /><div class="frmeditheadline">Add Label<?php if (roscms_security_check_kind($RosCMS_GET_d_id, "add")) { echo " or System Metadata"; } ?></div>
				<label for="addtags1"><b>Name:</b></label>&nbsp;
				<input type="text" id="addtags1" size="15" maxlength="100" value="" />&nbsp;
				<label for="addtags2"><b>Value:</b></label>&nbsp;
				<input type="text" id="addtags2" size="15" maxlength="100" value="" /> &nbsp;
				
				
				<button type="button" onclick="<?php
					echo "baddtag(".$RosCMS_GET_d_id.",".$RosCMS_GET_d_r_id.",'addtags1','addtags2','0')";
				?>">Add Label</button>&nbsp;
				
				<?php
					if (roscms_security_check_kind($RosCMS_GET_d_id, "add")) { // allowed only for someone with "add" rights
						?><button type="button" onclick="<?php
							echo "baddtag(".$RosCMS_GET_d_id.",".$RosCMS_GET_d_r_id.",'addtags1','addtags2','-1')";
						?>">Add Sys</button>
			<?php
					}
				}
			?>
			<br />
			<br />
	<?php
		}
		else if ($tmode == 1) { // edit fields
	?>
		
		<?php
			echo '<br /><div class="frmeditheadline">Short Text</div><br />';
			
			$t_counter_stext = 0;
			$query_edit_mef_data_stext = mysql_query("SELECT s.stext_name, s.stext_content 
														FROM data_revision".$h_a." r, data_stext".$h_a." s
														WHERE r.rev_id = s.data_rev_id
														AND r.rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."'
														ORDER BY stext_name ASC;");
			while($result_edit_mef_data_stext = mysql_fetch_array($query_edit_mef_data_stext)) {
				$t_counter_stext++;
				?>
				<input type="text" name="editstext<?php echo $t_counter_stext; ?>" id="editstext<?php echo $t_counter_stext; ?>" size="25" maxlength="100" value="<?php echo $result_edit_mef_data_stext['stext_name']; ?>" /> 
				<input type="checkbox" name="editstextdel<?php echo $t_counter_stext; ?>" id="editstextdel<?php echo $t_counter_stext; ?>" value="del" /><label for="editstextdel<?php echo $t_counter_stext; ?>">delete?</label>
				<input name="editstextorg<?php echo $t_counter_stext; ?>" id="editstextorg<?php echo $t_counter_stext; ?>" type="hidden" value="<?php echo $result_edit_mef_data_stext['stext_name']; ?>" />
				<br /><br />
				<?php
			}
			echo "<div id=\"editaddstext\"></div>";
			echo "<span id=\"editaddstextcount\" style=\"display: none;\">".$t_counter_stext."</span>";
			echo '<span class="filterbutton" onclick="editaddshorttext()"><img src="images/add.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Add</span>';


			echo '<br /><br /><br /><div class="frmeditheadline">Text</div><br />';
			
			$t_counter_text = 0;
			$query_edit_mef_data_text = mysql_query("SELECT t.text_name, t.text_content 
														FROM data_revision".$h_a." r, data_text".$h_a." t
														WHERE r.rev_id = t.data_rev_id
														AND r.rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."'
														ORDER BY text_name ASC;");
			while($result_edit_mef_data_text = mysql_fetch_array($query_edit_mef_data_text)) {
				$t_counter_text++;
				?>
				<input type="text" name="edittext<?php echo $t_counter_text; ?>" id="edittext<?php echo $t_counter_text; ?>" size="25" maxlength="100" value="<?php echo $result_edit_mef_data_text['text_name']; ?>" /> 
				<input type="checkbox" name="edittextdel<?php echo $t_counter_text; ?>" id="edittextdel<?php echo $t_counter_text; ?>" value="del" /><label for="edittextdel<?php echo $t_counter_text; ?>">delete?</label>
				<input name="edittextorg<?php echo $t_counter_text; ?>" id="edittextorg<?php echo $t_counter_text; ?>" type="hidden" value="<?php echo $result_edit_mef_data_text['text_name']; ?>" />
				<br /><br />
				<?php
			}
			echo "<div id=\"editaddtext\"></div>";
			echo "<span id=\"editaddtextcount\" style=\"display: none;\">".$t_counter_text."</span>";
			echo '<span class="filterbutton" onclick="editaddtext()"><img src="images/add.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Add</span>';

			echo '<br /><br /><br /><button type="button" id="beditsavefields" onclick="editsavefieldchanges('.$RosCMS_GET_d_id.','.$RosCMS_GET_d_r_id.')">Save Changes</button> &nbsp; ';
			echo " <button type=\"button\" id=\"beditclear\" onclick=\"balterfields(".$RosCMS_GET_d_id.",".$RosCMS_GET_d_r_id.", '".$roscms_intern_account_id."')\">Clear</button>";
			
			
		}
		else if ($tmode == 2) { // versions history
		
			$query_get_data_name = mysql_query("SELECT data_name, data_type 
											FROM data_".$h_a2." 
											WHERE data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."'
											LIMIT 1;");
			$result_get_data_name = mysql_fetch_array($query_get_data_name);

			//echo "<p>".$result_get_data_name['data_name']."-".$result_get_data_name['data_type']."</p>";

			if ($RosCMS_GET_d_arch) { // archive mode
				$query_get_data_name = mysql_query("SELECT data_name, data_type 
												FROM data_  
												WHERE data_name = '".mysql_real_escape_string($result_get_data_name['data_name'])."'
												AND data_type = '".mysql_real_escape_string($result_get_data_name['data_type'])."'
												LIMIT 1;");
				$result_get_data_name = mysql_fetch_array($query_get_data_name);
				
				$tmp_d_name = $result_get_data_name['data_name'];
				$tmp_d_type = $result_get_data_name['data_type'];
			}
			else {
				$tmp_d_name = $result_get_data_name['data_name'];
				$tmp_d_type = $result_get_data_name['data_type'];
			}
			
			//echo "<p>".$tmp_d_name."-".$tmp_d_type."</p>";
			
			
			for ($i = 0; $i < 2; $i++) { // first loop = normal, second lopp = archive
				echo '<br /><div class="frmeditheadline">Versions History';
				if ($i == 1) {
					echo " - Archive";
					$tmp_a = "_a";
					$tmp_a2 = "a";
				}
				else {
					$tmp_a = "";
					$tmp_a2 = "";
				}
				echo '</div><br />';
	
				
				$temp_cur_lang = "";
				
				$query_diff1_cbm = mysql_query("SELECT d.data_id, d.data_name, r.rev_id, r.rev_language,r.rev_version, r.rev_datetime, u.user_name, l.lang_name  
												FROM data_".$tmp_a2." d, data_revision".$tmp_a." r, languages l, users u 
												WHERE d.data_name = '".mysql_real_escape_string($tmp_d_name)."'
												AND d.data_type = '".mysql_real_escape_string($tmp_d_type)."'
												AND r.data_id = d.data_id
												AND r.rev_language = l.lang_id 
												AND r.rev_version > 0 
												AND u.user_id = r.rev_usrid 
												ORDER BY l.lang_name  ASC, r.rev_datetime DESC;");
				while ($result_diff_cbm = mysql_fetch_array($query_diff1_cbm)) {
					if ($result_diff_cbm['rev_language'] != $temp_cur_lang) {
						if ($temp_cur_lang != "") {
							echo "</ul>";
						}
			
						echo "<p><b>".$result_diff_cbm['lang_name']."</b></p><ul>"; 
						
						$temp_cur_lang = $result_diff_cbm['rev_language'];
					}
					
					echo "<li>";
					if ($result_diff_cbm['rev_id'] == $RosCMS_GET_d_r_id) {
						echo "<u>";
					}
					echo $result_diff_cbm['data_name'];
					
					$temp_dynamic = getTagValue($result_diff_cbm['data_id'], $result_diff_cbm['rev_id'],  '-1', 'number');
					if ($temp_dynamic != "") {
						echo "_".$temp_dynamic;
					}
					
					echo " (".$result_diff_cbm['rev_datetime'].") - v. ".$result_diff_cbm['rev_version']."; ".$result_diff_cbm['user_name'];
					if ($result_diff_cbm['rev_id'] == $RosCMS_GET_d_r_id) {
						echo "</u>";
					}
					echo "</li>";
				}
				echo "</ul>";
			}
		}
		else if ($tmode == 3) { // entry security
			
			$query_data_change = mysql_query("SELECT * 
											FROM data_".$h_a2." 
											WHERE data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."'
											LIMIT 1;");
			$result_data_change = mysql_fetch_array($query_data_change);
						
			echo '<br /><div class="frmeditheadline">Data-ID</div><br />';
			echo "<div>".$result_data_change['data_id']."</div>";
			echo '<br /><div class="frmeditheadline">Name</div><br />';
			echo '<input type="text" name="secdataname" id="secdataname" size="25" maxlength="100" value="'.$result_data_change['data_name'].'" /> (ASCII lowercase, no space) <img src="images/attention.gif" width="22" height="22" /><br /><br />';
			echo '<input type="checkbox" name="chdname" id="chdname" value="update" checked="checked" /><label for="chdname">update all links to this data-name</label><br />';
			echo '<br /><div class="frmeditheadline">Type</div><br />';
			echo '<select id="cbmdatatype">';
				echo '<option value="page"';
					if ($result_data_change['data_type'] == "page") echo ' selected="selected"'; 
				echo '>Page</option>';
				echo '<option value="content"';
					if ($result_data_change['data_type'] == "content") echo ' selected="selected"'; 
				echo '>Content</option>';
				echo '<option value="template"';
					if ($result_data_change['data_type'] == "template") echo ' selected="selected"'; 
				echo '>Template</option>';
				echo '<option value="script"';
					if ($result_data_change['data_type'] == "script") echo ' selected="selected"'; 
				echo '>Script</option>';
				echo '<option value="system"';
					if ($result_data_change['data_type'] == "system") echo ' selected="selected"'; 
				echo '>System</option>';
			echo "</select><br />";
			echo '<br /><div class="frmeditheadline">ACL</div><br />';
			echo '<select id="cbmdataacl" name="cbmdataacl">';
				$query_data_acl = mysql_query("SELECT sec_name, sec_fullname  
												FROM data_security 
												WHERE sec_branch = '".mysql_real_escape_string($RosCMS_GET_branch)."'
												ORDER BY sec_fullname ASC;");
				while ($result_data_acl = mysql_fetch_array($query_data_acl)) {
					echo '<option value="'.$result_data_acl['sec_name'].'"';
					
					if ($result_data_acl['sec_name'] == $result_data_change['data_acl']) {
						echo ' selected="selected"'; 
					}
					
					echo '>'.$result_data_acl['sec_fullname'].'</option>';
				}
			echo '</select>  <img src="images/attention.gif" width="22" height="22" />';

			echo '<br /><br /><br /><button type="button" id="beditsavefields" onclick="editsavesecuritychanges('.$RosCMS_GET_d_id.','.$RosCMS_GET_d_r_id.')">Save Changes</button> &nbsp; ';
			echo " <button type=\"button\" id=\"beditclear\" onclick=\"bshowsecurity(".$RosCMS_GET_d_id.",".$RosCMS_GET_d_r_id.", '".$roscms_intern_account_id."')\">Clear</button>";
		}
		else if ($tmode == 4) { // entry details
			$query_data_rev_change = mysql_query("SELECT * 
											FROM data_".$h_a2." d, data_revision".$h_a." r, users u 
											WHERE r.rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."'
											AND r.data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."'
											AND d.data_id = r.data_id 
											AND u.user_id = r.rev_usrid 
											LIMIT 1;");
			$result_data_rev_change = mysql_fetch_array($query_data_rev_change);
						
			echo '<br /><div class="frmeditheadline">Rev-ID</div><br />';
			echo "<div>".$result_data_rev_change['rev_id']."</div>";
			echo '<br /><div class="frmeditheadline">Language</div><br />';
			echo '<select id="cbmentrylang" name="cbmentrylang">';
				$query_data_lang = mysql_query("SELECT lang_id, lang_name 
												FROM languages 
												ORDER BY lang_name ASC;");
				while ($result_data_lang = mysql_fetch_array($query_data_lang)) {
					echo '<option value="'.$result_data_lang['lang_id'].'"';
					
					if ($result_data_lang['lang_id'] == $result_data_rev_change['rev_language']) {
						echo ' selected="selected"'; 
					}
					
					echo '>'.$result_data_lang['lang_name'].'</option>';
				}
			echo "</select><br />";
			echo '<br /><div class="frmeditheadline">Version</div><br />';
			echo '<input type="text" name="vernbr" id="vernbr" size="5" maxlength="11" value="'.$result_data_rev_change['rev_version'].'" /><br />';
			echo '<br /><div class="frmeditheadline">User</div><br />';
			echo '<input type="text" name="verusr" id="verusr" size="20" maxlength="20" value="'.$result_data_rev_change['user_name'].'" /> (account name) <img src="images/attention.gif" width="22" height="22" /><br />';
			echo '<br /><div class="frmeditheadline">Date</div><br />';
			echo '<input type="text" name="verdate" id="verdate" size="10" maxlength="10" value="'.$result_data_rev_change['rev_date'].'" /> (year-month-day) <img src="images/attention.gif" width="22" height="22" /><br />';
			echo '<br /><div class="frmeditheadline">Time</div><br />';
			echo '<input type="text" name="vertime" id="vertime" size="8" maxlength="8" value="'.$result_data_rev_change['rev_time'].'" /> (hour:minute:second) <img src="images/attention.gif" width="22" height="22" /><br />';

			echo '<br /><div class="frmeditheadline">Move Entry</div><br />';
			echo '<input type="text" name="chgdataname" id="chgdataname" size="25" maxlength="100" value="'.$result_data_rev_change['data_name'].'" /> ';
			echo '<select id="cbmchgdatatype">';
				echo '<option value="page"';
					if ($result_data_rev_change['data_type'] == "page") echo ' selected="selected"'; 
				echo '>Page</option>';
				echo '<option value="content"';
					if ($result_data_rev_change['data_type'] == "content") echo ' selected="selected"'; 
				echo '>Content</option>';
				echo '<option value="template"';
					if ($result_data_rev_change['data_type'] == "template") echo ' selected="selected"'; 
				echo '>Template</option>';
				echo '<option value="script"';
					if ($result_data_rev_change['data_type'] == "script") echo ' selected="selected"'; 
				echo '>Script</option>';
				echo '<option value="system"';
					if ($result_data_rev_change['data_type'] == "system") echo ' selected="selected"'; 
				echo '>System</option>';
			echo '</select> <img src="images/attention.gif" width="22" height="22" /><br /><br />';


			echo '<br /><button type="button" id="beditsaveentry" onclick="editsaveentrychanges('.$RosCMS_GET_d_id.','.$RosCMS_GET_d_r_id.')">Save Changes</button> &nbsp; ';
			echo " <button type=\"button\" id=\"beditclear\" onclick=\"bshowentry(".$RosCMS_GET_d_id.",".$RosCMS_GET_d_r_id.", '".$roscms_intern_account_id."')\">Clear</button>";
		}
	}
	
	function show_edit_data2() {
		echo "</div></div>";
	}
	
	function show_edit_data_form($RosCMS_intern_data_edit_stext, $RosCMS_intern_data_edit_text) {
		global $RosCMS_GET_debug;
		global $roscms_standard_language;
		global $roscms_standard_language_full;
		global $roscms_security_level;
		global $RosCMS_GET_d_id;
		global $RosCMS_GET_d_r_id;
		global $RosCMS_GET_d_arch;
		global $h_a;
		global $h_a2;
		?>
		<div style="background:#FFFFFF; border-bottom: 1px solid #bbb; border-bottom-width: 1px; border-right: 1px solid #bbb; border-right-width: 1px;"><div style="margin:10px;"><div style="width:95%;">
		<form method="post" action="#"><br />
		<?php
	
		if ($RosCMS_intern_data_edit_stext == true) {
			// Short Text
			$t_counter_stext = 0;
			$query_edit_mef_data_stext = mysql_query("SELECT s.stext_name, s.stext_content 
														FROM data_revision".$h_a." r, data_stext".$h_a." s
														WHERE r.rev_id = s.data_rev_id
														AND r.rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."'
														ORDER BY stext_name ASC;");
			while($result_edit_mef_data_stext = mysql_fetch_array($query_edit_mef_data_stext)) {
				$t_counter_stext++;
				?>
				<label for="estext"<?php echo $t_counter_stext; ?>" class="frmeditheadline"><b><?php echo ucfirst($result_edit_mef_data_stext['stext_name']); ?>:</b></label><span id="edstext<?php echo $t_counter_stext; ?>" style="display:none;"><?php echo $result_edit_mef_data_stext['stext_name']; ?></span><br />
				<input name="estext"<?php echo $t_counter_stext; ?>" type="text" id="estext<?php echo $t_counter_stext; ?>" size="50" maxlength="250" value="<?php echo $result_edit_mef_data_stext['stext_content']; ?>" /><br /><br />
				<?php
			}
			?>
			<span id="entrydataid" class="<?php echo $RosCMS_GET_d_id; ?>">&nbsp;</span><span id="entrydatarevid" class="<?php echo $RosCMS_GET_d_r_id; ?>">&nbsp;</span><div id="estextcount" class="<?php echo $t_counter_stext; ?>">&nbsp;</div>
			<?php
		}
		
		if ($RosCMS_intern_data_edit_text == true) {
			// Text
			$t_counter_text2 = 0;
			$t_text_lang = "";
			$query_edit_mef_data_text = mysql_query("SELECT t.text_name, t.text_content, r.rev_language  
														FROM data_revision".$h_a." r, data_text".$h_a." t
														WHERE r.rev_id = t.data_rev_id
														AND r.rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."'
														ORDER BY text_name ASC;");
			while($result_edit_mef_data_text = mysql_fetch_array($query_edit_mef_data_text)) {
				$t_counter_text2++;
				?>
				<label class="frmeditheadline" for="elm<?php echo $t_counter_text2; ?>"><?php echo ucfirst($result_edit_mef_data_text['text_name']); ?></label>
				<button type="button" id="butRTE<?php echo $t_counter_text2; ?>" onclick="toggleEditor('elm<?php echo $t_counter_text2; ?>', this.id)">Rich Text</button> <span id="swraped<?php echo $t_counter_text2; ?>">
				<input id="wraped<?php echo $t_counter_text2; ?>" type="checkbox" onclick="toggleWordWrap(this.id, 'elm<?php echo $t_counter_text2; ?>');" checked="checked" /><label for="wraped<?php echo $t_counter_text2; ?></label>">Word wrap</label></span>
				<span id="edtext<?php echo $t_counter_text2; ?>" style="display:none;"><?php echo $result_edit_mef_data_text['text_name']; ?></span><br />
				<textarea name="elm<?php echo $t_counter_text2; ?>" cols="80" rows="15" class="mceEditor" id="elm<?php echo $t_counter_text2; ?>" style="width: 100%; background-color:#FFFFFF;" ><?php echo $result_edit_mef_data_text['text_content']; ?></textarea>
				<br />
				<br />
				<?php
				$t_text_lang = $result_edit_mef_data_text['rev_language'];
			}
			?>
			<span id="elmcount" class="<?php echo $t_counter_text2; ?>">&nbsp;</span>
			<?php
		}
		?></form>
		<?php
			if (roscms_security_check_kind ($RosCMS_GET_d_id, "write")) {
		?>
			<?php
				if (roscms_security_check_kind($RosCMS_GET_d_id, "pub") && $roscms_security_level == 3) { // user with publish rights to speed up workflow (by-pass draft view) 
				/*
			?>
				<button type="button" id="bsavenew" onclick="edit_form_submit(<?php echo $RosCMS_GET_d_id.','.$RosCMS_GET_d_r_id; ?>)">Submit</button> &nbsp; 
			<?php*/
				}
			?>
				<button type="button" id="bsavedraft" onclick="edit_form_submit_draft(<?php echo $RosCMS_GET_d_id.','.$RosCMS_GET_d_r_id; ?>)">Save as Draft</button> &nbsp;<input name="editautosavemode" type="hidden" value="true" />
		<?php
			}
			else {
		?>
				<!--<button type="button" id="bsavenew" disabled="disabled">Submit</button> &nbsp; -->
				<button type="button" id="bsavedraft" disabled="disabled">Save as Draft</button> &nbsp; <img src="images/locked.gif" width="11" height="12" /> (not enough rights) &nbsp;<input name="editautosavemode" type="hidden" value="false" />
		<?php
			}
			
			//echo "<p>".$t_text_lang." vs. ".$roscms_standard_language."</p>";
			
			//if ($t_text_lang != $roscms_standard_language) {
				$query_get_data_name = mysql_query("SELECT data_name, data_type 
												FROM data_".$h_a2." 
												WHERE data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."'
												LIMIT 1;");
				$result_get_data_name = mysql_fetch_array($query_get_data_name);
		
				if ($RosCMS_GET_d_arch) { // archive mode
					$query_get_data_name = mysql_query("SELECT data_name, data_type 
													FROM data_  
													WHERE data_name = '".mysql_real_escape_string($result_get_data_name['data_name'])."'
													AND data_type = '".mysql_real_escape_string($result_get_data_name['data_type'])."'
													LIMIT 1;");
					$result_get_data_name = mysql_fetch_array($query_get_data_name);
					
					$tmp_d_name = $result_get_data_name['data_name'];
					$tmp_d_type = $result_get_data_name['data_type'];
				}
				else {
					$tmp_d_name = $result_get_data_name['data_name'];
					$tmp_d_type = $result_get_data_name['data_type'];
				}


				$query_choose_diff_count = mysql_query("SELECT COUNT(*)
													FROM data_a d, data_revision_a r 
													WHERE d.data_name = '".mysql_real_escape_string($tmp_d_name)."'
													AND d.data_id = r.data_id
													AND r.rev_version > 0 
													AND r.rev_language = '".mysql_real_escape_string($roscms_standard_language)."'
													ORDER BY r.rev_id DESC;");
				$result_choose_diff_count = mysql_fetch_row($query_choose_diff_count);


				$query_choose_diff_count_ar = mysql_query("SELECT COUNT(*)
													FROM data_a d, data_revision_a r 
													WHERE d.data_name = '".mysql_real_escape_string($tmp_d_name)."'
													AND d.data_id = r.data_id
													AND r.rev_version > 0 
													AND r.rev_language = '".mysql_real_escape_string($roscms_standard_language)."'
													ORDER BY r.rev_id DESC;");
				$result_choose_diff_count_ar = mysql_fetch_row($query_choose_diff_count_ar);

				if ($result_choose_diff_count[0] <= 0 || $result_choose_diff_count[0] <= 0) {
					?>
					<span id="bshowdiff" class="frmeditbutton" onclick="diffentries3(<?php echo $RosCMS_GET_d_r_id .",". $RosCMS_GET_d_r_id; ?>)"><img id="bshowdiffi" src="images/tab_closed.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Compare</span> (no related <?php echo strtolower($roscms_standard_language_full); ?> entry, choose yourself)&nbsp; 
					<?php
				}
				else {
					if ($RosCMS_GET_d_arch) {
						$query_choose_diff_ar = mysql_query("SELECT r.rev_id
															FROM data_a d, data_revision_a r 
															WHERE d.data_name = '".mysql_real_escape_string($tmp_d_name)."'
															AND d.data_id = r.data_id
															AND r.rev_version > 0 
															AND r.rev_language = '".mysql_real_escape_string($roscms_standard_language)."'
															ORDER BY r.rev_id DESC
															LIMIT 2;");
						$result_choose_diff2 = mysql_fetch_array($query_choose_diff_ar);
						$result_choose_diff1 = mysql_fetch_array($query_choose_diff_ar);
						//echo "<p>".$result_choose_diff1['rev_id']." vs. ".$result_choose_diff2['rev_id']."</p>";
					}
					else {
						$query_choose_diff = mysql_query("SELECT r.rev_id
															FROM data_ d, data_revision r 
															WHERE d.data_name = '".mysql_real_escape_string($tmp_d_name)."'
															AND d.data_id = r.data_id
															AND r.rev_version > 0 
															AND r.rev_language = '".mysql_real_escape_string($roscms_standard_language)."'
															ORDER BY r.rev_id DESC
															LIMIT 1;");
						$result_choose_diff2 = mysql_fetch_array($query_choose_diff);
						
						$query_choose_diff_ar = mysql_query("SELECT r.rev_id
															FROM data_a d, data_revision_a r 
															WHERE d.data_name = '".mysql_real_escape_string($tmp_d_name)."'
															AND d.data_id = r.data_id
															AND r.rev_version > 0 
															AND r.rev_language = '".mysql_real_escape_string($roscms_standard_language)."'
															ORDER BY r.rev_id DESC
															LIMIT 1;");
						$result_choose_diff1 = mysql_fetch_array($query_choose_diff_ar);
					}
					
					$t_diff1 = $result_choose_diff1['rev_id'];
					$t_diff2 = $result_choose_diff2['rev_id'];
					
					
					if ($RosCMS_GET_d_arch) {
						$t_diff1 = "ar".$t_diff1;
						$t_diff2 = "ar".$t_diff2;
					}
					else {
						$t_diff1 = "ar".$t_diff1;
					}
					
				
					?>
					<span id="bshowdiff" class="frmeditbutton" onclick="diffentries3('<?php echo $t_diff1 ."','". $t_diff2; ?>')"><img id="bshowdiffi" src="images/tab_closed.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Compare</span> &nbsp; 
					<?php
				}
			/*}
			else {
				?>
				<span id="bshowdiff" class="frmeditbutton" onclick="diffentries3(<?php echo $RosCMS_GET_d_r_id .",". $RosCMS_GET_d_r_id; ?>)"><img id="bshowdiffi" src="images/tab_closed.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Compare</span> (select two different entries)&nbsp; 
				<?php
			}		*/
		?>
		&nbsp;&nbsp;<span id="mefasi">&nbsp;</div>
		</div></div>
		<div id="frmdiff" style="display:none;"></div>
		<?php
	}
	
	
	function tag_add($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_GET_d_value, $RosCMS_GET_d_value2, $RosCMS_GET_d_value3) {
		global $roscms_intern_account_id;
		global $h_a;
		global $h_a2;
		
//		echo "<h4>TAG_ADD: ".$RosCMS_GET_d_id.", ".$RosCMS_GET_d_r_id.", ".$RosCMS_GET_d_value.", ".$RosCMS_GET_d_value2.", ".$RosCMS_GET_d_value3." == ".$roscms_intern_account_id."</h4>";

		$RosCMS_intern_current_tag_name_id = "";
		$RosCMS_intern_current_tag_value_id = "";

		if ($RosCMS_GET_d_value3 == $roscms_intern_account_id || $RosCMS_GET_d_value3 == 0|| $RosCMS_GET_d_value3 == -1) { /* @unimplemented: account group membership check */

			// tag name
			$query_edit_mef_tag_add_tn = mysql_query("SELECT tn_id, tn_name 
														FROM data_tag_name".$h_a."
														WHERE tn_name = '".mysql_real_escape_string($RosCMS_GET_d_value)."'
														LIMIT 1;");
			$result_edit_mef_tag_add_tn = mysql_fetch_array($query_edit_mef_tag_add_tn);
			if ($result_edit_mef_tag_add_tn['tn_name'] == $RosCMS_GET_d_value) {
				$RosCMS_intern_current_tag_name_id = $result_edit_mef_tag_add_tn['tn_id'];
			}
			else {
				// add tag name:
				$insert_edit_mef_tag_add_tn = mysql_query("INSERT INTO data_tag_name ( tn_id , tn_name ) 
															VALUES ( NULL , '".mysql_real_escape_string($RosCMS_GET_d_value)."' );");
				
				$query_edit_mef_tag_add_tn2 = mysql_query("SELECT tn_id, tn_name 
															FROM data_tag_name".$h_a." 
															WHERE tn_name = '".mysql_real_escape_string($RosCMS_GET_d_value)."'
															LIMIT 1;");
				$result_edit_mef_tag_add_tn2 = mysql_fetch_array($query_edit_mef_tag_add_tn2);
				if ($result_edit_mef_tag_add_tn2['tn_name'] == $RosCMS_GET_d_value) {
					$RosCMS_intern_current_tag_name_id = $result_edit_mef_tag_add_tn2['tn_id'];
				}
				else {
					die("ERROR: tag_add_name was not successful");
				}			
			}
	
//			echo "<p>TAG_NAME_ID: ".$RosCMS_intern_current_tag_name_id."</p>";
	
			// tag value
			$query_edit_mef_tag_add_tv = mysql_query("SELECT tv_id, tv_value 
														FROM data_tag_value".$h_a." 
														WHERE tv_value = '".mysql_real_escape_string($RosCMS_GET_d_value2)."'
														LIMIT 1;");
			$result_edit_mef_tag_add_tv = mysql_fetch_array($query_edit_mef_tag_add_tv);
			if ($result_edit_mef_tag_add_tv['tv_value'] == $RosCMS_GET_d_value2) {
				$RosCMS_intern_current_tag_value_id = $result_edit_mef_tag_add_tv['tv_id'];
			}
			else {
				// add tag value:
				$insert_edit_mef_tag_add_tv = mysql_query("INSERT INTO data_tag_value".$h_a." ( tv_id , tv_value ) 
															VALUES ( NULL , '".mysql_real_escape_string($RosCMS_GET_d_value2)."' );");
				
				$query_edit_mef_tag_add_tv2 = mysql_query("SELECT tv_id, tv_value 
															FROM data_tag_value".$h_a."
															WHERE tv_value = '".mysql_real_escape_string($RosCMS_GET_d_value2)."'
															LIMIT 1;");
				$result_edit_mef_tag_add_tv2 = mysql_fetch_array($query_edit_mef_tag_add_tv2);
				if ($result_edit_mef_tag_add_tv2['tv_value'] == $RosCMS_GET_d_value2) {
					$RosCMS_intern_current_tag_value_id = $result_edit_mef_tag_add_tv2['tv_id'];
				}
				else {
					die("ERROR: tag_add_value was not successful");
				}			
			}
//			echo "<p>TAG_VALUE_ID: ".$RosCMS_intern_current_tag_value_id."</p>";

			// tag
			$query_edit_mef_tag_add = mysql_query("SELECT COUNT('tag_id') 
													FROM data_tag".$h_a." 
													WHERE data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."'
													AND data_rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."'
													AND tag_name_id = '".mysql_real_escape_string($RosCMS_intern_current_tag_name_id)."'
													AND tag_value_id = '".mysql_real_escape_string($RosCMS_intern_current_tag_value_id)."'
													AND tag_usrid = '".mysql_real_escape_string($RosCMS_GET_d_value3)."' ;");
			$result_edit_mef_tag_add = mysql_fetch_row($query_edit_mef_tag_add);
			
//			echo "<p>ANZAHL_TAGS: ".$result_edit_mef_tag_add[0]."</p>";
			
			if ($result_edit_mef_tag_add[0] <= 0) {
				$insert_edit_mef_tag_add = mysql_query("INSERT INTO data_tag".$h_a." ( tag_id , data_id , data_rev_id , tag_name_id , tag_value_id , tag_usrid ) 
														VALUES (NULL , 
														'".mysql_real_escape_string($RosCMS_GET_d_id)."',
														'".mysql_real_escape_string($RosCMS_GET_d_r_id)."',
														'".mysql_real_escape_string($RosCMS_intern_current_tag_name_id)."',
														'".mysql_real_escape_string($RosCMS_intern_current_tag_value_id)."',
														'".mysql_real_escape_string($RosCMS_GET_d_value3)."');");
/*				echo "INSERT INTO data_tag ( tag_id , data_id , data_rev_id , tag_name_id , tag_value_id , tag_usrid ) 
														VALUES (NULL , 
														'".mysql_real_escape_string($RosCMS_GET_d_id)."',
														'".mysql_real_escape_string($RosCMS_GET_d_r_id)."',
														'".mysql_real_escape_string($RosCMS_intern_current_tag_name_id)."',
														'".mysql_real_escape_string($RosCMS_intern_current_tag_value_id)."',
														'".mysql_real_escape_string($RosCMS_GET_d_value3)."');";
*/
			}
		}
	}
	
	function delete_tag ($data_id, $rev_id, $usr_id, $tag_name) {
		//echo "<p>tag_delete(".getTagId($data_id, $rev_id, $usr_id, $tag_name)."|getTagId(".$data_id.", ".$rev_id.", ".$usr_id.", ".$tag_name.")|, ".$usr_id.");</p>";
		tag_delete(getTagId($data_id, $rev_id, $usr_id, $tag_name), $usr_id);
	}
	
	function tag_delete ($RosCMS_GET_d_value, $RosCMS_GET_d_value2) {
		global $roscms_intern_account_id;
		global $roscms_security_level;
		global $h_a;
		global $h_a2;

		//echo "<h4>".$RosCMS_GET_d_value.", ".$RosCMS_GET_d_value2." # ".$roscms_intern_account_id."</h4>";

		$RosCMS_intern_current_tag_name_id = "";
		$RosCMS_intern_current_tag_value_id = "";
		
		if ($RosCMS_GET_d_value2 == $roscms_intern_account_id || $RosCMS_GET_d_value2 == 0 || $RosCMS_GET_d_value2 == -1) { /* @unimplemented: account group membership check */

			// tag
			$query_edit_mef_tag_del = mysql_query("SELECT tag_name_id, tag_value_id
													FROM data_tag".$h_a." 
													WHERE tag_id = '".mysql_real_escape_string($RosCMS_GET_d_value)."'
													LIMIT 1;");
			$result_edit_mef_tag_del = mysql_fetch_array($query_edit_mef_tag_del);
			
			// tag name
			$query_edit_mef_tag_del_tn = mysql_query("SELECT COUNT('tag_name_id')
														FROM data_tag".$h_a." 
														WHERE tag_name_id = '".mysql_real_escape_string($result_edit_mef_tag_del['tag_name_id'])."';");
			$result_edit_mef_tag_del_tn = mysql_fetch_row($query_edit_mef_tag_del_tn);
			if ($result_edit_mef_tag_del_tn[0] <= 1) {
				$delete_edit_mef_tag_del_tn = mysql_query("DELETE FROM data_tag_name".$h_a."
															WHERE tn_id = '".mysql_real_escape_string($result_edit_mef_tag_del['tag_name_id'])."'
															LIMIT 1;");
			}
	
			// tag value
			$query_edit_mef_tag_del_tv = mysql_query("SELECT COUNT('tag_value_id')
														FROM data_tag".$h_a." 
														WHERE tag_value_id = '".mysql_real_escape_string($result_edit_mef_tag_del['tag_value_id'])."';");
			$result_edit_mef_tag_del_tv = mysql_fetch_row($query_edit_mef_tag_del_tv);
			if ($result_edit_mef_tag_del_tv[0] <= 1) {
				$delete_edit_mef_tag_del_tv = mysql_query("DELETE FROM data_tag_value".$h_a."
															WHERE tv_id = '".mysql_real_escape_string($result_edit_mef_tag_del['tag_value_id'])."'
															LIMIT 1;");
			}
			
			// tag
			$delete_edit_mef_tag_del_tag = mysql_query("DELETE FROM data_tag".$h_a."
														WHERE tag_id = '".mysql_real_escape_string($RosCMS_GET_d_value)."'
														LIMIT 1;");
		}
	}	
		
		
	function deleteRevision($temprevid) {
		log_event_medium("delete entry: rev-id [rev-id: ".$temprevid."] {deleteRevision}");

	
		$delete_data_save_rev = mysql_query("DELETE FROM data_revision WHERE rev_id = '".mysql_real_escape_string($temprevid)."' LIMIT 1;");
		$delete_data_save_stext = mysql_query("DELETE FROM data_stext WHERE data_rev_id = '".mysql_real_escape_string($temprevid)."';");
		$delete_data_save_text = mysql_query("DELETE FROM data_text WHERE data_rev_id = '".mysql_real_escape_string($temprevid)."';");
		
		$query_delete_tags = mysql_query("SELECT * 
											FROM data_tag 
											WHERE data_rev_id = '".mysql_real_escape_string($temprevid)."' ;");
		while ($result_delete_tags = mysql_fetch_array($query_delete_tags)) {
			$delete_data_save_stext = mysql_query("DELETE FROM data_tag WHERE data_rev_id = '".mysql_real_escape_string($temprevid)."' LIMIT 1;");

			// delete tag name
			$query_delete_tag_name = mysql_query("SELECT * 
												FROM data_tag 
												WHERE tag_name_id = '".mysql_real_escape_string($result_delete_tags['tag_name_id'])."' ;");
			$result_delete_tag_name = mysql_fetch_row($query_delete_tag_name);
			if ($result_delete_tag_name[0] <= 0) {
				$delete_data_save_tn = mysql_query("DELETE FROM data_tag_name WHERE tn_id = '".mysql_real_escape_string($result_delete_tags['tag_name_id'])."' LIMIT 1;");
			}
			
			// delete tag value
			$query_delete_tag_value = mysql_query("SELECT * 
												FROM data_tag 
												WHERE tag_name_id = '".mysql_real_escape_string($result_delete_tags['tag_value_id'])."' ;");
			$result_delete_tag_value = mysql_fetch_row($query_delete_tag_value);
			if ($result_delete_tag_value[0] <= 0) {
				$delete_data_save_tv = mysql_query("DELETE FROM data_tag_value WHERE tn_id = '".mysql_real_escape_string($result_delete_tags['tag_value_id'])."' LIMIT 1;");
			}
		}		
	}
?>
