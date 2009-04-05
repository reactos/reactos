<?php
    /*
    CompatDB - ReactOS Compatability Database
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


/**
 * class Category
 * 
 */
class Category
{



  /**
   * @FILLME
   *
   * @access public
   */
  public static 	function showTree($RSDB_TEMP_cat_path, $RSDB_TEMP_cat_id, $RSDB_TEMP_cat_level, $RSDB_TEMP_cat_level_newmain, $option) {
		global $RSDB_intern_link_category_cat;
		global $RSDB_TEMP_sortby;

		$stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_categories WHERE cat_path = :cat_path AND cat_visible = '1' AND cat_comp = '1' ORDER BY ".$RSDB_TEMP_sortby." ASC");
    $stmt->bindParam('cat_path',$RSDB_TEMP_cat_id,PDO::PARAM_STR);
    $stmt->execute();
					
		while($result_create_historybar=$stmt->fetch(PDO::FETCH_ASSOC)) { 
      if ($option) {
				self::showLeafAsOption($result_create_historybar['cat_id'], $RSDB_TEMP_cat_level_newmain);
      }
      else {
				self::showLeafAsRow($result_create_historybar['cat_id'], $RSDB_TEMP_cat_level_newmain);
      }
				self::showTree($result_create_historybar['cat_path'], $result_create_historybar['cat_id'], $RSDB_TEMP_cat_level, $RSDB_TEMP_cat_level_newmain,$option);
		}
	} // end of member function showTree



  /**
   * @FILLME
   *
   * @access public
   */
  public static 	function showLeafAsOption($RSDB_TEMP_entry_id, $RSDB_TEMP_cat_level_newmain) {
	global $RSDB_intern_selected;
		
		global $RSDB_intern_link_category_cat;
		global $cellcolor2;
		$cellcolor=$cellcolor2;
		

		
    $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_categories WHERE cat_id = :cat_id AND cat_visible = '1' AND cat_comp = '1'");
    $stmt->bindParam('cat_id',$RSDB_TEMP_entry_id,PDO::PARAM_STR);
    $stmt->execute();
					
		$result_create_tree_entry=$stmt->fetchOnce(PDO::FETCH_ASSOC);

		
		
		
		$RSDB_TEMP_cat_current_id_guess = $result_create_tree_entry['cat_id'];

		for ($guesslevel=1; ; $guesslevel++) {
      $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_categories WHERE cat_id = :cat_id AND cat_visible = '1' AND cat_comp = '1'");
      $stmt->bindParam('cat_id',$RSDB_TEMP_cat_current_id_guess,PDO::PARAM_STR);
      $stmt->execute();
				$result_category_tree_guesslevel=$stmt->fetchOnce(PDO::FETCH_ASSOC);
				$RSDB_TEMP_cat_current_id_guess = $result_category_tree_guesslevel['cat_path'];
				
				if (!$result_category_tree_guesslevel['cat_name']) {
					$RSDB_intern_catlevel = ($guesslevel-1);
					break;
				}
		}

		echo "<option value=\"". $result_create_tree_entry['cat_id']. "\"";
		if ($RSDB_intern_selected != "" && $RSDB_intern_selected == $result_create_tree_entry['cat_id']) {
			echo " selected "; 
		}		
		echo ">\n\n";

		for ($n=$RSDB_TEMP_cat_level_newmain;$n<$RSDB_intern_catlevel;$n++) {
			echo "&nbsp;&nbsp;&nbsp;&nbsp;";
		}

		echo $result_create_tree_entry['cat_name'];

		//echo "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(".$result_create_tree_entry['cat_description'] .")";
		
		echo "</option>";
	} // end of member function showLeafAsOption



  /**
   * @FILLME
   *
   * @access public
   */
  public static 	function showLeafAsRow($RSDB_TEMP_entry_id, $RSDB_TEMP_cat_level_newmain) {
		
		global $RSDB_intern_link_category_cat;
		global $cellcolor2;
		$cellcolor=$cellcolor2;
		
//		global $RSDB_TEMP_cat_icon;

		
    $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_categories WHERE cat_id = :cat_id AND cat_visible = '1' AND cat_comp = '1'");
    $stmt->bindParam('cat_id',$RSDB_TEMP_entry_id,PDO::PARAM_STR);
    $stmt->execute();
					
		$result_create_tree_entry=$stmt->fetch(PDO::FETCH_ASSOC);

/*		if ($result_create_tree_entry['cat_icon'] != "") {
			$RSDB_TEMP_cat_icon = $result_create_tree_entry['cat_icon'];
		}
*/
		
		echo "<tr><td width='45%' valign='top' bgcolor='".$cellcolor."'>"; 
		echo "<div align='left'><font size='2' face='Arial, Helvetica, sans-serif'>&nbsp;";
		
		
		$RSDB_TEMP_cat_current_id_guess = $result_create_tree_entry['cat_id'];

		// count the levels -> current category level
		for ($guesslevel=1; ; $guesslevel++) {
//				echo $guesslevel."#";
        $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_categories WHERE cat_id = :cat_id AND cat_visible = '1' AND cat_comp = '1'");
        $stmt->bindParam('cat_id',$RSDB_TEMP_cat_current_id_guess,PDO::PARAM_STR);
        $stmt->execute();
				$result_category_tree_guesslevel=$stmt->fetch(PDO::FETCH_ASSOC);
//					echo $result_category_tree_guesslevel['cat_name'];
				$RSDB_TEMP_cat_current_id_guess = $result_category_tree_guesslevel['cat_path'];
				
				if (!$result_category_tree_guesslevel['cat_name']) {
//					echo "ENDE:".($guesslevel-1);
					$RSDB_intern_catlevel = ($guesslevel-1);
					break;
				}
		}



		for ($n=$RSDB_TEMP_cat_level_newmain;$n<$RSDB_intern_catlevel;$n++) {
			echo "&nbsp;&nbsp;&nbsp;&nbsp;";
		}

//		echo "<img src='media/icons/categories/".$RSDB_TEMP_cat_icon."' width='16' height='16'> ";

		echo "<a href='".$RSDB_intern_link_category_cat.$result_create_tree_entry['cat_id']."'>".$result_create_tree_entry['cat_name']."</a>";

		echo "</font></div></td>";
		echo "<td width='45%' valign='top' bgcolor='".$cellcolor."'>";
		echo "<div align='left'><font face='Arial, Helvetica, sans-serif'>";
		
		echo "<font size='2' face='Arial, Helvetica, sans-serif'>".$result_create_tree_entry['cat_description']."</font>";
		
		echo "</font></div></td><td width='10%' valign='top' bgcolor='".$cellcolor."'><font size='2'>".count_tree_grouplist($result_create_tree_entry['cat_id'])."</font></td></tr>";
		
	} // end of member function showLeafAsOption



} // end of Category
?>
