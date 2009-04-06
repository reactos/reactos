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

?>
<a href="<?php echo $RSDB_intern_link_db_sec; ?>home"><?php echo $RSDB_langres['TEXT_compdb_short']; ?></a> 
&gt; <a href="<?php echo $RSDB_intern_link_category_cat; ?>0">Browse Database</a> &gt; By Category<a href="<?php echo $RSDB_intern_index_php; ?>?page=about"><img src="media/pictures/compatibility_small.jpg" vspace="1" border="0" align="right"></a>
</h1> 
<p><?php echo $RSDB_langres['CONTENT_compdb_description']; ?></p>
<?php
	
	//echo "<h1>Browse by Category</h1>";
	
	include("inc/tree/tree_historybar.php");	

	if (isset($_GET['item']) && $_GET['item'] != "") {
		include("inc/tree/tree_item.php");
	}
	else {
		if ($RSDB_SET_group != "") {
			if (isset($_GET['addbox']) && $_GET['addbox'] == 'submit') {
				include("inc/comp/comp_itemver_submit.php");
			}
			else {
				include("inc/comp/comp_group.php");
			}
		}
		else {
			switch (@$_GET['cat2']) {
				case "flat": // Flat Style
				default:
					include("inc/tree/tree_category_flat.php");
					break;
				case "tree": // Tree Style
					include("inc/tree/tree_category_tree.php");
					break;
			}
			include("inc/tree/tree_category_grouplist_1.php");
		}
	}
	
		
?>
