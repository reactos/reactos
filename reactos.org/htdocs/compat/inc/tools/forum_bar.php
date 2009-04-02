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



function forum_bar() { // show the forum bar
	global $RSDB_intern_link_item_item2_forum_bar;
	global $RSDB_intern_link_item_item2_forum_msg;
	global $RSDB_SET_threshold;
	global $RSDB_SET_fstyle;
	global $RSDB_SET_order;
	global $RSDB_SET_filter;
	
?>

	<form action="<?php echo $RSDB_intern_link_item_item2_forum_bar; ?>" method="post" name="forum_bar">
	<table border="0" align="center" cellpadding="0" cellspacing="0" class="forumbar">
	  <tr>
	    <td><div align="center">
	
	<nobr>
	
	Threshold: 
		<select name="threshold" size="1">
			<option value="1" <?php if ($RSDB_SET_threshold == "1") { echo 'selected="selected"'; } ?>>1 star (<?php echo Star::thresholdForum(0, 1, true, "forum", "fmsg"); ?>)</option>
			<option value="2" <?php if ($RSDB_SET_threshold == "2") { echo 'selected="selected"'; } ?>>2 stars (<?php echo Star::thresholdForum(0, 2, true, "forum", "fmsg"); ?>)</option>
			<option value="3" <?php if ($RSDB_SET_threshold == "3") { echo 'selected="selected"'; } ?>>3 stars (<?php echo Star::thresholdForum(0, 3, true, "forum", "fmsg"); ?>)</option>
			<option value="4" <?php if ($RSDB_SET_threshold == "4") { echo 'selected="selected"'; } ?>>4 stars (<?php echo Star::thresholdForum(0, 4, true, "forum", "fmsg"); ?>)</option>
			<option value="5" <?php if ($RSDB_SET_threshold == "5") { echo 'selected="selected"'; } ?>>5 stars (<?php echo Star::thresholdForum(0, 5, true, "forum", "fmsg"); ?>)</option>
		</select>
	 | </nobr><nobr>Style: 
	<select name="fstyle" size="1">
		<option value="bboard" <?php if ($RSDB_SET_fstyle == "bboard") { echo 'selected="selected"'; } ?>>Bulletin Board</option>
		<option value="flat" <?php if ($RSDB_SET_fstyle == "flat") { echo 'selected="selected"'; } ?>>Flat</option>
		<option value="nested" <?php if ($RSDB_SET_fstyle == "nested") { echo 'selected="selected"'; } ?>>Nested</option>
		<option value="threaded" <?php if ($RSDB_SET_fstyle == "threaded") { echo 'selected="selected"'; } ?>>Threaded</option>
		<option value="fthreads" <?php if ($RSDB_SET_fstyle == "fthreads") { echo 'selected="selected"'; } ?>>Full Threads</option>
	</select>
	 <select name="order" size="1">
	<option value="new" <?php if ($RSDB_SET_order == "new") { echo 'selected="selected"'; } ?>>Newest First</option>
	<option value="old" <?php if ($RSDB_SET_order == "old") { echo 'selected="selected"'; } ?>>Oldest First</option>
	</select>
<?php
	/* 
		 | </nobr>
		 <nobr>Filter: 
		 <select name="filter" size="1">
		   <option value="cur" <?php if ($RSDB_SET_filter == "cur") { echo 'selected="selected"'; } ?>>current version</option>
		   <option value="all" <?php if ($RSDB_SET_filter == "all") { echo 'selected="selected"'; } ?>>all version</option>
		 </select>
	*/
?>
	 | </nobr>
	 <nobr>
	 Save: 
	 <input name="forumsave" value="1" type="checkbox"> | </nobr><nobr>
	   <input value="Change" class="button" type="submit"> 
	</nobr></div></td>
	</tr>
	</table>
	</form>
<?php
}

?>