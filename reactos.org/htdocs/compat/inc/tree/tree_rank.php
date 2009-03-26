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
<a href="<?php echo $RSDB_intern_link_db_sec; ?>home"><?php echo $RSDB_intern_code_view_name; ?></a> 
&gt; <a href="<?php echo $RSDB_intern_link_name_letter; ?>all">Browse Database</a> &gt; By Rank
<a href="<?php echo $RSDB_intern_index_php; ?>?page=about"><img src="media/pictures/compatibility_small.jpg" vspace="1" border="0" align="right"></a>
</h1> 
<p><?php echo $RSDB_intern_code_view_description; ?></p>
<?php

	echo "<h1>Browse by Rank</h1>";
	
	echo "<br />";
	
	include("inc/comp/comp_rank.php");

?>
