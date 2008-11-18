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

	// To prevent hacking activity:
	if ( !defined('ROSCMS_SYSTEM') )
	{
		die("Hacking attempt");
	}

?>




<hr size="1" />

<address style="text-align:center;">
<?php echo $roscms_extern_brand." ".$roscms_extern_version_detail; ?> - (c) 2005-2007 Klemens Friedl<br />
<?php
 
	// This page was generated in ...
	$roscms_gentime = microtime(); 
	$roscms_gentime = explode(' ',$roscms_gentime); 
	$roscms_gentime = $roscms_gentime[1] + $roscms_gentime[0]; 
	$roscms_pg_end = $roscms_gentime; 
	$roscms_totaltime = ($roscms_pg_end - $roscms_pg_start); 
	$roscms_showtime = number_format($roscms_totaltime, 4, '.', ''); 
	echo "<br>Page generated in " . $roscms_showtime . " seconds";

?>

</address>

</body>
</html>