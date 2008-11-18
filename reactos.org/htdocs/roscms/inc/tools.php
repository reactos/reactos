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


	function compareDate ($i_sFirstDate, $i_sSecondDate)
	{
		// Break the Date strings into seperate components
		$arrFirstDate = explode ("-", $i_sFirstDate);
		$arrSecondDate = explode ("-", $i_sSecondDate);
		
		$intFirstYear = $arrFirstDate[0];
		$intFirstMonth = $arrFirstDate[1];
		$intFirstDay = $arrFirstDate[2];
		
		$intSecondYear = $arrSecondDate[0];
		$intSecondMonth = $arrSecondDate[1];
		$intSecondDay = $arrSecondDate[2];
		
		
		// Calculate the diference of the two dates and return the number of days
		$intDate1Jul = gregoriantojd($intFirstMonth, $intFirstDay, $intFirstYear);
		$intDate2Jul = gregoriantojd($intSecondMonth, $intSecondDay, $intSecondYear);
//		echo "|".$intDate1Jul."|".$intDate2Jul."|";
		return $intDate1Jul - $intDate2Jul;
	
	} // end Compare Date
	
?>