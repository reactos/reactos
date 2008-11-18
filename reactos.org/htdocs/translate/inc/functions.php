<?php

	function is_language ($ROST_temp_lang) {
		$query_langid=mysql_query("SELECT COUNT(*) 
									FROM languages 
									WHERE lang_active = 1
									AND lang_id = '".mysql_real_escape_string($ROST_temp_lang)."' 
									ORDER BY lang_id ASC ;");	
		$result_langid = mysql_fetch_row($query_langid);
		//echo "<br />aaa: ".$result_langid[0];
		if ($result_langid[0] > 0 || $ROST_temp_lang == "all") {
			return true;
		}
		return false;
	}

	function is_translations ($ROST_temp_trans) {
		$query_transid=mysql_query("SELECT COUNT(*) 
									FROM apps 
									WHERE app_enabled = 1
									AND app_name = '".mysql_real_escape_string($ROST_temp_trans)."' 
									ORDER BY app_name ASC ;");	
		$result_transid = mysql_fetch_row($query_transid);
		//echo "<br />aaa: ".$result_langid[0];
		if ($result_transid[0] > 0 || $ROST_temp_trans == "all") {
			return true;
		}
		return false;
	}


?>
