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
<a href="<?php echo $RSDB_intern_link_db_sec; ?>home"><?php echo $RSDB_langres['TEXT_compdb_short']; ?></a> &gt; Search
<a href="<?php echo $RSDB_intern_index_php; ?>?page=about"><img src="media/pictures/compatibility_small.jpg" vspace="1" border="0" align="right"></a>
</h1> 
<p>ReactOS Software and Hardware Compatibility Database</p>

<h1>Search</h1>
<form method="post" enctype="multipart/form-data" name="edit" target="_self">
  <table width="100%" border="0">
    <tr> 
      <td><fieldset><legend>Search</legend>
        <p> 
          <input name="itemsearch" type="text" id="itemsearch" size="50" maxlength="100" />
          <input type="submit" name="ok" value="Search" />
          <input name="reset" type="reset" value="Clear Fields" />
        </p>
        <table border="0">
          <tr>
            <td width="135"><fieldset>
              <input name="andor" type="radio" value="1" checked />
              <font color="#999999">AND</font>              
              <input type="radio" name="andor" value="2" />
              <font color="#999999">OR</font>              
            </fieldset>
            </td>
            <td width="285"> 
              <fieldset>
              <input name="word" type="radio" value="2" checked />
              word parts 
              <input name="word" type="radio" value="1" />
              the whole word 
              </fieldset></td>
          </tr>
        </table>
        <input name="startsearch" type="hidden" id="startsearch" value="1" />
        </fieldset>
        <p></td>
    </tr>
  </table>
</form>
<p>&nbsp;</p>
<?php 

	$RSDB_intern_TEMP_itemsearch = "";
	$RSDB_intern_TEMP_item_word = "";
	if (array_key_exists("itemsearch", $_POST)) $RSDB_intern_TEMP_itemsearch=htmlspecialchars($_POST["itemsearch"]);
	if (array_key_exists("word", $_POST)) $RSDB_intern_TEMP_item_word=htmlspecialchars($_POST["word"]);

	if ($RSDB_intern_TEMP_itemsearch != "") {
		if ($RSDB_intern_TEMP_item_word == "2") {
			$RSDB_SET_letter = "%".$RSDB_intern_TEMP_itemsearch."%";
		}
		elseif ($RSDB_intern_TEMP_item_word == "1") {
			$RSDB_SET_letter = $RSDB_intern_TEMP_itemsearch;
		}
		include("inc/tree/tree_name_flat.php"); 
	}

?>