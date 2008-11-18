<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>

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
	
	global $rpm_page;
	
	$roscms_extern_brand = "RosCMS";
	$roscms_extern_version = "v3";
	$roscms_extern_version_detail = "v3.0730";
	
	$roscms_intern_webserver_pages = "http://www.reactos.org/";
	
	$roscms_intern_webserver_roscms = "http://www.reactos.org/roscms/";
	$roscms_intern_page_link = $roscms_intern_webserver_roscms . "?page=";
	$roscms_intern_script_name =  $roscms_intern_page_link . $rpm_page;
	
	$roscms_intern_script_branch = $roscms_intern_script_name. "&branch=";

	$roscms_standard_language="en"; // en/de/fr/...
	$roscms_standard_language_full="English"; // English/German/...
	$roscms_standard_language_trans="de"; // en/de/fr/...

?>