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



function test_bar() { // show the forum bar
	global $RSDB_intern_link_item_item2_forum_bar;
	global $RSDB_intern_link_item_item2_forum_msg;
	global $RSDB_SET_threshold;
	global $RSDB_SET_fstyle;
	global $RSDB_SET_order;
	global $RSDB_SET_filter;
?>
<br />
	<form action="<?php echo $RSDB_intern_link_item_item2_forum_bar; ?>" method="post" name="forum_bar">
	
	<table border="0" align="center" cellpadding="0" cellspacing="0" class="forumbar" width="500">
	  <tr>
	    <td><div align="center">	<nobr>
	
	Threshold: 
		<select name="threshold" size="1">
			<option value="1" <?php if ($RSDB_SET_threshold == "1") { echo 'selected="selected"'; } ?>>1 star (<?php echo calc_threshold_stars_test(0, 1, true, "testresults", "test"); ?>)</option>
			<option value="2" <?php if ($RSDB_SET_threshold == "2") { echo 'selected="selected"'; } ?>>2 stars (<?php echo calc_threshold_stars_test(0, 2, true, "testresults", "test"); ?>)</option>
			<option value="3" <?php if ($RSDB_SET_threshold == "3") { echo 'selected="selected"'; } ?>>3 stars (<?php echo calc_threshold_stars_test(0, 3, true, "testresults", "test"); ?>)</option>
			<option value="4" <?php if ($RSDB_SET_threshold == "4") { echo 'selected="selected"'; } ?>>4 stars (<?php echo calc_threshold_stars_test(0, 4, true, "testresults", "test"); ?>)</option>
			<option value="5" <?php if ($RSDB_SET_threshold == "5") { echo 'selected="selected"'; } ?>>5 stars (<?php echo calc_threshold_stars_test(0, 5, true, "testresults", "test"); ?>)</option>
		</select>
	 | </nobr><nobr>
	 <select name="order" size="1">
	<option value="new" <?php if ($RSDB_SET_order == "new") { echo 'selected="selected"'; } ?>>Newest First</option>
	<option value="old" <?php if ($RSDB_SET_order == "old") { echo 'selected="selected"'; } ?>>Oldest First</option>
	</select> 
	 | </nobr><nobr>Save: 
	 <input name="testsave" value="1" type="checkbox"> | </nobr><nobr>
	   <input value="Change" class="button" type="submit"> 
	</nobr></div></td>
	</tr>
	</table>
	
	</form>
<?php
}

?>