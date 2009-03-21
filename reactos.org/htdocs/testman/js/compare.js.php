/*
  PROJECT:    ReactOS Web Test Manager
  LICENSE:    GNU GPLv2 or any later version as published by the Free Software Foundation
  PURPOSE:    JavaScript file for the Compare Page (parsed by PHP before)
  COPYRIGHT:  Copyright 2008-2009 Colin Finck <colin@reactos.org>

  charset=utf-8
*/

var CurrentLeftDragBorder;
var CurrentRightDragBorder;
var DragColumn;
var DragX;
var DragOffset;
var MaxLeftDragBorder;
var MaxRightDragBorder;
var MouseX;
var OverlappedForSwap;
var SwapColumn;
var TableRowEquiv;
var TempBlock;

var ColumnDefaultColor = "#5984C3";
var ColumnDragColor = "#8AA9D5";
var ColumnOverlapColor = "#D0DDEE";

function Cell_OnMouseOver(elem)
{
	elem.style.background = "#FFFFCC";
}

function Cell_OnMouseOut(elem)
{
	if(elem.parentNode.className == "odd")
		elem.style.background = "#DDDDDD";
	else
		elem.style.background = "#EEEEEE";
}

function GetColumnIndex(th)
{
	var childs = th.parentNode.childNodes;
	
	for(var i = 0; i < childs.length; i++)
		if(childs[i] == th)
			return i;
}

function ShowChangedCheckbox_OnClick(checkbox)
{
	var value = (checkbox.checked ? "none" : TableRowEquiv);
	
	for(var i = 0; i < UnchangedRows.length; i++)
		document.getElementById("suite_" + UnchangedRows[i]).style.display = value;
	
	document.cookie = "showchanged=" + (checkbox.checked ? "1" : "0");
}

function GetValueForResult(td)
{
	// If a test crashed, return a numeric value of 0, so that the comparison is accurate
	if(td.firstChild.data.replace(/ /g, "") == "CRASH")
		return 0;
	
	return parseInt(td.firstChild.data);
}

function AddDifferenceForColumn(th)
{
	var Index = GetColumnIndex(th);
	var trs = document.getElementById("comparetable").childNodes[1].childNodes;
	
	// Check whether this is the first real column
	if(Index == 1)
	{
		// Remove all difference data in this case as there is no previous element
		for(var i = 0; i < trs.length; i++)
		{
			var tds = trs[i].childNodes[Index].getElementsByTagName("td");
			
			for(var j = 0; j < tds.length; j++)
			{
				// \u00A0 = &nbsp;
				tds[j].getElementsByTagName("span")[0].firstChild.data = "\u00A0";
			}
		}
		
		return;
	}
	
	// No, then add the difference data accordingly
	for(var i = 0; i < trs.length; i++)
	{
		// We can only add difference data if the current table and the previous one contain tables with result data
		if(trs[i].childNodes[Index].firstChild.nodeName != "TABLE" || trs[i].childNodes[Index - 1].firstChild.nodeName != "TABLE")
			continue;
		
		var tds = trs[i].childNodes[Index].getElementsByTagName("td");
		
		for(var j = 0; j < tds.length; j++)
		{
			var CurrentValue = GetValueForResult(tds[j]);
			var PreviousValue = GetValueForResult(trs[i].childNodes[Index - 1].getElementsByTagName("td")[j]);
			
			// Calculate the difference
			var Diff = CurrentValue - PreviousValue;
			
			if(Diff > 0)
				var DiffString = String("(+" + Diff + ")");
			else if(Diff < 0)
				var DiffString = String("(" + Diff + ")");
			else
				var DiffString = "\u00A0";
			
			tds[j].getElementsByTagName("span")[0].firstChild.data = DiffString;
		}
	}
}

function GetAbsoluteOffsetLeft(elem)
{
	var left = 0;
	
	while(elem)
	{
		left += elem.offsetLeft;
		elem = elem.offsetParent;
	}
	
	return left;
}

function GetAbsoluteOffsetTop(elem)
{
	var top = 0;
	
	while(elem)
	{
		top += elem.offsetTop;
		elem = elem.offsetParent;
	}
	
	return top;
}

function Document_OnMouseMove(event)
{
	// IE stores the event in window.event...
	if(!event)
		event = window.event;
	
	MouseX = event.clientX;
	
	// Drag if there's something to do
	if(!DragColumn)
		return;

	var PosX = MouseX - DragOffset;
	
	// Check whether the user is allowed to move anything to the current mouse position
	if(PosX < CurrentLeftDragBorder || PosX > CurrentRightDragBorder)
		return;
	
	// Check whether the user is moving to left or right
	if(MouseX - DragX > 0)
		var NewSwapColumn = DragColumn.nextSibling;
	else if(MouseX - DragX < 0)
		var NewSwapColumn = DragColumn.previousSibling;
	
	// If we have any other swap column, reset it to the default color
	if(SwapColumn && NewSwapColumn != SwapColumn)
		SwapColumn.style.background = ColumnDefaultColor;
	
	SwapColumn = NewSwapColumn;
	
	if(!SwapColumn)
		return;
	
	SwapColumn.style.background = ColumnDragColor;
	
	if(!TempBlock)
	{
		// The only way we can "move the column header" flawlessy and compatible with all major browsers is emulating the behaviour :-)
		// Therefore we create a <div> element for moving and set the column header to move invisible
		TempBlock = document.createElement("div");
		
		for(var i = 0; i < DragColumn.childNodes.length; i++)
			TempBlock.appendChild(DragColumn.childNodes[i].cloneNode(true));
		
		TempBlock.id = "TempBlock";
		TempBlock.style.position = "absolute";
		TempBlock.style.top = GetAbsoluteOffsetTop(DragColumn) + "px";
		
		document.body.insertBefore(TempBlock, document.getElementById("comparetable"));
		DragColumn.style.visibility = "hidden";
	}
	
	// Move the block
	TempBlock.style.left = PosX + "px";
	
	var TempOffsetLeft = GetAbsoluteOffsetLeft(TempBlock);
	var SwapOffsetLeft = GetAbsoluteOffsetLeft(SwapColumn);

	// Check if the dragged column overlaps the swap column by at least the half
	OverlappedForSwap = (NewSwapColumn == DragColumn.nextSibling)
		? (TempOffsetLeft + TempBlock.offsetWidth > SwapOffsetLeft + SwapColumn.offsetWidth / 2)
		: (TempOffsetLeft < SwapOffsetLeft + SwapColumn.offsetWidth / 2);

	// Set the swap column to the overlap color in this case
	if(OverlappedForSwap)
		SwapColumn.style.background = ColumnOverlapColor;
}

function SwapElements(elem1, elem2)
{
	// outerHTML is too unsupported, so we have to copy all column's attributes one after another
	var TempClass = elem1.className;
	var TempHTML = elem1.innerHTML;
	var TempOnClick = elem1.onclick;
	var TempOnMouseOver = elem1.onmouseover;
	var TempOnMouseOut = elem1.onmouseout;
	
	elem1.className = elem2.className;
	elem1.innerHTML = elem2.innerHTML;
	elem1.onclick = elem2.onclick;
	elem1.onmouseover = elem2.onmouseover;
	elem1.onmouseout = elem2.onmouseout;
	
	elem2.className = TempClass;
	elem2.innerHTML = TempHTML;
	elem2.onclick = TempOnClick;
	elem2.onmouseover = TempOnMouseOver;
	elem2.onmouseout = TempOnMouseOut;
}

function Document_OnMouseUp()
{
	if(!DragColumn)
		return;
	
	// This marks the end of a Drag & Drop operation
	if(SwapColumn)
	{
		if(OverlappedForSwap)
		{
			// Swap the column headers
			SwapElements(DragColumn, SwapColumn);
			
			// Swap all cells of these columns
			var DragColumnIndex = GetColumnIndex(DragColumn);
			var SwapColumnIndex = GetColumnIndex(SwapColumn);
			var tbody_trs = document.getElementById("comparetable").childNodes[1].childNodes;
			
			for(var i = 0; i < tbody_trs.length; i++)
				SwapElements(tbody_trs[i].childNodes[DragColumnIndex], tbody_trs[i].childNodes[SwapColumnIndex]);
			
			AddDifferenceForColumn(DragColumn);
			AddDifferenceForColumn(SwapColumn);
			
			if(DragColumnIndex > SwapColumnIndex)
				var NextColumn = DragColumn.nextSibling;
			else
				var NextColumn = SwapColumn.nextSibling;
			
			if(NextColumn)
				AddDifferenceForColumn(NextColumn);
		}
		
		// Reset everything
		DragColumn.style.visibility = "visible";
		document.body.removeChild(TempBlock);
		SwapColumn.style.background = ColumnDefaultColor;
	}
	
	// Cleanup
	DragColumn = null;
	SwapColumn = null;
	TempBlock = null;
}

function ResultHead_OnMouseDown(th)
{
	DragColumn = th;
	DragX = MouseX;
	DragOffset = DragX - GetAbsoluteOffsetLeft(th);
	
	// The drag borders are set to the previous and next "real" columns
	// If there are no more "real" columns, they are set to the maximum borders
	CurrentLeftDragBorder = Math.max(GetAbsoluteOffsetLeft(th.previousSibling), MaxLeftDragBorder);
	
	if(th.nextSibling)
		CurrentRightDragBorder = Math.min(GetAbsoluteOffsetLeft(th.nextSibling), MaxRightDragBorder);
	else
		CurrentRightDragBorder = MaxRightDragBorder;
}

function GetCookieValue(cookie)
{
	var cookies = document.cookie.split(";");
	
	for(var i = 0; i < cookies.length; i++)
	{
		var data = cookies[i].split("=");
		
		if(data[0] == cookie)
			return data[1];
	}
	
	return null;
}

function Load()
{
	// Prepare the Drag & Drop feature
	document.onmousemove = Document_OnMouseMove;
	document.onmouseup = Document_OnMouseUp;
	
	var ths = document.getElementById("comparetable").firstChild.firstChild.childNodes;
	MaxLeftDragBorder = GetAbsoluteOffsetLeft(ths[1]);
	MaxRightDragBorder = GetAbsoluteOffsetLeft(ths[ths.length - 1]);
	
	// As always, IE needs a special handling (this time for the style of table rows)
	if(navigator.appName == "Microsoft Internet Explorer")
		TableRowEquiv = "block";
	else
		TableRowEquiv = "table-row";
	
	// The "showchanged" checkbox will only be available if we have more than one result
	var checkbox = document.getElementById("showchanged");
	
	if(checkbox)
	{
		// Get its value from the cookie
		checkbox.checked = parseInt(GetCookieValue("showchanged"));
		ShowChangedCheckbox_OnClick(checkbox);
	}
}

function Result_OnClick(id)
{
	window.open("detail.php?id=" + id);
}
