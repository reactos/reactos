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

	global $h_a;
	global $h_a2;


	function getTagId($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_intern_current_usrid, $RosCMS_intern_current_tag_name) {
		global $h_a;
		global $h_a2;

		// tag name
		$query_edit_mef_tag_get_id = mysql_query("SELECT tn_id, tn_name 
													FROM data_tag_name".$h_a." 
													WHERE tn_name = '".mysql_real_escape_string($RosCMS_intern_current_tag_name)."'
													LIMIT 1;");
		$result_edit_mef_tag_get_id = mysql_fetch_array($query_edit_mef_tag_get_id);
		
		// tag
		$query_edit_mef_tag_get_id = mysql_query("SELECT tag_id 
												FROM data_tag".$h_a." 
												WHERE data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."'
												AND data_rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."'
												AND tag_name_id = '".mysql_real_escape_string($result_edit_mef_tag_get_id['tn_id'])."'
												AND tag_usrid = '".mysql_real_escape_string($RosCMS_intern_current_usrid)."'
												LIMIT 1;");
		$result_edit_mef_tag_get_id = mysql_fetch_array($query_edit_mef_tag_get_id);
		
		return $result_edit_mef_tag_get_id['tag_id'];
	}
	
	function getTagValue($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_intern_current_usrid, $RosCMS_intern_current_tag_name) {
		global $h_a;
		global $h_a2;

		//echo "<p>=> getTagValue(".$RosCMS_GET_d_id.", ".$RosCMS_GET_d_r_id.", ".$RosCMS_intern_current_usrid.", ".$RosCMS_intern_current_tag_name.")</p>";
		// tag name
		$query_edit_mef_tag_get_id = mysql_query("SELECT tn_id, tn_name 
													FROM data_tag_name".$h_a." 
													WHERE tn_name = '".mysql_real_escape_string($RosCMS_intern_current_tag_name)."'
													LIMIT 1;");
		$result_edit_mef_tag_get_id = mysql_fetch_array($query_edit_mef_tag_get_id);
		
		//echo "<p>tagname-ID: ".$result_edit_mef_tag_get_id['tn_id']."</p>";
		
		// tag
		$query_edit_mef_tag_get_id_val = mysql_query("SELECT tag_value_id 
												FROM data_tag".$h_a." 
												WHERE data_id = '".mysql_real_escape_string($RosCMS_GET_d_id)."'
												AND data_rev_id = '".mysql_real_escape_string($RosCMS_GET_d_r_id)."'
												AND tag_name_id = '".mysql_real_escape_string($result_edit_mef_tag_get_id['tn_id'])."'
												AND tag_usrid = '".mysql_real_escape_string($RosCMS_intern_current_usrid)."'
												LIMIT 1;");
												
		$result_edit_mef_tag_get_id_val = mysql_fetch_array($query_edit_mef_tag_get_id_val);

		//echo "<p>tagvalue-ID: ".$result_edit_mef_tag_get_id_val['tag_value_id']."</p>";
		
		// tag value
		$query_edit_mef_tag_get_value = mysql_query("SELECT tv_value 
													FROM data_tag_value".$h_a." 
													WHERE tv_id = '".mysql_real_escape_string($result_edit_mef_tag_get_id_val['tag_value_id'])."'
													LIMIT 1;");
		$result_edit_mef_tag_get_value = mysql_fetch_array($query_edit_mef_tag_get_value);
		
		return $result_edit_mef_tag_get_value['tv_value'];
	}



?>