var allmessages_nodelist = false;
var allmessages_modified = false;
var allmessages_timeout = false;
var allmessages_running = false;

function allmessagesmodified() {
	allmessages_modified = !allmessages_modified;
	allmessagesfilter();
}

function allmessagesfilter() {
	if ( allmessages_timeout )
		window.clearTimeout( allmessages_timeout );

	if ( !allmessages_running )
		allmessages_timeout = window.setTimeout( 'allmessagesfilter_do();', 500 );
}

function allmessagesfilter_do() {
	if ( !allmessages_nodelist )
		return;

	var text = document.getElementById('allmessagesinput').value.toLowerCase();
	var nodef = allmessages_modified;

	allmessages_running = true;

	for ( var name in allmessages_nodelist ) {
		var nodes = allmessages_nodelist[name];
		var display = ( name.toLowerCase().indexOf( text ) == -1 ? 'none' : '' );

		for ( var i = 0; i < nodes.length; i++)
			nodes[i].style.display =
				( nodes[i].className == "def" && nodef
				  ? 'none' : display );
	}

	if ( text != document.getElementById('allmessagesinput').value.toLowerCase() ||
	     nodef != allmessages_modified )
		allmessagesfilter_do();  // repeat

	allmessages_running = false;
}

function allmessagesfilter_init() {
	if ( allmessages_nodelist )
		return;

	var nodelist = new Array();
	var templist = new Array();

	var table = document.getElementById('allmessagestable');
	if ( !table ) return;

	var rows = document.getElementsByTagName('tr');
	for ( var i = 0; i < rows.length; i++ ) {
		var id = rows[i].getAttribute('id')
		if ( id && id.substring(0,16) != 'sp-allmessages-r' ) continue;
		templist[ id ] = rows[i];
	}

	var spans = table.getElementsByTagName('span');
	for ( var i = 0; i < spans.length; i++ ) {
		var id = spans[i].getAttribute('id')
		if ( id && id.substring(0,17) != 'sp-allmessages-i-' ) continue;
		if ( !spans[i].firstChild || spans[i].firstChild.nodeType != 3 ) continue;

		var nodes = new Array();
		var row1 = templist[ id.replace('i', 'r1') ];
		var row2 = templist[ id.replace('i', 'r2') ];

		if ( row1 ) nodes[nodes.length] = row1;
		if ( row2 ) nodes[nodes.length] = row2;
		nodelist[ spans[i].firstChild.nodeValue ] = nodes;
	}

	var k = document.getElementById('allmessagesfilter');
	if (k) { k.style.display = ''; }

	allmessages_nodelist = nodelist;
}

hookEvent( "load", allmessagesfilter_init );
