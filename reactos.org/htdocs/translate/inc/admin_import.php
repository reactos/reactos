<?php
    /*
    RSDB - ReactOS Support Database
    Copyright (C) 2005-2006  Klemens Friedl <frik85@reactos.org>

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

/*
 *	ReactOS Support Database System - RSDB
 *	
 *	(c) by Klemens Friedl <frik85>
 *	
 *	2005 - 2006 
 */


	// To prevent hacking activity:
	if ( !defined('ROST') )
	{
		die(" ");
	}

?>
<h1><a href="<?php echo $ROST_intern_path_server; ?>?page=dev">Development</a> &gt; <a href="<?php echo $ROST_intern_link_section; ?>home">Translate <?php echo $ROST_intern_projectname; ?></a> &gt; <a href="<?php echo $ROST_intern_link_section; ?>admin">Admin</a> &gt; Import Translations</h1> 
<h2>Import Translations</h2> 
<h3>Checking import directory ...</h3>
<?php
	set_time_limit(0);


	$gentimeb="";
	$gentimeb = microtime(); 
	$gentimeb = explode(' ',$gentimeb); 
	$gentimeb = $gentimeb[1] + $gentimeb[0]; 
	$pg_startb = $gentimeb; 


	require_once('inc/tool_list_dir.php');


	$Files = LoadFiles("import/", "en-us.xml");
	SortByDate($Files);
	reset($Files);
	
	echo "<ul>";
	while (list($k,$v) =each($Files)) {
		$temp_filename_path = split("[\/]", $v[0]);
		$temp_filenamea = split("[_]", $temp_filename_path[1]);
		$temp_filename = $temp_filenamea[0];

		echo "<li><b>". $temp_filename ."</b> (". date('Y-m-d H:i:s', $v[1]) .")";
		
				
		$query_app_exist = SQL_query_array("SELECT * 
											FROM `apps` 
											WHERE `app_name` = '". SQL_escape($temp_filename) ."'
											AND `app_enabled` = '1'
											LIMIT 1 ;");
		if ($query_app_exist['app_name'] == $temp_filename) {
			echo "<ul><li>app already exist in DB</li></ul>";
			if (file_exists($temp_filename_path[0]."/".$temp_filename."_$.txt")) {
				$temp_file_content = file_get_contents($temp_filename_path[0]."/".$temp_filename."_$.txt");
				$temp_content_data = split("[|]", $temp_file_content);
				if ($temp_content_data[0]) {
					echo "<ul><li>dir-file found</li>";
					if ($query_app_exist['app_path'] == $temp_content_data[0]) {
						echo "<li>DB already up-to-date</li>";
					}
					else {
						$query_app_insert = SQL_query("UPDATE `apps` 
									SET `app_path` = '". SQL_escape($temp_content_data[0]) ."' 
									WHERE `app_id` =". SQL_escape($query_app_exist['app_id']) ." 
									LIMIT 1 ;");
						echo "<li><font color=\"#009900\">app path updated in DB</font></li>";
					}
					echo "</ul>";
				}
				else {
					echo "<ul><li><font color=\"#FF0000\">invalid dir-file &rarr; app not imported</font></li></ul>";
				}
			}
			else {
				echo "<ul><li><font color=\"#FF0000\">dir-file don't exist &rarr; app not imported</font></li></ul>";
			}

			
		}
		else {
			echo "<ul><li>app don't exist in DB</li></ul>";
			if (file_exists($temp_filename_path[0]."/".$temp_filename."_$.txt")) {
				$temp_file_content = file_get_contents($temp_filename_path[0]."/".$temp_filename."_$.txt");
				$temp_content_data = split("[|]", $temp_file_content);
				if ($temp_content_data[0]) {
					$query_app_insert = SQL_query("INSERT INTO `apps` ( `app_id` , `app_name` , `app_path` , `app_enabled` ) 
													VALUES (
														NULL , '". SQL_escape($temp_filename) ."', '". SQL_escape($temp_content_data[0]) ."', '1'
													);");
					echo "<ul><li>dir-file found &rarr; app added to DB</li></ul>";
				}
				else {
					echo "<ul><li><font color=\"#FF0000\">invalid dir-file &rarr; app not imported</font></li></ul>";
				}
			}
			else {
				echo "<ul><li><font color=\"#FF0000\">dir-file don't exist &rarr; app not imported</font></li></ul>";
			}
		}
		echo "</li>";


	} 
	echo "</ul>";
	
/*	
	$sql_original_content = "INSERT INTO `content` ( `content_id` , `content_name` , `content_lang` , `content_editor` , `content_text` , `content_version` , `content_active` , `content_visible` , `content_date` , `content_time` , `content_usrname_id`)
		VALUES ('', '". mysql_escape_string($content_contentid) ."', '". mysql_escape_string($content_langa) ."', '". mysql_escape_string($content_extra) ."', '". mysql_real_escape_string($content_data) ."', '". mysql_escape_string($content_version) ."', '". mysql_escape_string($content_act) ."', '". mysql_escape_string($content_vis) ."', CURDATE( ), CURTIME( ), '". mysql_escape_string($roscms_intern_account_id) ."');";
	
	$query_original_content = mysql_query($content_postb);
*/

?>

<h3>Importing XML translation files ...</h3>
<?php
	$Files = LoadFiles("import/", ".xml");
	SortByDate($Files);
	reset($Files);
	
	echo "<ul>";
	while (list($k,$v) =each($Files)) {
		$temp_filename_path = split("[\/]", $v[0]);
		$temp_filenamea = split("[_]", $temp_filename_path[1]);
		$temp_filenameb = split("[.]", $temp_filename_path[1]);
		$temp_filenamec = split("[.]", $temp_filenamea[1]);
		$temp_filename = $temp_filenamea[0];
		$temp_filename2 = $temp_filenameb[0];
		$temp_filename3 = $temp_filenamec[0];
		$temp_filenamed = split("[_]", $temp_filename2);
		$temp_filename4 = $temp_filenamed[0];
		$temp_filename5 = $temp_filenamed[1];
		
		//if ($temp_filename3 != $ROST_setting_default_language && $temp_filename3 != $ROST_setting_default_language_short) {
			//echo $temp_filename3 . " vs. ". $ROST_setting_default_language ." &amp; ". $temp_filename3 . " vs. ". $ROST_setting_default_language_short;
			echo "<li><b>". $temp_filename4 ." [". $temp_filename5 ."]</b> (". date('Y-m-d H:i:s', $v[1]) .")";
			if (file_exists($temp_filename_path[0]."/".$temp_filename."_$.txt")) {
				$temp_file_content = file_get_contents($temp_filename_path[0]."/".$temp_filename."_$.txt");
				if ($temp_file_content) {
					$temp_content_data = split("[|]", $temp_file_content);
					echo " - r".$temp_content_data[1];
					$temp_xml_file_content = file_get_contents($temp_filename_path[0]."/".$temp_filename2.".xml");
					if ($temp_xml_file_content) {
						$result_current_app = SQL_query_array("SELECT * 
																FROM `apps` 
																WHERE `app_name` = '". SQL_escape($temp_filename4) ."'
																LIMIT 1 ;");		
						$query_app_exist_a = SQL_query_array("SELECT * 
																FROM `translations` 
																WHERE `xml_rev` = '". SQL_escape($temp_content_data[1]) ."'
																AND `app_id` = '". SQL_escape($result_current_app['app_id']) ."'
																AND `xml_lang` = '". SQL_escape($temp_filename5) ."'
																LIMIT 1 ;");	
						if ($query_app_exist_a['app_id'] != "") {
							echo "<ul><li><font color=\"#FF0000\">app-revision already in DB &rarr; app-xml not imported</font></li></ul>";
							die ("</li></ul><p><font color=\"#FF0000\"><b>[NOTHING TO IMPORT] Please check & cleanup the import-dir; only then try again!</b></font></p>");
						}
						else {

							$temp_import = SQL_query("INSERT INTO `translations` 
															( `xml_id` , `app_id` , `xml_lang` , `xml_rev` , `xml_content` , `trans_checked` , `trans_active` , `trans_datetime` ) 
														VALUES (
															NULL , '". SQL_escape($result_current_app['app_id']) ."', '". SQL_escape($temp_filename5) ."', '". SQL_escape($temp_content_data[1]) ."', '". SQL_escape($temp_xml_file_content) ."', '0', '0', NOW( ) 
														);");			
							echo "<ul><li>app-xml imported in DB</li></ul>";
						}
					}
					else {
						echo "<ul><li><font color=\"#FF0000\">invalid app-xml file &rarr; app-xml not imported</font></li></ul>";
					}					
				}
			}
			else {
				echo "<ul><li><font color=\"#FF0000\">invalid dir-file &rarr; app not imported</font></li></ul>";
			}
		//}
		echo "</li>";
	}
	echo "</ul>";
?>


<h3>Checking and updating XML files ...</h3>

<?php
	include("inc/parser/parser_xml_transfer.php");
	include("inc/parser/parser_xml_compare.php");
	include("inc/parser/parser3.php");

	$result_latest_rev = SQL_query_array("SELECT * 
										FROM `translations` 
										ORDER BY `xml_rev` DESC 
										LIMIT 1 ;");		
										
	$query_latest_xml = SQL_query("SELECT * 
								FROM `translations` 
								WHERE `xml_rev` = '". SQL_escape($result_latest_rev['xml_rev']) ."'
								ORDER BY `app_id` ASC ;");	
	//echo "<p>rev: ".$result_latest_rev['xml_rev']."</p>";
	echo "<ul>";
	while ($result_latest_xml = SQL_loop_array($query_latest_xml)) {
		if ($result_latest_xml['xml_lang'] != $ROST_setting_default_language && $result_latest_xml['xml_lang'] != $ROST_setting_default_language_short) {
			$result_current_app = SQL_query_array("SELECT * 
													FROM `apps` 
													WHERE `app_id` = '". SQL_escape($result_latest_xml['app_id']) ."'
													LIMIT 1 ;");		
			$temp_filetwo = $result_current_app['app_name'] ."_". $result_latest_xml['xml_lang'] ."_". $result_latest_xml['xml_rev'];
			$temp_output = parser3 ($result_current_app['app_name'], "en-us", $result_latest_xml['xml_rev'], "trans_update", "", $temp_filetwo);

			echo "<li>".$result_current_app['app_name']." [".$result_latest_xml['xml_lang']."] - r".$result_latest_xml['xml_rev'];
			if ($temp_output != "") {
				$query_app_insert = SQL_query("UPDATE `translations` 
							SET `xml_content` = '". SQL_escape($temp_output) ."' 
							WHERE `app_id` =". SQL_escape($result_latest_xml['app_id']) ." 
							LIMIT 1 ;");
	
				//echo '<ul><li><i>Output:</i><br /><textarea name="textfield" cols="100" rows="5">'.$temp_output.'</textarea></li></ul></li>';
			}
		}
	}
	echo "</ul>";

/*
	echo "<hr /><b>###</b><hr />";
	
	include("inc/parser/parser_xml_transfer.php");
	$temp_a = parser_transfer ("notepad", "en-us", "24067", "STRING_CRLF");
	echo $temp_a;
	echo '<textarea name="textfield" cols="100" rows="5">'.write_xml_node($temp_a, "").'</textarea>';

	echo "<hr /><b>###</b><hr />";

	include("inc/parser/parser_xml_compare.php");
	echo parser_xc ("notepad", "en-us", "24067", "STRING_CRLF");
	
	echo "<hr /><b>###</b><hr />";
	
	include("inc/parser/parser3.php");
*/
/*	$temp_b = parser3 ("notepad", "en-us", "24067", "trans_rc");
	echo $temp_b;
	//echo '<textarea name="textfield" cols="100" rows="5">'.$temp_b.'</textarea>';

*/
/*	echo "<hr /><b>###</b><hr />";


	$temp_b = parser3 ("notepad", "en-us", "24067", "trans_update", "", "notepad_de-de_24020");
	echo $temp_b;
*/


	$gentimec = microtime(); 
	$gentimec = explode(' ',$gentimec); 
	$gentimec = $gentimec[1] + $gentimec[0]; 
	$pg_endb = $gentimec; 
	$totaltimef = ($pg_endb - $pg_startb); 
	$showtimef = number_format($totaltimef, 4, '.', ''); 
	
	echo "\n<p><i>Page generated in ".$showtimef." seconds.</i></p>";

	set_time_limit(30);


?>
