<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Klemens Friedl <frik85@reactos.org>

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
	
	$RosCMS_GET_branch = "website";

	global $roscms_security_level;
	global $roscms_intern_account_id;

	global $RosCMS_GET_d_use;
	global $RosCMS_GET_d_flag;
	global $RosCMS_GET_d_value;
	global $RosCMS_GET_d_value2;
	
	if (roscms_security_grp_member("transmaint") || $roscms_security_level == 3) {
		if ($RosCMS_GET_d_use == "usrtbl") {
		
			if (roscms_security_grp_member("transmaint")) {
				$query_usrlang = mysql_query("SELECT user_language 
												FROM users 
												WHERE user_id = '".mysql_real_escape_string($roscms_intern_account_id)."' 
												LIMIT 1;");
				$result_usrlang = mysql_fetch_array($query_usrlang);
				
				if ($result_usrlang['user_language'] != "") {
					$tmp_transmaintlang = $result_usrlang['user_language'];
				}
				else {
					die("Please set a valid language in the myReactOS settings!");
				}
			}
			else {
				$tmp_transmaintlang = false;
			}
		
			if ($RosCMS_GET_d_flag == "addmembership") {
				$query_insert_membership = mysql_query("INSERT INTO `usergroup_members` ( `usergroupmember_userid` , `usergroupmember_usergroupid` ) 
														VALUES (
															".mysql_real_escape_string($RosCMS_GET_d_value).", 
															'".mysql_real_escape_string($RosCMS_GET_d_value2)."'
														);");
				if ($tmp_transmaintlang) {
					log_event_lang_medium("add user account membership: user-id=".$RosCMS_GET_d_value.", group-id=".$RosCMS_GET_d_value2." done by ".$roscms_intern_account_id." {data_user_out}", $tmp_transmaintlang);
				}
				log_event_medium("add user account membership: user-id=".$RosCMS_GET_d_value.", group-id=".$RosCMS_GET_d_value2." done by ".$roscms_intern_account_id." {data_user_out}");
				$RosCMS_GET_d_flag = "detail";
			}
			else if ($RosCMS_GET_d_flag == "delmembership") {
				$query_delete_membership = mysql_query("DELETE FROM usergroup_members
														WHERE usergroupmember_userid = ".mysql_real_escape_string($RosCMS_GET_d_value)."
														AND usergroupmember_usergroupid = '".mysql_real_escape_string($RosCMS_GET_d_value2)."'
														LIMIT 1;");
				if ($tmp_transmaintlang) {
					log_event_lang_medium("delete user account membership: user-id=".$RosCMS_GET_d_value.", group-id=".$RosCMS_GET_d_value2." done by ".$roscms_intern_account_id." {data_user_out}", $tmp_transmaintlang);
				}
				log_event_medium("delete user account membership: user-id=".$RosCMS_GET_d_value.", group-id=".$RosCMS_GET_d_value2." done by ".$roscms_intern_account_id." {data_user_out}");
				$RosCMS_GET_d_flag = "detail";
			}
			else if ($RosCMS_GET_d_flag == "updateusrlang") {
				$query_update_usrlang = mysql_query("UPDATE users 
														SET user_timestamp_touch2 = NOW( ),
														user_language = '".mysql_real_escape_string($RosCMS_GET_d_value2)."' 
														WHERE user_id = ".mysql_real_escape_string($RosCMS_GET_d_value)." 
														LIMIT 1;");
				if ($tmp_transmaintlang) {
					log_event_lang_medium("change user's account language: user-id=".$RosCMS_GET_d_value.", lang-id=".$RosCMS_GET_d_value2." done by ".$roscms_intern_account_id." {data_user_out}", $tmp_transmaintlang);
				}
				log_event_medium("change user's account language: user-id=".$RosCMS_GET_d_value.", lang-id=".$RosCMS_GET_d_value2." done by ".$roscms_intern_account_id." {data_user_out}");
				$RosCMS_GET_d_flag = "detail";
			}
		
			if ($RosCMS_GET_d_flag == "list") {
				
				if (strlen($RosCMS_GET_d_value) > 2) {
					echo "<fieldset><legend>Results</legend>";
					echo "<ul>";
					$tmp_counter = 0;
					
					switch ($RosCMS_GET_d_value2) {
						default:
						case "accountname":
							$tmp_sql_search_opt = "u.user_name";
							break;
						case "fullname":
							$tmp_sql_search_opt = "u.user_fullname";
							break;
						case "email":
							$tmp_sql_search_opt = "u.user_email";
							break;
						case "website":
							$tmp_sql_search_opt = "u.user_website";
							break;
						case "language":
							$tmp_sql_search_opt = "u.user_language";
							break;
					}
					
					//echo "<p>OPT: ".$RosCMS_GET_d_value2."</p>";
					
					$query_user_list = mysql_query("SELECT u.user_id, u.user_name, u.user_fullname, u.user_language
													FROM users u 
													WHERE ". $tmp_sql_search_opt ." LIKE '".mysql_real_escape_string($RosCMS_GET_d_value)."%' 
													ORDER BY u.user_name ASC 
													LIMIT 25;");
					while ($result_user_list = mysql_fetch_array($query_user_list)) {
						$tmp_counter++;
						echo "<li><a href=\"javascript:getuserdetails('".$result_user_list['user_id']."')\">".$result_user_list['user_name']."</a> (".$result_user_list['user_language'].", ".$result_user_list['user_fullname'].")</li>";
					}
		
					echo "</ul>";
					
					if ($tmp_counter >= 25) {
						echo "<p>... more than 25 users</p>";
					}
					
					echo "</fieldset><br />";
				}
				else if (strlen($RosCMS_GET_d_value) > 0) {
					echo "<p>more than 2 characters requiered</p>";
				}
			}
			else if ($RosCMS_GET_d_flag == "detail") {
				$query_user_detail = mysql_query("SELECT user_id, user_name, user_timestamp_touch2 as 'visit', user_login_counter 'visitcount', user_register, user_fullname, user_email, user_language 
												FROM users  
												WHERE user_id = '".mysql_real_escape_string($RosCMS_GET_d_value)."' 
												LIMIT 1;");
				$result_user_detail = mysql_fetch_array($query_user_detail);
				
				echo "<fieldset><legend>Details for '".$result_user_detail['user_name']."'</legend>";
				
				echo "<p><b>Name:</b> ".$result_user_detail['user_name']." (".$result_user_detail['user_fullname'].") [".$result_user_detail['user_id']."]</p>";
				echo "<p><b>Lang:</b> ".$result_user_detail['user_language']."</p>";
				if ($roscms_security_level == 3) {
					echo "<p><b>E-Mail:</b> ".$result_user_detail['user_email']."</p>";
					echo "<p><b>Latest Login:</b> ".$result_user_detail['visit']."; ".$result_user_detail['visitcount']." logins</p>";
					echo "<p><b>Registered:</b> ".$result_user_detail['user_register']."</p>";
				}
				
				echo "<fieldset><legend>Usergroup Memberships</legend><ul>";
				
				$query_user_list = mysql_query("SELECT g.usrgroup_name_id, g.usrgroup_name 
												FROM users u, usergroups g, usergroup_members m 
												WHERE user_id = '".mysql_real_escape_string($RosCMS_GET_d_value)."' 
												AND u.user_id = m.usergroupmember_userid 
												AND g.usrgroup_name_id = m.usergroupmember_usergroupid 
												ORDER BY g.usrgroup_name ASC;");
				while ($result_user_list = mysql_fetch_array($query_user_list)) {
					echo "<li>".$result_user_list['usrgroup_name']." ";
					if ($roscms_security_level == 3) {
						echo "&nbsp; <span class=\"frmeditbutton\" onclick=\"delmembership(".$RosCMS_GET_d_value.", '".$result_user_list['usrgroup_name_id']."')\"><img src=\"images/remove.gif\" alt=\"\" style=\"width:11px; height:11px; border:0px;\" />&nbsp;Delete</span>";
					}
					echo "</li>";
				}
		
				echo "</ul>";
				
				if ($roscms_security_level == 3) {
					echo '<select id="cbmmemb" name="cbmmemb">';
						$query_data_lang = mysql_query("SELECT usrgroup_name_id, usrgroup_name 
														FROM usergroups 
														WHERE usrgroup_seclev  <= '".$roscms_security_level."'
														ORDER BY usrgroup_name ASC;");
						while ($result_data_lang = mysql_fetch_array($query_data_lang)) {
							if (!roscms_security_grp_member("ros_sadmin") && $result_data_lang['usrgroup_name_id'] == "ros_sadmin") {
								//
							}
							else {
								echo '<option value="'.$result_data_lang['usrgroup_name_id'].'">'.$result_data_lang['usrgroup_name'].'</option>';
							}
						}
					echo "</select> ";
					echo ' <input type="button" name="addmemb" id="addmemb" value="Add Membership" onclick="addmembership('.$RosCMS_GET_d_value.', document.getElementById(\'cbmmemb\').value)" /><br />';
				}
				else if (roscms_security_grp_member("transmaint")) {
					echo '<input type="button" name="addmemb" id="addmemb" value="Make this User a Translator" onclick="addmembership('.$RosCMS_GET_d_value.', \'translator\')" /><br />';
				}
				
				echo "<br />";
	
				if ($roscms_security_level == 3) {
					echo '<select id="cbmusrlang" name="cbmusrlang">';
						$query_data_lang = mysql_query("SELECT lang_id , lang_name 
														FROM languages 
														ORDER BY lang_name ASC;");
						while ($result_data_lang = mysql_fetch_array($query_data_lang)) {
							echo '<option value="'.$result_data_lang['lang_id'].'">'.$result_data_lang['lang_name'].'</option>';
						}
					echo "</select> ";
					echo ' <input type="button" name="addusrlang" id="addusrlang" value="Update User\'s language" onclick="updateusrlang('.$RosCMS_GET_d_value.', document.getElementById(\'cbmusrlang\').value)" /><br />';
				}
				else if (roscms_security_grp_member("transmaint")) {
					$query_usrlang = mysql_query("SELECT user_language 
													FROM users 
													WHERE user_id = '".mysql_real_escape_string($roscms_intern_account_id)."' 
													LIMIT 1;");
					$result_usrlang = mysql_fetch_array($query_usrlang);
					
					if ($result_usrlang['user_language'] != "") {
						echo '<input type="button" name="addusrlang" id="addusrlang" value="Switch User\'s language to \''.$result_usrlang['user_language'].'\'" onclick="updateusrlang('.$RosCMS_GET_d_value.', \''.$result_usrlang['user_language'].'\')" /><br />';
					}
				}
				
				echo "</fieldset><br />";
			}
			else {
				echo "<p>".$RosCMS_GET_d_flag."</p>";
			}
		}
	}
?>