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
	
	$g_lang = "";
	$g_cur_page_name = "";
	$g_page_dynid = "";
	
	
	function generate_page_output_update($g_data_id, $g_lang_id, $g_page_dynida) {
		//echo "<p>!!generate_page_output_update(".$g_data_id.", ".$g_lang_id.", ".$g_page_dynida.")</p>";
		global $roscms_standard_language;

		$query_data = mysql_query("SELECT * 
									FROM data_ d, data_revision r 
									WHERE r.data_id = '".mysql_real_escape_string($g_data_id)."' 
									AND r.data_id = d.data_id 
									AND (r.rev_language = '".mysql_real_escape_string($g_lang_id)."' 
										OR r.rev_language = '".mysql_real_escape_string($roscms_standard_language)."')
									AND rev_version > 0 
									LIMIT 2;");
		$result_data = mysql_fetch_assoc($query_data);
		
		// Try to get the dataset with rev_language == $g_lang, to boost the translated content
		if( mysql_num_rows($query_data) == 2 ) {
			if( $result_data['rev_language'] == $roscms_standard_language ) {
				$result_data = mysql_fetch_assoc($query_data);
			}
		}
			

		if ($g_lang_id == $roscms_standard_language) {
			$tmp_lang = "all";
		}
		else {
			$tmp_lang = $g_lang_id;
		}


		log_event_generate_low("-=&gt; ".$result_data['data_name']." (".$tmp_lang.") type: ".$result_data['data_type']." [generate_page_output_update(".$g_data_id.", ".$g_lang_id.", ".$g_page_dynida.")]");


		switch ($result_data['data_type']) {
			case 'page':
				echo "<p>generate_page_output(".$result_data['data_name'].", ".$tmp_lang.", ".$g_page_dynida.")</p>";
				log_event_generate_low($result_data['data_name']." (".$tmp_lang.")  type: ".$result_data['data_type']." ".$g_page_dynida);
				log_event_generate_medium($result_data['data_name']." (".$tmp_lang.") ".$g_page_dynida);
				generate_page_output($result_data['data_name'], $tmp_lang, $g_page_dynida);
				break;
			case 'template':
				echo "<p>(a) generate_update_helper(".$tmp_lang.", ".$result_data['data_type'].", ".$result_data['data_name'].")</p>";
				generate_update_helper($tmp_lang, $result_data['data_type'], $result_data['data_name']);
				break;
			case 'content':
				$tmp_dynamic = getTagValueG($result_data['data_id'], $result_data['rev_id'],  '-1', 'number'); // get dynamic content number
				if ($tmp_dynamic != "" && $result_data['data_type'] == "content") {
					echo "<p>#==&gt; ".$result_data['data_name']."_".$g_page_dynida." (".$result_data['data_type']."; ".$tmp_lang.")</p>";
					log_event_generate_low($result_data['data_name']." (".$tmp_lang.")  type: ".$result_data['data_type']." ".$g_page_dynida);
					log_event_generate_medium($result_data['data_name']." (".$tmp_lang.") ".$g_page_dynida);
					generate_page_output($result_data['data_name'], $tmp_lang, $g_page_dynida);
				}
				else {
					echo "<p>(b) generate_update_helper(".$tmp_lang.", ".$result_data['data_type'].", ".$result_data['data_name'].")</p>";
					generate_update_helper($tmp_lang, $result_data['data_type'], $result_data['data_name']);
				}
				break;
			case 'script':
				echo "<p>(c) generate_update_helper(".$tmp_lang.", ".$result_data['data_type'].", ".$result_data['data_name'].")</p>";
				generate_update_helper($tmp_lang, $result_data['data_type'], $result_data['data_name']);
				break;
			default:
			case 'system':
				// do nothing
				break;
		}
	}

	function generate_update_helper($h_lang, $h_data_type, $h_like) {
		global $roscms_standard_language;
		
		switch ($h_data_type) {
			case 'template':
				$tmp_type = "templ";
				$tmp_type_sql = "";
				break;	
			case 'content':
				$tmp_type = "cont";
				$tmp_type_sql = " OR d.data_type = 'template' ";
				break;	
			case 'script':
				$tmp_type = "inc";
				$tmp_type_sql = " OR d.data_type = 'template' OR d.data_type = 'content' OR d.data_type = 'script' ";
				break;
			default:
				die("should never happen: generate_update_helper(".$h_data_type.", ".$h_like.")");
				break;
		}
			
		log_event_generate_low("-=&copy; ".$h_lang." (".$h_like.") type: ".$h_data_type);
	

		$query_data = mysql_query("SELECT d.data_name, d.data_type, r.data_id, r.rev_id, r.rev_language 
									FROM data_ d, data_revision r, data_text t 
									WHERE (d.data_type = 'page' ".$tmp_type_sql." )
									AND r.data_id = d.data_id 
									AND r.rev_id = t.data_rev_id 
									AND t.text_content  LIKE '%[#".$tmp_type."_".mysql_real_escape_string($h_like)."]%' 
									AND r.rev_language = '".mysql_real_escape_string($h_lang)."'
									AND r.rev_version > 0;");
		while ($result_data = mysql_fetch_array($query_data)) {
			$tmp_dynamic = getTagValueG($result_data['data_id'], $result_data['rev_id'],  '-1', 'number'); // get dynamic content number
			
			if ($result_data['rev_language'] == $roscms_standard_language) {
				$tmp_lang = "all";
			}
			else {
				$tmp_lang = $result_data['rev_language'];
			}

			if ($result_data['data_type'] == "page") {
				echo "<p>=&gt; ".$result_data['data_name']." (".$result_data['data_type']."; ".$tmp_lang.")</p>";
				log_event_generate_low($result_data['data_name']." (".$tmp_lang.") type: ".$result_data['data_type']);
				log_event_generate_medium($result_data['data_name']." (".$tmp_lang.") ");
				generate_page_output($result_data['data_name'], $tmp_lang, "");
			}
			else {
				if ($tmp_dynamic != "" && $result_data['data_type'] == "content") {
					echo "<p>==&gt; ".$result_data['data_name']."_".$tmp_dynamic." (".$result_data['data_type']."; ".$tmp_lang.")</p>";
					log_event_generate_low($result_data['data_name']." (".$tmp_lang.") type: ".$result_data['data_type']." ".$tmp_dynamic);
					log_event_generate_medium($result_data['data_name']." (".$tmp_lang.") ".$tmp_dynamic);
					generate_page_output($result_data['data_name'], $tmp_lang, $tmp_dynamic);
				}
				echo "<p> ~ ".$result_data['data_name']." (".$result_data['data_type']."; ".$tmp_lang.")</p>";
				generate_update_helper($tmp_lang, $result_data['data_type'], $result_data['data_name']);
			}
		}
	}

	function generate_page_output($g_page_name, $g_page_lang, $g_page_dynida, $g_mode = "single") {
		
		global $roscms_extern_brand;
		global $roscms_extern_version;
		global $roscms_extern_version_detail;
		global $roscms_standard_language;


		$tmp_lang_cur = "";
		if ($g_page_lang == "all") {
			$tmp_lang_sql = "SELECT lang_id, lang_name  
								FROM languages  
								ORDER BY lang_id ASC;";
		}
		else {
			$tmp_lang_sql = "SELECT lang_id, lang_name  
								FROM languages 
								WHERE lang_id = '".mysql_real_escape_string($g_page_lang)."' 
								LIMIT 1;";
		}
		
		$query_g_lang = mysql_query($tmp_lang_sql);
		while ($result_g_lang = mysql_fetch_array($query_g_lang)) {
			if ($result_g_lang['lang_id'] != $tmp_lang_cur) {
				 $tmp_lang_cur = $result_g_lang['lang_id'];
				 echo "<p><b><u>".$result_g_lang['lang_name']."</u></b></p>";
			}

			if ($g_mode == "single") {
				$tmp_single_sql = " LIMIT 1";
				$tmp_single_sql2 = " AND data_name = '".mysql_real_escape_string($g_page_name)."' ";
			}
			else {
				$tmp_single_sql = "";
				$tmp_single_sql2 = "";
			}
	
			$query_g_page = mysql_query("SELECT d.data_name, r.data_id, r.rev_id, r.rev_language   
										FROM data_ d, data_revision r 
										WHERE data_type = 'page'
										AND r.data_id = d.data_id
										AND r.rev_version > 0
										AND (r.rev_language = '".mysql_real_escape_string($result_g_lang['lang_id'])."'
											OR r.rev_language = '".mysql_real_escape_string($roscms_standard_language)."')
										".$tmp_single_sql2."
										ORDER BY r.rev_version DESC
										".$tmp_single_sql.";");
			while ($result_g_page = mysql_fetch_array($query_g_page)) {
		
				$temp_dynamic = getTagValueG($result_g_page['data_id'], $result_g_page['rev_id'],  '-1', 'kind'); // get content kind (dynamic ?)
				
				if ($temp_dynamic == "dynamic" && $g_page_dynida == "") {
					//echo "<p>dynamic</p>";
					//$temp_dynamic_number = getTagValueG($result_g_page['data_id'], $result_g_page['rev_id'],  '-1', 'number'); // get dynamic content number
					$tmp_dynamic_sql = "SELECT r.rev_id, r.rev_version, r.rev_usrid, r.rev_datetime, r.rev_date, r.rev_time, v.tv_value   
													FROM data_ d, data_revision r, data_tag a, data_tag_name n, data_tag_value v 
													WHERE data_name = '".mysql_real_escape_string($result_g_page['data_name'])."' 
													AND data_type = 'content'
													AND r.data_id = d.data_id
													AND r.rev_version > 0
													AND (r.rev_language = '".mysql_real_escape_string($result_g_lang['lang_id'])."' 
														OR r.rev_language = '".mysql_real_escape_string($roscms_standard_language)."') 
													AND r.data_id = a.data_id 
													AND r.rev_id = a.data_rev_id 
													AND a.tag_usrid = '-1'  
													AND a.tag_name_id = n.tn_id  
													AND a.tag_value_id  = v.tv_id
													AND n.tn_name = 'number'
													ORDER BY v.tv_value ASC;";
				}
				else {
					$tmp_dynamic_sql = "SELECT 1 = 1 LIMIT 1;";
				}
		
				$query_g_page_dyn = mysql_query($tmp_dynamic_sql);
				while ($result_g_page_dyn = mysql_fetch_array($query_g_page_dyn)) {
					if ($temp_dynamic == "dynamic" && $g_page_dynida == "") {
						$temp_dynamic_number = $result_g_page_dyn['tv_value'];
						//echo "<p>dyn1: ".$temp_dynamic_number."</p>";
					}
					else if ($temp_dynamic == "dynamic" && $g_page_dynida != "") {
						$temp_dynamic_number = $g_page_dynida;
						//echo "<p>dyn2: ".$temp_dynamic_number."</p>";
					}
					else {
						$temp_dynamic_number = "";
						//echo "<p>dyn3</p>";
					}
					
					
				
					// file extension: 
					$temp_extension = getTagValueG($result_g_page['data_id'], $result_g_page['rev_id'],  '-1', 'extension'); // get page extension
					
					if ($temp_extension == "") {
						echo "<p><b>!! ".date("Y-m-d H:i:s")." - file extension missing: ".$result_g_page['data_name']."(".$result_g_page['data_id'].", ".$result_g_page['rev_id'].", ".$result_g_lang['lang_id'].")</b></p>";
						continue;
					}
					
					// file name:
					if ($temp_dynamic == "dynamic") {
						$RosCMS_current_page_out_file_pretty = $result_g_lang['lang_id']."/".$result_g_page['data_name']."_".$temp_dynamic_number.".".$temp_extension;
						$RosCMS_current_page_out_file = "../".$RosCMS_current_page_out_file_pretty;
					}
					else {
						$RosCMS_current_page_out_file_pretty = $result_g_lang['lang_id']."/".$result_g_page['data_name'].".".$temp_extension;
						$RosCMS_current_page_out_file = "../".$RosCMS_current_page_out_file_pretty;
					}
					
					$RosCMS_current_page_content = "";
					$RosCMS_current_page_content = generate_page($result_g_page['data_name'], $result_g_lang['lang_id'], $temp_dynamic_number, "output");
					
					/*if ($RosCMS_current_page_content == "") {
						
						$RosCMS_current_page_content = generate_page($result_g_page['data_name'], $roscms_standard_language, $temp_dynamic_number, "output");
					}*/
					
					
					// write file:
					$fp = fopen($RosCMS_current_page_out_file, "w");
					flock($fp,2);
					fputs($fp,$RosCMS_current_page_content); // write content
					fputs($fp,"\n\n<!-- Generated with ".$roscms_extern_brand." ".$roscms_extern_version." (".$roscms_extern_version_detail.") - ".date("Y-m-d H:i:s")." [RosCMS_v3] -->");
					flock($fp,3);
					fclose($fp);
					
					echo "<p> * ".date("Y-m-d H:i:s")." - ".$RosCMS_current_page_out_file_pretty."</p>";
					
					// deactived generator logging, as it seem that it has an impact on the overall generation-time
					//log_event_generate_medium($RosCMS_current_page_out_file_pretty);
				}
			}
		}
	}
	
	
	function generate_page($g_page_name, $g_page_lang, $g_page_dynida, $g_output_type) {
		//echo "generate_page(".$g_page_name.", ".$g_page_lang.", ".$g_page_dynida.", ".$g_output_type.")";


		global $roscms_intern_account_id;
		global $roscms_intern_webserver_pages;
		global $roscms_standard_language;
		global $roscms_intern_webserver_roscms;
		global $roscms_standard_language_full;

		global $g_lang;
		global $g_cur_page_name;
		global $g_page_dynid;
		global $g_linkstyle;

		$g_page_dynid = $g_page_dynida;
		$g_linkstyle = $g_output_type;
		
		//echo "<p>g_page_dynid: ".$g_page_dynid."</p>";
		
		set_time_limit(0); // unlimited script run time 
		
		
		$g_log = "";
		
		$g_lang = $g_page_lang;		

		$query_g_page = mysql_query("SELECT r.rev_id, r.rev_version, r.rev_usrid, r.rev_datetime, r.rev_date, r.rev_time, r.rev_language  
									FROM data_ d, data_revision r 
									WHERE data_name = '".mysql_real_escape_string($g_page_name)."' 
									AND data_type = 'page'
									AND r.data_id = d.data_id
									AND r.rev_version > 0
									AND (r.rev_language = '".mysql_real_escape_string($g_lang)."' 
										OR r.rev_language = '".mysql_real_escape_string($roscms_standard_language)."')
									ORDER BY r.rev_version DESC
									LIMIT 2;");
		
		$result_g_page = mysql_fetch_assoc($query_g_page);
		
		// Try to get the dataset with rev_language == $g_lang, to boost the translated content
		if( mysql_num_rows($query_g_page) == 2 ) {
			if( $result_g_page['rev_language'] == $roscms_standard_language ) {
				$result_g_page = mysql_fetch_assoc($query_g_page);
			}
		}
		
/*		
		$g_log .= "generate_page(".$g_page_name.", ".$g_lang.", ".$g_page_dynid.")";
		$g_log .= "<p>r.rev_id: ".$result_g_page['rev_id']."</p>";
		$g_log .= "<p>Titel: ".get_stext($result_g_page['rev_id'], "title")."</p>";
		$g_log .= "<p>Description: ".get_stext($result_g_page['rev_id'], "description")."</p>";
		$g_log .= "<p>extension: ".get_stext($result_g_page['rev_id'], "extension")."</p>";
		$g_log .= "<p>Content: ".get_text($result_g_page['rev_id'], "content")."</p>";
*/
		
		if ($result_g_page['rev_id'] != "") {
		
			$g_cur_page_name = $g_page_name;
			
			$g_content = get_text($result_g_page['rev_id'], "content");
			
			// Insert content entries (normal page content and dynamic content like news, newsletter, etc.):
			
			$g_content = preg_replace_callback("(\[#templ_[^][#[:space:]]+\])", "insert_template", $g_content);
			
			//for ($i=0; $i < 3; $i++) { // allow nested contents (3 levels)
				$g_content = preg_replace_callback("(\[#cont_[^][#[:space:]]+\])", "insert_content", $g_content);
			//}
			
			$g_content = preg_replace_callback("(\[#inc_[^][#[:space:]]+\])", "insert_script", $g_content);
		
			// Insert hyperlinks entries:
			$g_content = preg_replace_callback("(\[#link_[^][#[:space:]]+\])", "insert_hyperlink", $g_content);
	
	
	
		
			// RosCMS specific tags:
				// website url:
				$g_content = str_replace("[#roscms_path_homepage]", $roscms_intern_webserver_pages, $g_content);
				if ($g_page_dynid != "" || $g_page_dynid == "0") {
					// current filename:
					$g_content = str_replace("[#roscms_filename]", $g_page_name."_".$g_page_dynid.".html", $g_content);
					// current page name:
					$g_content = str_replace("[#roscms_pagename]", $g_page_name."_".$g_page_dynid, $g_content); 
					// current page title:
					$g_content = str_replace("[#roscms_pagetitle]", ucfirst(get_stext($result_g_page['rev_id'], "title"))." #".$g_page_dynid, $g_content); 
				}
				else {
					// current filename:
					$g_content = str_replace("[#roscms_filename]", $g_page_name.".html", $g_content); 
					// current page name:
					$g_content = str_replace("[#roscms_pagename]", $g_page_name, $g_content); 
					// current page title:
					$g_content = str_replace("[#roscms_pagetitle]", ucfirst(get_stext($result_g_page['rev_id'], "title")), $g_content); 
				}
				// current language:
					$RosCMS_query_current_language = mysql_query("SELECT * 
																	FROM languages 
																	WHERE lang_id = '".mysql_real_escape_string($g_lang)."'
																	LIMIT 1 ;");
					$RosCMS_result_current_language = mysql_fetch_array($RosCMS_query_current_language);
				$g_content = str_replace("[#roscms_language]", $RosCMS_result_current_language['lang_name'], $g_content); 
				// current language:
				$g_content = str_replace("[#roscms_language_short]", $g_lang, $g_content); 
				// current page format (xhtml/html):
				$g_content = str_replace("[#roscms_format]", "html", $g_content); 
				// current date:
				$g_content = str_replace("[#roscms_date]", date("Y-m-d"), $g_content); 
					$zeit = localtime(time() , 1);
				// current time:
				$g_content = str_replace("[#roscms_time]", sprintf("%02d", $zeit['tm_hour']).":".sprintf("%02d",$zeit['tm_min']), $g_content);
					
					$query_usraccount= mysql_query("SELECT user_name 
										FROM users 
										WHERE user_id = '".mysql_real_escape_string($roscms_intern_account_id)."' LIMIT 1 ;");
					$result_usraccount=mysql_fetch_array($query_usraccount);
				// account that generate:
				$g_content = str_replace("[#roscms_user]", $result_usraccount['user_name'], $g_content);
				// account that changed the include text:
				$g_content = str_replace("[#roscms_inc_author]", $result_usraccount['user_name'], $g_content); 
				
				// redirect all bad links to the 404 page:
				$g_content = str_replace("[#link_", $roscms_intern_webserver_pages."?page=404", $g_content);
					
				// current page version:
				$g_content = str_replace("[#roscms_page_version]", $result_g_page['rev_version'], $g_content); 
				
				// current page edit link:
				$g_content = str_replace("[#roscms_page_edit]", $roscms_intern_webserver_roscms."?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val=".$g_page_name."&amp;d_val2=".$g_lang."&amp;d_val3=".$g_page_dynid."&amp;d_val4=edit", $g_content); 


				// translation info:
				if ($g_lang == $roscms_standard_language) {
					$g_content = str_replace("[#roscms_trans]", "<p><a href=\"".$roscms_intern_webserver_roscms."?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val=".$g_page_name."&amp;d_val2=".$g_lang."&amp;d_val3=".$g_page_dynid."&amp;d_val4=edit"."\" style=\"font-size:10px !important;\">Edit page content</a> (RosCMS translator account membership required, visit the <a href=\"".$roscms_intern_webserver_pages."forum/\" style=\"font-size:10px !important;\">website forum</a> for help)</i></p><br />", $g_content); 
				}
				else {
					$g_content = str_replace("[#roscms_trans]", "<p><i>If the translation of the <a href=\"".$roscms_intern_webserver_pages."?page=".$g_page_name."&amp;lang=".$roscms_standard_language."\" style=\"font-size:10px !important;\">".$roscms_standard_language_full." language</a> of this page appears to be outdated or incorrect, please check-out the <a href=\"".$roscms_intern_webserver_pages."?page=".$g_page_name."&amp;lang=".$roscms_standard_language."\" style=\"font-size:10px !important;\">".$roscms_standard_language_full."</a> page and <a href=\"http://www.reactos.org/forum/viewforum.php?f=18\" style=\"font-size:10px !important;\">report</a> or <a href=\"".$roscms_intern_webserver_roscms."?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val=".$g_page_name."&amp;d_val2=".$g_lang."&amp;d_val3=".$g_page_dynid."&amp;d_val4=edit"."\" style=\"font-size:10px !important;\">update the content</a>.</i></p><br />", $g_content); 
				}
	
			
	//		$g_log .= "<hr />";
	
			$g_log .= $g_content;	
			
			return $g_log;
		}
	}

	function insert_template($matches) {
		global $roscms_intern_account_id;
		global $g_lang;
		global $g_cur_page_name;
				
		// extract the name, e.g. [#cont_about] ... "about"
		$g_content_name = substr($matches[0], 8, (strlen($matches[0])-9)); 
		
		$RosCMS_result_template_temp = insert_match("template", $g_content_name, $g_lang);
		$RosCMS_result_template_temp = str_replace("[#cont_%NAME%]", "[#cont_".$g_cur_page_name."]", $RosCMS_result_template_temp); 
		$RosCMS_result_template_temp = str_replace("[#%NAME%]", $g_cur_page_name, $RosCMS_result_template_temp); 

		return $RosCMS_result_template_temp;
	}	
	
	function insert_content($matches) {
		global $roscms_intern_account_id;
		global $g_lang;
				
		// extract the name, e.g. [#cont_about] ... "about"
		$g_content_name = substr($matches[0], 7, (strlen($matches[0])-8)); 
	
		return insert_match("content", $g_content_name, $g_lang);
	}

	function insert_script($matches) {
		global $roscms_intern_account_id;
		global $g_lang;
				
		// extract the name, e.g. [#inc_about] ... "about"
		$g_content_name = substr($matches[0], 6, (strlen($matches[0])-7)); 
		
		return insert_match("script", $g_content_name, $g_lang);
	}

	function insert_match($g_insert_match_type, $g_match_name, $g_match_lang) {
		global $roscms_intern_account_id;
		global $roscms_intern_webserver_roscms;
		global $roscms_intern_page_link;
		global $roscms_standard_language;
		
		global $RosCMS_GET_branch;
		global $RosCMS_GET_d_flag;
		global $RosCMS_GET_d_value4;
		
		global $g_page_dynid;
				
		$query_content_temp = "SELECT d.data_acl, t.text_content, r.rev_version, r.rev_usrid, r.rev_datetime , r.data_id, r.rev_id, r.rev_language
									FROM data_ d, data_revision r, data_text t
									WHERE data_name = '".mysql_real_escape_string($g_match_name)."' 
									AND data_type = '".mysql_real_escape_string($g_insert_match_type)."'
									AND r.data_id = d.data_id
									AND r.rev_version > 0
									AND (r.rev_language = '".mysql_real_escape_string($g_match_lang)."'
										OR r.rev_language = '".mysql_real_escape_string($roscms_standard_language)."')
									AND t.data_rev_id = r.rev_id 
									AND t.text_name = 'content'
									ORDER BY r.rev_version DESC
									LIMIT 2;";
		
//		echo $query_content_temp;

		$query_content = mysql_query($query_content_temp);
		$result_content = mysql_fetch_assoc($query_content);
	
		// Try to get the dataset with rev_language == $g_lang, to boost the translated content
		if( mysql_num_rows($query_content) == 2 ) {
			if( $result_content['rev_language'] == $roscms_standard_language ) {
				$result_content = mysql_fetch_assoc($query_content);
			}
		}
		
		$RosCMS_result_content_temp = "";
		// preview-edit-mode
		if ($RosCMS_GET_d_value4 == "edit" && $result_content['data_acl'] == "default" && $g_insert_match_type != "script") {
			$RosCMS_result_content_temp = "<div style=\"border: 1px dashed red;\"><div style=\"padding: 2px;\"><a href=\"".$roscms_intern_page_link."data&amp;branch=".$RosCMS_GET_branch."&amp;edit=rv".$result_content['data_id']."|".$result_content['rev_id']."\" style=\"background-color:#E8E8E8;\"> <img src=\"".$roscms_intern_webserver_roscms."images/edit.gif\" style=\"width:19px; height:19px; border:none;\" /><i>".$g_match_name."</i> </a></div>";
		}
		else {
			$RosCMS_result_content_temp = "";
		}	
	
		$RosCMS_result_content_temp .= $result_content['text_content'];
		
		if ($g_insert_match_type == "script") {
//			echo "<h3>!!".get_tag($result_content['data_id'], $result_content['rev_id'], "kind")."!! &lt;=&gt; get_tag(".$result_content['data_id'].", ".$result_content['rev_id'].", \"kind\")</h3>";
			if (get_tag($result_content['data_id'], $result_content['rev_id'], "kind") == "php") {
				$RosCMS_result_content_temp = eval_template($RosCMS_result_content_temp, $g_page_dynid, $g_match_lang);
				//echo "<p>REV_ID: ".$result_content['rev_id']."</p>";
			}
		}
//		echo "<p>".$g_match_name." - |".$g_insert_match_type."|</p>";
	
		// latest content changes:
		$query_usraccountc= mysql_query("SELECT user_name, user_fullname  
											FROM users 
											WHERE user_id = '".mysql_real_escape_string($result_content['rev_usrid'])."' LIMIT 1 ;");
		$result_usraccountc=mysql_fetch_array($query_usraccountc);
	
		if ($result_usraccountc['user_fullname']) {
			$RosCMS_result_user_temp = $result_usraccountc['user_fullname']." (".$result_usraccountc['user_name'].")";
		}
		else {
			$RosCMS_result_user_temp = $result_usraccountc['user_name'];
		}
			
		$RosCMS_result_content_temp = str_replace("[#roscms_".$g_insert_match_type."_version]", "<i>Last modified: ".$result_content['rev_datetime'].", rev. ".$result_content['rev_version']." by ".$RosCMS_result_user_temp."</i>", $RosCMS_result_content_temp); 
		
		// preview-edit-mode
		if ($RosCMS_GET_d_flag == "edit" && $result_content['data_acl'] == "default" && $g_insert_match_type != "script") {
			$RosCMS_result_content_temp .= "</div>";
		}
	
		return $RosCMS_result_content_temp;
	}
	
	function insert_hyperlink($matches) {
		global $roscms_intern_account_id;
		global $roscms_intern_webserver_pages;
		global $roscms_intern_webserver_roscms;
		global $g_lang;
		global $g_page_dynid;
		global $g_linkstyle;
		global $RosCMS_GET_d_value4;


		$g_hyperlink_sql1 = "";
		$g_hyperlink_sql2 = "";
		
		$g_link_page_name = substr($matches[0], 7, (strlen($matches[0])-8)); 
		

		$g_link_page_name2 = $g_link_page_name;
		$g_link_page_number = "";
		
		if ( is_numeric(substr(strrchr($g_link_page_name,"_"), 1, strlen(strrchr($g_link_page_name,"_")-1))) ) { // dynamic
			$g_link_page_name2 = substr($g_link_page_name, 0, strlen($g_link_page_name) - strlen(strrchr($g_link_page_name,"_")));
			$g_link_page_number = substr(strrchr($g_link_page_name,"_"), 1, strlen(strrchr($g_link_page_name,"_")-1));
		}
	
		if ($g_linkstyle == "show") { // dynamic preview
			$RosCMS_current_page_link = $roscms_intern_webserver_roscms."?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val=".$g_link_page_name2."&amp;d_val2=".$g_lang."&amp;d_val3=".$g_link_page_number;
			
			if ($g_link_page_name == "") {
				$RosCMS_current_page_link = $roscms_intern_webserver_roscms."?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val=index&amp;d_val2=".$g_lang."&amp;d_val3=";
			}
			
			if ($RosCMS_GET_d_value4 == "edit") {
				$RosCMS_current_page_link .= "&amp;d_val4=edit";
			}
		}
		else { // static pages
			$query_data = mysql_query("SELECT * 
										FROM data_ d, data_revision r 
										WHERE d.data_name  = '".mysql_real_escape_string($g_link_page_name)."' 
										AND r.data_id = d.data_id 
										AND r.rev_language = '".mysql_real_escape_string($g_lang)."' 
										AND rev_version > 0 
										LIMIT 1;");
			$result_data = mysql_fetch_array($query_data);
			$RosCMS_current_page_link_extension = getTagValueG($result_data['data_id'], $result_data['rev_id'],  '-1', 'extension'); // get extension
			
			if ($RosCMS_current_page_link_extension == "") {
				$RosCMS_current_page_link_extension = "html";
			}
			
			$RosCMS_current_page_link = $roscms_intern_webserver_pages.$g_lang."/".$g_link_page_name.".".$RosCMS_current_page_link_extension;
			
			if ($g_link_page_name == "") {
				$RosCMS_current_page_link = $roscms_intern_webserver_pages.$g_lang."/404.html";
			}
		}
		
		return $RosCMS_current_page_link;
	}


	function eval_template($code, $dyncontid, $dyncontlang) { // function code idea from: http://www.zend.com/zend/art/buffering.php
		//echo "<p>eval_template(code, ".$dyncontid.", ".$dyncontlang.")</p>";
		
		ob_start(); 
		
		$roscms_template_var_pageid = "";
		$roscms_template_var_lang = "";
		
		$roscms_template_var_pageid = $dyncontid;
		$roscms_template_var_lang = $dyncontlang;
		
		eval(' ?'.'>'.$code.' <'.'?php ');
		 
		$output = ob_get_contents(); 
		
		ob_end_clean(); 
		return $output; 
	} 
	
	
	
	function getTagValueG($RosCMS_GET_d_id, $RosCMS_GET_d_r_id, $RosCMS_intern_current_usrid, $RosCMS_intern_current_tag_name) {
		global $h_a;
		global $h_a2;

		//echo "<p>=> getTagValueG(".$RosCMS_GET_d_id.", ".$RosCMS_GET_d_r_id.", ".$RosCMS_intern_current_usrid.", ".$RosCMS_intern_current_tag_name.")</p>";

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