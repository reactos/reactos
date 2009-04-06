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


  $stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_item_vendor WHERE vendor_visible = '1' AND vendor_name LIKE :search");
  $stmt->bindValue('search','%'.@$_GET['search'].'%',PDO::PARAM_STR);
  $stmt->execute();
	$result_count_groups = $stmt->fetchOnce(PDO::FETCH_NUM);

header( 'Content-type: text/xml' );
echo '<?xml version="1.0" encoding="UTF-8"?>
<root>
';

	if (!$result_count_groups[0]) {
		echo "    #none#\n";
	}
	else {
		echo "    ".$result_count_groups[0]."\n";
	}
	
if (isset($_GET['search']) && strlen($_GET['search']) > 1) {

  $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_vendor WHERE vendor_visible = '1' AND vendor_name LIKE :search ORDER BY vendor_name ASC");
  $stmt->bindValue('search','%'.$_GET['search'].'%',PDO::PARAM_STR);
  $stmt->execute();
	
	while($result_page = $stmt->fetch(PDO::FETCH_ASSOC)) { // Pages
?>
	<dbentry>
		<vendor id="<?php echo $result_page['vendor_id']; ?>"><?php echo $result_page['vendor_name']; ?></vendor>
		<url><?php
				if (!strlen($result_page['vendor_url']) == "") {
					echo htmlentities($result_page['vendor_url'], ENT_QUOTES);
				}
				else {
					echo ".";
				}
		  ?></url>
	</dbentry>
<?
	}
}
?>
</root>