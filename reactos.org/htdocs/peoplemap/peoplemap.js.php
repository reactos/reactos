/*
  PROJECT:    People Map of the ReactOS Website
  LICENSE:    GNU GPLv2 or any later version as published by the Free Software Foundation
  PURPOSE:    Main JavaScript file (parsed by PHP before)
  COPYRIGHT:  Copyright 2007-2008 Colin Finck <mail@colinfinck.de>
  
  charset=utf-8
*/

<?php
	function AddJSSlashes($input)
	{
		return str_replace("</", "<\/", $input);
	}
?>

var Map;
var IconTable;
var MarkerTable;
var MyLocationMarker = null;
var MarkerCount = 0;

var CircleIcon;
var CurrentIcon;
var CurrentIconPrefix;
var MarkerIcon;

function UpdateCounts()
{
	document.getElementById("counttext").innerHTML = "<?php echo $peoplemap_langres["count1"]; ?>" + MarkerCount + "<?php echo $peoplemap_langres["count2"]; ?>" + LocationCount + "<?php echo AddJSSlashes($peoplemap_langres["count3"]); ?>" + UserCount + "<?php echo $peoplemap_langres["count4"]; ?>";
}

function GetIconPath(UserGroup)
{
	if(!UserGroup)
		IconColor = "white";
	else
		IconColor = IconTable[UserGroup];
	
	return "images/" + CurrentIconPrefix + "_" + IconColor + ".png";
}

function AddUserToMap(UserId, UserName, FullName, Latitude, Longitude, UserGroup)
{
	// Has this user already been added to the map?
	if(MarkerTable[UserId] || (UserId == MyUserId && MyLocationMarker))
		return;
		
	var IconColor;
	
	if(!UserGroup)
		UserGroup = "";

	CurrentIcon.image = GetIconPath(UserGroup);
	var Marker = new GMarker(new GLatLng(Latitude, Longitude), CurrentIcon);
	var html;
	
	html  = "<strong><a href=\"http://reactos.org/roscms/index.php/user/" + UserId + "\" target=\"_blank\">" + UserName + "<\/a><\/strong><br>";
	html += FullName + "<br><br>";
	
	// parseFloat strips off trailing zeros
	html += "<?php echo $peoplemap_langres["latitude"]; ?>: " + parseFloat(Latitude) + "&deg;<br>";
	html += "<?php echo $peoplemap_langres["longitude"]; ?>: " + parseFloat(Longitude) + "&deg;<br><br>";
	
	html += "<a href=\"javascript:RemoveUserFromMap(" + UserId + "); UpdateCounts();\"><?php echo $peoplemap_langres["removefrommap"]; ?><\/a>";
	
	MarkerTable[UserId] = new Object();
	MarkerTable[UserId].click = GEvent.addListener(Marker, "click", function() {Marker.openInfoWindowHtml(html);});
	MarkerTable[UserId].group = UserGroup;
	MarkerTable[UserId].html = html;
	MarkerTable[UserId].marker = Marker;
	
	MarkerCount++;
	Map.addOverlay(Marker);
}

function RemoveUserFromMap(UserId)
{
	MarkerCount--;
	Map.removeOverlay(MarkerTable[UserId].marker);
	delete MarkerTable[UserId];
}

function SetLoading(id, value)
{
	document.getElementById("ajaxloading_" + id).style.visibility = (value ? "visible" : "hidden");
}

function GetUsersByNameCallback(HttpRequest, data)
{
	// Check for an error
	if(HttpRequest.responseXML.getElementsByTagName("error").length > 0)
	{
		alert(HttpRequest.responseXML.getElementsByTagName("error")[0].firstChild.data);
	}
	else
	{
		var users = HttpRequest.responseXML.getElementsByTagName("user");
		
		if(!users.length)
		{
			document.getElementById("add_user_result").innerHTML = "<?php echo addslashes($peoplemap_langres["nousers"]); ?>";
		}
		else
		{
			var html = "<ul>";
			
			for(var i = 0; i < users.length; i++)
			{
				var UserId = users[i].getElementsByTagName("id")[0].firstChild.data;
				var UserName = users[i].getElementsByTagName("username")[0].firstChild.data;
				
				var FullName = "";
				if(users[i].getElementsByTagName("fullname")[0].firstChild)
					FullName = users[i].getElementsByTagName("fullname")[0].firstChild.data;
				
				var Latitude = users[i].getElementsByTagName("latitude")[0].firstChild.data;
				var Longitude = users[i].getElementsByTagName("longitude")[0].firstChild.data;
				
				html += "<li>";
				html += "<a href=\"javascript:AddUserToMap(" + UserId + ", '" + UserName + "', '" + FullName + "', " + Latitude + ", " + Longitude + "); UpdateCounts();\">";
				html += UserName;
				html += "<\/a>";
				
				if(FullName != "")
					html += " (" + FullName + ")";
				
				html += "<\/li>";
			}
			
			html += "<\/ul>";
			
			document.getElementById("add_user_result").innerHTML = html;
		}
	}
	
	SetLoading("add", false);
}

function GetUsersByGroupCallback(HttpRequest, data)
{
	// Check for an error
	if(HttpRequest.responseXML.getElementsByTagName("error").length > 0)
	{
		alert(HttpRequest.responseXML.getElementsByTagName("error")[0].firstChild.data);
	}
	else
	{
		var users = HttpRequest.responseXML.getElementsByTagName("user");
		
		for(var i = 0; i < users.length; i++)
		{
			var UserId = users[i].getElementsByTagName("id")[0].firstChild.data;
			var UserName = users[i].getElementsByTagName("username")[0].firstChild.data;
			
			var FullName = "";
			if(users[i].getElementsByTagName("fullname")[0].firstChild)
				FullName = users[i].getElementsByTagName("fullname")[0].firstChild.data;
			
			var Latitude = users[i].getElementsByTagName("latitude")[0].firstChild.data;
			var Longitude = users[i].getElementsByTagName("longitude")[0].firstChild.data;
			
			AddUserToMap(UserId, UserName, FullName, Latitude, Longitude, data["query"]);
		}
		
		UpdateCounts();
	}
	
	SetLoading(data["query"], false);
}

function GetUser()
{
	var len = document.getElementById("add_query").value.length;
	
	if(!len)
	{
		document.getElementById("add_user_result").innerHTML = "";
	}
	else if(len < 3)
	{
		document.getElementById("add_user_result").innerHTML = "<?php echo addslashes($peoplemap_langres["minquerylength"]); ?>";
	}
	else
	{
		var data = new Array();
		
		data["query"] = document.getElementById("add_query").value;
		
		if(document.getElementById("add_subject").selectedIndex == 0)
			data["subject"] = "username";
		else
			data["subject"] = "fullname";
		
		SetLoading("add", true);
		AjaxGet("ajax-getuser.php", "GetUsersByNameCallback", data);
	}
}

// Cancel scrolling the window, when the mouse wheel is used while the mouse is in the map
function OnMapMouseScroll(event)
{
	// First hack to get this to work with IE
	if(!event)
		event = window.event;
	
	if(event.preventDefault)
	{
		// The DOM Level 2 way
		event.preventDefault();
	}
	else
	{
		// As always, IE also needs an extra handler here.. :-/
		return false;
	}
}

function OnWindowResize()
{
	var MinMapHeight = 300;
	var MinMapWidth = 500;
	var SubtractHeight = 300;
	var SubtractWidth = 500;
	
	var MapBox = document.getElementById("map");
	var MapHeight;
	var MapWidth;
	
	// Set the height
	if(window.innerHeight)
	{
		// All browsers except IE
		MapHeight = window.innerHeight - SubtractHeight;
		MapWidth = window.innerWidth - SubtractWidth;
	}
	else
	{
		// IE in Strict Mode
		MapHeight = document.documentElement.clientHeight - SubtractHeight;
		MapWidth = document.documentElement.clientWidth - SubtractWidth;
	}
	
	if(MapHeight < MinMapHeight)
		MapHeight = MinMapHeight;
	
	if(MapWidth < MinMapWidth)
		MapWidth = MinMapWidth;
	
	MapBox.style.height = String(MapHeight) + "px";
	MapBox.style.width = String(MapWidth) + "px";
}

function Load()
{
	var i;
	
	// Exclude IE 5.5 as well, because it has problems with the CSS cursor attribute and various other stuff
	if(!GBrowserIsCompatible() || navigator.userAgent.indexOf("MSIE 5.5") >= 0)
	{
		alert("<?php echo $peoplemap_langres["unsupportedbrowser"]; ?>");
		return;
	}
	
	window.onresize = OnWindowResize;
	
	// Call it for setting the MapBox dimensions before creating the map in it
	OnWindowResize();
	
	Map = new GMap2(document.getElementById("map"));
	Map.addControl(new GLargeMapControl());
	Map.addControl(new GMapTypeControl());
	Map.addControl(new GOverviewMapControl(new GSize(150,150)));
	Map.setCenter(new GLatLng(0, 0), 1);
	Map.enableScrollWheelZoom();
	
	// There is no standard way for capturing mouse wheel events
	//   - The "DOMMouseScroll" event is XUL-specific, so it only works with Gecko browsers
	//   - onmousewheel works with Safari/WebKit, Opera and IE
	if(Map.getContainer().addEventListener)
		Map.getContainer().addEventListener("DOMMouseScroll", OnMapMouseScroll, false);
	
	Map.getContainer().onmousewheel = OnMapMouseScroll;
	
	MarkerTable = new Object();
	
	// Set up the icons
	MarkerIcon = new GIcon(G_DEFAULT_ICON);
	CircleIcon = new GIcon(G_DEFAULT_ICON);
	CircleIcon.shadow = "";
	CircleIcon.iconSize = new GSize(10, 10);
	CircleIcon.shadowSize = null;
	CircleIcon.iconAnchor = new GPoint(5, 5);
	
	// Firefox doesn't reset check marks on a reload, so do that manually here
	document.getElementsByName("icon")[0].checked = "checked";
	
	for(i = 0; i < document.getElementsByName("usergroups").length; i++)
		document.getElementsByName("usergroups")[i].checked = "";
	
	SwitchIcon("marker");
}

function Unload()
{
	delete IconTable;
	delete MarkerTable;
	GUnload();
}

function SwitchIconCreateMarker(id)
{
	// For some reason, this needs to be done in a separate function. Otherwise all markers will always open the info window of the last marker.
	var Marker = new GMarker(MarkerTable[id].marker.getLatLng(), CurrentIcon);
	
	MarkerTable[id].click = GEvent.addListener(Marker, "click", function() {Marker.openInfoWindowHtml(MarkerTable[id].html);});
	MarkerTable[id].marker = Marker;
	Map.addOverlay(Marker);
}

function SwitchIcon(prefix)
{
	if(CurrentIconPrefix == prefix)
		return;
	
	// Change the current icon and icon prefix
	if(prefix == "circle")
		CurrentIcon = CircleIcon;
	else
		CurrentIcon = MarkerIcon;
	
	CurrentIconPrefix = prefix;
	
	// Change the icons of all existing markers (we have to recreate them all :-/)
	for(id in MarkerTable)
	{
		GEvent.removeListener(MarkerTable[id].click);
		Map.removeOverlay(MarkerTable[id].marker);
		
		CurrentIcon.image = GetIconPath(MarkerTable[id].group);
		SwitchIconCreateMarker(id);
	}
		
}

function ToggleToolbox(id)
{
	var i;
	var ToolboxBubble;
	var ToolboxHead;
	var ToolboxControls;
	
	for(i = 0; ; i++)
	{
		ToolboxBubble = document.getElementById("toolbox" + i + "_bubble");
		ToolboxHead = document.getElementById("toolbox" + i + "_head");
		ToolboxControls = document.getElementById("toolbox" + i + "_controls");
		
		if(ToolboxBubble && ToolboxHead && ToolboxControls)
		{
			if(i == id)
			{
				// Show the toolbox identified by "id", hide all the others
				ToolboxBubble.className = "bubble_bg";
				ToolboxBubble.onmouseover = null;
				ToolboxBubble.onmouseout = null;
				ToolboxHead.style.cursor = "default";
				ToolboxControls.style.display = "block";
			}
			else
			{
				ToolboxBubble.className = "selectable_bubble_bg";
				ToolboxBubble.onmouseover = function() {BubbleHover(this, true);};
				ToolboxBubble.onmouseout = function() {BubbleHover(this, false);};
				ToolboxHead.style.cursor = "pointer";
				ToolboxControls.style.display = "none";
			}
		}
		else
			break;
	}
}

function ToggleUserGroup(CheckBox, UserGroup)
{
	var id;
	
	if(CheckBox.checked)
	{
		// Add all users belonging to this group to the map
		var data = new Array();
		
		data["query"] = UserGroup;
		data["subject"] = "group";
		
		SetLoading(UserGroup, true);
		AjaxGet("ajax-getuser.php", "GetUsersByGroupCallback", data);
	}
	else
	{
		// Remove all users from the map, which belong to the group, which has just been unchecked with the checkbox
		for(id in MarkerTable)
		{
			if(MarkerTable[id].group == UserGroup)
				RemoveUserFromMap(id);
		}
		
		UpdateCounts();
	}
}

function SetLocationMarker()
{
	RemoveMyMarkers();
	
	// Get the current location of the user
	var data = new Array();
	
	data["subject"] = "userid";
	data["query"] = MyUserId;
	
	AjaxGet("ajax-getuser.php", "SetLocationMarker_GetUserCallback", data);
}

function SetLocationMarker_GetUserCallback(HttpRequest, data)
{
	// GMarkerOptions has no constructor, so we use a normal JavaScript Object() here
	var opts = new Object();
	opts.draggable = true;
	opts.bouncy = true;
	
	var latitude = 0;
	var longitude = 0;
	
	if(HttpRequest.responseXML.getElementsByTagName("latitude")[0])
	{
		latitude = HttpRequest.responseXML.getElementsByTagName("latitude")[0].firstChild.data;
		longitude = HttpRequest.responseXML.getElementsByTagName("longitude")[0].firstChild.data;
	}
	else
	{
		// We set the marker at 0° N, 0° E. To ensure that the user sees the marker, reset the viewport.
		Map.setCenter(new GLatLng(0, 0), 1);
	}
	
	MyLocationMarker = new GMarker(new GLatLng(latitude, longitude), opts);
	var html;
	
	html  = "<?php echo $peoplemap_langres["mylocation_marker_save_intro"]; ?><br><br>";
	html += "<input type=\"button\" onclick=\"SetLocationMarker_Save();\" value=\"<?php echo $peoplemap_langres["mylocation_marker_save_button"]; ?>\"> ";
	html += "<input type=\"button\" onclick=\"SetLocationMarker_Cancel();\" value=\"<?php echo $peoplemap_langres["mylocation_marker_cancel"]; ?>\"> ";
	html += "<img id=\"ajaxloading_setlocation_marker\" style=\"visibility: hidden;\" src=\"../shared/images/ajax_loading.gif\" alt=\"\">";
	
	GEvent.addListener(MyLocationMarker, "click", function() {MyLocationMarker.openInfoWindowHtml(html);});
	GEvent.addListener(MyLocationMarker, "dragstart", function() {MyLocationMarker.closeInfoWindow();});
	GEvent.addListener(MyLocationMarker, "dragend", function() {MyLocationMarker.openInfoWindowHtml(html);});
	
	Map.addOverlay(MyLocationMarker);
}

function SetLocationMarker_Cancel()
{
	Map.removeOverlay(MyLocationMarker);
	MyLocationMarker = null;
}

function SetLocationMarker_Save()
{
	var latlng;
	var data = new Array();
	
	latlng = MyLocationMarker.getLatLng();
	data["latitude"] = latlng.lat().toFixed(6);
	data["longitude"] = latlng.lng().toFixed(6);
	
	SetLoading("setlocation_marker", true);
	AjaxGet("ajax-setlocation.php", "SetLocationCallback", data);
}

function SetLocationCallback(HttpRequest, data)
{
	// Check for an error
	if(HttpRequest.responseXML.getElementsByTagName("error").length > 0)
	{
		alert(HttpRequest.responseXML.getElementsByTagName("error")[0].firstChild.data);
	}
	else if(HttpRequest.responseXML.getElementsByTagName("userinformation").length > 0)
	{
		RemoveMyMarkers();
		
		// Add a normal white marker
		var UserId = HttpRequest.responseXML.getElementsByTagName("id")[0].firstChild.data;
		var UserName = HttpRequest.responseXML.getElementsByTagName("username")[0].firstChild.data;
		
		var FullName = "";
		if(HttpRequest.responseXML.getElementsByTagName("fullname")[0].firstChild)
			FullName = HttpRequest.responseXML.getElementsByTagName("fullname")[0].firstChild.data;
		
		var Latitude = HttpRequest.responseXML.getElementsByTagName("latitude")[0].firstChild.data;
		var Longitude = HttpRequest.responseXML.getElementsByTagName("longitude")[0].firstChild.data;
		
		AddUserToMap(UserId, UserName, FullName, Latitude, Longitude);
	}
	
	// Only hide the coordinates loading animation, the other one has already been deleted by the RemoveMyMarkers() call above
	if(data["coordinates"])
		SetLoading("setlocation_coordinates", false);
}

function DeleteMyLocation()
{
	RemoveMyMarkers();
	
	var data = new Array();
	data["delete"] = 1;
	
	AjaxGet("ajax-deletelocation.php", "DeleteMyLocationCallback", data);
}

function DeleteMyLocationCallback(HttpRequest, data)
{
	// Nothing to do	
}

function SetLocationCoordinates()
{
	lat = parseFloat(document.getElementById("mylocation_latitude").value);
	lng = parseFloat(document.getElementById("mylocation_longitude").value);
	
	if(isNaN(lat) || isNaN(lng))
	{
		alert("<?php echo $peoplemap_langres["mylocation_coordinates_invalid"]; ?>");
		return;
	}
	
	if(lat >= -90  && lat <= 90 &&
	   lng >= -180 && lng <= 180)
	{
		// These are correct coordinates, set them
		var data = new Array();
		data["latitude"] = lat.toFixed(6);
		data["longitude"] = lng.toFixed(6);
		data["coordinates"] = 1;
		
		SetLoading("setlocation_coordinates", true);
		AjaxGet("ajax-setlocation.php", "SetLocationCallback", data);
	}
	else
	{
		alert("<?php echo $peoplemap_langres["mylocation_coordinates_invalid"]; ?>");
	}
}

function CheckCoordinate(elem)
{
	var val = elem.value.replace(/[^[0-9-.]/g, "");
	
	// First check if something was changed by the replace function.
	// If not, don't set elem.value = val. Otherwise the cursor would always jump to the last character in IE, when you press any key.
	if(elem.value != val)
		elem.value = val;
}

function RemoveMyMarkers()
{
	// Is the user already setting his position?
	if(MyLocationMarker)
		SetLocationMarker_Cancel();
	
	// Has the user already been added to the map?
	if(MarkerTable[MyUserId])
		RemoveUserFromMap(MyUserId);
}

function BubbleHover(elem, hover)
{
	if(hover)
		elem.className = "hovered_bubble_bg";
	else
		elem.className = "selectable_bubble_bg";
}
