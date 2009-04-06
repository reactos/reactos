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


	


	$RSDB_viewpage = true;

	if (isset($_GET['page']) && $_GET['page'] == "category") {
?>
<table width="100%" border="1" cellpadding="3" cellspacing="0" bordercolor="#5984C3">
  <tr>
    <td>&nbsp;<font size="2">Browsing: </font><a href="<?php echo $RSDB_intern_link_category_cat."0";

		if (empty($_GET['cat2']) || $_GET['cat2'] == 'flat') {

		}
		else {
			echo "&amp;cat2=".htmlspecialchars(@$_GET['cat2']);
		}
		
	?>">Main</a><?php
		$RSDB_TEMP_current_category = "Main";
		$RSDB_TEMP_current_category_desc = "Root directory of the tree.";
	
		if (!empty($_GET['cat'])) {

			if (isset($_GET['item']) && $_GET['item'] != "" && $RSDB_viewpage != false) {
				$stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_visible = '1' AND comp_id = :item_id ORDER BY comp_name ASC");
        $stmt->bindParam('item_id',$_GET['item'],PDO::PARAM_STR);
        $stmt->execute();
				$result_itempid = $stmt->fetch(PDO::FETCH_ASSOC);
				if ($result_itempid['comp_groupid'] == "" || $result_itempid['comp_groupid'] == "0") {
					//die("");
					//echo "die1";
					$RSDB_viewpage = false;
				}
				$RSDB_SET_group = $result_itempid['comp_groupid'];
			}
			if ($RSDB_SET_group != "" && $RSDB_viewpage != false) {
				//echo "+++++".$RSDB_SET_group;
        $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_groups WHERE grpentr_visible = '1' AND grpentr_id = :group_id AND grpentr_comp = '1' ORDER BY grpentr_name ASC") ;
        $stmt->bindParam('group_id',$RSDB_SET_group,PDO::PARAM_STR);
        $stmt->execute();
				$result_groupid = $stmt->fetch(PDO::FETCH_ASSOC);
				if ($result_groupid['grpentr_category'] == "" || $result_groupid['grpentr_category'] == "0") {
					//die("");
					//echo "die2";
					$RSDB_viewpage = false;
				}
				if ($RSDB_viewpage != false) {
          $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_categories WHERE cat_id = :cat_id AND cat_visible = '1' AND cat_comp = '1'");
          $stmt->bindParam('cat_id',$result_groupid['grpentr_category'],PDO::PARAM_STR);
          $stmt->execute();
					$result_category_treehistory_groupid=$stmt->fetch(PDO::FETCH_ASSOC);
					
					$RSDB_TEMP_cat_path = $result_category_treehistory_groupid['cat_path'];
					$RSDB_TEMP_cat_id = $result_category_treehistory_groupid['cat_id'];
					//$RSDB_TEMP_cat_current_level = $result_category_treehistory_groupid['cat_level'];
	
					$RSDB_TEMP_cat_current_level=1;
					$RSDB_TEMP_cat_current_id = $RSDB_TEMP_cat_id;
					//$RSDB_TEMP_cat_current_counter = $result_category_treehistory_groupid['cat_level'];
					//$RSDB_TEMP_cat_treehist = $result_category_treehistory_groupid['cat_level'];
				}
			}
			elseif ($RSDB_viewpage != false) {
				//echo "hjall";
        $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_categories WHERE cat_id = :cat_id AND cat_visible = '1' AND cat_comp = '1'");
        $stmt->bindParam('cat_id',@$_GET['cat'],PDO::PARAM_STR);
        $stmt->execute();
				$result_category_treehistory=$stmt->fetch(PDO::FETCH_ASSOC);
				
				$RSDB_TEMP_cat_path = $result_category_treehistory['cat_path'];
				$RSDB_TEMP_cat_id = $result_category_treehistory['cat_id'];

				$RSDB_TEMP_cat_current_level=1;
				$RSDB_TEMP_cat_current_id = $RSDB_TEMP_cat_id;
			}
			if ($RSDB_viewpage != false) {
	//			echo "<p>";
				$RSDB_TEMP_cat_current_id_guess = $RSDB_TEMP_cat_current_id;
				$RSDB_intern_catlevel=0;
				
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
	//						echo "ENDE:".($guesslevel-1);
							$RSDB_intern_catlevel = ($guesslevel-1);
							$RSDB_TEMP_cat_current_counter = $RSDB_intern_catlevel;
							break;
						}
				}
	//			echo "<p>";
				
				
				for ($i=0; $i < $RSDB_intern_catlevel; $i++) {			
	//				echo "<br>Ring0: ".$i." ";
					for ($k=1; $k < ($RSDB_intern_catlevel+1-$i); $k++) {
	//					echo $k."|";
              $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_categories WHERE cat_id = :cat_id AND `cat_visible` = '1' AND cat_comp = '1'");
              $stmt->bindParam('cat_id',$RSDB_TEMP_cat_current_id,PDO::PARAM_STR);
              $stmt->execute();
							$result_category_tree_temp=$stmt->fetch(PDO::FETCH_ASSOC);
							$RSDB_TEMP_cat_current_id = $result_category_tree_temp['cat_path'];
							
	//						echo "K:".$k."|E:".($result_category_treehistory['cat_level']+1-$i);
							if ($k == $RSDB_TEMP_cat_current_counter) {
								$RSDB_TEMP_current_category = $result_category_tree_temp['cat_name'];
								$RSDB_TEMP_current_category_desc = $result_category_tree_temp['cat_description'];
								echo " <font size='2'>&rarr;</font> <a href='".$RSDB_intern_link_category_cat.$result_category_tree_temp['cat_id'];
								if (!empty($_GET['cat2']) && $_GET['cat2'] != 'flat') {
									echo "&amp;cat2=".htmlentities(@$_GET['cat2']);
								}
								echo "'>".$result_category_tree_temp['cat_name']."</a>";
							}
					}
					$RSDB_TEMP_cat_current_counter--;
					$k=1;
					$RSDB_TEMP_cat_current_id="";
					$RSDB_TEMP_cat_current_id = $RSDB_TEMP_cat_id;
					
				}		
				//create_historybar($RSDB_TEMP_cat_path, $RSDB_TEMP_cat_id, $RSDB_TEMP_cat_current_level);
			}
			
			if ($RSDB_SET_group != "" && $RSDB_viewpage != false) {
        $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_groups WHERE grpentr_visible = '1' AND grpentr_id = :group_id AND grpentr_comp = '1' ORDER BY grpentr_name ASC");
        $stmt->bindParam('group_id',$RSDB_SET_group,PDO::PARAM_STR);
        $stmt->execute();
				$result_current_group = $stmt->fetch(PDO::FETCH_ASSOC);
				echo " <font size='2'>&rarr;</font> <a href='".$RSDB_intern_link_group.$RSDB_SET_group."'>".$result_current_group['grpentr_name']."</a>";
			}
			if (isset($_GET['item']) && $_GET['item'] != "" && $RSDB_viewpage != false) {
        $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_visible = '1' AND comp_id = :item_id ORDER BY comp_name ASC");
        $stmt->bindParam('item_id',$_GET['item'],PDO::PARAM_STR);
        $stmt->execute();
				$result_current_group = $stmt->fetch(PDO::PARAM_STR);
				echo " <font size='2'>&rarr;</font> <a href='".$RSDB_intern_link_group.htmlspecialchars($_GET['item'])."'>".$result_current_group['comp_name'];
				
							echo " ["."ReactOS ".show_osversion($result_current_group['comp_osversion'])."]";
				
				echo "</a>";
			}
			
		}
    echo "</td></tr></table>";
	include('inc/tree/tree_menubar_sections.php');	

	if ($RSDB_SET_group == "" && isset($_GET['item']) && $_GET['item'] == "" && $RSDB_viewpage != false) {
		echo "<h2>".$RSDB_TEMP_current_category."</h2>";
		echo "<p>".$RSDB_TEMP_current_category_desc."</p>";
	}

	if ($RSDB_viewpage != false) {
		if ($RSDB_SET_group != "" || isset($_GET['item']) && $_GET['item'] != "") {
			echo "<br />";
		}
		else {
			echo "<p align='center'>";
			if (isset($_GET['cat2']) && $_GET['cat2'] == 'flat') {
				echo "<b>Flat Style</b> | <a href='".$RSDB_intern_link_category_cat.htmlspecialchars(@$_GET['cat'])."&amp;cat2=tree'>Tree Style</a>";
			}
			elseif (isset($_GET['cat2']) && $_GET['cat2'] == 'tree') {
				echo "<a href='".$RSDB_intern_link_category_cat.htmlspecialchars(@$_GET['cat'])."&amp;cat2=flat'>Flat Style</a> | <b>Tree Style</b>";
			}
			echo "</p>";
		}
	}
	}
?>


<?php
	if (isset($_GET['page']) && ($_GET['page'] == "name" || $_GET['page'] == "vendor")) {
		if ($_GET['page'] == "name") {
			if (isset($_GET['item']) && $_GET['item'] != "") {
        $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_visible = '1' AND comp_id = :item_id ORDER BY comp_name ASC") ;
        $stmt->bindParam('item_id',@$_GET['item'],PDO::PARAM_STR);
        $stmt->execute();
				$result_itempid = $stmt->fetch(PDO::FETCH_ASSOC);
				if ($result_itempid['comp_groupid'] == "" || $result_itempid['comp_groupid'] == "0") {
					$RSDB_viewpage = false;
				}
				$RSDB_SET_group = $result_itempid['comp_groupid'];
			}
			if ($RSDB_SET_group != "") {
      $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_groups WHERE grpentr_visible = '1' AND grpentr_id = :group_id ORDER BY grpentr_id ASC");
      $stmt->bindParam('group_id',$RSDB_SET_group,PDO::PARAM_STR);
      $stmt->execute();
			$result_groupid = $stmt->fetch(PDO::FETCH_ASSOC);
			$RSDB_SET_letter = strtolower(substr($result_groupid['grpentr_name'], 0, 1)); 
		}
	?>
	<table width="100%" border="1" cellpadding="3" cellspacing="0" bordercolor="#5984C3">
	  <tr>
		<td>&nbsp;<font size="2">Browsing: </font> <a href="<?php echo $RSDB_intern_link_name_letter_EX."all"; ?>"><?php
	
		if ($RSDB_SET_letter != "") {
			echo ucfirst($RSDB_SET_letter);
		}
		else {
			echo "All";
		}
		
		echo "</a>";
		 
		if (isset($_GET['item']) && $_GET['item'] != "" && $RSDB_viewpage != false) {
      $stmt=CDBConnection::getInstance("SELECT * FROM rsdb_item_comp WHERE comp_visible = '1' AND comp_id = :item_id ORDER BY comp_name ASC");
      $stmt->bindParam('item_id',$_GET['item'],PDO::PARAM_STR);
      $stmt->execute();
			$result_itempid = $stmt->fetch(PDO::FETCH_ASSOC);
			if ($result_itempid['comp_groupid'] == "" || $result_itempid['comp_groupid'] == "0") {
				//die("");
				$RSDB_viewpage = false;
			}
			$RSDB_SET_group = $result_itempid['comp_groupid'];
		}
		if ($RSDB_SET_group != "" && $RSDB_viewpage != false) {
			$stmt=CDBConnection::getInstance()->prepare("SELECT *  FROM rsdb_groups WHERE grpentr_visible = '1' AND grpentr_id = :group_id AND grpentr_comp = '1' ORDER BY grpentr_name ASC");
      $stmt->bindParam('group_id',$RSDB_SET_group,PDO::PARAM_STR);
      $stmt->execute();
			$result_current_group = $stmt->fetch(PDO::FETCH_ASSOC);
			if ($result_current_group['grpentr_category'] == "" || $result_current_group['grpentr_category'] == "0") {
				//die("");
				$RSDB_viewpage = false;
			}
			echo " <font size='2'>&rarr;</font> <a href='".$RSDB_intern_link_group.$RSDB_SET_group."'>".$result_current_group['grpentr_name']."</a>";
		}
		if (isset($_GET['item']) && $_GET['item'] != "" && $RSDB_viewpage != false) {
      $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_visible = '1' AND comp_id = :item_id ORDER BY comp_name ASC");
      $stmt->bindParam('item_id',$_GET['item'],PDO::PARAM_STR);
      $stmt->execute();
			$result_current_group = $stmt->fetch(PDO::FETCH_ASSOC);
			echo " <font size='2'>&rarr;</font> <a href='".$RSDB_intern_link_item_EX.htmlspecialchars($_GET['item'])."'>".$result_current_group['comp_name'];
			
						echo " ["."ReactOS ".show_osversion($result_current_group['comp_osversion'])."]";
			
			echo "</a>";
		}
	
		echo "</td></tr></table>";
	
	}
	elseif ($_GET['page'] == "vendor") {

			if (isset($_GET['vendor']) && $_GET['vendor'] != '') {
        $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_vendor WHERE vendor_id = :vendor_id ORDER BY vendor_name ASC");
        $stmt->bindParam('vendor_id',$_GET['vendor'],PDO::PARAM_STR);
        $stmt->execute();
				$result_itempid = $stmt->fetch(PDO::FETCH_ASSOC);
				if ($result_itempid['vendor_id'] == "" || $result_itempid['vendor_id'] == "0") {
					$RSDB_viewpage = false;
				}
				$RSDB_SET_letter = strtolower(substr($result_itempid['vendor_name'], 0, 1)); 
			}
	?>
	<table width="100%" border="1" cellpadding="3" cellspacing="0" bordercolor="#5984C3">
	  <tr>
		<td>&nbsp;<font size="2">Browsing: <a href="<?php echo $RSDB_intern_link_vendor_letter_EX.$RSDB_SET_letter; ?>">
		
		<?php
	
		if ($RSDB_SET_letter != "") {
			echo ucfirst($RSDB_SET_letter);
		}
		else {
			echo "All";
		}
		
		echo "</a></font>";
		 
		if (isset($_GET['vendor']) && $_GET['vendor'] != '' && $RSDB_viewpage != false) {
      $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_vendor WHERE vendor_id = :vendor_id ORDER BY vendor_name ASC");
      $stmt->bindParam('vendor_id',$_GET['vendor'],PDO::PARAM_STR);
      $stmt->execute();
			$result_itempid = $stmt->fetch(PDO::FETCH_ASSOC);
			if ($result_itempid['vendor_id'] == "" || $result_itempid['vendor_id'] == "0") {
				//die("");
				$RSDB_viewpage = false;
			}
			//$RSDB_SET_group = $result_itempid['vendor_id'];
			echo " <font size='2'>&rarr;</font> <a href='".$RSDB_intern_link_vendor_id_EX.htmlspecialchars($_GET['vendor'])."'>".$result_itempid['vendor_name']."</a>";
		}
		if ($RSDB_SET_group != "" && $RSDB_viewpage != false) {
      $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_groups WHERE grpentr_visible = '1' AND grpentr_id = :group_id AND grpentr_comp = '1' ORDER BY grpentr_name ASC");
      $stmt->bindParam('group_id',$RSDB_SET_group,PDO::PARAM_STR);
      $stmt->execute();
			$result_current_group = $stmt->fetch(PDO::FETCH_ASSOC);
			if ($result_current_group['grpentr_category'] == "" || $result_current_group['grpentr_category'] == "0") {
				//die("");
				$RSDB_viewpage = false;
			}
			echo " <font size='2'>&rarr;</font> <a href='".$RSDB_intern_link_name_group_EX.$RSDB_SET_group."'>".$result_current_group['grpentr_name']."</a>";
		}
		if (isset($_GET['item']) && $_GET['item'] != "" && $RSDB_viewpage != false) {
      $stmt=CDBCOnnection::getInstance()->prepare("SELECT * FROM rsdb_item_vendor WHERE vendor_id = :vendor_id ORDER BY vendor_name ASC");
      $stmt->bindParam('group_id',@$_GET['vendor'],PDO::PARAM_STR);
      $stmt->execute();
			$result_current_group = $stmt->fetch(PDO::PARAM_STR);
			echo " <font size='2'>&rarr;</font> <a href='".$RSDB_intern_link_name_item_EX.htmlspecialchars($_GET['item'])."'>".$result_current_group['vendor_name'];
			echo "</a>";
		}
	
		echo "</td></tr></table>";

	}




	include('inc/tree/tree_menubar_sections.php');	

	if (isset($_GET['page']) && $_GET['page'] == "name") {
		$RSDB_TEMP_link_letter = $RSDB_intern_link_name_letter_EX;
	}
	elseif (isset($_GET['page']) && $_GET['page'] == "vendor") {
		$RSDB_TEMP_link_letter = $RSDB_intern_link_vendor_letter_EX;
	}

	if ($RSDB_viewpage != false) {

		echo '<p align="center">';
		
		if ($RSDB_SET_letter == "all" || $RSDB_SET_letter == "") {
			echo '  <b>All</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'all" class="letterbarlink">All</a> ';
		}
	
		if ($RSDB_SET_letter == "a") {
			echo '  <b>A</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'a" class="letterbarlink">A</a> ';
		}
	
		if ($RSDB_SET_letter == "b") {
			echo '  <b>B</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'b" class="letterbarlink">B</a> ';
		}
	
		if ($RSDB_SET_letter == "c") {
			echo '  <b>C</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'c" class="letterbarlink">C</a> ';
		}
	
		if ($RSDB_SET_letter == "d") {
			echo '  <b>D</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'d" class="letterbarlink">D</a> ';
		}
	
		if ($RSDB_SET_letter == "e") {
			echo '  <b>E</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'e" class="letterbarlink">E</a> ';
		}
	
		if ($RSDB_SET_letter == "f") {
			echo '  <b>F</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'f" class="letterbarlink">F</a> ';
		}
	
		if ($RSDB_SET_letter == "g") {
			echo '  <b>G</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'g" class="letterbarlink">G</a> ';
		}
	
		if ($RSDB_SET_letter == "h") {
			echo '  <b>H</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'h" class="letterbarlink">H</a> ';
		}
	
		if ($RSDB_SET_letter == "i") {
			echo '  <b>I</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'i" class="letterbarlink">I</a> ';
		}
	
		if ($RSDB_SET_letter == "j") {
			echo '  <b>J</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'j" class="letterbarlink">J</a> ';
		}
	
		if ($RSDB_SET_letter == "k") {
			echo '  <b>K</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'k" class="letterbarlink">K</a> ';
		}
	
		if ($RSDB_SET_letter == "l") {
			echo '  <b>L</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'l" class="letterbarlink">L</a> ';
		}
	
		if ($RSDB_SET_letter == "m") {
			echo '  <b>M</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'m" class="letterbarlink">M</a> ';
		}
		
		if ($RSDB_SET_letter == "n") {
			echo '  <b>N</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'n" class="letterbarlink">N</a> ';
		}
		
		if ($RSDB_SET_letter == "o") {
			echo '  <b>O</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'o" class="letterbarlink">O</a> ';
		}
	
		if ($RSDB_SET_letter == "p") {
			echo '  <b>P</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'p" class="letterbarlink">P</a> ';
		}
	
		if ($RSDB_SET_letter == "q") {
			echo '  <b>Q</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'q" class="letterbarlink">Q</a> ';
		}
	
		if ($RSDB_SET_letter == "r") {
			echo '  <b>R</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'r" class="letterbarlink">R</a> ';
		}
	
		if ($RSDB_SET_letter == "s") {
			echo '  <b>S</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'s" class="letterbarlink">S</a> ';
		}
	
		if ($RSDB_SET_letter == "t") {
			echo '  <b>T</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'t" class="letterbarlink">T</a> ';
		}
	
		if ($RSDB_SET_letter == "u") {
			echo '  <b>U</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'u" class="letterbarlink">U</a> ';
		}
	
		if ($RSDB_SET_letter == "v") {
			echo '  <b>V</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'v" class="letterbarlink">V</a> ';
		}
	
		if ($RSDB_SET_letter == "w") {
			echo '  <b>W</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'w" class="letterbarlink">W</a> ';
		}
	
		if ($RSDB_SET_letter == "x") {
			echo '  <b>X</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'x" class="letterbarlink">X</a> ';
		}
	
		if ($RSDB_SET_letter == "y") {
			echo '  <b>Y</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'y" class="letterbarlink">Y</a> ';
		}
	
		if ($RSDB_SET_letter == "z") {
			echo '  <b>Z</b> ';
		}
		else {
			echo '  <a href="'. $RSDB_TEMP_link_letter .'z" class="letterbarlink">Z</a> ';
		}
	
		echo "</p><br />";

		}
	}
	
	if ($RSDB_viewpage == false) {
		echo "<p>No related database entry found!</p>";
	}
	
	
			//Message::show("Compatibility");
?>
