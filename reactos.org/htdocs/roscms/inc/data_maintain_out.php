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
	global $RosCMS_GET_d_value3;
	global $RosCMS_GET_d_value4;
	
	require("inc/data_export_page.php");


	if ($RosCMS_GET_d_use == "optimize") {
		$query_db_optimize = mysql_query("OPTIMIZE TABLE data_, data_a, data_revision, data_revision_a, data_security, data_stext, data_stext_a, data_tag, data_tag_a, data_tag_name, data_tag_name_a, data_tag_value, data_tag_value_a, data_text, data_text_a, data_user_filter, languages, subsys_mappings, usergroups, usergroup_members, users, user_sessions;");
		log_event_high("optimize database tables: done by ".$roscms_intern_account_id." {data_maintain_out}");
	}
	else if ($RosCMS_GET_d_use == "analyze") {
		$query_db_optimize = mysql_query("ANALYZE TABLE data_, data_a, data_revision, data_revision_a, data_security, data_stext, data_stext_a, data_tag, data_tag_a, data_tag_name, data_tag_name_a, data_tag_value, data_tag_value_a, data_text, data_text_a, data_user_filter, languages, subsys_mappings, usergroups, usergroup_members, users, user_sessions;");
		log_event_high("analyze database tables: done by ".$roscms_intern_account_id." {data_maintain_out}");
	}
	else if ($RosCMS_GET_d_use == "genpages") {
		//echo generate_page_output("index", "en", "");
		$gentimeb="";
		$gentimeb = microtime(); 
		$gentimeb = explode(' ',$gentimeb); 
		$gentimeb = $gentimeb[1] + $gentimeb[0]; 
		$pg_startb = $gentimeb; 
		
		log_event_generate_high("generate all pages - start");
		
		echo generate_page_output("index", "all", "", "all");
		
		$gentimec = microtime(); 
		$gentimec = explode(' ',$gentimec); 
		$gentimec = $gentimec[1] + $gentimec[0]; 
		$pg_endb = $gentimec; 
		$totaltimef = ($pg_endb - $pg_startb); 
		$showtimef = number_format($totaltimef, 4, '.', ''); 
		
		log_event_generate_high("generate all pages - end: ".$showtimef." seconds");
	}
	else if ($RosCMS_GET_d_use == "pupdate") {
		echo "<h4>Generate related pages of ".$RosCMS_GET_d_value." (".$RosCMS_GET_d_value2.", ".$RosCMS_GET_d_value3.", ".$RosCMS_GET_d_value4.")</h4>";
		
		$query_entry = mysql_query("SELECT data_id 
									FROM data_ 
									WHERE data_name = '".mysql_real_escape_string($RosCMS_GET_d_value)."'
									AND data_type = '".mysql_real_escape_string($RosCMS_GET_d_value2)."'
									LIMIT 1;");
		$result_entry = mysql_fetch_array($query_entry);

		//echo "<p>generate_page_output_update(".$result_entry['data_id'].", ".$RosCMS_GET_d_value3.", ".$RosCMS_GET_d_value4.")</p>";
		echo generate_page_output_update($result_entry['data_id'], $RosCMS_GET_d_value3, $RosCMS_GET_d_value4);
	}
?>
