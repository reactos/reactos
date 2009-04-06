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
<table width="100%"  border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td width="100%">&nbsp;</td>
    <td align="right" valign="top">
		<table width="380"  border="0" cellpadding="0" cellspacing="0" bordercolor="#5984C3">
		  <tr>
			<td width="18" height="18"><img src="images/corner_sections.jpg" width="18" height="18"></td>
			<td bgcolor="#5984C3"><div id="sectionMenu" align="center"><?php

			$RSDB_compare_string = '';

			if (isset($_GET['cat']) && $_GET['cat'] != '' && isset($_GET['item']) && $_GET['item'] =="" && $RSDB_SET_group =="") {
				if ($_GET['cat'] == 0) {
					$result_count_group['cat_comp']=1;
				}
				else {
          $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_categories WHERE cat_id = :cat_id AND cat_visible = '1'");
          $stmt->bindParam('cat_id',@htmlspecialchars(@$_GET['cat']),PDO::PARAM_STR);
          $stmt->execute();
					$result_count_group = $stmt->fetch(PDO::FETCH_ASSOC);
				}
				$RSDB_compare_string = 'cat';
			}
			else {
				if (isset($_GET['item']) && $_GET['item'] != "") {
						$result_count_group['item_comp']=1;
					$RSDB_compare_string = 'item';
				}
				elseif ($RSDB_SET_group != "") {
          $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_groups WHERE grpentr_id = :group_id AND grpentr_visible = '1'");
          $stmt->bindParam('group_id',$RSDB_SET_group,PDO::PARAM_STR);
          $stmt->execute();
					$result_count_group = $stmt->fetch(PDO::FETCH_ASSOC);
					$RSDB_compare_string = 'grpentr';
				}
			}
			if ($result_count_group[$RSDB_compare_string."_comp"] == "1") {
				echo '<a href="'. $RSDB_intern_link_db_view2 .'">'. $RSDB_langres['TEXT_compdb_short'] .'</a>'; 
			}
			else {
				echo '<font color="#cccccc">&nbsp;'. $RSDB_langres['TEXT_compdb_short'] .'&nbsp;</font>'; 
			}
			
			
			?></div></td>
			</tr>
		</table>
	</td>
  </tr>
</table>
