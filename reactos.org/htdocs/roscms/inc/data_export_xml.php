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
	
	
	switch ($RosCMS_GET_d_use) {
		default:
			die("");
			break;
		case "ptm": // page table main
			page_table_main();
			break;
		case "xyz":
			// temp
			break;
	}




	function page_table_main() {
		include("inc/data_edit_tag.php");

		global $roscms_intern_account_id;
		
		global $roscms_branch;
		global $RosCMS_GET_debug;
		
		global $roscms_security_level;
		global $roscms_security_sql_read;

		global $RosCMS_GET_d_curpos;
		global $RosCMS_GET_d_arch;
		global $RosCMS_GET_d_filter;
		global $RosCMS_GET_d_filter2;
		global $roscms_standard_language_full;
		
		$roscms_intern_entry_per_page = 25;
		
		$roscms_d_stext = false;
		$roscms_d_tags = false;
		$roscms_d_tags_counter = 0;
		$roscms_sql_tags = "";
		$roscms_sql_tags2 = "";
		$roscms_sql_tags3 = "";
		
		$roscms_d_filter2_string = "";
		$roscms_d_filter2 = "";
		$roscms_d_f2_typea = "";
		$roscms_d_f2_typeb = "";
		$roscms_d_f2_typec = "";
		
		$roscms_mtbl_order = "";
		$roscms_mtbl_order_where = "";
		$roscms_mtbl_cols = "";
		$roscms_mtbl_cols2 = "";
		$roscms_mtbl_colarr = array();
		$roscms_mtbl_translate = "";
		$RosCMS_d_a = "";
		$RosCMS_d_a2 = "";
		
		$temp_status_counter = 0;
		$temp_status_entry_public = 0;
		$temp_status_entry_private = 0;
		$temp_status_entry_system = 0;
		
		
		header("Content-type: text/xml");
		header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");    // Date in the past
		header("Last-Modified: " . gmdate("D, d M Y H:i:s") . " GMT"); // always modified
		header("Cache-Control: no-store, no-cache, must-revalidate");  // HTTP/1.1
		header("Cache-Control: post-check=0, pre-check=0", false);
		header("Pragma: no-cache");                          // HTTP/1.0col	
		
				
		if ($RosCMS_GET_d_curpos == "") {
			$RosCMS_GET_d_curpos = "0";
		}
		
		// Standard Search :
		if ($RosCMS_GET_d_filter != "") {
			$roscms_d_filter1 = " AND d.data_name LIKE '%". mysql_real_escape_string($RosCMS_GET_d_filter) ."%' ";
		}
		else {
			$roscms_d_filter1 = "";
		}
		
		// Advanced Search and Filters:
		if ($RosCMS_GET_d_filter2 != "") {
			$roscms_d_filter2_string = explode("|",$RosCMS_GET_d_filter2); 
			asort($roscms_d_filter2_string);
			
			$roscms_d_filter2_counter = array();
			$roscms_d_filter2_counter2 = array();
			
			foreach ($roscms_d_filter2_string as $key => $value) { // AND/OR algo
				if (array_key_exists(substr($value, 0, 1), $roscms_d_filter2_counter)) {
					$roscms_d_filter2_counter[substr($value, 0, 1)] = $roscms_d_filter2_counter[substr($value, 0, 1)] + 1;
				}
				else {
					$roscms_d_filter2_counter[substr($value, 0, 1)] = 1;
				}
			}
			
			foreach ($roscms_d_filter2_string as $key => $value) {
				$roscms_d_filter2_type = explode("_",$value); 
				$roscms_d_f2_typea = $roscms_d_filter2_type[0];
				$roscms_d_f2_typeb = $roscms_d_filter2_type[1];
				$roscms_d_f2_typec = $roscms_d_filter2_type[2];
				
				if ($roscms_d_f2_typea == "r" || $roscms_d_f2_typea == "c" || $roscms_d_f2_typea == "o" || ($roscms_d_f2_typea == "k" && $roscms_d_f2_typec == "archive")) {
					switch($roscms_d_f2_typea) {
						case 'r': // translate
							if ($roscms_mtbl_translate == "" && $roscms_d_f2_typea != "" && $roscms_d_f2_typec != "") { // only one such filter is allowed
								$roscms_mtbl_translate = $roscms_d_f2_typec;
							}
							break;
/*
						case 'c': // columns
							if ($roscms_mtbl_cols == "" && $roscms_d_f2_typea != "") { // only one such filter is allowed
								$roscms_mtbl_cols .= ucfirst($roscms_d_f2_typec);
							}
							break;
*/
						case 'c': // columns
							if ($roscms_mtbl_cols == "") {
								$roscms_mtbl_cols = "|";
							}
							
							
							//echo "<p>".ucfirst($roscms_d_f2_typec) ."</p>";
							$roscms_mtbl_cols .= ucfirst($roscms_d_f2_typec) ."|";
							
							$roscms_mtbl_colarr[count($roscms_mtbl_colarr)] = ucfirst($roscms_d_f2_typec);
							break;
						case 'o': // order
							if ($roscms_d_f2_typeb != "asc" && $roscms_d_f2_typeb != "desc") {
								$roscms_d_f2_typeb = "asc";
							}
							switch ($roscms_d_f2_typec) {
								case "datetime": // date-time
									$roscms_mtbl_order .= "r.rev_datetime ".$roscms_d_f2_typeb.", ";
									break;
								case "name": // name
									$roscms_mtbl_order .= "d.data_name ".$roscms_d_f2_typeb.", ";
									break;
								case "lang": // language
									$roscms_mtbl_order .= "r.rev_language ".$roscms_d_f2_typeb.", ";
									break;
								case "usr": // user
									$roscms_mtbl_order .= "r.rev_usrid ".$roscms_d_f2_typeb.", ";
									break;
								case "ver": // version
									$roscms_mtbl_order .= "r.rev_version ".$roscms_d_f2_typeb.", ";
									break;
								case "nbr": // number ("dynamic" entry)
									$roscms_d_tags_counter++;
									$roscms_mtbl_order .= " v".$roscms_d_tags_counter.".tv_value ".$roscms_d_f2_typeb.", ";
									$roscms_mtbl_order_where .= " AND n".$roscms_d_tags_counter.".tn_name = 'number_sort' ";
									break;
								case "type": // type
									$roscms_mtbl_order .= "d.data_type ".$roscms_d_f2_typeb.", ";
									break;
								case "security": // security (ACL)
									$roscms_mtbl_order .= "d.data_acl ".$roscms_d_f2_typeb.", ";
									break;
								case "revid": // revision-id
									$roscms_mtbl_order .= "r.rev_id ".$roscms_d_f2_typeb.", ";
									break;
								case "ext": // page extension 
									$roscms_d_tags_counter++;
									$roscms_mtbl_order .= " v".$roscms_d_tags_counter.".tv_value ".$roscms_d_f2_typeb.", ";
									$roscms_mtbl_order_where .= " AND n".$roscms_d_tags_counter.".tn_name = 'extension' ";
									break;
								case "status": // status 
									$roscms_d_tags_counter++;
									$roscms_mtbl_order .= " v".$roscms_d_tags_counter.".tv_value ".$roscms_d_f2_typeb.", ";
									$roscms_mtbl_order_where .= " AND n".$roscms_d_tags_counter.".tn_name = 'status' ";
									break;
								case "kind": // kind 
									$roscms_d_tags_counter++;
									$roscms_mtbl_order .= " v".$roscms_d_tags_counter.".tv_value ".$roscms_d_f2_typeb.", ";
									$roscms_mtbl_order_where .= " AND n".$roscms_d_tags_counter.".tn_name = 'kind' ";
									break;
							}
							break;
						case 'k': // kind
							if ($roscms_d_f2_typec == "archive") {
								$RosCMS_d_a = "_a";
								$RosCMS_d_a2 = "a";
							}
							break;
					}
				}
				else {
					// more than one of the same filter: AND (foo OR bar)
					if ($roscms_d_filter2_counter[substr($value, 0, 1)] > 1 && !array_key_exists(substr($value, 0, 1), $roscms_d_filter2_counter2)) {
						$roscms_d_filter2 .= " AND ( ";
						$roscms_d_filter2_counter2[substr($value, 0, 1)] = 1;
					}
					elseif (array_key_exists(substr($value, 0, 1), $roscms_d_filter2_counter2) && $roscms_d_filter2_counter2[substr($value, 0, 1)] < $roscms_d_filter2_counter[substr($value, 0, 1)]) {
						$roscms_d_filter2 .= " OR ";
						$roscms_d_filter2_counter2[substr($value, 0, 1)]++;
					}
					else {
						$roscms_d_filter2 .= " AND ";
						if (array_key_exists(substr($value, 0, 1), $roscms_d_filter2_counter2)) {
							$roscms_d_filter2_counter2[substr($value, 0, 1)]++;
						}
					}
	
					
					switch($roscms_d_f2_typea) {
						default:
							//die("hmm don't know this one: ".$roscms_d_f2_typea);
							break;
						case 'k': // kind (stable, new, draft, unknown)
							if ($roscms_d_f2_typec == "draft") {
								$temp_status_entry_private++;
							}
							else if ($roscms_d_f2_typec == "system") {
								$temp_status_entry_system++;
							}
							else {
								$temp_status_entry_public++;
							}


							$temp_status_counter++;

							//$roscms_d_tags = true;
							$roscms_d_tags_counter++;
						
							$roscms_d_filter2 .= " (n".$roscms_d_tags_counter.".tn_name = 'status'
													AND v".$roscms_d_tags_counter.".tv_value";
																	
							if ($roscms_d_f2_typeb == "is") {
								$roscms_d_filter2 .= " = ";
							}
							else {
								$roscms_d_filter2 .= " != ";
							}
							
							$roscms_d_filter2 .= "'".mysql_real_escape_string($roscms_d_f2_typec)."') ";
							break;
							
						case 'a': // tag
							//$roscms_d_tags = true;
							$roscms_d_tags_counter++;
							
							$roscms_d_filter2 .= " (n".$roscms_d_tags_counter.".tn_name = 'tag'
													AND v".$roscms_d_tags_counter.".tv_value";
																	
							if ($roscms_d_f2_typeb == "is") {
								$roscms_d_filter2 .= " = ";
							}
							else {
								$roscms_d_filter2 .= " != ";
							}
							
							$roscms_d_filter2 .= "'".mysql_real_escape_string($roscms_d_f2_typec)."') ";
							break;
	
						case 'n': // name
							$roscms_d_filter2 .= "d.data_name";
							
							if ($roscms_d_f2_typeb == "is") {
								$roscms_d_filter2 .= " = '".mysql_real_escape_string($roscms_d_f2_typec)."' ";
							}
							else if ($roscms_d_f2_typeb == "likea") {
								$roscms_d_filter2 .= " LIKE '%". mysql_real_escape_string($roscms_d_f2_typec) ."%' ";
							}
							else if ($roscms_d_f2_typeb == "likeb") {
								$roscms_d_filter2 .= " LIKE '". mysql_real_escape_string($roscms_d_f2_typec) ."%' ";
							}
							else {
								$roscms_d_filter2 .= " != '".mysql_real_escape_string($roscms_d_f2_typec)."' ";
							}
							
							break;
							
						case 'y': // type   (page, content, template, script, system)						
							$roscms_d_filter2 .= "d.data_type";
							
							if ($roscms_d_f2_typeb == "is") {
								$roscms_d_filter2 .= " = ";
							}
							else {
								$roscms_d_filter2 .= " != ";
							}
							
							$roscms_d_filter2 .= "'".mysql_real_escape_string($roscms_d_f2_typec)."'";
							
							break;
						
						case 's': // starred
							//$roscms_d_tags = true;
							$roscms_d_tags_counter++;
							
							if ($roscms_d_f2_typec == "true") {
								$roscms_d_f2_typec = "on";
							}
							else {
								$roscms_d_f2_typec = "off";
							}
							
							$roscms_d_filter2 .= " (n".$roscms_d_tags_counter.".tn_name = 'star'
													AND v".$roscms_d_tags_counter.".tv_value = '".mysql_real_escape_string($roscms_d_f2_typec)."') ";
						
							break;
						
						case 'd': // date
						
							$roscms_d_filter2 .= "r.rev_date";
							
							if ($roscms_d_f2_typeb == "is") {
								$roscms_d_filter2 .= " = ";
							}
							else if ($roscms_d_f2_typeb == "sm") {
								$roscms_d_filter2 .= " < ";
							}
							else if ($roscms_d_f2_typeb == "la") {
								$roscms_d_filter2 .= " > ";
							}
							else {
								$roscms_d_filter2 .= " != ";
							}
							
							$roscms_d_filter2 .= "'".mysql_real_escape_string($roscms_d_f2_typec)."'";
							break;
						case 't': // time
							$roscms_d_filter2 .= "r.rev_time";
							
							if ($roscms_d_f2_typeb == "is") {
								$roscms_d_filter2 .= " = ";
							}
							else if ($roscms_d_f2_typeb == "sm") {
								$roscms_d_filter2 .= " < ";
							}
							else if ($roscms_d_f2_typeb == "la") {
								$roscms_d_filter2 .= " > ";
							}
							else {
								$roscms_d_filter2 .= " != ";
							}
							
							$roscms_d_filter2 .= "'".mysql_real_escape_string($roscms_d_f2_typec)."'";
							break;
						case 'l': // language
							$roscms_d_filter2 .= "r.rev_language";
							
							if ($roscms_d_f2_typeb == "is") {
								$roscms_d_filter2 .= " = ";
							}
							else {
								$roscms_d_filter2 .= " != ";
							}
							
							$roscms_d_filter2 .= "'".mysql_real_escape_string($roscms_d_f2_typec)."'";
							break;
						case 'i': // security (ACL)
							$roscms_d_filter2 .= "d.data_acl";
							
							if ($roscms_d_f2_typeb == "is") {
								$roscms_d_filter2 .= " = ";
							}
							else {
								$roscms_d_filter2 .= " != ";
							}
							
							$roscms_d_filter2 .= "'".mysql_real_escape_string($roscms_d_f2_typec)."'";
							break;
						case 'm': // metadata
							$roscms_d_tags_counter++;
							
							$roscms_d_filter2 .= " (n".$roscms_d_tags_counter.".tn_name = '".mysql_real_escape_string(strtolower($roscms_d_f2_typeb))."'
													AND v".$roscms_d_tags_counter.".tv_value = '".mysql_real_escape_string($roscms_d_f2_typec)."') ";
							break;
						case 'u': // user
							$roscms_d_filter2 .= "r.rev_usrid";
							
							if ($roscms_d_f2_typeb == "is") {
								$roscms_d_filter2 .= " = ";
							}
							else {
								$roscms_d_filter2 .= " != ";
							}
							
							$query_xml_ptm_usrid = mysql_query("SELECT user_id 
																FROM users 
																WHERE user_name = '".mysql_real_escape_string($roscms_d_f2_typec)."'
																LIMIT 1;");
							$result_xml_ptm_usrid = mysql_fetch_array($query_xml_ptm_usrid);
							
							$roscms_d_filter2 .= "'".mysql_real_escape_string($result_xml_ptm_usrid['user_id'])."'";
							break;
	
						case 'v': // version
							$roscms_d_filter2 .= "r.rev_version";
							
							if ($roscms_d_f2_typeb == "is") {
								$roscms_d_filter2 .= " = ";
							}
							else if ($roscms_d_f2_typeb == "sm") {
								$roscms_d_filter2 .= " < ";
							}
							else if ($roscms_d_f2_typeb == "la") {
								$roscms_d_filter2 .= " > ";
							}
							else {
								$roscms_d_filter2 .= " != ";
							}
							
							$roscms_d_filter2 .= "'".mysql_real_escape_string($roscms_d_f2_typec)."'";
							break;
						case 'e': // system
							switch ($roscms_d_f2_typeb) {
								default:
								case 'dataid':
									$roscms_d_filter2 .= "d.data_id = ";
									break;
								case 'revid':
									$roscms_d_filter2 .= "r.rev_id = ";
									break;
								case 'usrid':
									$roscms_d_filter2 .= "r.rev_usrid = ";
									break;
								case 'langid':
									$roscms_d_filter2 .= "r.rev_language = ";
									break;
							}
						
							$roscms_d_filter2 .= "'".mysql_real_escape_string($roscms_d_f2_typec)."'";
							break;
					}
	
					if (array_key_exists(substr($value, 0, 1), $roscms_d_filter2_counter2)) {
						if ($roscms_d_filter2_counter[substr($value, 0, 1)] == $roscms_d_filter2_counter2[substr($value, 0, 1)]) {
							$roscms_d_filter2 .= " ) ";
						}
					}
					
					//echo "<p>".$key." - ".$value." # ".$roscms_d_filter2_counter[substr($value, 0, 1)]."</p><hr />";
				}
			}
			//echo $roscms_d_filter2;
		
		}
		else {
			$roscms_d_filter2 = "";
		}
		
		// @FIXME:
		/*if ($temp_status_counter == 0) {
			$roscms_d_tags = true;
			//echo "<p>COUNTER: ".$temp_status_counter."</p>";
			if ($roscms_security_level > 1) {
				$roscms_d_filter2 .= " AND (n.tn_name = 'status' AND v.tv_value != 'draft') ";
			}
			else {
				$roscms_d_filter2 .= " AND (n.tn_name = 'status' AND (v.tv_value = 'new' OR v.tv_value = 'stable')) ";
			}
		}*/


		if ($temp_status_entry_private <= 0 && $temp_status_entry_system <= 0 && $temp_status_entry_public <= 0) { // no status filter set, e.g. "all" view
			$roscms_d_tags = true;

			if ($roscms_security_level == 3) { // everything except draft
				$roscms_d_filter2 .= " AND (n.tn_name = 'status' AND v.tv_value != 'draft') ";
			}
			if ($roscms_security_level == 2) { // new, stable and unknown
				$roscms_d_filter2 .= " AND (n.tn_name = 'status' AND (v.tv_value = 'new' OR v.tv_value = 'stable' OR v.tv_value = 'unknown')) ";
			}
			else { // new and stable
				$roscms_d_filter2 .= " AND (n.tn_name = 'status' AND (v.tv_value = 'new' OR v.tv_value = 'stable')) ";
			}
		}
		if ($roscms_security_level < 2 && (($temp_status_entry_private > 0 && $temp_status_entry_public > 0) OR $temp_status_entry_system > 0)) {
			// either show draft (private) OR stable & new (public) entries
			// private AND public entries together are NOT allowed => block 
			$roscms_d_filter2 .= " AND 1=0 "; // simple but effective hack
		}

		if ($roscms_security_level < 3 && $temp_status_entry_private > 0) {
			// make sure only private drafts are visible		
			$roscms_d_filter2 .= " AND r.rev_usrid = '".$roscms_intern_account_id."' ";
		}

		
		if ($roscms_d_stext == true) {
			$roscms_sql_stext = ", data_stext".$RosCMS_d_a." s ";
			$roscms_sql_stext2 = ", s.stext_content ";
			$roscms_sql_stext3 = " AND r.rev_id = s.data_rev_id ";
			
		}
		else {
			$roscms_sql_stext = "";
			$roscms_sql_stext2 = "";
			$roscms_sql_stext3 = "";
		}
		
		if ($roscms_d_tags == true) {
			$roscms_sql_tags .= ", data_tag".$RosCMS_d_a." a, data_tag_name".$RosCMS_d_a." n, data_tag_value".$RosCMS_d_a." v ";
			$roscms_sql_tags2 .= ", n.tn_name, v.tv_value ";
			$roscms_sql_tags3 .= " AND r.data_id = a.data_id 
									AND r.rev_id = a.data_rev_id 
									AND (a.tag_usrid = '-1' OR a.tag_usrid = '0' OR a.tag_usrid = '".mysql_real_escape_string($roscms_intern_account_id)."') 
									AND a.tag_name_id = n.tn_id  
									AND a.tag_value_id  = v.tv_id ";			
		}
		else {
			$roscms_sql_tags = "";
			$roscms_sql_tags2 = "";
			$roscms_sql_tags3 = "";
		}
		
		if ($roscms_d_tags_counter > 0) {
			for ($i = 1; $i <= $roscms_d_tags_counter; $i++) {
				$roscms_sql_tags .= ", data_tag".$RosCMS_d_a." a".$i.", data_tag_name".$RosCMS_d_a." n".$i.", data_tag_value".$RosCMS_d_a." v".$i." ";
				$roscms_sql_tags2 .= ", n".$i.".tn_name, v".$i.".tv_value ";
				$roscms_sql_tags3 .= " AND r.data_id = a".$i.".data_id 
										AND r.rev_id = a".$i.".data_rev_id 
										AND (a".$i.".tag_usrid = '-1' OR a".$i.".tag_usrid = '0' OR a".$i.".tag_usrid = '".mysql_real_escape_string($roscms_intern_account_id)."') 
										AND a".$i.".tag_name_id = n".$i.".tn_id  
										AND a".$i.".tag_value_id  = v".$i.".tv_id ";			
			}
		}
						

		$tzemp = "SELECT COUNT('d.data_id')
											FROM data_revision".$RosCMS_d_a." r, data_".$RosCMS_d_a2." d ".$roscms_sql_stext." ".$roscms_sql_tags." , data_security y
											WHERE r.rev_version >= 0 
											AND r.data_id = d.data_id 
											AND d.data_acl = y.sec_name
											AND y.sec_branch = '".mysql_real_escape_string($roscms_branch)."' 
											". $roscms_security_sql_read ."
											". $roscms_sql_stext3 ."
											". $roscms_sql_tags3 ."
											". $roscms_d_filter1 ."
											". $roscms_d_filter2 ."
											". $roscms_mtbl_order_where;
		//echo "<p>".$tzemp."</p>";
		//echo "<p>".$tzemp." LIMIT ".mysql_real_escape_string($RosCMS_GET_d_curpos) ." , ".$roscms_intern_entry_per_page.";</p>";
		$query_xml_ptm_count = mysql_query($tzemp);
		$result_xml_ptm_count = mysql_fetch_row($query_xml_ptm_count);

	
		
		echo '<?xml version="1.0" encoding="UTF-8"?>
		<root>
		';
	
		if (!$result_xml_ptm_count[0]) {
			echo "  #none#\n";
		}
		else {
			echo "  ".$result_xml_ptm_count[0]."\n";
			echo "  <table>";
			$tdata = "";
			$temp_star = "";
			$temp_star2 = "";
			$temp_status = "";
			$temp_status2 = "";
			$temp_counter = 1;
			$temp_security = "";
			
						
			if ($roscms_mtbl_order != "") {
				$roscms_mtbl_order = " ORDER BY ".substr($roscms_mtbl_order, 0, strlen($roscms_mtbl_order)-2);
			}
			//echo "<p>".$roscms_mtbl_order."</p>";
			/*if ($roscms_mtbl_order == "") {
				$roscms_mtbl_order = "r.rev_id DESC";
			}	*/
			

			
			$tdata .= "    <view curpos=\"".$RosCMS_GET_d_curpos."\" pagelimit=\"".$roscms_intern_entry_per_page."\" pagemax=\"".$result_xml_ptm_count[0]."\" tblcols=\"".$roscms_mtbl_cols."\" /> \n";
			
			
			$sql_xml_ptm = "SELECT d.data_id, d.data_name, d.data_type, d.data_acl, r.rev_id, r.rev_version, r.rev_language, r.rev_datetime, r.rev_date, r.rev_usrid ".$roscms_sql_stext2." ".$roscms_sql_tags2." , y.sec_lev".mysql_real_escape_string($roscms_security_level)."_write  
											FROM data_revision".$RosCMS_d_a." r, data_".$RosCMS_d_a2." d ".$roscms_sql_stext." ".$roscms_sql_tags." , data_security y 
											WHERE r.rev_version >= 0 
											AND r.data_id = d.data_id 
											AND d.data_acl = y.sec_name
											AND y.sec_branch = '".mysql_real_escape_string($roscms_branch)."' 
											". $roscms_security_sql_read ."
											". $roscms_sql_stext3 ."
											". $roscms_sql_tags3 ."
											". $roscms_d_filter1 ."
											". $roscms_d_filter2 ."
											". $roscms_mtbl_order_where ." 
											". $roscms_mtbl_order ." 
											LIMIT ".mysql_real_escape_string($RosCMS_GET_d_curpos)." , ".$roscms_intern_entry_per_page.";";
			
			if ($RosCMS_GET_debug) {
				echo "<p>".$sql_xml_ptm."</p>";
			}
			
			$query_xml_ptm = mysql_query($sql_xml_ptm);	
			while ($result_xml_ptm = mysql_fetch_array($query_xml_ptm)) {
				$temp_star = "";
				$temp_star2 = "";
				$temp_status = "";
				$temp_status2 = "";
				
				
				$temp_status2 = getTagValue($result_xml_ptm['data_id'], $result_xml_ptm['rev_id'],  '-1', 'status');
				$temp_dynamic = getTagValue($result_xml_ptm['data_id'], $result_xml_ptm['rev_id'],  '-1', 'number');
				
				
				if ($roscms_mtbl_translate == "") {
					if ($temp_status2 == "stable") {
						if ($temp_counter%2) {
							$temp_status = "odd";
						}
						else {
							$temp_status = "even";
						}
					}
					else if ($temp_status2 == "new") {
						$temp_status = "new";
					}
					else if ($temp_status2 == "draft") {
						$temp_status = "draft";
					}
					else {
						$temp_status = "unknown";
					}
					
					
					$tblentry_d_id = $result_xml_ptm['data_id'];
					$tblentry_d_id2 = $result_xml_ptm['data_id'];
					$tblentry_d_name = $result_xml_ptm['data_name'];
					$tblentry_d_type = $result_xml_ptm['data_type'];
					$tblentry_d_acl = $result_xml_ptm['data_acl'];
					$tblentry_d_r_id = $result_xml_ptm['rev_id'];
					$tblentry_d_r_ver = $result_xml_ptm['rev_version'];
					$tblentry_d_r_lang = $result_xml_ptm['rev_language'];
					$tblentry_d_r_date = $result_xml_ptm['rev_datetime'];
					$tblentry_d_r_usrid = $result_xml_ptm['rev_usrid'];
				}
				else { // translation
					if ($temp_dynamic != "") {
						$tmp_trans_sql_1 = " , data_tag".$RosCMS_d_a." a, data_tag_name".$RosCMS_d_a." n, data_tag_value".$RosCMS_d_a." v  ";
						$tmp_trans_sql_2 = " AND r.data_id = a.data_id 
											AND r.rev_id = a.data_rev_id 
											AND (a.tag_usrid = '-1') 
											AND a.tag_name_id = n.tn_id  
											AND a.tag_value_id  = v.tv_id 
											AND v.tv_value = '".mysql_real_escape_string($temp_dynamic)."' ";
					}
					else {
						$tmp_trans_sql_1 = "";
						$tmp_trans_sql_2 = "";
					}
					
					$query_xml_ptm_trans = mysql_query("SELECT d.data_id, d.data_name, d.data_type, r.rev_id, r.rev_version, r.rev_language, r.rev_datetime, r.rev_date, r.rev_usrid 
													FROM data_".$RosCMS_d_a2." d, data_revision".$RosCMS_d_a." r ". $tmp_trans_sql_1 ."
													WHERE d.data_id = '".mysql_real_escape_string($result_xml_ptm['data_id'])."'
													AND r.rev_version > 0 
													AND d.data_id = r.data_id 
													AND r.rev_language = '".mysql_real_escape_string($roscms_mtbl_translate)."' 
													". $tmp_trans_sql_2 ."
													LIMIT 1;");
					$result_xml_ptm_trans = mysql_fetch_array($query_xml_ptm_trans);
					
//					echo "<p>DATE: ".$result_xml_ptm_trans['rev_datetime']."</p>";
					
					if ($result_xml_ptm_trans['rev_datetime'] == "") { // translation doesn't exist, so enable "translate mode"
						$temp_status = "transb";

						$tblentry_d_id2 = $result_xml_ptm['data_id'];
						$tblentry_d_name = $result_xml_ptm['data_name'];
						$tblentry_d_type = $result_xml_ptm['data_type'];
						$tblentry_d_acl = $result_xml_ptm['data_acl'];
						
						if (roscms_security_check_kind($result_xml_ptm['data_id'], "trans") == true) {
							$tblentry_d_id = "tr".$result_xml_ptm['data_id'];
							$tblentry_d_r_id = "tr".$result_xml_ptm['rev_id'];
							$tblentry_d_r_date = "translate!";
						}
						else {
							$tblentry_d_id = "notrans";
							$tblentry_d_r_id = "notrans";
							$tblentry_d_r_date = "-";
						}
						
						$tblentry_d_r_ver = "1";
						$tblentry_d_r_lang = $result_xml_ptm['rev_language'];

						$tblentry_d_r_usrid = "";
					}
					else {
						//echo "<p>TIME: |".$result_xml_ptm['rev_date']."| vs. |".$result_xml_ptm_trans['rev_date']."|</p>";
						$temp_datecompare = compareDate($result_xml_ptm['rev_date'], $result_xml_ptm_trans['rev_date']);
						if ($temp_datecompare <= 0) {
							$temp_status = "transg";
						}
						else {
							$temp_status = "transr";
						}

						$tblentry_d_id = $result_xml_ptm_trans['data_id'];
						$tblentry_d_id2 = $result_xml_ptm_trans['data_id'];
						$tblentry_d_name = $result_xml_ptm_trans['data_name'];
						$tblentry_d_type = $result_xml_ptm_trans['data_type'];
						$tblentry_d_acl = $result_xml_ptm['data_acl'];
						$tblentry_d_r_id = $result_xml_ptm_trans['rev_id'];
						$tblentry_d_r_ver = $result_xml_ptm_trans['rev_version'];
						$tblentry_d_r_lang = $result_xml_ptm_trans['rev_language'];
						$tblentry_d_r_date = $result_xml_ptm_trans['rev_datetime'];
						$tblentry_d_r_usrid = $result_xml_ptm_trans['rev_usrid'];
					}
				}
			
				if ($result_xml_ptm['data_type'] == "content" && $temp_dynamic != "") {
					$tblentry_d_name .= "_".$temp_dynamic;
				}
				
				
				$temp_star = getTagValue($tblentry_d_id2, $tblentry_d_r_id,  $roscms_intern_account_id, 'star');
				if ($temp_star == "on") {
					$temp_star = "1";
				}
				else {
					$temp_star = "0";
				}
				$temp_star2 = getTagId($tblentry_d_id2, $tblentry_d_r_id, $roscms_intern_account_id, 'star');
				
				
				$query_xml_ptm_stext = mysql_query("SELECT stext_content as 'content'
												FROM data_stext".$RosCMS_d_a." 
												WHERE data_rev_id = '".mysql_real_escape_string($tblentry_d_r_id)."' 
												AND stext_name = 'description' 
												LIMIT 1;");
				$result_xml_ptm_stext = mysql_fetch_array($query_xml_ptm_stext);
				
				if ($result_xml_ptm_stext['content'] == "") {
					$query_xml_ptm_stext = mysql_query("SELECT stext_content as 'content'
													FROM data_stext".$RosCMS_d_a." 
													WHERE data_rev_id = '".mysql_real_escape_string($tblentry_d_r_id)."' 
													AND stext_name = 'title' 
													LIMIT 1;");
					$result_xml_ptm_stext = mysql_fetch_array($query_xml_ptm_stext);
				}
				

				$roscms_mtbl_cols2 = "|";
				
				for ($i=0; $i < count($roscms_mtbl_colarr); $i++) {
					//$roscms_mtbl_colarr[
					switch ($roscms_mtbl_colarr[$i]) {
						default:
							$roscms_mtbl_cols2 .= "Unknown" ."|";
							break;
						case "Language":
							$query_xml_ptm_lang = mysql_query("SELECT lang_name 
											FROM languages
											WHERE lang_id = '".mysql_real_escape_string($tblentry_d_r_lang)."'
											LIMIT 1;");
							$result_xml_ptm_lang = mysql_fetch_array($query_xml_ptm_lang);
							if ($result_xml_ptm_lang['lang_name'] != "") {
								$roscms_mtbl_cols2 .= $result_xml_ptm_lang['lang_name'] ."|";
							}
							else {
								$roscms_mtbl_cols2 .= "? (".$tblentry_d_r_lang.")" ."|";
							}
							break;
						case "User":
							$query_xml_ptm_lang = mysql_query("SELECT user_name 
											FROM users
											WHERE user_id = '".mysql_real_escape_string($tblentry_d_r_usrid)."'
											LIMIT 1;");
							$result_xml_ptm_lang = mysql_fetch_array($query_xml_ptm_lang);
							if ($result_xml_ptm_lang['user_name'] != "") {
								$roscms_mtbl_cols2 .= $result_xml_ptm_lang['user_name'] ."|";
							}
							else {
								$roscms_mtbl_cols2 .= "Unknown" ."|";
							}
							break;
						case "Type":
							$roscms_mtbl_cols2 .= $tblentry_d_type ."|";
							break;
						case "Security":
							$roscms_mtbl_cols2 .= $tblentry_d_acl ."|";
							break;
						case "Rights":
							$roscms_mtbl_cols2 .= roscms_security_explain($tblentry_d_id2) ."|";
							break;
						case "Version":
							$roscms_mtbl_cols2 .= $tblentry_d_r_ver ."|";
							break;
					}
				}
				
				//if ($result_xml_ptm['sec_lev'.$roscms_security_level.'_write'] == 1) {
				if (roscms_security_check_kind ($tblentry_d_id2, "write") == true) {
					$temp_security = "write";
				}
				else {
					$temp_security = "";
				}
				
								
				$tdata .= "    <row id=\"".$tblentry_d_id."\"";
				$tdata .= " dname=\"".$tblentry_d_name."\"";
				$tdata .= " type=\"".$tblentry_d_type."\"";
				$tdata .= " rid=\"".$tblentry_d_r_id."\"";
				$tdata .= " rver=\"".$tblentry_d_r_ver."\"";
				$tdata .= " rlang=\"".$tblentry_d_r_lang."\"";
				$tdata .= " rdate=\"".$tblentry_d_r_date."\"";
				$tdata .= " rusrid=\"".$tblentry_d_r_usrid."\"";
				$tdata .= " star=\"". $temp_star ."\""; /* starred (on=1, off=0) */
				$tdata .= " starid=\"". $temp_star2 ."\""; /* star tag id (from getTagId(...)) */
				$tdata .= " status=\"". $temp_status ."\""; /* status (odd/even (=stable), new, draft, etc.) */
				$tdata .= " security=\"". $temp_security ."\""; /* security (read, write, add, pub, trans) */
				$tdata .= " xtrcol=\"".$roscms_mtbl_cols2."\"";
				$tdata .= "><![CDATA[".urlencode(substr($result_xml_ptm_stext['content'], 0, 30))."]]></row>";	
				
				$temp_counter++;		
			}
			echo $tdata;
					
			echo "  </table>";
		}
		echo "</root>";
		
	}

?>
