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

  if (isset($_GET['search'])) {
    $search_phrase = $_GET['search'];
  }
  else {
    $search_phrase = '';
  }

  $stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_groups JOIN rsdb_item_vendor ON grpentr_vendor = vendor_id WHERE (grpentr_name LIKE :search OR vendor_name LIKE :search) AND grpentr_comp = '1' AND vendor_visible = '1' AND grpentr_visible = '1'");
  $stmt->bindValue('search','%'.$search_phrase.'%',PDO::PARAM_STR);
  $stmt->execute();
	$result_count_groups = $stmt->fetchOnce(PDO::FETCH_NUM);

	if (!$result_count_groups[0]) {
		// Remove temp characters and try again
		$search_phrase = str_replace("-","",$search_phrase);
		$search_phrase = str_replace("_","",$search_phrase);
		$search_phrase = str_replace(" ","",$search_phrase);
    $stmt->bindValue('search','%'.$search_phrase.'%',PDO::PARAM_STR);
    $stmt->execute();
		$result_count_groups = $stmt->fetchOnce(PDO::FETCH_NUM);
	}

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
	
if ($search_phrase != "" || strlen($search_phrase) > 1) {

  $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_groups JOIN rsdb_item_vendor ON grpentr_vendor = vendor_id WHERE (grpentr_name LIKE :search OR `vendor_name` LIKE :search) AND `grpentr_comp` = '1' AND `vendor_visible` = '1' AND `grpentr_visible` = '1' ORDER BY `grpentr_name` ASC LIMIT 20") ; // Limit to 20 entry
  $stmt->bindValue('search',$search_phrase,PDO::PARAM_STR);
  $stmt->execute();
	
	while($result_page = $stmt->fetch(PDO::FETCH_ASSOC)) {
?>
	<dbentry>
		<item id="<?php echo $result_page['grpentr_id']; ?>"><?php echo $result_page['grpentr_name']; ?></item>
		<desc><?php
				if (!strlen($result_page['grpentr_description']) == "0") {
					if (strlen(htmlentities($result_page['grpentr_description'], ENT_QUOTES)) <= 30) {
						echo $result_page['grpentr_description'];
					}
					else {
						echo substr(htmlentities($result_page['grpentr_description'], ENT_QUOTES), 0, 30)."...";
					}
				}
				else {
					echo ".";
				}
		  ?></desc>
		<vendor id="<?php echo $result_page['grpentr_vendor']; ?>"><?php
				if ($result_page['grpentr_vendor'] == "0") {
					echo ".";
				}
				else {
          $stmt_vendor=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_vendor WHERE vendor_id = :vendor_id");
          $stmt_vendor->bindParam('vendor_id',$result_page['grpentr_vendor'],PDO::PARAM_STR);
          $stmt_vendor->execute();
					$result_entry_vendor = $stmt_vendor->fetchOnce(PDO::FETCH_ASSOC);
					echo $result_entry_vendor['vendor_name'];
				}
		  ?> </vendor>
		<comp><?php echo $result_page['grpentr_comp']; ?></comp>
		<devnet><?php echo $result_page['grpentr_devnet']; ?></devnet>
	</dbentry>
<?
	}
}
?>
</root>