

function category( id, name, help ) {
	this.id=id;
	this.name=name;
	this.help=help;
}
var gCategoryList = [
	new category( "sa", "Search Assistant","Internet Explorer's easy-to-use search interface."),
	new category( "pp", "Preferred Providers", "The popular, general-purpose Web search engines." ),
	new category( "wp", "People and Businesses", "Names, e-mail, phone numbers, mailing addresses for people and businesses." ),
	new category( "ft", "Full-Web", "Search every word on every page of the Web." ),
	new category( "gu", "Directories and Guides", "Categorized and reviewed indexes to Web sites." ),
	new category( "ng", "Newsgroups", "Search and participate in Usenet newsgroups on the Web." ),
	new category( "sp", "Specialty", "Specialized services and reference information on the Web." ),
	new category( "usp", "User Specified", "If you know of a search provider that isn't in this list, but you'd like to use it in the Search Bar, enter the URL here.<BR><BR><B>WARNING:</B> URLs that are not designed for the search bar will not work properly." )
];


function engine( cat, id, name, url, dom, help ) {
	this.cat=cat;
	this.id=id;
	this.name=name;
	this.url=url;
	this.dom=dom;
	this.help=help;
	this.count = 0;
}
var gEngineList = [];
var gNumEngines = 0;
function AddEngine( cat, id, name, url, dom, help ) {
	gEngineList[id] = new engine( cat, id, name, url, dom, help );
	gNumEngines++;
}
AddEngine("sa", "SA", "Search Assistant", "", "", "Return to using Internet Explorer's simple category-based search page.");
AddEngine("ft", "AV", "AltaVista", "http://ie4.altavista.digital.com/cgi-bin/queryie4", "",
	   "Use AltaVista to search the Web or Usenet. Features like language-specific results make AltaVista your searching choice.");
AddEngine("pp", "AOL", "AOL NetFind", "http://www.aol.com/netfind/refer/microsoft.ie4.html", "",
	   "Welcome to AOL NetFind, the easiest, most comprehensive way to find what you're looking for on the World Wide Web, and beyond!");
AddEngine("wp", "BF", "Bigfoot", "http://search.bigfoot.com/RUN?locale=SearchBar&Ref=MS_SearchBar", "",
	   "Bigfoot, Tools for the Net! The most complete & powerful set of online communication services. Your Internet is served!");
AddEngine("ng", "DN", "Deja News", "http://www.dejanews.com/forms/mssb.shtml", "",
	   "Deja News is the premiere Web site for participating in and searching discussion forums, including Usenet newsgroups.");
AddEngine("sp", "EN", "Encarta", "http://find.msn.com/encarta/iesearchresults.asp", "",
		"Start your Internet search with Microsoft® Encarta® Concise Encyclopedia, a free, condensed version of the world's best-selling electronic encyclopedia.");
AddEngine("sp", "EU", "Euroseek", "http://www.euroseek.net/page?ifl=uk&page=msie", "",
		"EuroSeek, The *first* true multilingual search engine in the world! With 40 European languages supported. Come and give us a try!");
AddEngine("pp", "EX", "Excite", "http://www.excite.com/microsoft/ie40/", "",
	   "Excite’s patented search technology gives you access to more than 50 million Web pages, 140,000 pre-selected Web site listings, and thousands of Usenet postings.");
AddEngine("ft", "HB", "HotBot", "http://www.hotbot.com/msiesearchpanel/mshotbot.html", "",
	   "HotBot is the most comprehensive search engine. Consistently rated #1, it is the most effective at helping you find exactly what you are looking for.");
AddEngine("pp", "IS", "Infoseek", "http://www.infoseek.com/Home?pg=Home.html&sv=M6", "www.infoseek.com",
	   "World Wide Web searching at its finest. Just enter your topic of interest. Advanced search features allow you to search for a specific site, URL, title, or links to a page.");
AddEngine("wp", "IF", "InfoSpace", "http://www.infospace.com/info.iebar/", "",
	   "InfoSpace, the Ultimate Directory, is the best place to find people, businesses, places and things on the Internet.");
AddEngine("gu", "LS", "Look Smart", "http://www.looksmart.com/ie/looksmart.html", "",
	   "Search LookSmart's database of over 250,000 selected and reviewed quality sites or the entire Web via AltaVista.");
AddEngine("pp", "LY", "Lycos", "http://www.lycos.com/ie.html", "",
	   "Let Lycos be your Personal Guide to the Internet. Surf our Web Guides by category, vote for your favorite sites, and join the Lycos community.");
AddEngine("gu", "NG", "NetGuide", "http://www.netguide.com/mssearchbox", "",
	   "New sites, best sites, live events, Internet tips and tricks--we provide everything you need to know to get the most out of your online experience.");
AddEngine("wp", "SB", "Switchboard", "http://www.switchboard.com/sb_ie4.htm", "",
	   "Your one-stop resource for finding people, businesses, email addresses and related Web sites.");
AddEngine("wp", "WP", "WorldPages", "http://www.worldpages.com/micro_side/", "",
	   "WorldPages' easy to use search tools help you find businesses, people, email addresses and websites worldwide!");
AddEngine("pp", "YA", "Yahoo", "http://www.yahoo.com/search/ie.html", "", 
	   "Yahoo! Inc. offers a globally-branded Internet navigational service to information and entertainment on the Web.");
AddEngine("sp", "YK", "Yack", "http://www.yack.com/ie4search.html", "", 
	   "Yack! Is the ultimate guide to Internet chat. Easily find chat rooms on your favorite topics.");

AddEngine("usp", "USER", "User Specified", "","","");

gNumEngines--;

function PickOfDay( )
{
	var cnt = 0;
	for (var e in gEngineList) {
		var eng = gEngineList[e];
		if (eng.cat == 'pp') cnt++;
	}
	var today = new Date();
	var pickoday = today.getSeconds() % cnt;

	var cnt = 0;
	for (var e in gEngineList) {
		var eng = gEngineList[e];
		if (eng.cat == 'pp' && cnt++ == pickoday) return eng.id;
	}
}


function SetUserSP( url )
{
	gEngineList["USER"].url = url;
}


function SetProp( prop, value )
{
	var str = document.cookie;
	var offset = str.indexOf( "IE4Search=" );
	var endoff = str.indexOf( ";", offset );
	if (endoff == -1) endoff = str.length;

	if (value == "") value = "x";

	if (offset >= 0) {
		str = str.substring( offset+10, endoff );
		if (str.substring(str.length-1, str.length) == "&")
		{
			str = str.substring(0, str.length-1);
			endoff = endoff - 1;
		}

		offset = str.indexOf( prop + "=", 0 );
		if (offset >= 0) {
			endoff = str.indexOf( "&", offset + prop.length + 1 );
			if (endoff == -1) endoff = str.length;
			str = str.substring( 0, offset - 1) + str.substring( endoff, str.length );
		}
		str = "&" + str
	} else {
		str = "";
	}

	document.cookie = "IE4Search=" + prop + "=" + escape(value) + str + ";path=/;expires=Friday, 21-Nov-08 01:23:45 GMT";	
}

function GetProp( prop )
{
	var str = document.cookie;
	var offset = str.indexOf( "IE4Search=" );
	var endoff = str.indexOf( ";", offset );
	if (endoff == -1) endoff = str.length;

	if (offset >= 0) {
		offset = str.indexOf( prop + "=", offset );
		if (offset >= 0) {
			offset += prop.length + 1;

			var iNextAmp = str.indexOf("&", offset);
	
			if ( (iNextAmp > endoff) || (iNextAmp == -1) )
				endoff = endoff
			else 
				endoff = iNextAmp

			//if (endoff != -1) return unescape(str.substring( offset, endoff ));

			var result = unescape(str.substring( offset, endoff ));
			if (result == "x" || result == "xYz") result = "";
			return result;
		}
	}
	return "";
}
