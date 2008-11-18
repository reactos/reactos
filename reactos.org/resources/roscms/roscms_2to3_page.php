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
	

	echo "<h1>RosCMS Database Update - v2 to v3 - Page</h1>";

	set_time_limit(0); // unlimited script run time 

	echo "<h3>Pages</h3>";
	echo "<ul>";
	$query_i_page = mysql_query("SELECT * 
									FROM pages
									WHERE page_active = 1
									ORDER BY page_name ASC, page_version DESC, page_active DESC;");
	while ($result_i_page = mysql_fetch_array($query_i_page)) {
		$roscms_i_lang = $result_i_page['page_language'];		
		if ($roscms_i_lang == "all") {
			$roscms_i_lang = "en";
		}
		
		echo "<li>".$result_i_page['page_name']." (".$roscms_i_lang.")</li>";
		
		$query_import_page = mysql_query("SELECT * 
												FROM data_ 
												WHERE data_name = '".mysql_real_escape_string($result_i_page['page_name'])."'
												AND data_type = 'page'
												LIMIT 1;");
		$result_import_page = mysql_fetch_array($query_import_page);
		echo "<h6>data-ID: ".$result_import_page['data_id']."</h6>";
		
		
		if ($result_import_page['data_id'] == "") {
			$insert_page = mysql_query("INSERT INTO `data_` ( `data_id` , `data_name` , `data_type` , `data_acl` ) 
											VALUES (
												NULL , 
												'".mysql_real_escape_string($result_i_page['page_name'])."', 
												'page',
												'defaultpage'
											);");
		}
		
		
		$query_import_page3 = mysql_query("SELECT * 
												FROM data_ 
												WHERE data_name = '".mysql_real_escape_string($result_i_page['page_name'])."'
												AND data_type = 'page'
												LIMIT 1;");
		$result_import_page3 = mysql_fetch_array($query_import_page3);
		
		echo "<p>data_id: ".$result_import_page3['data_id']."</p>";
		
		$query_import_page2 = mysql_query("SELECT COUNT(*)
												FROM data_revision
												WHERE data_id = '".mysql_real_escape_string($result_import_page3['data_id'])."'
												AND rev_language = '".mysql_real_escape_string($roscms_i_lang)."'
												LIMIT 1;");
		$result_import_page2 = mysql_fetch_row($query_import_page2);
		echo "<p>data_id-count: ".$result_import_page2[0]."</p>";
		
		if ($result_import_page2[0] <= 0) {
			// revision entry doesn't exist
			$insert_page_rev = mysql_query("INSERT INTO data_revision ( rev_id , data_id , rev_version , rev_language , rev_usrid , rev_datetime , rev_date , rev_time ) 
												VALUES (
													NULL , 
													'".mysql_real_escape_string($result_import_page3['data_id'])."', 
													'".mysql_real_escape_string($result_i_page['page_version'])."', 
													'".mysql_real_escape_string($roscms_i_lang)."', 
													'".mysql_real_escape_string($result_i_page['page_usrname_id'])."', 
													'".mysql_real_escape_string($result_i_page['page_date']." ".$result_i_page['page_time'])."',
													'".mysql_real_escape_string($result_i_page['page_date'])."',
													'".mysql_real_escape_string($result_i_page['page_time'])."'
												);");
												
		
			$query_import_page4 = mysql_query("SELECT rev_id
													FROM data_revision
													WHERE data_id = '".mysql_real_escape_string($result_import_page3['data_id'])."'
													AND rev_language = '".mysql_real_escape_string($roscms_i_lang)."'
													ORDER BY rev_id DESC
													LIMIT 1;");
			$result_import_page4 = mysql_fetch_array($query_import_page4);
			
			if ($result_import_page4['rev_id'] != "") {
			
				if ($result_i_page['pages_extention'] == "default") {
					$roscms_i_page_extention = "html";
				}
				else {
					$roscms_i_page_extention = $result_i_page['pages_extention'];
				}
				
				/*
					if ($result_i_page['pages_extra'] == "dynamic") {
						$roscms_i_page_extra = ", (
													NULL , 
													'".mysql_real_escape_string($result_import_page4['rev_id'])."', 
													'property', 
													'".mysql_real_escape_string($result_i_page['pages_extra'])."'
												)";
					}
					else {
						$roscms_i_page_extra = "";
					}
					".$roscms_i_page_extra."
					,(
						NULL , 
						'".mysql_real_escape_string($result_import_page4['rev_id'])."', 
						'extention', 
						'".mysql_real_escape_string($roscms_i_page_extention)."'
					)
				*/

				// stext: titel, description, extention, (property)
				$insert_import_page_stext = mysql_query("INSERT INTO data_stext ( stext_id , data_rev_id , stext_name , stext_content ) 
														VALUES 
														(
															NULL , 
															'".mysql_real_escape_string($result_import_page4['rev_id'])."', 
															'description', 
															'".mysql_real_escape_string($result_i_page['page_description'])."'
														),
														(
															NULL , 
															'".mysql_real_escape_string($result_import_page4['rev_id'])."', 
															'title', 
															'".mysql_real_escape_string($result_i_page['page_title'])."'
														)
														;");
				
				// text: content
				$insert_import_page_text = mysql_query("INSERT INTO data_text ( text_id , data_rev_id , text_name , text_content ) 
														VALUES (
															NULL , 
															'".mysql_real_escape_string($result_import_page4['rev_id'])."', 
															'content', 
															'".mysql_real_escape_string($result_i_page['page_text'])."'
														);");
				
				// tag: status = stable
				tag_add($result_import_page3['data_id'], $result_import_page4['rev_id'], "status" /* name */, "stable" /* value */, "-1" /* usrid */);

				if ($result_i_page['pages_extra'] == "dynamic") {
					// tag: kind = dynamic
					tag_add($result_import_page3['data_id'], $result_import_page4['rev_id'], "kind" /* name */, "dynamic" /* value */, "-1" /* usrid */);
				}
				else {
					// tag: kind = default
					tag_add($result_import_page3['data_id'], $result_import_page4['rev_id'], "kind" /* name */, "default" /* value */, "-1" /* usrid */);
				}

				if ($result_i_page['pages_extention'] == "default") {
					// tag: extention = html
					tag_add($result_import_page3['data_id'], $result_import_page4['rev_id'], "extention" /* name */, "html" /* value */, "-1" /* usrid */);
				}
				else {
					// tag: extention = ...
					tag_add($result_import_page3['data_id'], $result_import_page4['rev_id'], "extention" /* name */, $result_i_page['pages_extention'] /* value */, "-1" /* usrid */);
				}
			}
		}
	}
	echo "</ul>";


	echo "\n<hr />\n";



	echo "\n<hr />\n";


?>