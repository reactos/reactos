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



if ($RSDB_SET_letter == "all") {
	$RSDB_SET_letter = "%";
}

$query_count_cat=mysql_query("SELECT COUNT('cat_id')
									FROM `rsdb_item_vendor` 
									WHERE `vendor_name` LIKE  '" . $RSDB_SET_letter . "%' 
									AND `vendor_visible` = '1' ;");	
$result_count_cat = mysql_fetch_row($query_count_cat);
if ($result_count_cat[0]) {

	echo "<p align='center'>";
	$j=0;
	for ($i=0; $i < $result_count_cat[0]; $i += $RSDB_intern_items_per_page) {
		$j++;
		if ($RSDB_SET_curpos == $i) {
			echo "<b>".$j."</b> ";
		}
		else {
			echo "<a href='".$RSDB_intern_link_name_curpos.$i."'>".$j."</a> ";
		}
	}
	$j=0;
	echo "</p>";

?> 
<table width="100%" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3"> 
    <td width="15%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif" ><strong>Vendor</strong></font></div></td>
    <td width="30%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif" ><strong>Fullname</strong></font></div></td>
    <td width="27%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif" ><strong>Website</strong></font></div></td>
    <td width="18%">
      <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Information</strong></font></div></td>
  </tr>
  <?php
	
		$query_page = mysql_query("SELECT * 
									FROM `rsdb_item_vendor` 
									WHERE `vendor_name` LIKE  '" . mysql_real_escape_string($RSDB_SET_letter) . "%' 
									AND `vendor_visible` = '1' 
									ORDER BY `vendor_name` ASC
									LIMIT " . mysql_real_escape_string($RSDB_SET_curpos) . " , " . mysql_real_escape_string($RSDB_intern_items_per_page) . " ;") ;
	
		$farbe1="#E2E2E2";
		$farbe2="#EEEEEE";
		$zaehler="0";
		//$farbe="#CCCCC";
		
		while($result_page = mysql_fetch_array($query_page)) { // Pages
	?>
  <tr> 
    <td valign="top" bgcolor="<?php
									$zaehler++;
									if ($zaehler == "1") {
										echo $farbe1;
										$farbe = $farbe1;
									}
									elseif ($zaehler == "2") {
										$zaehler="0";
										echo $farbe2;
										$farbe = $farbe2;
									}
								 ?>"> <div align="left"><font face="Arial, Helvetica, sans-serif">&nbsp;<b><a href="<?php echo $RSDB_intern_link_vendor_id_EX.$result_page['vendor_id'].$RSDB_URI_slash; ?>"><?php echo $result_page['vendor_name']; ?></a></b></font> 
        </div></td>
    <td valign="top" bgcolor="<?php echo $farbe; ?>"><font size="2"><?php echo $result_page['vendor_fullname']; ?></font></td>
    <td valign="top" bgcolor="<?php echo $farbe; ?>"><div align="left"><font size="2"><a href="<?php echo $result_page['vendor_url']; ?>" onmousedown="return clk(this.href,'res','')"><?php echo $result_page['vendor_url']; ?></a></font></div></td>
    <td valign="top" bgcolor="<?php echo $farbe; ?>"><div align="left"><font size="2"><?php echo $result_page['vendor_infotext']; ?>
    </font></div></td>
  </tr>
  <?php	
		}	// end while
	?>
</table>
<p align="center"><b><?php

	echo ($RSDB_SET_curpos+1)." to ";

	if (($RSDB_SET_curpos + $RSDB_intern_items_per_page) > $result_count_cat[0]) {
		echo $result_count_cat[0];
	}
	else {
		echo ($RSDB_SET_curpos + $RSDB_intern_items_per_page);
	}
		
	echo " of ".$result_count_cat[0]; 
	
?></b></p>
<?php
}
include("inc/tree/tree_vendor_flat_maintainer.php");
?>

