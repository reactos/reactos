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
	if ( !defined('RSDB') )
	{
		die(" ");
	}



		global $RSDB_SET_page;
		global $RSDB_SET_sec;
		global $RSDB_SET_cat;
		global $RSDB_SET_cat2;
		global $RSDB_SET_letter;
		global $RSDB_SET_curpos;
		global $RSDB_SET_group;
		global $RSDB_SET_group2;
		global $RSDB_SET_item;
		global $RSDB_SET_item2;
		global $RSDB_langres;
		global $RSDB_vote;
		global $RSDB_vote2;
		global $RSDB_SET_vendor;
		global $RSDB_SET_rank;
		global $RSDB_SET_rank2;
		global $RSDB_SET_addbox;
		global $RSDB_SET_entry;
		
		
		global $RSDB_SET_export;
		
		
		global $RSDB_SET_threshold;
		global $RSDB_SET_fstyle;
		global $RSDB_SET_order;
		global $RSDB_SET_save;
		global $RSDB_SET_msg;
		global $RSDB_SET_filter;
		global $RSDB_SET_filter2;


		global $RSDB_SET_lang;



	// Settings:
	// *********

		$RSDB_setting_stars_threshold = 6;

	// Links:
	// ******
		
		// RosCMS
		$RSDB_intern_link_roscms = $RSDB_intern_path_server.$RSDB_intern_loginsystem_path."index.php";
		$RSDB_intern_link_roscms_page = $RSDB_intern_link_roscms."?page=";
	
		// Section
		$RSDB_intern_link_db_sec = $RSDB_intern_index_php."?page=";
		$RSDB_intern_link_db_sec_javascript = $RSDB_intern_index_php."?page=";
		$RSDB_intern_link_db_sec_javascript2 = $RSDB_intern_index_php."?page=".$RSDB_SET_sec;
		
		
		// View
		$RSDB_intern_link_db_view = $RSDB_intern_index_php."?page=".$RSDB_SET_sec."";
		$RSDB_intern_link_db_view2 = $RSDB_intern_index_php."?page=".$RSDB_SET_sec."&amp;cat=".$RSDB_SET_cat."&amp;cat2=".$RSDB_SET_cat2."&amp;letter=".$RSDB_SET_letter."&amp;group=".$RSDB_SET_group."&amp;item=".$RSDB_SET_item."&amp;vendor=".$RSDB_SET_vendor."";
		
		// Category	
		$RSDB_intern_link_category_all = $RSDB_intern_link_db_sec.$RSDB_SET_sec."&amp;cat=".$RSDB_SET_cat."&amp;cat2=".$RSDB_SET_cat2;
		
		$RSDB_intern_link_category_cat = $RSDB_intern_link_db_sec.$RSDB_SET_sec."&amp;cat2=".$RSDB_SET_cat2."&amp;cat=";
		$RSDB_intern_link_category_cat2 = $RSDB_intern_link_db_sec.$RSDB_SET_sec."&amp;cat=".$RSDB_SET_cat."&amp;cat2=";
		
		
			$RSDB_intern_link_category_EX = $RSDB_intern_link_db_sec."category&amp;cat2=".$RSDB_SET_cat2."&amp;cat=";
			$RSDB_intern_link_cat_EX = $RSDB_intern_link_db_sec."category&amp;cat2=".$RSDB_SET_cat2."&amp;cat=";
			$RSDB_intern_link_cat2_EX = $RSDB_intern_link_db_sec."category&amp;cat=";
		
		
		// Name
		$RSDB_intern_link_name_letter = $RSDB_intern_link_db_sec.$RSDB_SET_sec."&amp;letter=";
		$RSDB_intern_link_name_letter2 = $RSDB_intern_link_db_sec.$RSDB_SET_sec."&amp;letter=".$RSDB_SET_letter;
		$RSDB_intern_link_name_curpos = $RSDB_intern_link_db_sec.$RSDB_SET_sec."&amp;letter=".$RSDB_SET_letter."&amp;curpos=";
		//$RSDB_intern_link_name_cat = $RSDB_intern_link_db_sec.$RSDB_SET_sec."&amp;cat2=".$RSDB_SET_cat2."&amp;cat=";

			$RSDB_intern_link_name_letter_EX = $RSDB_intern_link_db_sec."name&amp;letter=";
			$RSDB_intern_link_name_group_EX = $RSDB_intern_link_db_sec."name&amp;group=";
			$RSDB_intern_link_name_item_EX = $RSDB_intern_link_db_sec."name&amp;item=";
			$RSDB_intern_link_name_EX = $RSDB_intern_link_db_sec."name&amp;letter=";


		// Group
		$RSDB_intern_link_group = $RSDB_intern_link_db_sec.$RSDB_SET_sec."&amp;group=";
		$RSDB_intern_link_group_group2 = $RSDB_intern_link_group.$RSDB_SET_group."&amp;group2=";
		$RSDB_intern_link_group_group2_both = $RSDB_intern_link_group_group2.$RSDB_SET_group2;
		$RSDB_intern_link_group_group2_both_javascript = $RSDB_intern_link_db_sec_javascript.$RSDB_SET_sec."&group=".$RSDB_SET_group."&group2=".$RSDB_SET_group2;
		$RSDB_intern_link_group_sort = $RSDB_intern_link_group.$RSDB_SET_group."&amp;group2=".$RSDB_SET_group2."&amp;sort=";
		$RSDB_intern_link_group_comp = $RSDB_intern_index_php."?page=category&amp;group=";
		$RSDB_intern_link_group_comp_javascript = $RSDB_intern_index_php."?page=category&group=";

		// Item
		$RSDB_intern_link_item = $RSDB_intern_link_db_sec.$RSDB_SET_sec."&amp;item=";
		$RSDB_intern_link_item_javascript = $RSDB_intern_index_php."?page=".$RSDB_SET_sec."&item=";
		$RSDB_intern_link_item_item2 = $RSDB_intern_link_item.$RSDB_SET_item."&amp;item2=";
		$RSDB_intern_link_item_item2_both = $RSDB_intern_link_item_item2.$RSDB_SET_item2;
		$RSDB_intern_link_item_item2_both_javascript = $RSDB_intern_link_db_sec_javascript.$RSDB_SET_sec."&item=".$RSDB_SET_item."&item2=".$RSDB_SET_item2;
	
		$RSDB_intern_link_item_comp = $RSDB_intern_index_php."?page=category&amp;item=";
		
		$RSDB_intern_link_item_item2_vote = $RSDB_intern_link_item_item2.$RSDB_SET_item2."&amp;vote=";
		$RSDB_intern_link_item_item2_forum_bar = $RSDB_intern_link_item_item2.$RSDB_SET_item2;
		$RSDB_intern_link_item_item2_forum_msg = $RSDB_intern_link_item_item2.$RSDB_SET_item2."&amp;threshold=".$RSDB_SET_threshold."&amp;fstyle=".$RSDB_SET_fstyle."&amp;order=".$RSDB_SET_order."&amp;filter=".$RSDB_SET_filter."&amp;msg=";

		// Vendor
		$RSDB_intern_link_vendor = $RSDB_intern_link_db_sec.$RSDB_SET_sec."&amp;vendor=";
		$RSDB_intern_link_vendor_both_javascript = $RSDB_intern_link_db_sec_javascript.$RSDB_SET_sec."&vendor=".$RSDB_SET_vendor;
		$RSDB_intern_link_vendor2_group = $RSDB_intern_link_db_sec."name&amp;group=";
		$RSDB_intern_link_vendor_sec = $RSDB_intern_link_db_sec."vendor&amp;vendor=";
		$RSDB_intern_link_vendor_sec_comp = $RSDB_intern_index_php."?page=vendor&amp;vendor=";

			$RSDB_intern_link_vendor_letter_EX = $RSDB_intern_link_db_sec."vendor&amp;letter=";
			$RSDB_intern_link_vendor_id_EX = $RSDB_intern_link_db_sec."vendor&amp;vendor=";
			$RSDB_intern_link_vendor_EX = $RSDB_intern_link_db_sec."vendor&amp;letter=";

		// Rank
		$RSDB_intern_link_rank = $RSDB_intern_link_db_sec.$RSDB_SET_sec."&amp;rank=";
		$RSDB_intern_link_rank_rank2 = $RSDB_intern_link_rank.$RSDB_SET_rank."&amp;rank2=";
		$RSDB_intern_link_rank2_group = $RSDB_intern_link_db_sec."category&amp;group=";
		$RSDB_intern_link_rank2_item = $RSDB_intern_link_db_sec."category&amp;item=";
		$RSDB_intern_link_rank_sec = $RSDB_intern_link_db_sec."rank&amp;rank=";
		$RSDB_intern_link_rank_sec_comp = $RSDB_intern_index_php."?page=rank&amp;vendor=";
		$RSDB_intern_link_rank_curpos = $RSDB_intern_link_rank_rank2.$RSDB_SET_rank2."&amp;curpos=";
		$RSDB_intern_link_rank_filter = $RSDB_intern_link_rank_curpos.$RSDB_SET_curpos."&amp;filter=";

		// Submit		
			// Compatibility Test Report
			$RSDB_intern_link_submit_comp_test = $RSDB_intern_link_item_item2.$RSDB_SET_item2."&amp;addbox=";
			// Compatibility Screenshot
			$RSDB_intern_link_submit_comp_screenshot = $RSDB_intern_link_item_item2.$RSDB_SET_item2."&amp;addbox=";
			// Forum Post
			$RSDB_intern_link_submit_forum_post = $RSDB_intern_link_item_item2.$RSDB_SET_item2."&amp;addbox=reply&amp;entry=";
			// Application Version
			$RSDB_intern_link_submit_appver = $RSDB_intern_link_item_item2.$RSDB_SET_item2."&amp;group=".$RSDB_SET_group."&amp;addbox=submit";
			$RSDB_intern_link_submit_appver_javascript = $RSDB_intern_link_item_javascript.$RSDB_SET_item."&item2=".$RSDB_SET_item2."&group=".$RSDB_SET_group."&addbox=submit";


		// Language
		$RSDB_intern_link_language = $RSDB_intern_index_php."?page=".$RSDB_SET_page."&amp;sec=".$RSDB_SET_sec."&amp;cat=".$RSDB_SET_cat."&amp;cat2=".$RSDB_SET_cat2."&amp;letter=".$RSDB_SET_letter."&amp;group=".$RSDB_SET_group."&amp;item=".$RSDB_SET_item."&amp;order=".$RSDB_SET_order."&amp;filter=".$RSDB_SET_filter."&amp;vendor=".$RSDB_SET_vendor."&amp;lang=";

		// Export
		$RSDB_intern_link_export = $RSDB_intern_index_php."?page=dat&sec=".$RSDB_SET_sec."&cat=".$RSDB_SET_cat."&cat2=".$RSDB_SET_cat2."&letter=".$RSDB_SET_letter."&group=".$RSDB_SET_group."&item=".$RSDB_SET_item."&order=".$RSDB_SET_order."&filter=".$RSDB_SET_filter."&vendor=".$RSDB_SET_vendor."&lang=".$RSDB_SET_lang."&export=";
		$RSDB_intern_link_export2 = $RSDB_intern_index_php."?page=dat&export=";

		

?>
