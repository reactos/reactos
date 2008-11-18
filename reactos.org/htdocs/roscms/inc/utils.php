<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Ge van Geldorp <gvg@reactos.org>

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
 * This will return the last two components of the server name, with a leading
 * dot (i.e. usually .reactos.com or .reactos.org for us). See the PHP docs
 * on setcookie() why we need the leading dot.
 */
function cookie_domain()
{
	/* Server name might be just an IP address */
	if(preg_match("#[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}#", $_SERVER["SERVER_NAME"]))
		return $_SERVER["SERVER_NAME"];
	
	/* If it' a DNS address, return the domain name along with the suffix */
	if(preg_match("#(\.[^.]+\.[^.]+$)#", $_SERVER["SERVER_NAME"], $matches))
		return $matches[1];
	
	/* Otherwise return nothing */
	return "";
}

function row_exists($table_name, $where)
{
  $query = "SELECT COUNT(*) AS existing " .
           "  FROM $table_name ";
  if ($where != '')
    {
      $query .= "WHERE $where";
    }
  $set = mysql_query($query) or die("DB error row_exists($table_name, $where)");
  $row = mysql_fetch_array($set);
  return $row['existing'] != 0;
}

?>
