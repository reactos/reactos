/*
javascript for Bubble Tooltips by Alessandro Fulciniti
- http://pro.html.it - http://web-graphics.com 
obtained from: http://web-graphics.com/mtarchive/001717.php

phpBB Development Team:
	- modified to adhere to our coding guidelines
	- integration into our design
	- added ability to perform tooltips on select elements
	- further adjustements
*/

var head_text, tooltip_mode;

/**
* Enable tooltip replacements for links
*/
function enable_tooltips_link(id, headline, sub_id)
{
	var links, i, hold;
	
	head_text = headline;

	if (!document.getElementById || !document.getElementsByTagName)
	{
		return;
	}

	hold = document.createElement('span');
	hold.id = '_tooltip_container';
	hold.setAttribute('id', '_tooltip_container');
	hold.style.position = 'absolute';

	document.getElementsByTagName('body')[0].appendChild(hold);

	if (id == null)
	{
		links = document.getElementsByTagName('a');
	}
	else
	{
		links = document.getElementById(id).getElementsByTagName('a');
	}

	for (i = 0; i < links.length; i++)
	{
		if (sub_id)
		{
			if (links[i].id.substr(0, sub_id.length) == sub_id)
			{
				prepare(links[i]);
			}
		}
		else
		{
			prepare(links[i]);
		}
	}

	tooltip_mode = 'link';
}

/**
* Enable tooltip replacements for selects
*/
function enable_tooltips_select(id, headline, sub_id)
{
	var links, i, hold;
	
	head_text = headline;

	if (!document.getElementById || !document.getElementsByTagName)
	{
		return;
	}

	hold = document.createElement('span');
	hold.id = '_tooltip_container';
	hold.setAttribute('id', '_tooltip_container');
	hold.style.position = 'absolute';

	document.getElementsByTagName('body')[0].appendChild(hold);

	if (id == null)
	{
		links = document.getElementsByTagName('option');
	}
	else
	{
		links = document.getElementById(id).getElementsByTagName('option');
	}

	for (i = 0; i < links.length; i++)
	{
		if (sub_id)
		{
			if (links[i].parentNode.id.substr(0, sub_id.length) == sub_id)
			{
				prepare(links[i]);
			}
		}
		else
		{
			prepare(links[i]);
		}
	}

	tooltip_mode = 'select';
}

/**
* Prepare elements to replace
*/
function prepare(element)
{
	var tooltip, text, desc, title;

	text = element.getAttribute('title');

	if (text == null || text.length == 0)
	{
		return;
	}

	element.removeAttribute('title');
	tooltip = create_element('span', 'tooltip');

	title = create_element('span', 'top');
	title.appendChild(document.createTextNode(head_text));
	tooltip.appendChild(title);

	desc = create_element('span', 'bottom');
	desc.innerHTML = text;
	tooltip.appendChild(desc);

	set_opacity(tooltip);

	element.tooltip = tooltip;
	element.onmouseover = show_tooltip;
	element.onmouseout = hide_tooltip;

	if (tooltip_mode == 'link')
	{
		element.onmousemove = locate;
	}
}

/**
* Show tooltip
*/
function show_tooltip(e)
{
	document.getElementById('_tooltip_container').appendChild(this.tooltip);
	locate(this);
}

/**
* Hide tooltip
*/
function hide_tooltip(e)
{
	var d = document.getElementById('_tooltip_container');
	if (d.childNodes.length > 0)
	{
		d.removeChild(d.firstChild);
	}
}

/**
* Set opacity on tooltip element
*/
function set_opacity(element)
{
	element.style.filter = 'alpha(opacity:95)';
	element.style.KHTMLOpacity = '0.95';
	element.style.MozOpacity = '0.95';
	element.style.opacity = '0.95';
}

/**
* Create new element
*/
function create_element(tag, c)
{
	var x = document.createElement(tag);
	x.className = c;
	x.style.display = 'block';
	return x;
}

/**
* Correct positioning of tooltip container
*/
function locate(e)
{
	var posx = 0;
	var posy = 0;

	e = e.parentNode;

	if (e.offsetParent)
	{
		for (var posx = 0, posy = 0; e.offsetParent; e = e.offsetParent)
		{
			posx += e.offsetLeft;
			posy += e.offsetTop;
		}
	}
	else
	{
		posx = e.offsetLeft;
		posy = e.offsetTop;
	}

	if (tooltip_mode == 'link')
	{
		document.getElementById('_tooltip_container').style.top=(posy+20) + 'px';
		document.getElementById('_tooltip_container').style.left=(posx-20) + 'px';
	}
	else
	{
		document.getElementById('_tooltip_container').style.top=(posy+30) + 'px';
		document.getElementById('_tooltip_container').style.left=(posx-205) + 'px';
	}

/*
	if (e == null)
	{
		e = window.event;
	}

	if (e.pageX || e.pageY)
	{
		posx = e.pageX;
		posy = e.pageY;
	}
	else if (e.clientX || e.clientY)
	{
		if (document.documentElement.scrollTop)
		{
			posx = e.clientX+document.documentElement.scrollLeft;
			posy = e.clientY+document.documentElement.scrollTop;
		}
		else
		{
			posx = e.clientX+document.body.scrollLeft;
			posy = e.clientY+document.body.scrollTop;
		}
	}
*/
}
