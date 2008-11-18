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
	
	global $roscms_intern_account_id;
	
	switch ($RosCMS_GET_d_use) {
		default:
			die("");
			break;
		case "mef": // main edit frame
			main_edit_frame();
			break;
		case "asi": // auto save info
			auto_save_info();
			break;
		case "ufs": // user filter storage
			user_filter_storage();
			break;
		case "uqi": // user quick info
			user_quick_info();
			break;
	}



	function main_edit_frame() {
		global $RosCMS_GET_d_flag;

		require_once("inc/data_edit.php");
	}
	
	
	function user_quick_info() { // user quick info
		global $roscms_intern_account_id;
		global $roscms_security_level;

		global $h_a;
		global $h_a2;

		global $RosCMS_GET_d_value; // uqi-type
		global $RosCMS_GET_d_id; // data_id
		global $RosCMS_GET_d_r_id; // data rev id


		$query_uqi_revision = mysql_query("SELECT d.data_id, d.data_name, d.data_type, d.data_acl, r.rev_id, r.rev_version, r.rev_usrid, r.rev_language, r.rev_datetime, r.rev_date, r.rev_time  
									FROM data_".$h_a2." d, data_revision".$h_a." r 
									WHERE r.rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."' 
									AND r.data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."' 
									AND r.data_id = d.data_id
									ORDER BY r.rev_version DESC
									LIMIT 1;");
		$result_uqi_revision = mysql_fetch_array($query_uqi_revision);

		
		$t_s = '<span style="color:#FF6600;">';
		$t_e = ":</span> ";
		$t_lb = "<br />";
		
		echo $t_s."Name".$t_e.wordwrap($result_uqi_revision['data_name'],14,"<br />\n",1).$t_lb;
		echo $t_s."Type".$t_e.$result_uqi_revision['data_type'].$t_lb;
		echo $t_s."Version".$t_e.$result_uqi_revision['rev_version'].$t_lb;
		
			$RosCMS_query_current_language = mysql_query("SELECT * 
															FROM languages 
															WHERE lang_id = '".mysql_real_escape_string($result_uqi_revision['rev_language'])."'
															LIMIT 1 ;");
			$RosCMS_result_current_language = mysql_fetch_array($RosCMS_query_current_language);
		echo $t_s."Lang".$t_e.$RosCMS_result_current_language['lang_name'].$t_lb;

			$query_usraccount= mysql_query("SELECT user_name 
								FROM users 
								WHERE user_id = '".mysql_real_escape_string($result_uqi_revision['rev_usrid'])."' LIMIT 1 ;");
			$result_usraccount=mysql_fetch_array($query_usraccount);
		echo $t_s."User".$t_e.wordwrap($result_usraccount['user_name'],13,"<br />\n",1).$t_lb;
		
		// Tags
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
												AND (a.tag_usrid = '-1' OR a.tag_usrid = '0' OR a.tag_usrid = '".mysql_real_escape_string($roscms_intern_account_id)."')
												AND a.tag_name_id = n.tn_id 
												AND a.tag_value_id  = v.tv_id 
												ORDER BY tag_usrid ASC, tn_name ASC;");
		while($result_edit_mef_data_tag = mysql_fetch_array($query_edit_mef_data_tag)) {
			echo $t_s.ucfirst($result_edit_mef_data_tag['tn_name']).$t_e.$result_edit_mef_data_tag['tv_value'].$t_lb;
		}
			
		
		if ($roscms_security_level > 1) {
			echo $t_s."Rev-ID".$t_e.$result_uqi_revision['rev_id'].$t_lb;
			echo $t_s."Data-ID".$t_e.$result_uqi_revision['data_id'].$t_lb;
			echo $t_s."ACL".$t_e.$result_uqi_revision['data_acl'].$t_lb;
		}
		
		echo $t_s."Date / Time".$t_e.$t_lb.$result_uqi_revision['rev_date'].$t_lb.$result_uqi_revision['rev_time'];
	}
	
	function user_filter_storage() { // user filter storage
		global $roscms_intern_account_id;
		
		global $RosCMS_GET_d_value; // ufs-action
		global $RosCMS_GET_d_value2; // ufs-type
		global $RosCMS_GET_d_value3; // filter-title
		global $RosCMS_GET_d_value4; // filter-string
	

		$temp_filter_type = "";
		
		if ($RosCMS_GET_d_value2 == "label") {
			$temp_filter_type = "2";
		}
		else {
			$temp_filter_type = "1";
		}

		if ($RosCMS_GET_d_value == "add") {
		
			$query_user_filter_count = mysql_query("SELECT COUNT(*) 
														FROM data_user_filter 
														WHERE filt_usrid = '".mysql_real_escape_string($roscms_intern_account_id)."'
														AND filt_title = '".mysql_real_escape_string($RosCMS_GET_d_value3)."'
														AND filt_type = '".mysql_real_escape_string($temp_filter_type)."' ;");
			$result_user_filter_count = mysql_fetch_row($query_user_filter_count);
			
			if ($result_user_filter_count[0] <= 0) {
				$insert_user_filter = mysql_query("INSERT INTO data_user_filter ( filt_id , filt_usrid , filt_title , filt_type , filt_string , filt_datetime , filt_usage , filt_usagedate ) 
													VALUES (
														NULL , 
														'".mysql_real_escape_string($roscms_intern_account_id)."', 
														'".mysql_real_escape_string($RosCMS_GET_d_value3)."', 
														'".mysql_real_escape_string($temp_filter_type)."', 
														'".mysql_real_escape_string($RosCMS_GET_d_value4)."', 
														NOW( ) , 
														'1', 
														NOW( )
													);");
			}
			//echo "<p>type: ".$RosCMS_GET_d_value2."</p>";
			echo user_filter_list($RosCMS_GET_d_value2);
		}
		else if($RosCMS_GET_d_value == "del") {
			$delete_user_filter = mysql_query("DELETE FROM data_user_filter WHERE filt_id = ".mysql_real_escape_string($RosCMS_GET_d_value3)." AND filt_usrid = '".mysql_real_escape_string($roscms_intern_account_id)."' LIMIT 1;");
			echo user_filter_list($RosCMS_GET_d_value2);
		}
		else if($RosCMS_GET_d_value == "load") {
			echo user_filter_list($RosCMS_GET_d_value2);
		}
		if ($RosCMS_GET_d_value2 == "label") {
			echo "<div style=\"cursor: pointer; text-align:right;\" onclick=\"add_user_filter('label', '')\"  style=\"text-decoration:underline;\">Add</div>";
		}
	}
	
	function user_filter_list($temp2_filter_type) {
		global $roscms_intern_account_id;

		$temp_entry_counter = 0;
		$temp_filter_type = "";
		
		if ($temp2_filter_type == "label") {
			$temp_filter_type = "2";
		}
		else {
			$temp_filter_type = "1";
		}
		
		$query_user_filter = mysql_query("SELECT filt_id, filt_title, filt_string 
											FROM data_user_filter 
											WHERE filt_usrid = '".mysql_real_escape_string($roscms_intern_account_id)."'
											AND filt_type = '".mysql_real_escape_string($temp_filter_type)."'
											ORDER BY filt_title ASC;");
		while ($result_user_filter = mysql_fetch_array($query_user_filter)) {
			//echo "<span style=\"cursor: pointer;\" onclick=\"tbl_user_filter('".$result_user_filter['filt_string']."')\">".$result_user_filter['filt_title']."</span> <span onclick=\"#\"><img src=\"images/remove.gif\" alt=\"-\" style=\"width:11px; height:11px; border:0px;\" /></span><br />";
			echo "<span style=\"cursor: pointer;\" onclick=\"tbl_user_filter('".$result_user_filter['filt_string']."', '".$temp_filter_type."', '".$result_user_filter['filt_title']."')\" style=\"text-decoration:underline;\">".$result_user_filter['filt_title']."</span>  <span style=\"cursor: pointer;\" onclick=\"delete_user_filter('".$result_user_filter['filt_id']."', '".$temp2_filter_type."', '".$result_user_filter['filt_title']."')\"><img src=\"images/remove.gif\" alt=\"-\" style=\"width:11px; height:11px; border:0px;\" /></span><br />";
			//echo $result_user_filter['filt_title']."<br />";
			$temp_entry_counter++;
		}
		
		if ($temp_entry_counter <= 0) {
			if ($temp2_filter_type == "label") {
				echo "<span>Tag entries with custom labels to organize the data as it makes sense to you.</span>";
			}
			else {
				echo "<span>Compose your favorite filter combinations and afterwards use the \"save\" function.</span>";
			}
		}
	}
	
	function auto_save_info() { // (auto-) save entry to database
		$RosCMS_POST_d_stextsum = "";
		$RosCMS_POST_d_textsum = "";
		if (array_key_exists("stextsum", $_POST)) $RosCMS_POST_d_stextsum=htmlspecialchars($_POST["stextsum"]);
		if (array_key_exists("plmsum", $_POST)) $RosCMS_POST_d_textsum=htmlspecialchars($_POST["plmsum"]);

		//echo "<h3>POST: ".$_POST["plm1"]."</h3>";

		require_once("inc/data_edit.php");
		require_once("inc/data_edit_tag.php");
		
		
		global $roscms_intern_account_id;

		global $RosCMS_GET_d_id;
		global $RosCMS_GET_d_r_id;
		global $RosCMS_GET_d_r_lang;
	
		global $RosCMS_GET_d_value;
		global $RosCMS_GET_d_value2;
		global $RosCMS_GET_d_value3;
		global $RosCMS_GET_d_value4;
		
		$RosCMS_intern_save_rev_id = "";
		$RosCMS_intern_d_rev_version = 0;
		//$RosCMS_intern_d_rev_lang = "fuck";
		$RosCMS_intern_d_rev_lang = $RosCMS_GET_d_r_lang;
//		echo "<h2>lang: ".$RosCMS_intern_d_rev_lang."</h2>";
		$RosCMS_intern_d_rev_usrid = "";
		
		$RosCMS_intern_d_rev_number = "";
		
		echo "<p>!!!!!!!!!!!!!!!!!</p><hr /><p>asdadasdawddsda</p>";
		
		echo "<p>DynNumber: ".$RosCMS_GET_d_value4."</p>";
		


		if ($RosCMS_GET_d_value3 == "draft") { // draft
			$roscms_sql_tags = "";
			$roscms_sql_tags2 = "";
			$roscms_sql_tags3 = "";

			if ($RosCMS_GET_d_value4 != "no") {
				$roscms_sql_tags .= ", data_tag a, data_tag_name n, data_tag_value v ";
				$roscms_sql_tags2 .= ", n.tn_name, v.tv_value ";
				$roscms_sql_tags3 .= " AND r.data_id = a.data_id 
										AND r.rev_id = a.data_rev_id 
										AND a.tag_usrid = '-1' 
										AND a.tag_name_id = n.tn_id  
										AND n.tn_name = 'number'  
										AND a.tag_value_id  = v.tv_id
										AND v.tv_value = '".mysql_real_escape_string($RosCMS_GET_d_value4)."' ";			
			}

			$query_data_save_find_draft = mysql_query("SELECT r.rev_id, r.data_id, r.rev_version, r.rev_language, r.rev_usrid, r.rev_datetime ".$roscms_sql_tags2."
														FROM data_revision r ".$roscms_sql_tags."
														WHERE r.data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."'
														AND r.rev_usrid = '".mysql_real_escape_string($roscms_intern_account_id)."'
														AND r.rev_date = '".mysql_real_escape_string(date("Y-m-d"))."'
														AND r.rev_language = '".mysql_real_escape_string($RosCMS_intern_d_rev_lang)."'
														".$roscms_sql_tags3."
														ORDER BY r.rev_id DESC
														LIMIT 1;");
			$result_data_save_find_draft = mysql_fetch_array($query_data_save_find_draft);

			//echo "<p>DraftEntry: ".$result_data_save_find_draft['rev_id'].", ".$result_data_save_find_draft['data_id'].", ".$result_data_save_find_draft['rev_version'].", ".$result_data_save_find_draft['tv_value']."</p>";

			if (getTagValue($RosCMS_GET_d_id, $result_data_save_find_draft['rev_id'], "-1", "status") == "draft") {
				//echo "<p>draft-lang: ".$RosCMS_GET_d_value3."|".$RosCMS_GET_d_value4."|".$result_data_save_find_draft['rev_language']."</p>";
				$RosCMS_intern_save_rev_id = $result_data_save_find_draft['rev_id'];
				$RosCMS_intern_d_rev_lang = $result_data_save_find_draft['rev_language'];
				$RosCMS_intern_d_rev_usrid = $result_data_save_find_draft['rev_usrid'];
				$RosCMS_intern_d_rev_version = 0;
			}
			else { // save changes as draft based on a new/stable entry
				//echo "<p>else-lang: ".$RosCMS_GET_d_value3."|".$RosCMS_GET_d_value4."|".$RosCMS_GET_d_r_lang."</p>";
				//$RosCMS_GET_d_value3 = "submit"; // save instead of update
				$RosCMS_intern_save_rev_id = "";
				$RosCMS_intern_d_rev_version = 0;
				$RosCMS_intern_d_rev_lang = $RosCMS_GET_d_r_lang;
				$RosCMS_intern_d_rev_usrid = $roscms_intern_account_id;
				//die("Saving draft not possible !?!");
			}


		}
		
		
		if (($RosCMS_GET_d_value3 == "draft" && $RosCMS_intern_save_rev_id == "") || $RosCMS_GET_d_value3 == "submit") { // add 
			//echo "<p>ADD</p>";
			
/*			if ($RosCMS_GET_d_value3 == "submit") {
				$query_data_save_find_submit = mysql_query("SELECT rev_id, data_id  rev_version, rev_language, rev_usrid, rev_datetime
															FROM data_revision
															WHERE data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."'
															AND rev_language = '".mysql_real_escape_string($RosCMS_intern_d_rev_lang)."'
															ORDER BY rev_id DESC
															LIMIT 1;");
				$result_data_save_find_submit = mysql_fetch_array($query_data_save_find_submit);
				
				$RosCMS_intern_d_rev_version = 0;
				
				$RosCMS_intern_d_rev_lang = $result_data_save_find_submit['rev_language'];
				$RosCMS_intern_d_rev_usrid = $roscms_intern_account_id;


//				echo "<p>sumit-lang: ".$result_data_save_find_submit['rev_language']."</p>";
				
				$query_data_delete_old_drafts = mysql_query("SELECT * 
															FROM data_revision 
															WHERE data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."'
															AND rev_version = 0
															AND rev_language = '".mysql_real_escape_string($RosCMS_intern_d_rev_lang)."'
															AND rev_usrid = '".mysql_real_escape_string($RosCMS_intern_d_rev_usrid)."' ;");
				while ($result_data_delete_old_drafts = mysql_fetch_array($query_data_delete_old_drafts)) {
//					echo "<p>delete draft: ".$result_data_delete_old_drafts['rev_id']."</p>";
					deleteRevision($result_data_delete_old_drafts['rev_id']);
				}
			}
*/
			
			$insert_data_save = mysql_query("INSERT INTO data_revision ( rev_id , data_id , rev_version , rev_language , rev_usrid , rev_datetime , rev_date , rev_time ) 
												VALUES (
													NULL , 
													'".mysql_real_escape_string($RosCMS_GET_d_id)."', 
													'0', 
													'".mysql_real_escape_string($RosCMS_intern_d_rev_lang)."', 
													'".mysql_real_escape_string($RosCMS_intern_d_rev_usrid)."', 
													NOW( ),
													CURDATE( ),
													CURTIME( )
												);");//$RosCMS_intern_d_rev_version
			$query_data_save_find_submit2 = mysql_query("SELECT rev_id
														FROM data_revision
														WHERE data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."'
														AND rev_version = '0'
														AND rev_language = '".mysql_real_escape_string($RosCMS_intern_d_rev_lang)."'
														AND rev_usrid = '".mysql_real_escape_string($roscms_intern_account_id)."'
														ORDER BY rev_datetime DESC;");
			$result_data_save_find_submit2 = mysql_fetch_array($query_data_save_find_submit2);
			//$result_data_save_find_submit2['rev_id'];
			
			for ($i=1; $i <= $RosCMS_POST_d_stextsum; $i++) {  //short text
				$insert_data_save_stext = mysql_query("INSERT INTO data_stext ( stext_id , data_rev_id , stext_name , stext_content ) 
														VALUES (
															NULL , 
															'".mysql_real_escape_string($result_data_save_find_submit2['rev_id'])."', 
															'".mysql_real_escape_string($_POST["pdstext".$i])."', 
															'".mysql_real_escape_string($_POST["pstext".$i])."'
														);");
			}
			for ($i=1; $i <= $RosCMS_POST_d_textsum; $i++) { // text
				$insert_data_save_text = mysql_query("INSERT INTO data_text ( text_id , data_rev_id , text_name , text_content ) 
														VALUES (
															NULL , 
															'".mysql_real_escape_string($result_data_save_find_submit2['rev_id'])."', 
															'".mysql_real_escape_string($_POST["pdtext".$i])."', 
															'".mysql_real_escape_string($_POST["plm".$i])."'
														);");
//				echo "<p>CONTENT###\n<br />".$_POST["plm".$i]."</p>";
			}
			
			if ($RosCMS_GET_d_value3 == "submit") {
//				echo "<h4>submit_process: tag_add(".$RosCMS_GET_d_id.", ".$result_data_save_find_submit2['rev_id'].", 'status', 'new', '-1');</h4>";
				tag_add($RosCMS_GET_d_id, $result_data_save_find_submit2['rev_id'], 'status', 'new', '-1');
			}
			else if ($RosCMS_GET_d_value3 == "draft") {
//				echo "<h4>submit_process: tag_add(".$RosCMS_GET_d_id.", ".$result_data_save_find_submit2['rev_id'].", 'status', 'draft', '-1');</h4>";
				tag_add($RosCMS_GET_d_id, $result_data_save_find_submit2['rev_id'], 'status', 'draft', '-1');
			}
			
			if ($RosCMS_GET_d_value4 != "no") {
				tag_add($RosCMS_GET_d_id, $result_data_save_find_submit2['rev_id'], 'number', $RosCMS_GET_d_value4, '-1');
			}
			//tag_add($RosCMS_GET_d_id, $result_data_save_find_submit2['rev_id'], 'debug', '1', '-1');
		}
		else if ($RosCMS_intern_save_rev_id != "" && $RosCMS_GET_d_value3 == "draft") {
			//echo "<p>UPDATE</p>";
			
			$insert_data_save_stext = mysql_query("DELETE FROM data_stext WHERE data_rev_id = '".mysql_real_escape_string($RosCMS_intern_save_rev_id)."';");
			$insert_data_save_stext = mysql_query("DELETE FROM data_text WHERE data_rev_id = '".mysql_real_escape_string($RosCMS_intern_save_rev_id)."';");
				
			for ($i=1; $i <= $RosCMS_POST_d_stextsum; $i++) { // short text
				if (array_key_exists("pdstext".$i, $_POST)) {
					$insert_data_save_stext = mysql_query("INSERT INTO data_stext ( stext_id , data_rev_id , stext_name , stext_content ) 
															VALUES (
																NULL , 
																'".mysql_real_escape_string($RosCMS_intern_save_rev_id)."', 
																'".mysql_real_escape_string($_POST["pdstext".$i])."', 
																'".mysql_real_escape_string($_POST["pstext".$i])."'
															);");
				}
			}
			for ($i=1; $i <= $RosCMS_POST_d_textsum; $i++) { // text 
				if (array_key_exists("pdtext".$i, $_POST)) {
					$insert_data_save_text = mysql_query("INSERT INTO data_text ( text_id , data_rev_id , text_name , text_content ) 
															VALUES (
																NULL , 
																'".mysql_real_escape_string($RosCMS_intern_save_rev_id)."', 
																'".mysql_real_escape_string($_POST["pdtext".$i])."', 
																'".mysql_real_escape_string($_POST["plm".$i])."'
															);");
				}
				//echo "<p><pre>".$_POST["pdtext".$i].": <br />\n".$_POST["plm".$i]."</pre></p>";
			}
			//tag_add($RosCMS_GET_d_id, $RosCMS_intern_save_rev_id, 'debug', '2', '-1');
				
		}
		else {
			echo "not enough data ...";
		
		}
		

		
	}

?>
