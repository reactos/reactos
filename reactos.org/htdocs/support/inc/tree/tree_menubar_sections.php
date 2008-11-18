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

			if ($RSDB_SET_cat != "" && $RSDB_SET_item =="" && $RSDB_SET_group =="") {
				if ($RSDB_SET_cat == 0) {
					$result_count_group['cat_comp']=1;
					$result_count_group['cat_pack']=1;
					$result_count_group['cat_devnet']=1;
					$result_count_group['cat_media']=1;
				}
				else {
					$query_count_group=mysql_query("SELECT *
													FROM `rsdb_categories` 
													WHERE `cat_id` = ". $RSDB_SET_cat ."
													AND `cat_visible` = '1' ;");	
					$result_count_group = mysql_fetch_array($query_count_group);
				}
				$RSDB_compare_string = 'cat';
			}
			else {
				if ($RSDB_SET_item != "") {
					$result_count_group['item_comp']=0;
					$result_count_group['item_pack']=0;
					$result_count_group['item_devnet']=0;
					$result_count_group['item_media']=0;
						
					if ($RSDB_SET_view == "comp")  {
						$result_count_group['item_comp']=1;
					}
					if ($RSDB_SET_view == "pack")  { 
						$result_count_group['item_pack']=1;
					}
					if ($RSDB_SET_view == "devnet")  { 
						$result_count_group['item_devnet']=1;
					}
					if ($RSDB_SET_view == "media")  { 
						$result_count_group['item_media']=1;
					}
					$RSDB_compare_string = 'item';
				}
				elseif ($RSDB_SET_group != "") {
					$query_count_group=mysql_query("SELECT *
													FROM `rsdb_groups` 
													WHERE `grpentr_id` = ". $RSDB_SET_group ."
													AND `grpentr_visible` = '1' ;");	
					$result_count_group = mysql_fetch_array($query_count_group);
					$RSDB_compare_string = 'grpentr';
				}
			}
			if ($result_count_group[$RSDB_compare_string."_comp"] == "1") {
				echo '<a href="'. $RSDB_intern_link_db_view2 .'comp">'. $RSDB_langres['TEXT_compdb_short'] .'</a>'; 
			}
			else {
				echo '<font color="#cccccc">&nbsp;'. $RSDB_langres['TEXT_compdb_short'] .'&nbsp;</font>'; 
			}
			echo ' <font color="#ffffff">|</font> ';
			if ($result_count_group[$RSDB_compare_string."_pack"] == "1") {
				echo '<a href="'. $RSDB_intern_link_db_view2 .'pack">'. $RSDB_langres['TEXT_packdb_short'] .'</a>';
			}
			else {
				echo '<font color="#cccccc">&nbsp;'. $RSDB_langres['TEXT_packdb_short'] .'&nbsp;</font>'; 
			}
			echo ' <font color="#ffffff">|</font> ';
			if ($result_count_group[$RSDB_compare_string."_devnet"] == "1") {
				echo '<a href="'. $RSDB_intern_link_db_view2 .'devnet">'. $RSDB_langres['TEXT_devnet_short'] .'</a>';
			}
			else {
				echo '<font color="#cccccc">&nbsp;'. $RSDB_langres['TEXT_devnet_short'] .'&nbsp;</font>'; 
			}
			echo ' <font color="#ffffff">|</font> ';
			if ($result_count_group[$RSDB_compare_string."_media"] == "1") {
				echo '<a href="'. $RSDB_intern_link_db_view2 .'media">'. $RSDB_langres['TEXT_mediadb_short'] .'</a>'; 
			}
			else {
				echo '<font color="#cccccc">&nbsp;'. $RSDB_langres['TEXT_mediadb_short'] .'&nbsp;</font>'; 
			}
			
			
			?></div></td>
			</tr>
		</table>
	</td>
  </tr>
</table>