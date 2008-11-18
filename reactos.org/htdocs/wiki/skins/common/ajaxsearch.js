// remote scripting library
// (c) copyright 2005 modernmethod, inc

var started;
var typing;
var memory=null;
var body=null;
var oldbody=null;

// Remove the typing barrier to allow call() to complete
function Search_doneTyping()
{
	typing=false;
}

// Wait 500ms to run call()
function Searching_Go()
{
        setTimeout("Searching_Call()", 500);
}

// If the user is typing wait until they are done.
function Search_Typing() {
	started=true;
	typing=true;
	setTimeout("Search_doneTyping()", 500);

	// I believe these are needed by IE for when the users press return?
	if (window.event)
	{
		if (event.keyCode == 13)
		{
			event.cancelBubble = true;
			event.returnValue = true;
		}
	}
}

// Set the body div to the results
function Searching_SetResult( request )
{
	if ( request.status != 200 ) {
		alert("Error: " + request.status + " " + request.statusText + ": " + request.responseText);
		return;
	}

	var result = request.responseText;

        //body.innerHTML = result;
	t = document.getElementById("searchTarget");
	if ( t == null ) {
		oldbody=body.innerHTML;
		body.innerHTML= '<div id="searchTargetContainer"><div id="searchTarget" ></div></div>' ;
		t = document.getElementById("searchTarget");
	}
	t.innerHTML = result;
	t.style.display='block';
}

function Searching_Hide_Results()
{
	t = document.getElementById("searchTarget");
	t.style.display='none';
	body.innerHTML = oldbody;
}


// This will call the php function that will eventually
// return a results table
function Searching_Call()
{
	var x;
	Searching_Go();

	//Don't proceed if user is typing
	if (typing)
		return;

	x = document.getElementById("searchInput").value;

	// Don't search again if the query is the same
	if (x==memory)
		return;

	memory=x;
	if (started) {
		// Don't search for blank or < 3 chars.
		if ((x=="") || (x.length < 3))
		{
			return;
		}

		sajax_do_call( "wfSajaxSearch", [ x ], Searching_SetResult );
	}
}

//Initialize
function sajax_onload() {
	x = document.getElementById( 'searchInput' );
	x.onkeypress= function() { Search_Typing(); };
	Searching_Go();
	body = document.getElementById("content");
}
