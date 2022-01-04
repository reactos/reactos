function toggle (elt)
{
	if (elt == null)
		return;

	var eltLink = elt.firstChild;
	if (eltLink != null && eltLink.className == 't')	// toggle
	{
		var ich = elt.className.indexOf ('_');
		if (ich < 0)
		{
			eltLink.src = 'tp.gif';
			elt.className += '_';
		}
		else
		{
			eltLink.src = 'tm.gif';
			elt.className = elt.className.slice (0, ich);
		}
	}
}

function setView (elt, fView)
{
	var eltLink = elt.firstChild;
	if (eltLink != null && eltLink.className == 't')	// toggle
	{
		var ich = elt.className.indexOf ('_');
		if (ich < 0 && !fView)
		{
			eltLink.src = 'tp.gif';
			elt.className += '_';
		}
		else if (ich >= 0 && fView)
		{
			eltLink.src = 'tm.gif';
			elt.className = elt.className.slice (0, ich);
		}
	}
}

function trimSrc (strSrc)
{
	return strSrc.slice (strSrc.lastIndexOf ('/') + 1, strSrc.lastIndexOf ('.'));
}

function getChildrenByTagName (elt, strTag)
{
	strTag = strTag.toLowerCase ();
	var rgChildren = new Array ();
	var eltChild = elt.firstChild;
	while (eltChild)
	{
		if (eltChild.tagName && eltChild.tagName.toLowerCase () == strTag)
			rgChildren.push (eltChild);
		eltChild = eltChild.nextSibling;
	}
	return rgChildren;
}

function viewAll (elt, dictTypes)
{
	var fView = false;
	var rgImages = getChildrenByTagName (elt, 'IMG');
	var cImages = rgImages.length;
	for (var iImage = 0; iImage < cImages; iImage++)
	{
		var strImage = trimSrc (rgImages [iImage].src);
		if (dictTypes [strImage])
		{
			fView = true;
			break;
		}
	}
	var rgElts = getChildrenByTagName (elt, 'DIV');
	var cElts = rgElts.length;
	if (cElts != 0)
	{
		var iElt;
		for (iElt = 0; iElt < cElts; iElt ++)
			fView |= viewAll (rgElts [iElt], dictTypes);
	}
	elt.style.display = fView ? '' : 'none';
	return fView;
}

function getView (elt)
{
	var eltLink = elt.firstChild;
	if (eltLink != null && eltLink.className == 't')	// toggle
	{
		var ich = elt.className.indexOf ('_');
		if (ich < 0)
			return true;
	}
	return false;
}

function getParentDiv (elt)
{
	if (elt)
	{
		do
		{
			elt = elt.parentNode;
		}
		while (elt && elt.tagName != 'DIV');
	}

	return elt;
}

function getName (elt)
{
	var rgSpans = getChildrenByTagName (elt, 'SPAN');
	for (var iSpan = 0; iSpan < rgSpans.length; iSpan ++)
	{
		var span = rgSpans [iSpan];
		if (span.className == 'l')	// label
		{
			if (span.innerText)
				return span.innerText;
			else
				return span.firstChild.nodeValue;
		}
	}
	return null;
}

function clickHandler (evt)
{
	var elt;
	if (document.layers)
		elt = evt.taget;
	else if (window.event && window.event.srcElement)
	{
		elt = window.event.srcElement;
		evt = window.event;
	}
	else if (evt && evt.stopPropagation)
		elt = evt.target;

	if (!elt.className && elt.parentNode)
		elt = elt.parentNode;

	if (elt.className == 'l')	// label
	{
		var strName;

		eltDiv = getParentDiv (elt);
		var strEltClass = eltDiv.className;
		if (strEltClass.charAt (strEltClass.length - 1) == '_')
			strEltClass = strEltClass.slice (0, strEltClass.length - 1);
		strName = getName (eltDiv);

		if (strEltClass == 'f') // Function
		{
			var strFilename = elt.nextSibling;
			if (strFilename && strFilename.innerText)
			{
				var strRoot = 'http://svn.reactos.org/svn/reactos/trunk/reactos/';
				var strExtra = '?view=markup';
				window.open (strRoot + strFilename.innerText + strExtra, 'SVN');
			}
		}
	}
	else
	{
		if (elt.parentNode && elt.parentNode.className == 't')	// toggle
			elt = elt.parentNode;
		else if (elt.className != 't')	// toggle
			return;

		while (elt != null && elt.tagName != 'DIV')
			elt = elt.parentNode;

		if (evt.shiftKey)
		{
			var rgElts = getChildrenByTagName (elt, 'DIV');
			var cElts = rgElts.length;
			if (cElts != 0)
			{
				var fView = false;
				var iElt;
				for (iElt = 0; iElt < cElts; iElt ++)
				{
					if (getView (rgElts [iElt]))
					{
						fView = true;
						break;
					}
				}
				for (iElt = 0; iElt < cElts; iElt ++)
				{
					setView (rgElts [iElt], !fView);
				}
			}
		}
		else if (evt.ctrlKey)
		{
			setView (elt, true);
			var eltParent = getParentDiv (elt);
			while (eltParent)
			{
				var rgSiblings = getChildrenByTagName (eltParent, 'DIV');
				var cSiblings = rgSiblings.length;
				for (var iSibling = 0; iSibling < cSiblings; iSibling++)
				{
					var eltSibling = rgSiblings [iSibling];
					if (eltSibling != elt)
					{
						setView (eltSibling, false);
					}
				}
				elt = eltParent;
				eltParent = getParentDiv (elt);
			}
		}
		else
			toggle (elt);
	}

	return false;
}

function filterTree ()
{
	var eltImplemented = document.getElementById ('implemented');
	var eltUnimplemented = document.getElementById ('unimplemented');

	var dictTypes = new Object ();
	if (eltImplemented.checked)
		dictTypes ['i'] = true;
	if (eltUnimplemented.checked)
		dictTypes ['u'] = true;

	viewAll (document.getElementById ('ROOT'), dictTypes);
}

function selectImplemented ()
{
	toggleFilter ('implemented');
}

function selectUnimplemented ()
{
	toggleFilter ('unimplemented');
}

function toggleFilter (strFilter)
{
	var eltImplemented = document.getElementById ('implemented');
	var eltUnimplemented = document.getElementById ('unimplemented');

	var eltToggle = document.getElementById (strFilter);
	if (window && window.event && window.event.shiftKey)
	{
		eltImplemented.checked = eltUnimplemented.checked;
		eltUnimplemented.checked = true;
	}
	else
	if (!eltUnimplemented.checked && !eltImplemented.checked)
	{
		eltImplemented.checked = eltUnimplemented.checked = true;
		eltToggle.checked = false;
	}
	filterTree ();
}

function onLoad ()
{
	var eltImplemented = document.getElementById ('implemented');
	var eltUnimplemented = document.getElementById ('unimplemented');
	eltImplemented.checked = eltUnimplemented.checked = true;
}

if (document.layers)
{
	document.captureEvents (Event.MOUSEUP);
	document.onmouseup = clickHandler;
}
else if (document.attachEvent)
{
	document.attachEvent('onclick', clickHandler);
}
else if (document.addEventListener)
{
	document.addEventListener('click', clickHandler, false);
}
else
	document.onclick = clickHandler;
