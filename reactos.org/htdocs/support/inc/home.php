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
<h1><?php echo $RSDB_langres['RSDB_name']; ?></h1> 
<p><?php echo $RSDB_langres['CONTENT_description']; ?></p>
<h1><?php echo $RSDB_langres['TEXT_overview']; ?></h1> 
<h2><img src="media/pictures/supportdb.jpg" alt="ReactOS Support Database" align="right" height="130" width="290"><?php echo $RSDB_langres['TEXT_about']; ?></h2>

<p><?php echo $RSDB_langres['CONTENT_about_short']; ?> <a href="<?php echo $RSDB_intern_index_php; ?>?page=about"><?php echo $RSDB_langres['TEXT_more']; ?></a> </p>
<h2><?php echo $RSDB_langres['TEXT_sections']; ?></h2>
<h3><?php echo $RSDB_langres['TEXT_compdb']; ?></h3>
<p>
	<?php echo $RSDB_langres['CONTENT_compdb_description']; ?> <a href="<?php echo $RSDB_intern_link_db_view_comp; ?>&amp;sec=home"><?php echo $RSDB_langres['TEXT_more']; ?></a>
</p>

<h3><?php echo $RSDB_langres['TEXT_packdb']; ?></h3>
<p>
	<?php echo $RSDB_langres['CONTENT_packdb_description']; ?> <a href="<?php echo $RSDB_intern_link_db_view_pack; ?>&amp;sec=home"><?php echo $RSDB_langres['TEXT_more']; ?></a>
</p>

<h3><?php echo $RSDB_langres['TEXT_devnet']; ?></h3>
<p> <?php echo $RSDB_langres['CONTENT_devnet_description']; ?> 
  <a href="<?php echo $RSDB_intern_link_db_view_devnet; ?>&amp;sec=home"><?php echo $RSDB_langres['TEXT_more']; ?></a> 
</p>
<h3><?php echo $RSDB_langres['TEXT_mediadb']; ?></h3>
<p><?php echo $RSDB_langres['CONTENT_mediadb_description']; ?> <a href="<?php echo $RSDB_intern_link_db_view_media; ?>&amp;sec=home"><?php echo $RSDB_langres['TEXT_more']; ?></a> 
</p>
<p>&nbsp;</p>

