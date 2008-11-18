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
<h1><a href="<?php echo $ROST_intern_path_server; ?>?page=dev">Development</a> &gt; <a href="<?php echo $ROST_intern_link_section; ?>home">Translate <?php echo $ROST_intern_projectname; ?></a> &gt; Admin</h1> 
<h2>Admin Interface </h2> 
<p>Welcome to the ReactOS Translation Service administrator interface.</p>
<p>Features:</p>
<ul>
  <li><b><a href="<?php echo $ROST_intern_link_section; ?>import">Import XML files</a></b><a href="<?php echo $ROST_intern_link_section; ?>import"></a></li>
</ul>
<ul>
  <li><b><a href="<?php echo $ROST_intern_link_section; ?>export">Export RC files</a></b><a href="<?php echo $ROST_intern_link_section; ?>export"></a></li>
</ul>
<p>&nbsp;</p>
