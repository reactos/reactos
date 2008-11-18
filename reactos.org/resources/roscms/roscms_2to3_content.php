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
	

	echo "<h1>RosCMS Database Update - v2 to v3 - Content</h1>";

	set_time_limit(0); // unlimited script run time 


	echo "<h3>Contents</h3>";
	echo "<ul>";
	$query_i_content = mysql_query("SELECT * 
									FROM content
									WHERE content_active = 1
									AND content_lang != 'xhtml'
									ORDER BY content_lang ASC, content_name ASC, content_version DESC, content_active DESC;");
	while ($result_i_content = mysql_fetch_array($query_i_content)) {
		$roscms_i_lang = $result_i_content['content_lang'];		
		if ($roscms_i_lang == "all") {
			$roscms_i_lang = "en";
		}
		if ($roscms_i_lang == "html") {
			$roscms_i_lang = "en";
		}
		
		echo "<li>".$result_i_content['content_name']." (".$result_i_content['content_lang'].")</li>";
		
		$query_import_content = mysql_query("SELECT * 
												FROM data_ 
												WHERE data_name = '".mysql_real_escape_string($result_i_content['content_name'])."'
												AND data_type = 'content'
												LIMIT 1;");
		$result_import_content = mysql_fetch_array($query_import_content);
		
		
		if ($result_import_content['data_id'] == "") {
			$insert_page = mysql_query("INSERT INTO `data_` ( `data_id` , `data_name` , `data_type` , `data_acl`) 
											VALUES (
												NULL , 
												'".mysql_real_escape_string($result_i_content['content_name'])."', 
												'content',
												'default'
											);");
		}
		
		$query_import_content3 = mysql_query("SELECT * 
												FROM data_ 
												WHERE data_name = '".mysql_real_escape_string($result_i_content['content_name'])."'
												AND data_type = 'content'
												ORDER BY data_id DESC
												LIMIT 1;");
		$result_import_content3 = mysql_fetch_array($query_import_content3);
		
		$query_import_content2 = mysql_query("SELECT COUNT(*)
												FROM data_revision
												WHERE data_id = '".mysql_real_escape_string($result_import_content3['data_id'])."'
												AND rev_language = '".mysql_real_escape_string($roscms_i_lang)."'
												LIMIT 1;");
		$result_import_content2 = mysql_fetch_row($query_import_content2);
		
		if ($result_import_content2[0] <= 0) {
			echo "<h6>lang: ".$result_i_content['content_lang']."</h6>";
			// revision entry doesn't exist
			$insert_content_rev = mysql_query("INSERT INTO data_revision ( rev_id , data_id , rev_version , rev_language , rev_usrid , rev_datetime , rev_date , rev_time ) 
												VALUES (
													NULL , 
													'".mysql_real_escape_string($result_import_content3['data_id'])."', 
													'".mysql_real_escape_string($result_i_content['content_version'])."', 
													'".mysql_real_escape_string($roscms_i_lang)."', 
													'".mysql_real_escape_string($result_i_content['content_usrname_id'])."', 
													'".mysql_real_escape_string($result_i_content['content_date']." ".$result_i_content['content_time'])."',
													'".mysql_real_escape_string($result_i_content['content_date'])."',
													'".mysql_real_escape_string($result_i_content['content_time'])."'
												);");
												
		
			$query_import_content4 = mysql_query("SELECT rev_id
													FROM data_revision
													WHERE data_id = '".mysql_real_escape_string($result_import_content3['data_id'])."'
													AND rev_language = '".mysql_real_escape_string($roscms_i_lang)."'
													ORDER BY rev_id DESC
													LIMIT 1;");
			$result_import_content4 = mysql_fetch_array($query_import_content4);
			
			if ($result_import_content4['rev_id'] != "") {
			
				// stext: titel, description, extention, (property)
				$insert_import_content_stext = mysql_query("INSERT INTO data_stext ( stext_id , data_rev_id , stext_name , stext_content ) 
														VALUES 
														(
															NULL , 
															'".mysql_real_escape_string($result_import_content4['rev_id'])."', 
															'description', 
															'".mysql_real_escape_string($result_i_content['content_name'])."'
														);");
				
				// text: content
				$insert_import_content_text = mysql_query("INSERT INTO data_text ( text_id , data_rev_id , text_name , text_content ) 
														VALUES (
															NULL , 
															'".mysql_real_escape_string($result_import_content4['rev_id'])."', 
															'content', 
															'".mysql_real_escape_string($result_i_content['content_text'])."'
														);");
				
				// tag: status = stable
				tag_add($result_import_content3['data_id'], $result_import_content4['rev_id'], "status", "stable", "-1");
				
				// tag: visible = all
				tag_add($result_import_content3['data_id'], $result_import_content4['rev_id'], "visible", "all", "-1");

				// tag: kind = default
				tag_add($result_import_content3['data_id'], $result_import_content4['rev_id'], "kind", "default", "-1");
			}
		}
	}
	echo "</ul>";
			
			
	echo "\n<hr />\n";





	echo "\n<hr />\n";


?>