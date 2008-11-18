<?php

// WunderWuzzi


	// Debug setting:
	#$RTRANS_parser3_debug = true;
	$RTRANS_parser3_debug = false;
	
	$ROST_parser3_mode = "";
	$ROST_parser3_mode2 = "";
	$ROST_parser3_secondfile = "";
	$ROST_parser3_secondfile2 = "";

	
	
	function parser3 ($RTRANS_parser3_rcfile, $RTRANS_parser3_rclang, $RTRANS_parser3_revision, $RTRANS_parser3_mode, $RTRANS_parser_xd_changes = array (), $RTRANS_parser3_mode_additionaldata) {
		global $RTRANS_parser3_debug;
		global $ROST_parser3_mode;
		global $ROST_parser3_mode2;
		global $ROST_parser3_secondfile;
		global $ROST_parser3_secondfile2;
		
		$RTRANS_parser3_content = "";
		$RTRANS_parser3_content_trans = "";
		$RTRANS_temp_a = "";
		
		$RTRANS_temp_stringtable_color_reset = 0;
		$RTRANS_temp_stringtable_content1 = "";
		$RTRANS_temp_stringtable_content2 = "";
		
		if ($RTRANS_parser3_mode == "") {
			return 0;
		}
		
		switch ( $RTRANS_parser3_mode ) {
			default:
			case "trans_rc":
				$ROST_parser3_mode = "trans_rc";
				break;
			case "trans_xml":
			case "trans_compare":
			case "trans_translate":
				$ROST_parser3_mode = "trans_xml";
				break;
			case "trans_changes":
				$ROST_parser3_mode = "trans_xml";
				$ROST_parser3_mode2 = "trans_changes";
				break;
			case "trans_update":
				$ROST_parser3_mode = "trans_xml";
				$ROST_parser3_mode2 = "trans_update";
				break;
		}
		
		/*echo "<p>".$ROST_parser3_mode."</p>";
		
		echo "<hr />";
		echo "<h2>PARSER-XD</h2>";
		echo "<hr />";
		foreach ($RTRANS_parser_xd_changes as $key => $value) {
			echo "<br><b>".$key."</b>: ".$value;
		}
		echo "<hr />";*/
		
		
		$result_app = SQL_query_array("SELECT * 
										FROM `apps` 
										WHERE `app_name` = '". SQL_escape($RTRANS_parser3_rcfile) ."' 
										AND `app_enabled` = '1'
										LIMIT 1 ;");		

		$result_translation = SQL_query_array("SELECT * 
												FROM `translations`
												WHERE `app_id` = '". SQL_escape($result_app['app_id']) ."'  
												AND `xml_lang` = '". SQL_escape($RTRANS_parser3_rclang) ."'  
												AND `xml_rev` = '". SQL_escape($RTRANS_parser3_revision) ."'  
												LIMIT 1 ;");	
					
										
		// "Update" translations
		if ($ROST_parser3_mode2 == "trans_update") {
//			echo "<hr /><hr /><h2>trans_update</h2>";
			$ROST_parser3_secondfile = "";
			$ROST_parser3_secondfile = split("[_]", $RTRANS_parser3_mode_additionaldata); // e.g. "notepad_de-de_24020"
		}
		
//		echo "<hr />MODE2: ".$ROST_parser3_mode2;
		// Find changes
		if ($ROST_parser3_mode2 == "trans_changes") {
			$result_changes = SQL_query_array("SELECT * 
												FROM `translations`
												WHERE `app_id` = '". SQL_escape($result_app['app_id']) ."'  
												AND `xml_lang` = '". SQL_escape($RTRANS_parser3_rclang) ."'  
												ORDER BY `xml_rev` DESC 
												LIMIT 1, 1 ;");	
			$ROST_parser3_secondfile2 = "";
			$ROST_parser3_secondfile2[0] = $RTRANS_parser3_rcfile;
			$ROST_parser3_secondfile2[1] = $RTRANS_parser3_rclang;
			$ROST_parser3_secondfile2[2] = $result_changes['xml_rev'];
			//echo "<hr /><hr />";
			//print_r($ROST_parser3_secondfile2);
			//echo "####<hr /><hr />";
		}
		

		if ($result_translation['xml_content'] != "") {

			$xml = new EFXMLElement( utf8_encode( $result_translation['xml_content'] ) );
			//var_dump($tst->group[0]->obj[0]->attributes()->type);
			//echo $xml->group[0]->obj[0]->attributes()->type;		
			//echo "<p>&nbsp;</p><hr /><p>&nbsp;</p>";

			$RTRANS_parser3_content = "\n".'<?xml version="1.0" encoding="UTF-8"?>';
			$RTRANS_parser3_content .= "\n<resource>";

			for ($counter_grp = 0; ($xml->group[$counter_grp] != "" && $xml->group[$counter_grp]->obj[0]->attributes()->type != ""); $counter_grp++) {
				$counter_obj = 0;
				foreach ($xml->group[$counter_grp] as $obj_node) {
				
					if ($RTRANS_parser3_debug) {
						$RTRANS_temp_a = "<p><font color=\"#999999\"><b>". $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->type;
						$RTRANS_temp_a .= "</b><br /> ... <u>data:</u> ". $obj_node ."</font></p>";
						$RTRANS_parser3_content .= $RTRANS_temp_a;
						$RTRANS_parser3_content_trans .= $RTRANS_temp_a;
					}
		
					switch ( strtoupper($xml->group[$counter_grp]->obj[$counter_obj]->attributes()->type) ) {
						default:
							if ($RTRANS_parser3_debug) {
								$RTRANS_temp_a = "<p><b><u>Unknown-Type:</u></b> ". $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->type ."</p>";
								$RTRANS_parser3_content .= $RTRANS_temp_a;
								$RTRANS_parser3_content_trans .= $RTRANS_temp_a;
							}
							break;
						// STRINGTABLE:
							case "STRINGTABLE":
								switch ( $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->command ) {
									default:
									case "":
										$RTRANS_parser3_content .= out_stringtable( $xml->group[$counter_grp]->obj[$counter_obj] );
										break;
									case "BEGIN":
										$RTRANS_parser3_content .= out_stringtable_begin();
										break;
									case "END":
										$RTRANS_parser3_content .= out_stringtable_end();
										break;
								}
								break;
							case "STRING":
								switch ( $RTRANS_parser3_mode ) {
									case "trans_compare":
										$RTRANS_parser3_content .= out_string( 
																					check_and_replace_text(
																												$xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name,
																												$xml->group[$counter_grp]->obj[$counter_obj], 
																												$RTRANS_parser_xd_changes
																											),
																					$xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name
																				);							
										break;
									default:
									case "trans_rc":
									case "trans_xml":
										$RTRANS_parser3_content .= out_string( $xml->group[$counter_grp]->obj[$counter_obj], $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->info);
										break;
								}
								//$RTRANS_parser3_content_trans .= '<p><b>'. $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name .'</b>:<br /><textarea name="'. $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name .'" cols="50" rows="3" readonly="true">'. $xml->group[$counter_grp]->obj[$counter_obj] .'</textarea><i><textarea name="'. $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name .'_comp" cols="50" rows="3" readonly="true">'.parser_xc("notepad_en.xml", $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name).'</textarea></i></p>';
								$RTRANS_temp_stringtable_content1 = $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name;
								$RTRANS_temp_stringtable_content2 = create_textfield_translate_readonly( $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name ."_comp", parser_xc("notepad", "en-us", "24067", $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name) );
								$RTRANS_temp_stringtable_content3 = create_textfield_translate( $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name, $xml->group[$counter_grp]->obj[$counter_obj] );
								$RTRANS_parser3_content_trans .= create_table_body_translate($RTRANS_temp_stringtable_content1, $RTRANS_temp_stringtable_content2, $RTRANS_temp_stringtable_content3);
								break;
						// Comment:
							case "COMMENT":
								$RTRANS_parser3_content .= out_comment( $xml->group[$counter_grp]->obj[$counter_obj] );
								break;
						// Language:
							case "LANGUAGE":
								$RTRANS_parser3_content .= out_language( $xml->group[$counter_grp]->obj[$counter_obj], $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->sublang );
								break;
						// Short-Cut Keys:
							case "ACCELERATORS":
								switch ( $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->command ) {
									default:
									case "":
										$RTRANS_parser3_content .= out_accelerators( $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name );
										break;
									case "BEGIN":
										$RTRANS_parser3_content .= out_accelerators_begin();
										break;
									case "END":
										$RTRANS_parser3_content .= out_accelerators_end();
										break;
								}
								break;
							case "KEY":
								$RTRANS_parser3_content .= out_key( $xml->group[$counter_grp]->obj[$counter_obj], $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name );
								break;
						// Menu:
							case "MENU":
								switch ( $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->command ) {
									default:
									case "":
										$RTRANS_parser3_content .= out_menu( $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name );
										break;
									case "BEGIN":

										$RTRANS_parser3_content .= out_menu_begin();
										break;
									case "END":
										$RTRANS_parser3_content .= out_menu_end();
										break;
								}
								break;
							case "POPUP":
								switch ( $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->command ) {
									default:
									case "":
										$RTRANS_parser3_content .= out_popup( $xml->group[$counter_grp]->obj[$counter_obj] );
										break;
									case "BEGIN":
										$RTRANS_parser3_content .= out_popup_begin();
										break;
									case "END":
										$RTRANS_parser3_content .= out_popup_end();
										break;
								}
								break;
							case "MENUITEM":
								$RTRANS_parser3_content .= out_menuitem( $xml->group[$counter_grp]->obj[$counter_obj], $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name );
								break;
							case "MENUITEMSEPARATOR":
								$RTRANS_parser3_content .= out_menuitemseparator();
								break;
						// Dialog:
							case "DIALOG":
								switch ( $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->command ) {
									default:
									case "":
										$RTRANS_parser3_content .= out_dialog( $xml->group[$counter_grp]->obj[$counter_obj], $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->top, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->left, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->height, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->width );
										break;
									case "BEGIN":
										$RTRANS_parser3_content .= out_dialog_begin();
										break;
									case "END":
										$RTRANS_parser3_content .= out_dialog_end();
										break;
								}
								break;
							case "STYLE":
								$RTRANS_parser3_content .= out_style( $xml->group[$counter_grp]->obj[$counter_obj] );
								break;
							case "EXSTYLE":
								$RTRANS_parser3_content .= out_exstyle( $xml->group[$counter_grp]->obj[$counter_obj] );
								break;
							case "CAPTION":
								$RTRANS_parser3_content .= out_caption( $xml->group[$counter_grp]->obj[$counter_obj] );
								break;
							case "FONT":
								$RTRANS_parser3_content .= out_font( $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->size, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->name  );
								break;
							// Controls:
								case "CONTROL":
									$RTRANS_parser3_content .= out_control( $xml->group[$counter_grp]->obj[$counter_obj], $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->prop, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->style, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->top, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->left, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->height, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->width );
									break;
								case "DEFPUSHBUTTON":
									$RTRANS_parser3_content .= out_defpushbutton( $xml->group[$counter_grp]->obj[$counter_obj], $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->style, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->top, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->left, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->height, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->width );
									break;
								case "PUSHBUTTON":
									$RTRANS_parser3_content .= out_pushbutton( $xml->group[$counter_grp]->obj[$counter_obj], $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->style, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->top, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->left, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->height, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->width );
									break;
								case "LTEXT":
									$RTRANS_parser3_content .= out_ltext( $xml->group[$counter_grp]->obj[$counter_obj], $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->style, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->top, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->left, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->height, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->width );
									break;
								case "GROUPBOX":
									$RTRANS_parser3_content .= out_groupbox( $xml->group[$counter_grp]->obj[$counter_obj], $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->style, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->top, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->left, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->height, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->width );
									break;
								case "ICON":
									$RTRANS_parser3_content .= out_icon( $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->style, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->top, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->left, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->height, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->width );
									break;
								case "EDITTEXT":
									$RTRANS_parser3_content .= out_edittext( $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->style, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->top, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->left, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->height, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->width );
									break;
								case "LISTBOX":
									$RTRANS_parser3_content .= out_listbox( $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->style, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->top, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->left, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->height, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->width );
									break;
								case "COMBOBOX":
									$RTRANS_parser3_content .= out_combobox( $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->style, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->top, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->left, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->height, $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->width );
									break;
									
					}
		
					$counter_obj++;
				}
			}
			
			$RTRANS_parser3_content .= "\n</resource>";
			
			// Output content:
			switch ( $RTRANS_parser3_mode ) {
				default:
				case "trans_rc":
				case "trans_xml":
					return $RTRANS_parser3_content;
					break;
				case "trans_xml_translate":
					return $RTRANS_parser3_content_trans;
					break;
			}
			
		}
		else {
			return 0;
		}
	}
	
	// check_and_replace_text
	function check_and_replace_text ( $str_original, $str_original_long, $str_changes ) {
//		echo "<h3>check_and_replace_text: ".$str_original."</h3>";
		foreach ($str_changes as $key => $value) {
			if ($key == $str_original) {
//				echo "<p><b>".$key."</b>: ".$value."</p>";
				return $value;
			}
		}
//		echo "<p><s><b>".$key."</b>: ".$value."</s></p>";
		return $str_original_long;
	}
	
	function check_for_changes ( $str_name, $str_content ) {
		global $ROST_parser3_mode2;
		global $ROST_parser3_secondfile2;
		
		//echo "<h3>check_for_changes: ".$str_name."</h3>";
		
		if ($ROST_parser3_mode2 == "trans_changes") {
			$ROST_temp_b = parser_xc ($ROST_parser3_secondfile2[0], $ROST_parser3_secondfile2[1], $ROST_parser3_secondfile2[2], $str_name);
			if ($str_content == $ROST_temp_b) {
				return "";
			}
			else {
				return " info=\"changed\"";
			}
		}
		return "";
	}
	
	
	// group_begin
	function out_group_begin ( $str_content ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_xml":
				return "\n<group name=\"". $str_content ."\">";
				break;
		}
		return 0;
	}
	
	// group_end
	function out_group_end () {
		global $ROST_parser3_mode;
		//echo "<p>END: ".$ROST_parser3_mode."</p>";
		switch($ROST_parser3_mode) {
			case "trans_xml":
				return "\n</group>";
				break;
		}
		return 0;
	}
	
	// stringtable_begin
	function out_stringtable_begin () {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				$RTRANS_temp_content .= out_curly_bracket_open(0);
//				$RTRANS_temp_content .= "<h2>STRINGTABLE</h2>\n";
				$RTRANS_temp_content .= create_table_header_translate();
				reset_color_counter();
				return $RTRANS_temp_content;
				break;
			case "trans_xml":
				return "\n  <obj type=\"STRINGTABLE\" command=\"BEGIN\" />";
				break;
		}
		return 0;
	}
	
	// stringtable_end
	function out_stringtable_end () {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return out_curly_bracket_close(0)."\n".create_table_footer_translate();
				break;
			case "trans_xml":
				return "\n  <obj type=\"STRINGTABLE\" command=\"END\" />". out_group_end();
				break;
		}
		return 0;
	}
	
	// stringtable
	function out_stringtable ( $str_content ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\nSTRINGTABLE ". $str_content;
				break;
			case "trans_xml":
				return out_group_begin("DISCARDABLE") ."\n  <obj type=\"STRINGTABLE\">". $str_content ."</obj>";
				break;
		}
		return 0;
	}
	
	// string
	function out_string ( $str_content, $str_rc_name, $str_info ) {
		global $ROST_parser3_mode;
		global $ROST_parser3_mode2;
		global $ROST_parser3_secondfile;
		global $ROST_parser3_secondfile2;
		
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n  ". $str_rc_name .", ". $str_content;
				break;
			case "trans_xml":
				switch($ROST_parser3_mode2) {
					case "trans_update":
						$temp_ccc = parser_xc ($ROST_parser3_secondfile[0], $ROST_parser3_secondfile[1], $ROST_parser3_secondfile[2], $str_rc_name);
//						echo "<hr /><h3>parser_xc:</h3>".$temp_ccc."<hr />";
						if ($temp_ccc != "") {
							$temp_ddd = parser_transfer ($ROST_parser3_secondfile[0], $ROST_parser3_secondfile[1], $ROST_parser3_secondfile[2], $str_rc_name);
//							echo "<h4>parser_xc:</h4>".$temp_ddd."<hr />";
							if ($str_info == "changed") {
								return write_xml_node ( $temp_ddd, "", "info=\"outdated\"" );	
							}
							else {
								return write_xml_node ( $temp_ddd, "", "" );	
							}		
							break;
						}
//						echo "<h1>No Entry for: ".$str_rc_name."</h1>";
						// else: go to the default section
					default:
					case "trans_changes":
						$ROST_temp_a = "\n  <obj type=\"STRING\" rc_name=\"". $str_rc_name ."\"";
						$ROST_temp_a .= check_for_changes($str_rc_name, $str_content);
						$ROST_temp_a .= "><![CDATA[". $str_content ."]]></obj>";
						return $ROST_temp_a;
						break;
				}
				break;
		}
		return 0;
	}
	
	// comment
	function out_comment ( $str_content ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n". $str_content;
				break;
			case "trans_xml":
				return out_group_begin("COMMENT") ."\n  <obj type=\"COMMENT\"><![CDATA[". $str_content ."]]></obj>". out_group_end ();
				break;
		}
		return 0;
	}
	
	// language
	function out_language ( $str_content, $str_sublang ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\nLANGUAGE ". $str_content .", ". $str_sublang;
				break;
			case "trans_xml":
				return out_group_begin("LANGUAGE") ."\n  <obj type=\"LANGUAGE\" sublang=\"". $str_sublang ."\">". $str_content ."</obj>". out_group_end ();
				break;
		}
		return 0;	
	}
	
	// accelerators_begin
	function out_accelerators_begin () {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return out_curly_bracket_open(0);
				break;
			case "trans_xml":
				return "\n  <obj type=\"ACCELERATORS\" command=\"BEGIN\" />";
				break;
		}
		return 0;
	}
	
	// accelerators_end
	function out_accelerators_end () {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return out_curly_bracket_close(0)."\n";
				break;
			case "trans_xml":
				return "\n  <obj type=\"ACCELERATORS\" command=\"END\" />". out_group_end();
				break;
		}
		return 0;
	}

	// accelerators
	function out_accelerators ( $str_content ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n". $str_content ." ACCELERATORS";
				break;
			case "trans_xml":
				return out_group_begin("ACCELERATORS") ."\n  <obj type=\"ACCELERATORS\" rc_name=\"". $str_content ."\"></obj>";
				break;
		}
		return 0;
	}

	// key
	function out_key ( $str_content, $str_rc_name ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n  \"".$str_content."\", ". $str_rc_name;
				break;
			case "trans_xml":
				return "\n  <obj type=\"KEY\" rc_name=\"". $str_content ."\"><![CDATA[". $str_rc_name ."]]></obj>";
				break;
		}
		return 0;
	}
	
	// menu_begin
	function out_menu_begin () {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "  ".out_curly_bracket_open(0);
				break;
			case "trans_xml":
				return "\n  <obj type=\"MENU\" command=\"BEGIN\" />";
				break;
		}
		return 0;
	}
	
	// menu_end
	function out_menu_end () {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "  ".out_curly_bracket_close(0)."\n";
				break;
			case "trans_xml":
				return "\n  <obj type=\"MENU\" command=\"END\" />". out_group_end();
				break;
		}
		return 0;
	}
	
	// menu
	function out_menu ( $str_rc_name ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n". $str_rc_name ." MENU";
				break;
			case "trans_xml":
				return out_group_begin("MENU") ."\n  <obj type=\"MENU\" rc_name=\"". $str_rc_name ."\"></obj>";
				break;
		}
		return 0;
	}
	
	// popup_begin
	function out_popup_begin () {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return out_begin(2);
				break;
			case "trans_xml":
				return "\n  <obj type=\"POPUP\" command=\"BEGIN\" />";
				break;
		}
		return 0;
	}
	
	// popup_end
	function out_popup_end () {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return out_end(2);
				break;
			case "trans_xml":
				return "\n  <obj type=\"POPUP\" command=\"END\" />";
				break;
		}
		return 0;
	}

	// popup
	function out_popup ( $str_content ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n  POPUP \"". $str_content ."\"";
				break;
			case "trans_xml":
				return "\n  <obj type=\"POPUP\"><![CDATA[". $str_content ."]]></obj>";
				break;
		}
		return 0;
	}
	
	// menuitem
	function out_menuitem ( $str_content , $str_rc_name ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n    MENUITEM \"". $str_rc_name ."\" \t". $str_content;
				break;
			case "trans_xml":
				return "\n  <obj type=\"MENUITEM\" rc_name=\"". $str_rc_name ."\"><![CDATA[". $str_content ."]]></obj>";
				break;
		}
		return 0;
	}
	
	// menuitemseparator
	function out_menuitemseparator () {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n    MENUITEM SEPARATOR";
				break;
			case "trans_xml":
				return "\n  <obj type=\"MENUSEPARATOR\"></obj>";
				break;
		}
		return 0;
	}
	
	// dialog_begin
	function out_dialog_begin () {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return out_begin(0);
				break;
			case "trans_xml":
				return "\n  <obj type=\"DIALOG\" command=\"BEGIN\" />";
				break;
		}
		return 0;
	}

	// dialog_end
	function out_dialog_end () {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return out_end(0)."\n";
				break;
			case "trans_xml":
				return "\n  <obj type=\"DIALOG\" command=\"END\" />". out_group_end();
				break;
		}
		return 0;
	}
	
	// dialog
	function out_dialog ( $str_content, $str_rc_name, $str_top, $str_left, $str_height, $str_width ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n". $str_rc_name ." DIALOG ". $str_content ."  ". $str_top .",". $str_left .",". $str_height .",". $str_width;
				break;
			case "trans_xml":
				return out_group_begin("DIALOG") ."\n  <obj type=\"DIALOG\" rc_name=\"". $str_rc_name ."\" left=\"". $str_left ."\" top=\"". $str_top ."\" width=\"". $str_width ."\" height=\"". $str_height ."\">". $str_content ."</obj>";
				break;
		}
		return 0;
	}

	// style
	function out_style ( $str_content ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\nSTYLE ". $str_content;
				break;
			case "trans_xml":
				return "\n  <obj type=\"STYLE\"><![CDATA[". $str_content ."]]></obj>";
				break;
		}
		return 0;
	}
	
	// exstyle
	function out_exstyle ( $str_content ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\nEXSTYLE ". $str_content;
				break;
			case "trans_xml":
				return "\n  <obj type=\"EXSTYLE\"><![CDATA[". $str_content ."]]></obj>";
				break;
		}
		return 0;
	}
	
	// caption
	function out_caption ( $str_content ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\nCAPTION ". $str_content;
				break;
			case "trans_xml":
				return "\n  <obj type=\"CAPTION\"><![CDATA[". $str_content ."]]></obj>";
				break;
		}
		return 0;
	}

	// font
	function out_font ( $str_size, $str_name ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\nFONT ". $str_size .", \"". $str_name ."\"";
				break;
			case "trans_xml":
				return "\n  <obj type=\"FONT\" size=\"". $str_size ."\"  name=\"". $str_name ."\"></obj>";
				break;
		}
		return 0;
	}

	// control
	function out_control ( $str_content, $str_rc_name, $str_prop, $str_style, $str_top, $str_left, $str_height, $str_width ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n    CONTROL         ". $str_content .",\n                    ". $str_rc_name .",\"". $str_prop ."\",". $str_style .",". $str_top .",". $str_left .",". $str_height .",". $str_width;
				break;
			case "trans_xml":
				return "\n  <obj type=\"CONTROL\" rc_name=\"". $str_rc_name ."\" prop=\"". $str_prop ."\" style=\"". $str_style ."\" left=\"". $str_left ."\" top=\"". $str_top ."\" width=\"". $str_width ."\" height=\"". $str_height."\"><![CDATA[". $str_content ."]]></obj>";
				break;
		}
		return 0;
	}
	
	// defpushbutton
	function out_defpushbutton ( $str_content, $str_rc_name, $str_style, $str_top, $str_left, $str_height, $str_width ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n    DEFPUSHBUTTON   ". $str_content .",". $str_rc_name .",". $str_top .",". $str_left .",". $str_height .",". $str_width .",". $str_style;
				break;
			case "trans_xml":
				return "\n  <obj type=\"DEFPUSHBUTTON\" rc_name=\"". $str_rc_name ."\" style=\"". $str_style ."\" left=\"". $str_left ."\" top=\"". $str_top ."\" width=\"". $str_width ."\" height=\"". $str_height."\"><![CDATA[". $str_content ."]]></obj>";
				break;
		}
		return 0;
	}
	
	// pushbutton
	function out_pushbutton ( $str_content, $str_rc_name, $str_style, $str_top, $str_left, $str_height, $str_width ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n    PUSHBUTTON      ". $str_content .",". $str_rc_name .",". $str_top .",". $str_left .",". $str_height .",". $str_width .",". $str_style;
				break;
			case "trans_xml":
				return "\n  <obj type=\"PUSHBUTTON\" rc_name=\"". $str_rc_name ."\" style=\"". $str_style ."\" left=\"". $str_left ."\" top=\"". $str_top ."\" width=\"". $str_width ."\" height=\"". $str_height."\"><![CDATA[". $str_content ."]]></obj>";
				break;
		}
		return 0;
	}

	// ltext
	function out_ltext ( $str_content, $str_rc_name, $str_style, $str_top, $str_left, $str_height, $str_width ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n    LTEXT           ". $str_content .",". $str_rc_name .",". $str_top .",". $str_left .",". $str_height .",". $str_width .",". $str_style;
				break;
			case "trans_xml":
				return "\n  <obj type=\"LTEXT\" rc_name=\"". $str_rc_name ."\" style=\"". $str_style ."\" left=\"". $str_left ."\" top=\"". $str_top ."\" width=\"". $str_width ."\" height=\"". $str_height."\"><![CDATA[". $str_content ."]]></obj>";
				break;
		}
		return 0;
	}

	// groupbox
	function out_groupbox ( $str_content, $str_rc_name, $str_style, $str_top, $str_left, $str_height, $str_width ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n    GROUPBOX        ". $str_content .",". $str_rc_name .",". $str_top .",". $str_left .",". $str_height .",". $str_width .",". $str_style;
				break;
			case "trans_xml":
				return "\n  <obj type=\"GROUPBOX\" rc_name=\"". $str_rc_name ."\" style=\"". $str_style ."\" left=\"". $str_left ."\" top=\"". $str_top ."\" width=\"". $str_width ."\" height=\"". $str_height."\"><![CDATA[". $str_content ."]]></obj>";
				break;
		}
		return 0;
	}

	// icon
	function out_icon ( $str_rc_name, $str_style, $str_top, $str_left, $str_height, $str_width ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n    ICON            ". $str_rc_name .",". $str_top .",". $str_left .",". $str_height .",". $str_width .",". $str_style;
				break;
			case "trans_xml":
				return "\n  <obj type=\"ICON\" rc_name=\"". $str_rc_name ."\" style=\"". $str_style ."\" left=\"". $str_left ."\" top=\"". $str_top ."\" width=\"". $str_width ."\" height=\"". $str_height."\"></obj>";
				break;
		}
		return 0;
	}

	// edittext
	function out_edittext ( $str_rc_name, $str_style, $str_top, $str_left, $str_height, $str_width ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n    EDITTEXT        ". $str_rc_name .",". $str_top .",". $str_left .",". $str_height .",". $str_width .",". $str_style;
				break;
			case "trans_xml":
				return "\n  <obj type=\"EDITTEXT\" rc_name=\"". $str_rc_name ."\" style=\"". $str_style ."\" left=\"". $str_left ."\" top=\"". $str_top ."\" width=\"". $str_width ."\" height=\"". $str_height."\"></obj>";
				break;
		}
		return 0;
	}

	// listbox
	function out_listbox ( $str_rc_name, $str_style, $str_top, $str_left, $str_height, $str_width ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n    LISTBOX         ". $str_rc_name .",". $str_top .",". $str_left .",". $str_height .",". $str_width .",". $str_style;
				break;
			case "trans_xml":
				return "\n  <obj type=\"LISTBOX\" rc_name=\"". $str_rc_name ."\" style=\"". $str_style ."\" left=\"". $str_left ."\" top=\"". $str_top ."\" width=\"". $str_width ."\" height=\"". $str_height."\"></obj>";
				break;
		}
		return 0;
	}

	// combobox
	function out_combobox ( $str_rc_name, $str_style, $str_top, $str_left, $str_height, $str_width ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n    COMBOBOX        ". $str_rc_name .",". $str_top .",". $str_left .",". $str_height .",". $str_width .",". $str_style;
				break;
			case "trans_xml":
				return "\n  <obj type=\"COMBOBOX\" rc_name=\"". $str_rc_name ."\" style=\"". $str_style ."\" left=\"". $str_left ."\" top=\"". $str_top ."\" width=\"". $str_width ."\" height=\"". $str_height."\"></obj>";
				break;
		}
		return 0;
	}	
	
	// spaces
	function out_spaces ( $spaces ) {
		$tempa = "";
		for($x=1; $x<=$spaces; $x++){
			$tempa = " ";
		}
		return $tempa;
	}
		
	// curly_bracket_open
	function out_curly_bracket_open( $spaces ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n" . out_spaces( $spaces ) . "{";
				break;
		}
		return 0;
	}
	
	// curly_bracket_close
	function out_curly_bracket_close( $spaces ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n" . out_spaces( $spaces ) . "}";
				break;
		}
		return 0;
	}
	
	// begin
	function out_begin ( $spaces ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n" . out_spaces( $spaces ) . "BEGIN";
				break;
		}
		return 0;
	}

	// end	
	function out_end ( $spaces ) {
		global $ROST_parser3_mode;
		switch($ROST_parser3_mode) {
			case "trans_rc":
				return "\n" . out_spaces( $spaces ) . "END";
				break;
		}
		return 0;
	}
	
?>
