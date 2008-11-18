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

?><br>
Packages</h1> 
<p>ReactOS Package Database, the official online database for the ReactOS Package Manager</p>
<?php msg_bar("The Package Section is under heavy construction!"); ?>
<br />
<h1>Overview</h1>
<p><img src="media/pictures/packages.jpg" alt="ReactOS Package Database" align="right" height="300" width="400">This 
  is the ReactOS Package Database (PackDB), a subsection of the ReactOS Support 
  Database (RSDB). From here you get info on application and driver compatibility 
  with ReactOS.</p>
<p>Some of the features of the ReactOS Package Database require that you have 
  installed the <a href="#">ReactOS Package Manager</a> and a <a href="<?php echo $RSDB_intern_loginsystem_fullpath; ?>?page=register">myReactOS 
  account</a> and are <a href="<?php echo $RSDB_intern_loginsystem_fullpath; ?>?page=login">logged 
  in</a>.</p>
<p>&nbsp;</p>
<p>Some of the features of the Package Database are:</p>
<p> </p>
<ul>
  <li>[...]</li>
</ul>
<p>&nbsp;</p>
<p>There are <b> 
  <?php

	$query_count_cat=mysql_query("SELECT COUNT('cat_id')
							FROM `rsdb_groups`
							WHERE `grpentr_visible` = '1' 
							AND `grpentr_pack` = '1' ;");	
	$result_count_cat = mysql_fetch_row($query_count_cat);
	echo $result_count_cat[0];


?>
  </b> packages currently in the database.</p>
<p>&nbsp;</p>
<h2>ReactOS Package Manager</h2>
<p>The ReactOS Package Manager [...]</p>
