/*
  PROJECT:    ReactOS Web Test Manager
  LICENSE:    GNU GPLv2 or any later version as published by the Free Software Foundation
  PURPOSE:    JavaScript file for the Index Page (parsed by PHP before)
  COPYRIGHT:  Copyright 2008-2009 Colin Finck <colin@reactos.org>

  charset=utf-8
*/

var CurrentPage;
var data;
var FullRange;
var inputbox_startrev;
var inputbox_endrev;
var PageCount;
var ResultCount;
var SelectedResults = new Array();
var SelectedResultCount = 0;

var REQUESTTYPE_FULLLOAD = 1;
var REQUESTTYPE_ADDPAGE = 2;
var REQUESTTYPE_PAGESWITCH = 3;

function SetRowColor(elem, color)
{
	tdl = elem.getElementsByTagName("td");
	
	for(var i = 0; i < tdl.length; i++)
		tdl[i].style.background = color;
}

function Result_OnMouseOver(elem)
{
	SetRowColor(elem, "#FFFFCC");
}

function Result_OnMouseOut(elem)
{
	if(elem.className == "odd")
		SetRowColor(elem, "#DDDDDD");
	else
		SetRowColor(elem, "#EEEEEE");
}

/**
 * Make sure that all checkboxes for the result identified by the given checkbox are checked.
 * Also update our SelectedResults array appropriately.
 */
function UpdateSelectedResults(checkbox)
{
	// Make sure the user doesn't select more than he's allowed to :-)
	if(checkbox.checked && SelectedResultCount == <?php echo MAX_COMPARE_RESULTS; ?>)
	{
		alert("<?php printf(addslashes($testman_langres["maxselection"]), MAX_COMPARE_RESULTS); ?>");
		checkbox.checked = false;
		return;
	}
	
	var id = checkbox.name.substr(5);
	
	// Make sure all checkboxes belonging to this test show the same state
	var elems = document.getElementsByName(checkbox.name);
	
	for(var i = 0; i < elems.length; i++)
		elems[i].checked = checkbox.checked;
	
	if(checkbox.checked)
	{
		SelectedResults[id] = true;
		SelectedResultCount++;
	}
	else
	{
		delete SelectedResults[id];
		SelectedResultCount--;
	}
	
	// Update the status message
	document.getElementById("status").innerHTML = "<?php printf($testman_langres["status"], '<b>" + SelectedResultCount + "<\/b>'); ?>";
}

/**
 * Make sure that all checkboxes for the results in SelectedResults are checked.
 */
function UpdateAllCheckboxes()
{
	for(id in SelectedResults)
	{
		var elems = document.getElementsByName("test_" + id);
		
		for(var i = 0; i < elems.length; i++)
			elems[i].checked = true;
	}
}

function Result_OnCheckboxClick(checkbox)
{
	UpdateSelectedResults(checkbox);
}

function Result_OnCellClick(elem)
{
	var checkbox = elem.parentNode.firstChild.firstChild;
	checkbox.checked = !checkbox.checked;
	
	UpdateSelectedResults(checkbox);
}

function SearchInputs_OnKeyPress(event)
{
	// IE vs. other browsers again
	if(window.event)
		var KeyCode = window.event.keyCode;
	else
		var KeyCode = event.which;
	
	// Submit the search form in case the user pressed the Return key
	if(KeyCode == 13)
		SearchButton_OnClick();
}

function SearchRevisionInput_OnKeyUp(elem)
{
	var val = elem.value.replace(/[^[0-9-]/g, "");
	
	// First check if something was changed by the replace function.
	// If not, don't set elem.value = val. Otherwise the cursor would always jump to the last character in IE, when you press any key.
	if(elem.value != val)
		elem.value = val;
}

function GetRevNums()
{
	var rev = document.getElementById("search_revision").value;
	
	// If the user didn't enter any revision number at all, he doesn't want to search for a specific revision
	if(!rev)
	{
		inputbox_startrev = "";
		inputbox_endrev = "";
		return true;
	}
	
	if(isNaN(rev) || rev < 1)
	{
		// Maybe the user entered a revision range
		var hyphen = rev.indexOf("-");
		
		if(hyphen > 0)
		{
			inputbox_startrev = rev.substr(0, hyphen);
			inputbox_endrev = rev.substr(hyphen + 1);
		}
		
		if(hyphen <= 0 || isNaN(inputbox_startrev) || isNaN(inputbox_endrev))
		{
			alert("Invalid revision number!");
			return false;
		}
	}
	else
	{
		inputbox_startrev = rev;
		inputbox_endrev = rev;
	}
	
	return true;
}

function SearchCall()
{
	document.getElementById("ajax_loading_search").style.visibility = "visible";
	AjaxGet("ajax-search.php", "SearchCallback", data);
}

function SearchButton_OnClick()
{
	if(!GetRevNums())
		return;
	
	data = new Array();
	
	CurrentPage = 1;
	FullRange = document.getElementById("search_revision").value;
	
	data["startrev"] = inputbox_startrev;
	data["endrev"] = inputbox_endrev;
	data["user"] = document.getElementById("search_user").value;
	data["platform"] = document.getElementById("search_platform").value;
	
	data["resultlist"] = 1;
	data["requesttype"] = REQUESTTYPE_FULLLOAD;
	
	SearchCall();
}

function GetTagData(RootElement, TagName)
{
	var Child = RootElement.getElementsByTagName(TagName)[0].firstChild;
	
	if(!Child)
		return "";
	
	return Child.data;
}

function SearchCallback(HttpRequest)
{
	// Check for an error
	if(HttpRequest.responseXML.getElementsByTagName("error").length > 0)
	{
		alert(HttpRequest.responseXML.getElementsByTagName("error")[0].firstChild.data)
		return;
	}
	
	var html = "";
	
	if(data["resultlist"])
	{
		// Build a new infobox
		html += '<table id="infotable" cellspacing="0" cellpadding="0"><tr><td id="infobox">';
		
		if(data["requesttype"] == REQUESTTYPE_FULLLOAD)
		{
			ResultCount = parseInt(HttpRequest.responseXML.getElementsByTagName("resultcount")[0].firstChild.data);
			html += '<?php printf(addslashes($testman_langres["foundresults"]), "<span id=\"resultcount\">' + ResultCount + '<\/span>"); ?>';
		}
		else
		{
			html += document.getElementById("infobox").innerHTML;
		}
		
		html += '<\/td>';
		
		// Page number boxes
		html += '<td id="pagesbox">';
		
		if(CurrentPage == 1)
		{
			html += '&laquo; ';
			html += '&lsaquo; <?php echo addslashes($shared_langres["prevpage"]); ?> ';
		}
		else
		{
			html += '<a href="javascript:FirstPage_OnClick()" title="<?php echo addslashes($shared_langres["firstpage_title"]); ?>">&laquo;<\/a> ';
			html += '<a href="javascript:PrevPage_OnClick()" title="<?php echo addslashes($shared_langres["prevpage_title"]); ?>">&lsaquo; <?php echo addslashes($shared_langres["prevpage"]); ?><\/a> ';
		}
		
		html += '<select id="pagesel" size="1" onchange="PageBox_OnChange(this)">';
		
		if(data["requesttype"] == REQUESTTYPE_FULLLOAD)
		{
			PageCount = 1;
			
			html += '<option value="' + CurrentPage + '-' + HttpRequest.responseXML.getElementsByTagName("firstid")[0].firstChild.data + '"><?php echo addslashes($shared_langres["page"]); ?> ' + CurrentPage;
			
			if(HttpRequest.responseXML.getElementsByTagName("resultcount")[0].firstChild.data > 0)
				html += ' - ' + HttpRequest.responseXML.getElementsByTagName("firstrev")[0].firstChild.data + ' ... ' + HttpRequest.responseXML.getElementsByTagName("lastrev")[0].firstChild.data + '<\/option>';
		}
		else
		{
			html += document.getElementById("pagesel").innerHTML;
		}
		
		html += '<\/select> ';
		
		if(HttpRequest.responseXML.getElementsByTagName("moreresults")[0].firstChild.data == 0)
		{
			html += '<?php echo addslashes($shared_langres["nextpage"]); ?> &rsaquo; ';
			html += '&raquo;';
		}
		else
		{
			html += '<a href="javascript:NextPage_OnClick()" title="<?php echo addslashes($shared_langres["nextpage_title"]); ?>"><?php echo addslashes($shared_langres["nextpage"]); ?> &rsaquo;<\/a> ';
			html += '<a href="javascript:LastPage_OnClick()" title="<?php echo addslashes($shared_langres["lastpage_title"]); ?>">&raquo;<\/a>';
		}
		
		html += '<\/td><\/tr><\/table>';

		// File table
		html += '<table class="datatable" cellspacing="0" cellpadding="0">';
		
		html += '<thead><tr class="head">';
		html += '<th class="TestCheckbox"><\/th>';
		html += '<th><?php echo addslashes($testman_langres["revision"]); ?><\/th>';
		html += '<th><?php echo addslashes($testman_langres["date"]); ?><\/th>';
		html += '<th><?php echo addslashes($testman_langres["user"]); ?><\/th>';
		html += '<th><?php echo addslashes($testman_langres["platform"]); ?><\/th>';
		html += '<th><?php echo addslashes($testman_langres["comment"]); ?><\/th>';
		html += '<\/tr><\/thead>';
		html += '<tbody>';
		
		var results = HttpRequest.responseXML.getElementsByTagName("result");
	
		if(!results.length)
		{
			html += '<tr class="even"><td colspan="5"><?php echo addslashes($testman_langres["noresults"]); ?><\/td><\/tr>';
		}
		else
		{
			var oddeven = false;
			
			for(var i = 0; i < results.length; i++)
			{
				var ResultID = GetTagData(results[i], "id");
				var ResultRevision = GetTagData(results[i], "revision");
				var ResultDate = GetTagData(results[i], "date");
				var ResultUser = GetTagData(results[i], "user");
				var ResultPlatform = GetTagData(results[i], "platform");
				var ResultComment = GetTagData(results[i], "comment");
				
				html += '<tr class="' + (oddeven ? "odd" : "even") + '" onmouseover="Result_OnMouseOver(this)" onmouseout="Result_OnMouseOut(this)">';
				html += '<td><input onclick="Result_OnCheckboxClick(this)" type="checkbox" name="test_' + ResultID + '" \/><\/td>';
				html += '<td onclick="Result_OnCellClick(this)">' + ResultRevision + '<\/td>';
				html += '<td onclick="Result_OnCellClick(this)">' + ResultDate + '<\/td>';
				html += '<td onclick="Result_OnCellClick(this)">' + ResultUser + '<\/td>';
				html += '<td onclick="Result_OnCellClick(this)">' + ResultPlatform + '<\/td>';
				html += '<td onclick="Result_OnCellClick(this)">' + ResultComment + '<\/td>';
				html += '<\/tr>';

				oddeven = !oddeven;
			}
		}
		
		html += '<\/tbody><\/table>';
		
		document.getElementById("searchtable").innerHTML = html;

		if(data["requesttype"] == REQUESTTYPE_PAGESWITCH)
		{
			// Switch the selected page in the Page ComboBox
			document.getElementById("pagesel").getElementsByTagName("option")[CurrentPage - 1].selected = true;
		}
		
		UpdateAllCheckboxes();
	}
	else
	{
		// Just add a new page to the Page combo box and the information for it
		PageCount++;
		ResultCount += parseInt(HttpRequest.responseXML.getElementsByTagName("resultcount")[0].firstChild.data);
		
		document.getElementById("resultcount").firstChild.data = ResultCount;
		
		// As always, we have to work around an IE bug
		// If I use "innerHTML" here, the first <OPTION> start tag gets dropped in the IE...
		// Therefore I have to use the DOM functions in this case.
		var OptionElem = document.createElement("option");
		var OptionText = document.createTextNode('<?php echo addslashes($shared_langres["page"]); ?> ' + PageCount + ' - ' + HttpRequest.responseXML.getElementsByTagName("firstrev")[0].firstChild.data + ' ... ' + HttpRequest.responseXML.getElementsByTagName("lastrev")[0].firstChild.data);
		
		OptionElem.value = PageCount + "-" + HttpRequest.responseXML.getElementsByTagName("firstid")[0].firstChild.data;
		OptionElem.appendChild(OptionText);
		
		document.getElementById("pagesel").appendChild(OptionElem);
	}
	
	if(HttpRequest.responseXML.getElementsByTagName("moreresults")[0].firstChild.data == 1 && (data["requesttype"] == REQUESTTYPE_FULLLOAD || data["requesttype"] == REQUESTTYPE_ADDPAGE))
	{
		// There are more results available in the full range. Therefore we have to start another request and add a new page.
		data["resultlist"] = 0;
		data["startid"] = HttpRequest.responseXML.getElementsByTagName("nextid")[0].firstChild.data;
		data["requesttype"] = REQUESTTYPE_ADDPAGE;
		SearchCall();
		return;
	}
	
	document.getElementById("ajax_loading_search").style.visibility = "hidden";
}

function PageSwitch(NewPage, StartID)
{
	CurrentPage = NewPage;
	data["resultlist"] = 1;
	data["startid"] = StartID;
	data["requesttype"] = REQUESTTYPE_PAGESWITCH;
	
	SearchCall();
}

function FirstPage_OnClick()
{
	var info = document.getElementById("pagesel").getElementsByTagName("option")[0].value.split("-");
	PageSwitch(info[0], info[1]);
}

function PrevPage_OnClick()
{
	var info = document.getElementById("pagesel").getElementsByTagName("option")[CurrentPage - 2].value.split("-");
	PageSwitch(info[0], info[1]);
}

function PageBox_OnChange(elem)
{
	var info = elem.value.split("-");
	PageSwitch(info[0], info[1]);
}

function NextPage_OnClick()
{
	var info = document.getElementById("pagesel").getElementsByTagName("option")[CurrentPage].value.split("-");
	PageSwitch(info[0], info[1]);
}

function LastPage_OnClick()
{
	var info = document.getElementById("pagesel").getElementsByTagName("option")[ document.getElementById("pagesel").getElementsByTagName("option").length - 1 ].value.split("-");
	PageSwitch(info[0], info[1]);
}

function CompareButton_OnClick()
{
	var first = true;
	var parameters = "ids=";
	
	for(id in SelectedResults)
	{
		if(first)
		{
			parameters += id;
			first = false;
			continue;
		}
		
		parameters += "," + id;
	}
	
	// If first is still true, no results were selected at all
	if(first)
	{
		alert("<?php echo addslashes($testman_langres["noselection"]); ?>");
		return;
	}
	
	window.open("compare.php?" + parameters);
}
