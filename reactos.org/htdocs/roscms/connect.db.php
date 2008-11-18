<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Klemens Friedl <frik85@reactos.org>

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



// Database:
$dbHost = "localhost";
$dbUser = "root";
$dbPass = "";
$dbName = "roscms";


$connect = @mysql_connect($dbHost, $dbUser, $dbPass) or die("ERROR: Cannot connect to the database!");
$selectDB = @mysql_select_db($dbName, $connect) or die("Cannot find and select <b>$dbName</b>!");

// Delete (set nothing) the vars after usage:
$dbHost = "";
$dbUser = "";
$dbPass = "";
$dbName = "";


?>