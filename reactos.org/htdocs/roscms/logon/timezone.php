<?php

	function tz($date_org) {
		global $rdf_user_timezone;
		global $rdf_user_timezone_name;
		global $rdf_server_timezone;
		global $rdf_user_id;
		
		//$date_org = "2008-03-23 10:03:42";
		//$rdf_user_timezone = -8;
		//$rdf_server_timezone = -1;
		
		if ($rdf_user_id > 1) {
			$timezone_str = ($rdf_user_timezone+$rdf_server_timezone)." hours";
		}
		else { // guest visitors get UTC time
			return date("Y-m-d H:i", $date_org);
		}

		//echo $date_org."<br />";
		//echo $timezone_str;
		$basedate = strtotime($date_org);
		$date_new = strtotime($timezone_str, $basedate);
		$date_new = date("Y-m-d H:i", $date_new)." ".$rdf_user_timezone_name;
		//echo "<br />".$date_new;
		
		return $date_new;
	}
	
	function user_timezone() {
		global $rdf_user_timezone;
		
		return timezone_string($rdf_user_timezone);
	}
	
	function timezone_string($timezone_value) {
		if ($timezone_value >= 0 && $timezone_value < 10) {
			$timezone_str = "+0".number_format($timezone_value, 2, '', '');
		}
		else if ($timezone_value >= 10) {
			$timezone_str = "+".number_format($timezone_value, 2, '', '');
		}
		else if ($timezone_value > -10 && $timezone_value < 0) {
			$timezone_str = "-0".substr(number_format($timezone_value, 2, '', ''), 1);
		}
		else if ($timezone_value <= -10) {
			$timezone_str = number_format($timezone_value, 2, '', '');
		}
		
		return $timezone_str;
	}
	
	function user_check_timezone($tmp_tz) {
		$sql_timezone = "SELECT tz_code, tz_name, tz_value2   
							FROM user_timezone 
							WHERE tz_code = '".mysql_real_escape_string($tmp_tz)."'
							LIMIT 1;";
		$query_timezone = mysql_query($sql_timezone);
		$result_timezone = mysql_fetch_array($query_timezone);
		if ($result_timezone['tz_name'] != "") {
			return 1;
		}
		else {
			return 0;
		}
	}
	
	function user_check_lang($tmp_lang) {
		$sql_language = "SELECT lang_id, lang_name    
							FROM user_language 
							WHERE lang_id = '".mysql_real_escape_string($tmp_lang)."'
							LIMIT 1;";
		$query_language = mysql_query($sql_language);
		$result_language = mysql_fetch_array($query_language);
		if ($result_language['lang_name'] != "") {
			return 1;
		}
		else {
			return 0;
		}
	}
	
	function user_check_country($tmp_country) {
		$sql_country = "SELECT coun_id, coun_name  
						FROM user_countries 
						WHERE coun_id = '".mysql_real_escape_string($tmp_country)."'
						LIMIT 1;";
		$query_country = mysql_query($sql_country);
		$result_country = mysql_fetch_array($query_country);
		if ($result_country['coun_name'] != "") {
			return 1;
		}
		else {
			return 0;
		}
	}

	
?>