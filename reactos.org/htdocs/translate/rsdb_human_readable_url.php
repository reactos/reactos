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



	// To prevent hacking activity:
	if ( !defined('ROST') )
	{
		die(" ");
	}

	$RSDB_URI_mode = "";


	$RSDB_intern_SET_path = "http://localhost/reactos.org/translate/";
	$RSDB_intern_SET_path_ex = "http://localhost/reactos.org/translate/index.php/";
	$RSDB_intern_dirs = "reactos.org/translate/";


	$RSDB_URI_tree = $_SERVER['REQUEST_URI'];
	echo $RSDB_URI_tree . "<br>";
	
	
	$RSDB_URI_tree = str_replace($RSDB_intern_dirs."index.php/","",$RSDB_URI_tree);
	$RSDB_URI_tree = str_replace($RSDB_intern_dirs,"",$RSDB_URI_tree);
	echo $RSDB_URI_tree . "<br>";
	
	
	$RSDB_URI_tree_split = explode("/", $RSDB_URI_tree);
	
	
	@$RSDB_URI_tree_array = array();
	$RSDB_URI_tree_counter = 0;
	echo "<hr />";
	foreach(@$RSDB_URI_tree_split as $value) {
		echo "<br>".$value;
		$RSDB_URI_tree_array[$RSDB_URI_tree_counter] = @$value;
		$RSDB_URI_tree_counter++;
	}
	

	echo "<br>#0: ".@$RSDB_URI_tree_array[0];
	echo "<br>#1: ".@$RSDB_URI_tree_array[1];
	echo "<br>#2: ".@$RSDB_URI_tree_array[2];
	echo "<br>#3: ".@$RSDB_URI_tree_array[3];
	echo "<br>#4: ".@$RSDB_URI_tree_array[4];


	// Set HRU mode as default
	$RSDB_URI_mode = "HRU"; // Human Readable URI

	
	if (!strchr($RSDB_URI_tree,"?page=")) {
	
		$RSDB_URI_mode = "HRU"; // Human Readable URI
		
		echo "<p>### HUMAN URI ### - ".$ROST_SET_sec."</p>";
		
		$RSDB_HUMAN_URI_tree_1 = @$RSDB_URI_tree_array[1];
		$RSDB_HUMAN_URI_tree_2 = @$RSDB_URI_tree_array[2];
		$RSDB_HUMAN_URI_tree_3 = @$RSDB_URI_tree_array[3];
		$RSDB_HUMAN_URI_tree_4 = @$RSDB_URI_tree_array[4];


		// URI: .../comp/category/group/xx
		$ROST_SET_view = "";
		
		switch( $RSDB_HUMAN_URI_tree_1 ) {
			case "home":
				$ROST_SET_sec = "home";
				echo "<br>?home?";
				break;
			case "about":
				$ROST_SET_sec = "about";
				break;
			case "help":
				$ROST_SET_sec = "help";
				break;
			case "conditions":
				$ROST_SET_sec = "conditions";
				break;
			case "admin":
				$ROST_SET_sec = "admin";
				break;
			case "import":
				$ROST_SET_sec = "import";
				break;
			case "export":
				$ROST_SET_sec = "export";
				break;
			case "all":
				$ROST_SET_sec = "all";
				break;
			default:
				if (check_langcode($RSDB_HUMAN_URI_tree_1)) {
					$ROST_SET_sec = $RSDB_HUMAN_URI_tree_1;
					//echo "<br />(".$ROST_SET_sec.")";
				}
				else {
					$ROST_SET_page = "home";
					echo "<br>?home?";
				}
				break;
				
			case "test":
				$ROST_SET_sec = "test";
				break;
			
			
/*				$ROST_SET_page = "db";
				$ROST_SET_view = "comp";
				switch( $RSDB_HUMAN_URI_tree_2 ) {
					case "cat": // short
					case "category":
						$ROST_SET_sec = "category";
						switch( $RSDB_HUMAN_URI_tree_3 ) {
							case "id": // short
							case "cat":
//								echo "<hr />CATEGORY_ID: ".$RSDB_HUMAN_URI_tree_4;
								$ROST_SET_cat = $RSDB_HUMAN_URI_tree_4;
								break;
							case "group":
//								echo "<hr />CATEGORY_GROUP: ".$RSDB_HUMAN_URI_tree_4;
								$ROST_SET_group = $RSDB_HUMAN_URI_tree_4;
								break;
							case "ver": // short
							case "item":
								$ROST_SET_item = $RSDB_HUMAN_URI_tree_4;
								break;
						}
						break;
					case "name": // .../comp/name/id/xx
						$ROST_SET_sec = "name";
						switch( $RSDB_HUMAN_URI_tree_3 ) {
							case "id": // short
							case "group":
								$ROST_SET_group = $RSDB_HUMAN_URI_tree_4;
								break;
							case "ver": // short
							case "item":
								$ROST_SET_item = $RSDB_HUMAN_URI_tree_4;
								break;
							case "letter":
								$ROST_SET_letter = $RSDB_HUMAN_URI_tree_4;
								break;
						}
						break;
					case "vendor": // .../comp/vendor/id/xx
						$ROST_SET_sec = "vendor";
						$ROST_SET_letter = $RSDB_HUMAN_URI_tree_3;
						switch( $RSDB_HUMAN_URI_tree_3 ) {
							case "ver": // short
							case "item":
								$ROST_SET_item = $RSDB_HUMAN_URI_tree_4;
								break;
							case "letter":
								$ROST_SET_letter = $RSDB_HUMAN_URI_tree_4;
								break;
							case "id": // short
							case "vendor":
								$ROST_SET_vendor = $RSDB_HUMAN_URI_tree_4;
								break;
						}
						break;
					case "help":
						$ROST_SET_sec = "help";
						break;
				}
				break;
*/				

		}
		
		/*$ROST_SET_entry = "";
		$ROST_SET_entry = @$RSDB_URI_tree_array[2];
		$ROST_SET_action = "";
		$ROST_SET_action = @$RSDB_URI_tree_array[3];*/
	}




?>
