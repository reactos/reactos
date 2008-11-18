<?php

	error_reporting(E_ALL);
	ini_set('error_reporting', E_ALL);
	
	if (get_magic_quotes_gpc()) {
		die("ERROR: Disable 'magic quotes' in php.ini (=Off)");
	}
	
	if ( !defined('ROSCMS_SYSTEM') ) {
		define ("ROSCMS_SYSTEM", "Version 0.1"); // to prevent hacking activity
	}


	require("connect.db.php");
	require("inc/data_edit.php");
	

	echo "<h1>RosCMS Database Update - v2 to v3 - Script</h1>";

	set_time_limit(0); // unlimited script run time 
		


	echo "<h3>Scripts</h3>";
	echo "<ul>";
	$query_i_script = mysql_query("SELECT * 
									FROM include_text
									WHERE inc_vis  = '1'
									ORDER BY inc_word ASC;");
	while ($result_i_script = mysql_fetch_array($query_i_script)) {
		echo "<li>".$result_i_script['inc_word']."</li>";
		
		$query_import_script = mysql_query("SELECT * 
												FROM data_ 
												WHERE data_name = '".mysql_real_escape_string($result_i_script['inc_word'])."'
												AND data_type = 'script'
												LIMIT 1;");
		$result_import_script = mysql_fetch_array($query_import_script);
		
		
		if ($result_import_script['data_id'] == "") {
			$insert_page = mysql_query("INSERT INTO `data_` ( `data_id` , `data_name` , `data_type` , `data_acl` ) 
											VALUES (
												NULL , 
												'".mysql_real_escape_string($result_i_script['inc_word'])."', 
												'script',
												'script'
											);");
		}
		
		$query_import_script3 = mysql_query("SELECT * 
												FROM data_ 
												WHERE data_name = '".mysql_real_escape_string($result_i_script['inc_word'])."'
												AND data_type = 'script'
												ORDER BY data_id DESC
												LIMIT 1;");
		$result_import_script3 = mysql_fetch_array($query_import_script3);
		
		echo "<p>data_id: ".$result_import_script3['data_id']."</p>";
		
		$query_import_script2 = mysql_query("SELECT COUNT(*)
												FROM data_revision
												WHERE data_id = '".mysql_real_escape_string($result_import_script3['data_id'])."'
												AND rev_language = 'en'
												LIMIT 1;");
		$result_import_script2 = mysql_fetch_row($query_import_script2);
		
		echo "<p>data_id-count: ".$result_import_script2[0]."</p>";
		
		if ($result_import_script2[0] <= 0) {
			// revision entry doesn't exist
			$insert_script_rev = mysql_query("INSERT INTO data_revision ( rev_id , data_id , rev_version , rev_language , rev_usrid , rev_datetime , rev_date , rev_time ) 
												VALUES (
													NULL , 
													'".mysql_real_escape_string($result_import_script3['data_id'])."', 
													'1', 
													'en', 
													'".mysql_real_escape_string($result_i_script['inc_usrname_id'])."', 
													'".mysql_real_escape_string($result_i_script['inc_date']." ".$result_i_script['inc_time'])."',
													'".mysql_real_escape_string($result_i_script['inc_date'])."',
													'".mysql_real_escape_string($result_i_script['inc_time'])."'
												);");
			
												
		
			$query_import_script4 = mysql_query("SELECT rev_id
													FROM data_revision
													WHERE data_id = '".mysql_real_escape_string($result_import_script3['data_id'])."'
													AND rev_language = 'en'
													ORDER BY rev_id DESC
													LIMIT 1;");
			$result_import_script4 = mysql_fetch_array($query_import_script4);
			
			if ($result_import_script4['rev_id'] != "") {
			
				// stext: titel, description
				$insert_import_script_stext = mysql_query("INSERT INTO data_stext ( stext_id , data_rev_id , stext_name , stext_content ) 
														VALUES 
														(
															NULL , 
															'".mysql_real_escape_string($result_import_script4['rev_id'])."', 
															'description', 
															'".mysql_real_escape_string($result_i_script['inc_word'])."'
														);");
				
				// text: content
				$insert_import_script_text = mysql_query("INSERT INTO data_text ( text_id , data_rev_id , text_name , text_content ) 
														VALUES (
															NULL , 
															'".mysql_real_escape_string($result_import_script4['rev_id'])."', 
															'content', 
															'".mysql_real_escape_string($result_i_script['inc_text'])."'
														);");
				
				// tag: status = stable
				tag_add($result_import_script3['data_id'], $result_import_script4['rev_id'], "status", "stable", "-1");
				
				// tag: visible = admin
				tag_add($result_import_script3['data_id'], $result_import_script4['rev_id'], "visible", "admin", "-1");
				
				if ($result_i_script['inc_extra'] == "template_php") {
					// tag: kind = php
					tag_add($result_import_script3['data_id'], $result_import_script4['rev_id'], "kind", "php", "-1");
				}
				else {
					// tag: kind = var
					tag_add($result_import_script3['data_id'], $result_import_script4['rev_id'], "kind", "var", "-1");
				}
			}
		}
	}
	echo "</ul>";


	echo "\n<hr />\n";




	echo "\n<hr />\n";


?>