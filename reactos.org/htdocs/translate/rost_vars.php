<?php
/*
    React Operating System Translate - ROST
    Copyright (C) 2006  Klemens Friedl <frik85@reactos.org>

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

		global $ROST_intern_path_ex;

		global $ROST_SET_page;
		global $ROST_SET_sec;
		global $ROST_SET_item;
		global $ROST_SET_entry;
		global $ROST_SET_lang;
		global $ROST_SET_action;


		global $ROST_SET_view;
		global $ROST_SET_cat;
		global $ROST_SET_cat2;
		global $ROST_SET_letter;
		global $ROST_SET_curpos;
		global $ROST_SET_group;
		global $ROST_SET_group2;
		global $ROST_SET_item;
		global $ROST_SET_item2;
		global $ROST_langres;
		global $ROST_vote;
		global $ROST_vote2;
		global $ROST_SET_vendor;
		global $ROST_SET_rank;
		global $ROST_SET_rank2;
		global $ROST_SET_addbox;

		global $ROST_SET_export;
		
		global $ROST_SET_threshold;
		global $ROST_SET_fstyle;
		global $ROST_SET_order;
		global $ROST_SET_save;
		global $ROST_SET_msg;
		global $ROST_SET_filter;
		global $ROST_SET_filter2;





	// Settings:
	// *********

		$ROST_setting_stars_threshold = 6;
		
		$ROST_setting_default_language = "en-us";
		$ROST_setting_default_language_short = "en";
		$ROST_setting_default_language_name = "English";
		
		

	// Links:
	// ******
	
		// General
		$ROST_intern_link_trans_edit = $ROST_intern_path_ex.$ROST_SET_lang.'/'.$ROST_SET_entry.'/edit/';
		$ROST_intern_link_section = $ROST_intern_path_ex;
		
	
	
		// RosCMS
		$ROST_intern_link_roscms = $ROST_intern_path_server.$ROST_intern_loginsystem_path."index.php";
		$ROST_intern_link_roscms_page = $ROST_intern_link_roscms."?page=";
	
		// Section
		$ROST_intern_link_db_sec = $ROST_intern_index_php."?page=db&amp;view=".$ROST_SET_view."&amp;sec=";
		$ROST_intern_link_db_sec_javascript = $ROST_intern_index_php."?page=db&view=".$ROST_SET_view."&sec=";
		$ROST_intern_link_db_sec_javascript2 = $ROST_intern_index_php."?page=db&view=".$ROST_SET_view."&sec=".$ROST_SET_sec;		
		
		// View
		$ROST_intern_link_db_view = $ROST_intern_index_php."?page=db&amp;sec=".$ROST_SET_sec."&amp;view=";
		$ROST_intern_link_db_view2 = $ROST_intern_index_php."?page=db&amp;sec=".$ROST_SET_sec."&amp;cat=".$ROST_SET_cat."&amp;cat2=".$ROST_SET_cat2."&amp;letter=".$ROST_SET_letter."&amp;group=".$ROST_SET_group."&amp;item=".$ROST_SET_item."&amp;vendor=".$ROST_SET_vendor."&amp;view=";
		
		// Views	
		$ROST_intern_link_db_view_comp = $ROST_intern_link_db_view."comp";
		$ROST_intern_link_db_view_pack = $ROST_intern_link_db_view."pack";
		$ROST_intern_link_db_view_devnet = $ROST_intern_link_db_view."devnet";
		$ROST_intern_link_db_view_media = $ROST_intern_link_db_view."media";
		
		// Category	
		$ROST_intern_link_category_all = $ROST_intern_link_db_sec.$ROST_SET_sec."&amp;cat=".$ROST_SET_cat."&amp;cat2=".$ROST_SET_cat2;
		
		$ROST_intern_link_category_cat = $ROST_intern_link_db_sec.$ROST_SET_sec."&amp;cat2=".$ROST_SET_cat2."&amp;cat=";
		$ROST_intern_link_category_cat2 = $ROST_intern_link_db_sec.$ROST_SET_sec."&amp;cat=".$ROST_SET_cat."&amp;cat2=";
		
		// Name
		$ROST_intern_link_name_letter = $ROST_intern_link_db_sec.$ROST_SET_sec."&amp;letter=";
		$ROST_intern_link_name_letter2 = $ROST_intern_link_db_sec.$ROST_SET_sec."&amp;letter=".$ROST_SET_letter;
		$ROST_intern_link_name_curpos = $ROST_intern_link_db_sec.$ROST_SET_sec."&amp;letter=".$ROST_SET_letter."&amp;curpos=";
		//$ROST_intern_link_name_cat = $ROST_intern_link_db_sec.$ROST_SET_sec."&amp;cat2=".$ROST_SET_cat2."&amp;cat=";

		// Group
		$ROST_intern_link_group = $ROST_intern_link_db_sec.$ROST_SET_sec."&amp;group=";
		$ROST_intern_link_group_group2 = $ROST_intern_link_group.$ROST_SET_group."&amp;group2=";
		$ROST_intern_link_group_group2_both = $ROST_intern_link_group_group2.$ROST_SET_group2;
		$ROST_intern_link_group_group2_both_javascript = $ROST_intern_link_db_sec_javascript.$ROST_SET_sec."&group=".$ROST_SET_group."&group2=".$ROST_SET_group2;
		$ROST_intern_link_group_sort = $ROST_intern_link_group.$ROST_SET_group."&amp;group2=".$ROST_SET_group2."&amp;sort=";
		$ROST_intern_link_group_comp = $ROST_intern_index_php."?page=db&amp;view=comp&amp;sec=category&amp;group=";
		$ROST_intern_link_group_comp_javascript = $ROST_intern_index_php."?page=db&view=comp&sec=category&group=";

		// Item
		$ROST_intern_link_item = $ROST_intern_link_db_sec.$ROST_SET_sec."&amp;item=";
		$ROST_intern_link_item_javascript = $ROST_intern_index_php."?page=db&view=".$ROST_SET_view."&sec=".$ROST_SET_sec."&item=";
		$ROST_intern_link_item_item2 = $ROST_intern_link_item.$ROST_SET_item."&amp;item2=";
		$ROST_intern_link_item_item2_both = $ROST_intern_link_item_item2.$ROST_SET_item2;
		$ROST_intern_link_item_item2_both_javascript = $ROST_intern_link_db_sec_javascript.$ROST_SET_sec."&item=".$ROST_SET_item."&item2=".$ROST_SET_item2;
	
		$ROST_intern_link_item_comp = $ROST_intern_index_php."?page=db&amp;view=comp&amp;sec=category&amp;item=";
		
		$ROST_intern_link_item_item2_vote = $ROST_intern_link_item_item2.$ROST_SET_item2."&amp;vote=";
		$ROST_intern_link_item_item2_forum_bar = $ROST_intern_link_item_item2.$ROST_SET_item2;
		$ROST_intern_link_item_item2_forum_msg = $ROST_intern_link_item_item2.$ROST_SET_item2."&amp;threshold=".$ROST_SET_threshold."&amp;fstyle=".$ROST_SET_fstyle."&amp;order=".$ROST_SET_order."&amp;filter=".$ROST_SET_filter."&amp;msg=";

		// Vendor
		$ROST_intern_link_vendor = $ROST_intern_link_db_sec.$ROST_SET_sec."&amp;vendor=";
		$ROST_intern_link_vendor_both_javascript = $ROST_intern_link_db_sec_javascript.$ROST_SET_sec."&vendor=".$ROST_SET_vendor;
		$ROST_intern_link_vendor2_group = $ROST_intern_link_db_sec."name&amp;group=";
		$ROST_intern_link_vendor_sec = $ROST_intern_link_db_sec."vendor&amp;vendor=";
		$ROST_intern_link_vendor_sec_comp = $ROST_intern_index_php."?page=db&amp;view=comp&amp;sec=vendor&amp;vendor=";

		// Rank
		$ROST_intern_link_rank = $ROST_intern_link_db_sec.$ROST_SET_sec."&amp;rank=";
		$ROST_intern_link_rank_rank2 = $ROST_intern_link_rank.$ROST_SET_rank."&amp;rank2=";
		$ROST_intern_link_rank2_group = $ROST_intern_link_db_sec."category&amp;group=";
		$ROST_intern_link_rank2_item = $ROST_intern_link_db_sec."category&amp;item=";
		$ROST_intern_link_rank_sec = $ROST_intern_link_db_sec."rank&amp;rank=";
		$ROST_intern_link_rank_sec_comp = $ROST_intern_index_php."?page=db&amp;view=comp&amp;sec=rank&amp;vendor=";
		$ROST_intern_link_rank_curpos = $ROST_intern_link_rank_rank2.$ROST_SET_rank2."&amp;curpos=";
		$ROST_intern_link_rank_filter = $ROST_intern_link_rank_curpos.$ROST_SET_curpos."&amp;filter=";


		// Submit
			// Compatibility Test Report
			$ROST_intern_link_submit_comp_test = $ROST_intern_link_item_item2.$ROST_SET_item2."&amp;addbox=";
			// Compatibility Screenshot
			$ROST_intern_link_submit_comp_screenshot = $ROST_intern_link_item_item2.$ROST_SET_item2."&amp;addbox=";
			// Forum Post
			$ROST_intern_link_submit_forum_post = $ROST_intern_link_item_item2.$ROST_SET_item2."&amp;addbox=reply&amp;entry=";
			// Application Version
			$ROST_intern_link_submit_appver = $ROST_intern_link_item_item2.$ROST_SET_item2."&amp;group=".$ROST_SET_group."&amp;addbox=submit";
			$ROST_intern_link_submit_appver_javascript = $ROST_intern_link_item_javascript.$ROST_SET_item."&item2=".$ROST_SET_item2."&group=".$ROST_SET_group."&addbox=submit";


		// Language
		$ROST_intern_link_language = $ROST_intern_index_php."?page=".$ROST_SET_page."&amp;sec=".$ROST_SET_sec."&amp;view=".$ROST_SET_view."&amp;cat=".$ROST_SET_cat."&amp;cat2=".$ROST_SET_cat2."&amp;letter=".$ROST_SET_letter."&amp;group=".$ROST_SET_group."&amp;item=".$ROST_SET_item."&amp;order=".$ROST_SET_order."&amp;filter=".$ROST_SET_filter."&amp;vendor=".$ROST_SET_vendor."&amp;lang=";

		// Ajax
		$ROST_intern_link_ajax = $ROST_intern_link_language.$ROST_SET_lang."&amp;ajax=";

		// Export
		//$ROST_intern_link_export = $ROST_intern_index_php."?page=data&amp;sec=".$ROST_SET_sec."&amp;view=".$ROST_SET_view."&amp;cat=".$ROST_SET_cat."&amp;cat2=".$ROST_SET_cat2."&amp;letter=".$ROST_SET_letter."&amp;group=".$ROST_SET_group."&amp;item=".$ROST_SET_item."&amp;order=".$ROST_SET_order."&amp;filter=".$ROST_SET_filter."&amp;vendor=".$ROST_SET_vendor."&amp;lang=".$ROST_SET_lang."&amp;export=";
		$ROST_intern_link_export = $ROST_intern_index_php."?page=dat&sec=".$ROST_SET_sec."&view=".$ROST_SET_view."&cat=".$ROST_SET_cat."&cat2=".$ROST_SET_cat2."&letter=".$ROST_SET_letter."&group=".$ROST_SET_group."&item=".$ROST_SET_item."&order=".$ROST_SET_order."&filter=".$ROST_SET_filter."&vendor=".$ROST_SET_vendor."&lang=".$ROST_SET_lang."&export=";
		$ROST_intern_link_export2 = $ROST_intern_index_php."?page=dat&export=";


		// Media
		$ROST_intern_media_link = $ROST_intern_path_server.$ROST_intern_path."media/";
		
			// Picture
			$ROST_intern_media_link_picture = $ROST_intern_media_link."picture/";
			
			// Icons
			$ROST_intern_media_link_icon = $ROST_intern_media_link."icons/";
		

	// Triggers:
	// *********
	
		// Views
		$ROST_intern_trigger_comp = "0";
		$ROST_intern_trigger_pack = "0";
		$ROST_intern_trigger_devnet = "0";
		$ROST_intern_trigger_media = "0";
		switch ($ROST_SET_view) {
			case "comp": // Compatibility
			default:
				$ROST_intern_trigger_comp = "1";
				break;
			case "pack": // Packages
				$ROST_intern_trigger_pack = "1";
				break;
			case "devnet": // Developer Network
				$ROST_intern_trigger_devnet = "1";
				break;
			case "media": // Media
				$ROST_intern_trigger_media = "1";
				break;
		}
	
	// Code Snips:
	// ***********

		// Views
	
			switch ($ROST_SET_view) {
				case "comp": // Compatibility
				default:
					$ROST_intern_code_view_name = $ROST_langres['TEXT_compdb_short'];
					$ROST_intern_code_view_shortname = "comp";
					$ROST_intern_code_view_description = $ROST_langres['CONTENT_compdb_description'];
					break;
				case "pack": // Packages
					$ROST_intern_code_view_name = $ROST_langres['TEXT_packdb_short'];
					$ROST_intern_code_view_shortname = "pack";
					$ROST_intern_code_view_description = $ROST_langres['CONTENT_packdb_description'];
					break;
				case "devnet": // Developer Network
					$ROST_intern_code_view_name = $ROST_langres['TEXT_devnet_long'];
					$ROST_intern_code_view_shortname = "devnet";
					$ROST_intern_code_view_description = $ROST_langres['CONTENT_devnet_description_short'];
					break;
				case "media": // Media
					$ROST_intern_code_view_name = $ROST_langres['TEXT_mediadb_short'];
					$ROST_intern_code_view_shortname = "media";
					$ROST_intern_code_view_description = $ROST_langres['CONTENT_mediadb_description'];
					break;
			}
	
		// Database
		
			// ROST_categories
			$ROST_intern_code_db_ROST_categories_comp = "";
			$ROST_intern_code_db_ROST_categories_pack = "";
			$ROST_intern_code_db_ROST_categories_devnet = "";
			$ROST_intern_code_db_ROST_categories_media = "";
			switch ($ROST_SET_view) {
				case "comp": // Compatibility
				default:
					$ROST_intern_code_db_ROST_categories_comp = " AND `cat_comp` = '" . $ROST_intern_trigger_comp . "' ";
					break;
				case "pack": // Packages
					$ROST_intern_code_db_ROST_categories_pack = " AND `cat_pack` = '" . $ROST_intern_trigger_pack . "' ";
					break;
				case "devnet": // Developer Network
					$ROST_intern_code_db_ROST_categories_devnet = " AND `cat_devnet` = '" . $ROST_intern_trigger_devnet . "' ";
					break;
				case "media": // Media
					$ROST_intern_code_db_ROST_categories_media = " AND `cat_media` = '" . $ROST_intern_trigger_media . "' ";
					break;
			}
			$ROST_intern_code_db_ROST_categories = $ROST_intern_code_db_ROST_categories_comp.$ROST_intern_code_db_ROST_categories_pack.$ROST_intern_code_db_ROST_categories_devnet.$ROST_intern_code_db_ROST_categories_media;
			// Code: " . $ROST_intern_code_db_ROST_categories . "

			// ROST_groups
			$ROST_intern_code_db_ROST_groups_comp = "";
			$ROST_intern_code_db_ROST_groups_pack = "";
			$ROST_intern_code_db_ROST_groups_devnet = "";
			$ROST_intern_code_db_ROST_groups_media = "";
			switch ($ROST_SET_view) {
				case "comp": // Compatibility
				default:
					$ROST_intern_code_db_ROST_groups_comp = " AND `grpentr_comp` = '" . $ROST_intern_trigger_comp . "' ";
					break;
				case "pack": // Packages
					$ROST_intern_code_db_ROST_groups_pack = " AND `grpentr_pack` = '" . $ROST_intern_trigger_pack . "' ";
					break;
				case "devnet": // Developer Network
					$ROST_intern_code_db_ROST_groups_devnet = " AND `grpentr_devnet` = '" . $ROST_intern_trigger_devnet . "' ";
					break;
				case "media": // Media
					$ROST_intern_code_db_ROST_groups_media = " AND `grpentr_media` = '" . $ROST_intern_trigger_media . "' ";
					break;
			}
			$ROST_intern_code_db_ROST_groups = $ROST_intern_code_db_ROST_groups_comp.$ROST_intern_code_db_ROST_groups_pack.$ROST_intern_code_db_ROST_groups_devnet.$ROST_intern_code_db_ROST_groups_media;
			// Code: " . $ROST_intern_code_db_ROST_groups . "

?>
