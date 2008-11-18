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
	

	echo "<h1>RosCMS Database Update - v2 to v3 - DynContent</h1>";

	set_time_limit(0); // unlimited script run time 

	echo "<h3>Dynamic Contents</h3>";
	echo "<ul>";
	$query_i_content = mysql_query("SELECT * 
									FROM dyn_content
									WHERE dyn_content_active = 1
									AND dyn_content_nr = 1
									AND dyn_content_lang = 'all'
									ORDER BY dyn_content_lang ASC, dyn_content_name ASC, dyn_content_id ASC;");
	while ($result_i_content = mysql_fetch_array($query_i_content)) {
		$roscms_i_lang = $result_i_content['dyn_content_lang'];		
		if ($roscms_i_lang == "all") {
			$roscms_i_lang = "en";
		}
		
		echo "<li>".$result_i_content['dyn_content_name']."_".$result_i_content['dyn_content_id']." (".$result_i_content['dyn_content_lang'].")</li>";
		
		$query_import_content = mysql_query("SELECT * 
												FROM data_ 
												WHERE data_name = '".mysql_real_escape_string($result_i_content['dyn_content_name'])."'
												AND data_type = 'content'
												LIMIT 1;");
		$result_import_content = mysql_fetch_array($query_import_content);
		
		
		if ($result_import_content['data_id'] == "") {
			$insert_page = mysql_query("INSERT INTO `data_` ( `data_id` , `data_name` , `data_type` , `data_acl` ) 
											VALUES (
												NULL , 
												'".mysql_real_escape_string($result_i_content['dyn_content_name'])."', 
												'content',
												'default'
											);");
		}
		
		$query_import_content3 = mysql_query("SELECT * 
												FROM data_ 
												WHERE data_name = '".mysql_real_escape_string($result_i_content['dyn_content_name'])."'
												AND data_type = 'content'
												ORDER BY data_id DESC
												LIMIT 1;");
		$result_import_content3 = mysql_fetch_array($query_import_content3);
		
		
		// revision entry doesn't exist
		$insert_content_rev = mysql_query("INSERT INTO data_revision ( rev_id , data_id , rev_version , rev_language , rev_usrid , rev_datetime , rev_date , rev_time ) 
											VALUES (
												NULL , 
												'".mysql_real_escape_string($result_import_content3['data_id'])."', 
												'1', 
												'".mysql_real_escape_string($roscms_i_lang)."', 
												'".mysql_real_escape_string($result_i_content['dyn_content_usrname_id'])."', 
												'".mysql_real_escape_string($result_i_content['dyn_content_date']." ".$result_i_content['dyn_content_time'])."',
												'".mysql_real_escape_string($result_i_content['dyn_content_date'])."',
												'".mysql_real_escape_string($result_i_content['dyn_content_time'])."'
											);");
		echo "<li>&nbsp;&nbsp; =&gt; ".$result_i_content['dyn_content_name']."_".$result_i_content['dyn_content_id']." (".$result_i_content['dyn_content_text1'].")</li>";
											
	
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
														'".mysql_real_escape_string($result_i_content['dyn_content_text3'])."'
													),
													(
														NULL , 
														'".mysql_real_escape_string($result_import_content4['rev_id'])."', 
														'title', 
														'".mysql_real_escape_string($result_i_content['dyn_content_text1'])."'
													);");
			
			// text: content
			$insert_import_content_text = mysql_query("INSERT INTO data_text ( text_id , data_rev_id , text_name , text_content ) 
													VALUES (
														NULL , 
														'".mysql_real_escape_string($result_import_content4['rev_id'])."', 
														'content', 
														'".mysql_real_escape_string($result_i_content['dyn_content_text4'])."'
													);");
			
			// tag: status = stable
			tag_add($result_import_content3['data_id'], $result_import_content4['rev_id'], "status", "stable", "-1");
			
			// tag: visible = all
			tag_add($result_import_content3['data_id'], $result_import_content4['rev_id'], "visible", "all", "-1");

			// tag: kind = dynamic
			tag_add($result_import_content3['data_id'], $result_import_content4['rev_id'], "kind", "dynamic", "-1");
			
			// tag: number = ...
			tag_add($result_import_content3['data_id'], $result_import_content4['rev_id'], "number", $result_i_content['dyn_content_id'], "-1");
		}
	}
	echo "</ul>";
			
			
	echo "\n<hr />\n";



	echo "\n<hr />\n";


?>