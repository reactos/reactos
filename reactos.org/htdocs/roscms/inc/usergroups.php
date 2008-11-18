<?php


//	echo "<p>Sec_Lev: ".$roscms_security_level."</p>";
//	echo "<p>Sec_Memberships: ".$roscms_security_memberships."</p>";
	
	
	$roscms_security_sql_read = roscms_security_sql("read");
	


	//echo "<p>Sec_Read_SQL: ".$roscms_security_sql_read."</p>";
	//echo "<p>?: ".roscms_security_check (180)."</p>";
	//echo "<p>?: ".roscms_security_check_kind(87, "trans")."</p>";


	function roscms_security_sql($kind) {
		global $roscms_branch;
		global $roscms_security_level;
		global $roscms_currentuser_id;
		
		$temp_sql = "";
		$temp_sec_counter = 0;
		$pos = 0;
		
		if ($roscms_security_level > 0) {
			$query_data_security = mysql_query("SELECT * 
											FROM data_security 
											WHERE sec_branch = '".mysql_real_escape_string($roscms_branch)."';");
			while ($result_data_security = mysql_fetch_array($query_data_security)) {
				if ($result_data_security['sec_lev'.$roscms_security_level.'_'.$kind] == 1) {
					if ($temp_sec_counter > 0) {
						$temp_sql .= " OR";
					}
		
					$temp_sql .= " data_acl = '".mysql_real_escape_string($result_data_security['sec_name'])."' ";
					$temp_sec_counter++;
	
					$query_usergroup_sec2 = mysql_query("SELECT * 
														FROM usergroup_members 
														WHERE usergroupmember_userid = ".mysql_real_escape_string($roscms_currentuser_id).";");
					while($result_usergroup_sec2 = mysql_fetch_array($query_usergroup_sec2)) {
						$pos = strpos($result_data_security['sec_deny'], "|".$result_usergroup_sec2['usergroupmember_usergroupid']."|");
						if ($pos === false) {
							// string not found
						} else {
							$pos = strpos($temp_sql, "OR data_acl = '".$result_data_security['sec_name']."'");
							if ($pos === false) { // string not found
								die("roscms_security_sql(): problem");
							} else {
								$temp_sql = str_replace("OR data_acl = '".$result_data_security['sec_name']."'", "", $temp_sql);
								//echo "<p>deny - remove '".$result_data_security['sec_name']."'</p>";
							}
						}
					}
				}
				else {
					$query_usergroup_sec = mysql_query("SELECT * 
														FROM usergroup_members 
														WHERE usergroupmember_userid = ".mysql_real_escape_string($roscms_currentuser_id).";");
					while($result_usergroup_sec = mysql_fetch_array($query_usergroup_sec)) {
						$pos = strpos($result_data_security['sec_allow'], "|".$result_usergroup_sec['usergroupmember_usergroupid']."|");
						if ($pos === false) {
							// string not found
						} else {
							if ($temp_sec_counter > 0) {
								$temp_sql .= " OR";
							}
							$temp_sql .= " data_acl = '".mysql_real_escape_string($result_data_security['sec_name'])."' ";
							$temp_sec_counter++;
						}
					}
				}
			}	
			
			if ($temp_sec_counter == 1) {
				$temp_sql = " AND ". $temp_sql ." ";
			}
			else if ($temp_sec_counter > 1) {
				$temp_sql = " AND (". $temp_sql .") ";
			}
			else {
				 $temp_sql = " AND ( 0 = 1 ) "; // FAIL, as the user has no rights to access RosCMS
			}
			
		}
		else {
			 $temp_sql = " AND ( 0 = 1 ) "; // FAIL, as the user has no rights to access RosCMS
		}
		return $temp_sql;
	}

	function roscms_security_check ($d_id) {
		global $roscms_branch;
		global $roscms_security_memberships;
		global $roscms_security_level;
		global $roscms_currentuser_id;
		
		global $h_a;
		global $h_a2;
		
		if ($roscms_security_level < 1) {
			return "";
		}
		
		$temp_sec_rights = "|";

		$query_data_sec_rights = mysql_query("SELECT * 
										FROM data_".$h_a2." d, data_security y
										WHERE data_id = ".mysql_real_escape_string($d_id)." 
										AND y.sec_name = d.data_acl 
										AND y.sec_branch = '".mysql_real_escape_string($roscms_branch)."' 
										LIMIT 1;") or die("Data-Entry \"".$d_id."\"not found [usergroups].");
		$result_data_sec_rights = mysql_fetch_array($query_data_sec_rights);
		
		$tmp_acl_allow = false;
		
		$query_usergroup_sec = mysql_query("SELECT * 
											FROM usergroup_members 
											WHERE usergroupmember_userid = ".mysql_real_escape_string($roscms_currentuser_id).";");
		while($result_usergroup_sec = mysql_fetch_array($query_usergroup_sec)) {
			$pos = strpos($result_data_sec_rights['sec_allow'], "|".$result_usergroup_sec['usergroupmember_usergroupid']."|");
			if ($pos === false) {
				// string not found
			} 
			else {
				$tmp_acl_allow = true;
				//echo "<p>allow - ".$result_usergroup_sec['usergroupmember_usergroupid']."</p>";
			}
		}
		
		$tmp_acl_deny = false;
		
		$query_usergroup_sec2 = mysql_query("SELECT * 
												FROM usergroup_members 
												WHERE usergroupmember_userid = ".mysql_real_escape_string($roscms_currentuser_id).";");
		while($result_usergroup_sec2 = mysql_fetch_array($query_usergroup_sec2)) {
			$pos = strpos($result_data_sec_rights['sec_deny'], "|".$result_usergroup_sec2['usergroupmember_usergroupid']."|");
			if ($pos === false) {
				// string not found
			}
			else {
				$tmp_acl_deny = true;
				//echo "<p>deny - ".$result_usergroup_sec2['usergroupmember_usergroupid']."</p>";
			}
		}
		
		
		if (($result_data_sec_rights['sec_lev'.$roscms_security_level.'_read'] == 1 || $tmp_acl_allow == true) && $tmp_acl_deny == false) {
			$temp_sec_rights .= "read|";
		}
		if (($result_data_sec_rights['sec_lev'.$roscms_security_level.'_write'] == 1 || $tmp_acl_allow == true) && $tmp_acl_deny == false) {
			$temp_sec_rights .= "write|";
			//echo "<p>write".$result_data_sec_rights['sec_lev'.$roscms_security_level.'_write']."-".$roscms_security_level.$result_data_sec_rights['data_name']."</p>";
		}
		if (($result_data_sec_rights['sec_lev'.$roscms_security_level.'_add'] == 1 || ($tmp_acl_allow == true && $roscms_security_level == 3)) && $tmp_acl_deny == false) {
			$temp_sec_rights .= "add|";
		}
		if (($result_data_sec_rights['sec_lev'.$roscms_security_level.'_pub'] == 1 || ($tmp_acl_allow == true && $roscms_security_level == 3)) && $tmp_acl_deny == false) {
			$temp_sec_rights .= "pub|";
		}
		if (($result_data_sec_rights['sec_lev'.$roscms_security_level.'_trans'] == 1 || ($tmp_acl_allow == true && $roscms_security_level == 3)) && $tmp_acl_deny == false) {
			$temp_sec_rights .= "trans|";
		}
		
		return $temp_sec_rights;
	}

	function roscms_security_check_kind ($d_id, $kind) {
		global $roscms_security_level;
		//echo "<p>KIND: ".$kind."</p>";

		if ($roscms_security_level < 1) {
			return false;
		}
		
		$pos = strpos(roscms_security_check ($d_id), "|".$kind."|");
		if ($pos === false) { // string not found
			//echo "<p>return false</p>";
			return false;
		}
		else { // string found
			//echo "<p>return true</p>";
			return true;
		}
	}

	function roscms_security_explain ($d_id) {
		global $roscms_security_level;

		if ($roscms_security_level < 1) {
			return "";
		}

		$tmp_sec_explain = "";
		
		$pos = strpos(roscms_security_check ($d_id), "|read|");
		if ($pos === false) { // string not found
			$tmp_sec_explain .= "-";
		}
		else { // string found
			$tmp_sec_explain .= "r";
		}
		
		$pos = strpos(roscms_security_check ($d_id), "|write|");
		if ($pos === false) { // string not found
			$tmp_sec_explain .= "-";
		}
		else { // string found
			$tmp_sec_explain .= "w";
		}

		$pos = strpos(roscms_security_check ($d_id), "|add|");
		if ($pos === false) { // string not found
			$tmp_sec_explain .= "-";
		}
		else { // string found
			$tmp_sec_explain .= "a";
		}

		$pos = strpos(roscms_security_check ($d_id), "|pub|");
		if ($pos === false) { // string not found
			$tmp_sec_explain .= "-";
		}
		else { // string found
			$tmp_sec_explain .= "p";
		}

		$pos = strpos(roscms_security_check ($d_id), "|trans|");
		if ($pos === false) { // string not found
			$tmp_sec_explain .= "-";
		}
		else { // string found
			$tmp_sec_explain .= "t";
		}
		
		$tmp_sec_explain .= " ".$roscms_security_level;
		
		return $tmp_sec_explain;
	}
	
	function roscms_security_grp_member($group_str) { 
		global $roscms_intern_account_id;
		
		$query_usergroup_sec = mysql_query("SELECT COUNT(*) 
											FROM usergroup_members 
											WHERE usergroupmember_userid = ".mysql_real_escape_string($roscms_intern_account_id)."
											AND usergroupmember_usergroupid = '".mysql_real_escape_string($group_str)."'
											LIMIT 1;");
		$result_usergroup_sec = mysql_fetch_row($query_usergroup_sec);
		
		if ($result_usergroup_sec[0] == 1) {
			return true;
		}
		else {
			return false;
		}
	}



	$roscms_intern_usrgrp_policy_view_basic = false;
	$roscms_intern_usrgrp_policy_view_advanced = false;
	$roscms_intern_usrgrp_policy_translate = false;
	$roscms_intern_usrgrp_policy_edit_basic = false;
	$roscms_intern_usrgrp_policy_edit_advanced = false;


	// view_basic
	if ($roscms_intern_usrgrp_trans == true || 
		$roscms_intern_usrgrp_transm == true || 
		$roscms_intern_usrgrp_men == true || 
		$roscms_intern_usrgrp_team == true || 
		$roscms_intern_usrgrp_dev == true || 
		$roscms_intern_usrgrp_admin == true || 
		$roscms_intern_usrgrp_sadmin == true)
	{
		$roscms_intern_usrgrp_policy_view_basic = true;
	}

	// view_advanced
	if ($roscms_intern_usrgrp_team == true || 
		$roscms_intern_usrgrp_dev == true || 
		$roscms_intern_usrgrp_admin == true || 
		$roscms_intern_usrgrp_sadmin == true)
	{
		$roscms_intern_usrgrp_policy_view_advanced = true;
	}

	// translate
	if ($roscms_intern_usrgrp_trans == true || 
		$roscms_intern_usrgrp_transm == true || 
		$roscms_intern_usrgrp_men == true || 
		$roscms_intern_usrgrp_team == true || 
		$roscms_intern_usrgrp_dev == true || 
		$roscms_intern_usrgrp_admin == true || 
		$roscms_intern_usrgrp_sadmin == true)
	{
		$roscms_intern_usrgrp_policy_translate = true;
	}

	// edit_basic
	if ($roscms_intern_usrgrp_men == true || 
		$roscms_intern_usrgrp_team == true || 
		$roscms_intern_usrgrp_dev == true || 
		$roscms_intern_usrgrp_admin == true || 
		$roscms_intern_usrgrp_sadmin == true)
	{
		$roscms_intern_usrgrp_policy_edit_basic = true;
	}

	// edit_advanced
	if ($roscms_intern_usrgrp_admin == true || 
		$roscms_intern_usrgrp_sadmin == true)
	{
		$roscms_intern_usrgrp_policy_edit_advanced = true;
	}

?>