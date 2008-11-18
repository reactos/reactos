// Exif metadata display for MediaWiki file uploads
//
// Add an expand/collapse link and collapse by default if set to
// (with JS disabled, user will see all items)
//
// attachMetadataToggle("mw_metadata", "More...", "Fewer...");


function attachMetadataToggle(tableId, showText, hideText) {
	if (document.createTextNode) {
		var box = document.getElementById(tableId);
		if (!box)
			return false;

		var tbody = box.getElementsByTagName('tbody')[0];

		var row = document.createElement('tr');

		var col = document.createElement('td');
		col.colSpan = 2;

		var link = document.createElement('a');
		link.href = '#';

		link.onclick = function() {
			if (box.className == 'mw_metadata collapsed') {
				changeText(link, hideText);
				box.className = 'mw_metadata expanded';
			} else {
				changeText(link, showText);
				box.className = 'mw_metadata collapsed';
			}
			return false;
		}

		var text = document.createTextNode(hideText);

		link.appendChild(text);
		col.appendChild(link);
		row.appendChild(col);
		tbody.appendChild(row);

		// And collapse!
		link.onclick();

		return true;
	}
	return false;
}
