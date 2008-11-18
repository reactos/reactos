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
	
	function log_prep_info ($d_id, $d_rev_id) {
		global $roscms_intern_account_id;

		return " [dataid: ".$d_id."; revid: ".$d_rev_id."; userid: ".$roscms_intern_account_id."; security: ".roscms_security_explain($d_id)."] ";
	}

	function log_event_low ($log_str) {
		return log_event_intern($log_str, 1);
	}
	function log_event_medium ($log_str) {
		return log_event_intern($log_str, 2);
	}
	function log_event_high ($log_str) {
		return log_event_intern($log_str, 3);
	}

	function log_event_lang_medium ($log_str, $log_lang) {
		global $roscms_standard_language;
		
		if ($log_lang == "") {
			$log_lang = $roscms_standard_language;
		}
		
		return log_event_intern($log_str, 2, "log_website_".$log_lang."_");
	}

	function log_event_generate_low ($log_str) {
		return log_event_intern($log_str, 1, "log_website_generate_");
	}
	function log_event_generate_medium ($log_str) {
		return log_event_intern($log_str, 2, "log_website_generate_");
	}
	function log_event_generate_high ($log_str) {
		return log_event_intern($log_str, 3, "log_website_generate_");
	}


	function log_event_intern ($log_str, $log_mode = 3, $log_entry = "log_website_") {
		global $roscms_intern_account_id;
		global $roscms_standard_language;
	
		// data
		$query_log_data = mysql_query("SELECT data_id   
											FROM data_a
											WHERE data_name = '".mysql_real_escape_string($log_entry.date("Y-W"))."'
											LIMIT 1;");
		$result_log_data = mysql_fetch_array($query_log_data);
		
		//echo "<p>data-id: ".$result_log_data['data_id']."</p>";
		
		if ($result_log_data['data_id'] == "") {
			$insert_data = mysql_query("INSERT INTO data_a ( data_id, data_name, data_type, data_acl ) 
										VALUES (
											NULL , 
											'".mysql_real_escape_string($log_entry.date("Y-W"))."', 
											'system',
											'readonly'
										);");
			$query_log_data = mysql_query("SELECT data_id   
												FROM data_a
												WHERE data_name = '".mysql_real_escape_string($log_entry.date("Y-W"))."'
												LIMIT 1;");
			$result_log_data = mysql_fetch_array($query_log_data);
		}

		//echo "<p>data-id (2): ".$result_log_data['data_id']."</p>";

		// revision
		$query_log_rev = mysql_query("SELECT rev_id     
											FROM data_revision_a
											WHERE data_id = '".mysql_real_escape_string($result_log_data['data_id'])."'
											LIMIT 1;");
		$result_log_rev = mysql_fetch_array($query_log_rev);

		//echo "<p>rev-id: ".$result_log_rev['rev_id']."</p>";

		if ($result_log_rev['rev_id'] == "") {
			//echo "<p>insert rev</p>";
			$insert_revision = mysql_query("INSERT INTO data_revision_a ( rev_id , data_id , rev_version , rev_language , rev_usrid , rev_datetime , rev_date , rev_time ) 
											VALUES (
												NULL , 
												'".mysql_real_escape_string($result_log_data['data_id'])."', 
												'1', 
												'".mysql_real_escape_string($roscms_standard_language)."', 
												'".mysql_real_escape_string($roscms_intern_account_id)."', 
												NOW( ),
												CURDATE( ),
												CURTIME( )
											);");
						
			$query_log_rev = mysql_query("SELECT rev_id     
												FROM data_revision_a
												WHERE data_id = '".mysql_real_escape_string($result_log_data['data_id'])."'
												LIMIT 1;");
			$result_log_rev = mysql_fetch_array($query_log_rev);

			//echo "<p>rev-id (2): ".$result_log_rev['rev_id']."</p>";


			$insert_stext = mysql_query("INSERT INTO data_stext_a ( stext_id , data_rev_id , stext_name , stext_content ) 
											VALUES (
												NULL , 
												'".mysql_real_escape_string($result_log_rev['rev_id'])."', 
												'description', 
												'".mysql_real_escape_string($log_entry.date("Y-W"))."' 
											);");
											
			$insert_text = mysql_query("INSERT INTO data_text_a ( text_id , data_rev_id , text_name , text_content ) 
										VALUES (
											NULL , 
											'".mysql_real_escape_string($result_log_rev['rev_id'])."', 
											'low_security_log', 
											'".mysql_real_escape_string($log_entry.date("Y-W"))."'
										),
										(
											NULL , 
											'".mysql_real_escape_string($result_log_rev['rev_id'])."', 
											'medium_security_log', 
											'".mysql_real_escape_string($log_entry.date("Y-W"))."'
										),										
										(
											NULL , 
											'".mysql_real_escape_string($result_log_rev['rev_id'])."', 
											'high_security_log', 
											'".mysql_real_escape_string($log_entry.date("Y-W"))."'
										);");
										
			tag_add_intern($result_log_data['data_id'], $result_log_rev['rev_id'], 'status', 'stable', '-1');
		}

		//echo "<p>rev-id (3): ".$result_log_rev['rev_id']."</p>";
	
		
		if ($log_mode == 1) {
			$tmp_mode_str = "low_security_log";
		}		
		else if ($log_mode == 2) {
			$tmp_mode_str = "medium_security_log";
		}		
		else if ($log_mode == 3) {
			$tmp_mode_str = "high_security_log";
		}		
		
		$query_data_text = mysql_query("SELECT text_id, text_content  
											FROM data_text_a  
											WHERE data_rev_id = '".mysql_real_escape_string($result_log_rev['rev_id'])."'
											AND text_name = '".mysql_real_escape_string($tmp_mode_str)."'
											LIMIT 1;");
		$result_data_text = mysql_fetch_array($query_data_text);
		
		
		if ($result_data_text['text_id'] != "") {
			$query_sec_username = mysql_query("SELECT user_name  
													FROM users 
													WHERE user_id = '".mysql_real_escape_string($roscms_intern_account_id)."'
													LIMIT 1;");
			$result_sec_username = mysql_fetch_array($query_sec_username);
			
			$tmp_prep_str = " * ".date("Y-m-d H:i:s")." - ".$result_sec_username['user_name'].": ".$log_str."\n".$result_data_text['text_content'];
		
			$update_data_text = mysql_query("UPDATE data_text_a 
												SET text_content = '".mysql_real_escape_string($tmp_prep_str)."' 
												WHERE data_rev_id = ".mysql_real_escape_string($result_log_rev['rev_id'])." 
												AND text_name = '".mysql_real_escape_string($tmp_mode_str)."'
												LIMIT 1;");
			//echo "<p>logged! ".$result_data_text['text_id']."</p>";
			return true;
		}
		else {
			return false;
		}
	}
	
	
	// copy of data_edit tag_add function
	function tag_add_intern($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_GET_d_value, $RosCMS_GET_d_value2, $RosCMS_GET_d_value3) {
		global $roscms_intern_account_id;
		
		//echo "<h4>TAG_ADD: ".$RosCMS_GET_d_id.", ".$RosCMS_GET_d_r_id.", ".$RosCMS_GET_d_value.", ".$RosCMS_GET_d_value2.", ".$RosCMS_GET_d_value3." == ".$roscms_intern_account_id."</h4>";

		$RosCMS_intern_current_tag_name_id = "";
		$RosCMS_intern_current_tag_value_id = "";

		if ($RosCMS_GET_d_value3 == $roscms_intern_account_id || $RosCMS_GET_d_value3 == 0|| $RosCMS_GET_d_value3 == -1) { /* @unimplemented: account group membership check */

			// tag name
			$query_edit_mef_tag_add_tn = mysql_query("SELECT tn_id, tn_name 
														FROM data_tag_name_a
														WHERE tn_name = '".mysql_real_escape_string($RosCMS_GET_d_value)."'
														LIMIT 1;");
			$result_edit_mef_tag_add_tn = mysql_fetch_array($query_edit_mef_tag_add_tn);
			if ($result_edit_mef_tag_add_tn['tn_name'] == $RosCMS_GET_d_value) {
				$RosCMS_intern_current_tag_name_id = $result_edit_mef_tag_add_tn['tn_id'];
			}
			else {
				// add tag name:
				$insert_edit_mef_tag_add_tn = mysql_query("INSERT INTO data_tag_name_a ( tn_id , tn_name ) 
															VALUES ( NULL , '".mysql_real_escape_string($RosCMS_GET_d_value)."' );");
				
				$query_edit_mef_tag_add_tn2 = mysql_query("SELECT tn_id, tn_name 
															FROM data_tag_name_a 
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
	
			//echo "<p>TAG_NAME_ID: ".$RosCMS_intern_current_tag_name_id."</p>";
	
			// tag value
			$query_edit_mef_tag_add_tv = mysql_query("SELECT tv_id, tv_value 
														FROM data_tag_value_a 
														WHERE tv_value = '".mysql_real_escape_string($RosCMS_GET_d_value2)."'
														LIMIT 1;");
			$result_edit_mef_tag_add_tv = mysql_fetch_array($query_edit_mef_tag_add_tv);
			if ($result_edit_mef_tag_add_tv['tv_value'] == $RosCMS_GET_d_value2) {
				$RosCMS_intern_current_tag_value_id = $result_edit_mef_tag_add_tv['tv_id'];
			}
			else {
				// add tag value:
				$insert_edit_mef_tag_add_tv = mysql_query("INSERT INTO data_tag_value_a ( tv_id , tv_value ) 
															VALUES ( NULL , '".mysql_real_escape_string($RosCMS_GET_d_value2)."' );");
				
				$query_edit_mef_tag_add_tv2 = mysql_query("SELECT tv_id, tv_value 
															FROM data_tag_value_a
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
			//echo "<p>TAG_VALUE_ID: ".$RosCMS_intern_current_tag_value_id."</p>";

			// tag
			$query_edit_mef_tag_add = mysql_query("SELECT COUNT('tag_id') 
													FROM data_tag_a 
													WHERE data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."'
													AND data_rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."'
													AND tag_name_id = '".mysql_real_escape_string($RosCMS_intern_current_tag_name_id)."'
													AND tag_value_id = '".mysql_real_escape_string($RosCMS_intern_current_tag_value_id)."'
													AND tag_usrid = '".mysql_real_escape_string($RosCMS_GET_d_value3)."' ;");
			$result_edit_mef_tag_add = mysql_fetch_row($query_edit_mef_tag_add);
			
			//echo "<p>ANZAHL_TAGS: ".$result_edit_mef_tag_add[0]."</p>";
			
			if ($result_edit_mef_tag_add[0] <= 0) {
				$insert_edit_mef_tag_add = mysql_query("INSERT INTO data_tag_a ( tag_id , data_id , data_rev_id , tag_name_id , tag_value_id , tag_usrid ) 
														VALUES (NULL , 
														'".mysql_real_escape_string($RosCMS_GET_d_id)."',
														'".mysql_real_escape_string($RosCMS_GET_d_r_id)."',
														'".mysql_real_escape_string($RosCMS_intern_current_tag_name_id)."',
														'".mysql_real_escape_string($RosCMS_intern_current_tag_value_id)."',
														'".mysql_real_escape_string($RosCMS_GET_d_value3)."');");
			}
		}
	}

?>
