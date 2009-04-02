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
	if ( !defined('RSDB') )
	{
		die(" ");
	}

	$RSDB_URI_mode = "";


	$RSDB_intern_SET_path = "http://localhost/reactos.org/support/";
	$RSDB_intern_SET_path_ex = "http://localhost/reactos.org/support/index.php/";
	$RSDB_intern_dirs = "reactos.org/support/";


	$RSDB_URI_tree = $_SERVER['REQUEST_URI'];
//	echo $RSDB_URI_tree . "<br>";
	
	
	$RSDB_URI_tree = str_replace($RSDB_intern_dirs."index.php/","",$RSDB_URI_tree);
	$RSDB_URI_tree = str_replace($RSDB_intern_dirs,"",$RSDB_URI_tree);
//	echo $RSDB_URI_tree . "<br>";
	
	
	$RSDB_URI_tree_split = explode("/", $RSDB_URI_tree);
	
	
	@$RSDB_URI_tree_array = array();
	$RSDB_URI_tree_counter = 0;
//	echo "<hr />";
	foreach(@$RSDB_URI_tree_split as $value) {
//		echo "<br>".$value;
		$RSDB_URI_tree_array[$RSDB_URI_tree_counter] = @$value;
		$RSDB_URI_tree_counter++;
	}
	
/*
	echo "<br>#0: ".@$RSDB_URI_tree_array[3];
	echo "<br>#1: ".@$RSDB_URI_tree_array[4];
	echo "<br>#2: ".@$RSDB_URI_tree_array[5];
	echo "<br>#3: ".@$RSDB_URI_tree_array[6];
	echo "<br>#4: ".@$RSDB_URI_tree_array[7];
*/

	// Set HRU mode as default
	$RSDB_URI_mode = "HRU"; // Human Readable URI

	
	if (!strchr($RSDB_URI_tree,"?page=")) {
	
		$RSDB_URI_mode = "HRU"; // Human Readable URI
		
//		echo "<p>### HUMAN URI ### - ".$RSDB_SET_sec."</p>";
		
		$RSDB_HUMAN_URI_tree_1 = @$RSDB_URI_tree_array[3];
		$RSDB_HUMAN_URI_tree_2 = @$RSDB_URI_tree_array[4];
		$RSDB_HUMAN_URI_tree_3 = @$RSDB_URI_tree_array[5];
		$RSDB_HUMAN_URI_tree_4 = @$RSDB_URI_tree_array[6];


		// URI: .../comp/category/group/xx
		
		switch( $RSDB_HUMAN_URI_tree_1 ) {
			default:
			case "home":
				$RSDB_SET_page = "home";
				break;
			case "about":
				$RSDB_SET_page = "about";
				break;
			case "conditions":
				$RSDB_SET_page = "conditions";
				break;
			case "comp": // .../comp/cat/id/xx
				$RSDB_SET_page = "db";
				switch( $RSDB_HUMAN_URI_tree_2 ) {
					case "cat": // short
					case "category":
						$RSDB_SET_sec = "category";
						switch( $RSDB_HUMAN_URI_tree_3 ) {
							case "id": // short
							case "cat":
//								echo "<hr />CATEGORY_ID: ".$RSDB_HUMAN_URI_tree_4;
								$RSDB_SET_cat = $RSDB_HUMAN_URI_tree_4;
								break;
							case "group":
//								echo "<hr />CATEGORY_GROUP: ".$RSDB_HUMAN_URI_tree_4;
								$RSDB_SET_group = $RSDB_HUMAN_URI_tree_4;
								break;
							case "ver": // short
							case "item":
								$RSDB_SET_item = $RSDB_HUMAN_URI_tree_4;
								break;
						}
						break;
					case "name": // .../comp/name/id/xx
						$RSDB_SET_sec = "name";
						switch( $RSDB_HUMAN_URI_tree_3 ) {
							case "id": // short
							case "group":
								$RSDB_SET_group = $RSDB_HUMAN_URI_tree_4;
								break;
							case "ver": // short
							case "item":
								$RSDB_SET_item = $RSDB_HUMAN_URI_tree_4;
								break;
							case "letter":
								$RSDB_SET_letter = $RSDB_HUMAN_URI_tree_4;
								break;
						}
						break;
					case "vendor": // .../comp/vendor/id/xx
						$RSDB_SET_sec = "vendor";
						$RSDB_SET_letter = $RSDB_HUMAN_URI_tree_3;
						switch( $RSDB_HUMAN_URI_tree_3 ) {
							case "ver": // short
							case "item":
								$RSDB_SET_item = $RSDB_HUMAN_URI_tree_4;
								break;
							case "letter":
								$RSDB_SET_letter = $RSDB_HUMAN_URI_tree_4;
								break;
							case "id": // short
							case "vendor":
								$RSDB_SET_vendor = $RSDB_HUMAN_URI_tree_4;
								break;
						}
						break;
					case "rank":
						$RSDB_SET_sec = "rank";
						break;
					case "submit":
						$RSDB_SET_sec = "submit";
						break;
					case "stats":
						$RSDB_SET_sec = "stats";
						break;
					case "help":
						$RSDB_SET_sec = "help";
						break;
				}
				break;
		}
		
		$RSDB_SET_page = "db";
		$RSDB_SET_lang = "en";
		/*$RSDB_SET_entry = "";
		$RSDB_SET_entry = @$RSDB_URI_tree_array[2];
		$RSDB_SET_action = "";
		$RSDB_SET_action = @$RSDB_URI_tree_array[3];*/
	}




?>
