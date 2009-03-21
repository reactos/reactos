/*
  PROJECT:    ReactOS Web Test Manager
  LICENSE:    GNU GPLv2 or any later version as published by the Free Software Foundation
  PURPOSE:    JavaScript file for the Result Details Page (parsed by PHP before)
  COPYRIGHT:  Copyright 2008-2009 Colin Finck <colin@reactos.org>

  charset=utf-8
*/

function SetRowColor(elem, color)
{
	tdl = elem.getElementsByTagName("td");
	
	for(var i = 0; i < tdl.length; i++)
		tdl[i].style.background = color;
}

function Row_OnMouseOver(elem)
{
	SetRowColor(elem, "#FFFFCC");
}

function Row_OnMouseOut(elem)
{
	if(elem.className == "odd")
		SetRowColor(elem, "#DDDDDD");
	else
		SetRowColor(elem, "#EEEEEE");
}
