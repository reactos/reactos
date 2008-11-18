<?php
    /*
    RosCMS - Search Function
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


	require_once("connect.db.php");

	$RSDB_SET_search=""; // Search string
	$RSDB_SET_search_lang=""; // Search language
	if (array_key_exists("search", $_GET)) $RSDB_SET_search=htmlspecialchars($_GET["search"]);
	if (array_key_exists("searchlang", $_GET)) $RSDB_SET_search_lang=htmlspecialchars($_GET["searchlang"]);
	if ($RSDB_SET_search_lang == "en") {
		$RSDB_SET_search_lang = "all";
	}	

	$query_count_groups=mysql_query("SELECT COUNT(content_id)
							FROM `content`, `pages`
							WHERE content_active = '1'
							AND 
							( content_name LIKE '%" . mysql_real_escape_string($RSDB_SET_search) . "%'
							  OR content_text LIKE '%" . mysql_real_escape_string($RSDB_SET_search) . "%'
							)
							AND content_name = page_name
							AND page_active = '1'
							AND content_lang = '". mysql_real_escape_string($RSDB_SET_search_lang) ."' 
							ORDER BY `page_name` ASC 
							LIMIT 0 , 20  ;");	
	$result_count_groups = mysql_fetch_row($query_count_groups);

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
	
if ($RSDB_SET_search != "" || strlen($RSDB_SET_search) > 1) {

	$query_page = mysql_query("SELECT content.content_name, content.content_id, pages.page_title
							FROM `content`, `pages`
							WHERE content_active = '1'
							AND 
							( content_name LIKE '%" . mysql_real_escape_string($RSDB_SET_search) . "%'
							  OR content_text LIKE '%" . mysql_real_escape_string($RSDB_SET_search) . "%'
							)
							AND content_name = page_name
							AND page_active = '1'
							AND content_lang = '". mysql_real_escape_string($RSDB_SET_search_lang) ."' 
							GROUP BY page_name, content_name
							ORDER BY `content_id` DESC 
							LIMIT 0 , 15  ;");	
	while($result_page = mysql_fetch_array($query_page)) { // Pages
?>
	<dbentry>
		<content id="<?php echo $result_page['page_title']; ?>"><?php echo $result_page['content_name']; ?></content>
	</dbentry>
<?
	}
}
?>
</root>