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


	  </td>
  </tr>
</table>


<hr size="1" />
<address>
 <p align="center"><a href="http://www.reactos.org" onmousedown="return clk(this.href,'res','')">ReactOS</a> is a registered trademark or a trademark of <a href="http://www.reactos.org/?page=foundation">ReactOS Foundation</a> in the United States and other countries.<br />
   Languages: <a href="#">EN</a> | <a href="#">DE</a> | <a href="#">FR</a> &nbsp;&nbsp;&nbsp; Ajax: <a href="<?php echo $RSDB_intern_link_ajax; ?>true"><?php if ($RSDB_ENV_ajax == "true") { echo "<b>"; } echo "enabled"; if ($RSDB_ENV_ajax == "true") { echo "</b>"; } ?></a> | <a href="<?php echo $RSDB_intern_link_ajax; ?>false"><?php if ($RSDB_ENV_ajax != "true") { echo "<b>"; } echo "disabled"; if ($RSDB_ENV_ajax != "true") { echo "</b>"; } ?></a> &nbsp;&nbsp;&nbsp;<?php
 
	// This page was generated in ...
	$roscms_gentime = microtime(); 
	$roscms_gentime = explode(' ',$roscms_gentime); 
	$roscms_gentime = $roscms_gentime[1] + $roscms_gentime[0]; 
	$roscms_pg_end = $roscms_gentime; 
	$roscms_totaltime = ($roscms_pg_end - $roscms_pg_start); 
	$roscms_showtime = number_format($roscms_totaltime, 4, '.', ''); 
	echo $roscms_showtime . " seconds";

?> &nbsp;&nbsp;&nbsp;Copyright  &#169; Klemens Friedl 2005-<?php echo date("Y");?>, All rights reserved.</p>
</address>

</body>
</html>