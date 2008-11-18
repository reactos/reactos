function setupRightClickEdit() {
	if (document.getElementsByTagName) {
		var spans = document.getElementsByTagName('span');
		for (var i = 0; i < spans.length; i++) {
			var el = spans[i];
			if(el.className == 'editsection') {
				addRightClickEditHandler(el);
			}
		}
	}
}

function addRightClickEditHandler(el) {
	for (var i = 0; i < el.childNodes.length; i++) {
		var link = el.childNodes[i];
		if (link.nodeType == 1 && link.nodeName.toLowerCase() == 'a') {
			var editHref = link.getAttribute('href');
			// find the enclosing (parent) header
			var prev = el.parentNode;
			if (prev && prev.nodeType == 1 &&
			prev.nodeName.match(/^[Hh][1-6]$/)) {
				prev.oncontextmenu = function(e) {
					if (!e) { e = window.event; }
					// e is now the event in all browsers
					var targ;
					if (e.target) { targ = e.target; }
					else if (e.srcElement) { targ = e.srcElement; }
					if (targ.nodeType == 3) { // defeat Safari bug
						targ = targ.parentNode;
					}
					// targ is now the target element

					// We don't want to deprive the noble reader of a context menu
					// for the section edit link, do we?  (Might want to extend this
					// to all <a>'s?)
					if (targ.nodeName.toLowerCase() != 'a'
					|| targ.parentNode.className != 'editsection') {
						document.location = editHref;
						return false;
					}
					return true;
				};
			}
		}
	}
}

hookEvent("load", setupRightClickEdit);
