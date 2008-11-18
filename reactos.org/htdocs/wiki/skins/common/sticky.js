// Make a layer that stays in the same place on screen when scrolling the browser window.
// Version 1.2
// See http://www.mark.ac/help for browser support.

var mySticky;
var theLayer;

// Setup variables for sliding.
// lastY and staticYOffset should match your CSS top definition.

lastY=10;YOffset=0;staticYOffset=10;refreshMS=25;


// Setup function that runs when the page loads.
	function setup(eID){
		bw=new checkBrowser;
		if(bw.ns4||bw.opera){MM_reloadPage(true);}
		var noFix=bw.ie4||bw.ns4||(bw.macie50)?true:false;
		if (window.attachEvent){fix_bind()}
		else if(noFix){	
			if(bw.ns6){document.getElementById(eID).style.position="absolute";}
			if(bw.macie50){document.getElementById(eID).style.position="absolute";document.getElementById(eID).style.backgroundColor="#ccffcc";}
			if(bw.ns6&&YOffset==0){YOffset=-15}
			mySticky=new makeLayerObj(eID);
			layerSlide(eID)}
		else{
			mySticky=new makeLayerObj(eID);
			mySticky.css.position="fixed";}

		if(!mySticky){mySticky=new makeLayerObj(eID);}
		//mySticky.css.visibility="visible";
	}


// -------------------------
// emulate css 'position: fixed' in IE5+ Win
// code by aclover@1value.com
	fix_elements = new Array();

	function fix_event(){
		var i;
		for (i=0; i < fix_elements.length; i++){
			fix_elements[i].style.left = parseInt(fix_elements[i].fix_left)+document.getElementsByTagName('html')[0].scrollLeft+document.getElementsByTagName('body')[0].scrollLeft+'px';
			fix_elements[i].style.top = parseInt(fix_elements[i].fix_top)+document.getElementsByTagName('html')[0].scrollTop+document.getElementsByTagName('body')[0].scrollTop+'px';
		}
	}

	function fix_bind(){
		var i;
		for (i=0; i < document.all.length; i++){
			if (document.all[i].currentStyle.position=='fixed'){
				document.all[i].fix_left = document.all[i].currentStyle.left;
				document.all[i].fix_top = document.all[i].currentStyle.top;
				document.all[i].style.position = 'absolute';
				fix_elements[fix_elements.length] = document.all[i];
				window.attachEvent('onscroll', fix_event);
				window.attachEvent('onresize', fix_event);
			} 
		}
	}
// -------------------------


// -------------------------
// DHTML menu sliding. Requires checkBrowser()
// Based on source at http://www.simplythebest.net/
	function layerSlide(layerID) {
		if(bw.dhtml){
			if(!mySticky){mySticky=new makeLayerObj(layerID);}
			if (bw.ns) {winY = window.pageYOffset;}
			else if (bw.ie) {winY = document.body.scrollTop;}
			if (bw.ie||bw.ns) {
				if (winY!=lastY&&winY>YOffset-staticYOffset){smooth = .3 * (winY - lastY - YOffset + staticYOffset);}
				else if (YOffset-staticYOffset+lastY>YOffset-staticYOffset){smooth = .3 * (winY - lastY - (YOffset-(YOffset-winY)));}
				else{smooth=0}
				if(smooth > 0) {smooth = Math.ceil(smooth);}
				else{smooth = Math.floor(smooth);}
				if (bw.ie){mySticky.css.pixelTop+=smooth;}
				else if (bw.ns){mySticky.css.top=parseInt(mySticky.css.top)+smooth;}
				lastY = lastY+smooth;
				top.window.status=new Date()
				setTimeout('layerSlide("'+layerID+'")', refreshMS)}}}
// -------------------------

// Netscape 4.x browser resize fix
	function MM_reloadPage(init) {
  	if (init==true) with (navigator) {if ((appName=="Netscape")&&(parseInt(appVersion)==4)) {
    	document.MM_pgW=innerWidth; document.MM_pgH=innerHeight; top.onresize=MM_reloadPage; }}
  	else if (innerWidth!=document.MM_pgW || innerHeight!=document.MM_pgH) {location.reload();}}

// Create browser-independent layer and browser objects
	function makeLayerObj(eID){
		if(document.getElementById){this.css=document.getElementById(eID).style}
		else if(document.layers){this.css=document.layers[eID];}
		else if(document.all){this.css=document.all[eID].style;}
		return this
	}

	function checkBrowser(){
		this.ver=navigator.appVersion;
		this.name=navigator.appName;
		this.mac=(navigator.platform.toLowerCase().indexOf('mac')>-1)?true:false;
		this.opera=(navigator.userAgent.toLowerCase().indexOf('opera')>-1)?true:false;
		this.dom=document.getElementById?true:false;
		this.ns=(this.name=='Netscape');
		this.ie4=(document.all && !this.dom)?true:false;
		this.ie=(this.name =='Microsoft Internet Explorer'&&!this.opera)?true:false;
		this.ie5=(this.ie && (navigator.userAgent.indexOf("MSIE 5")!=-1))?true:false;
		this.macie50=(this.mac&&this.ie5&&(navigator.userAgent.indexOf("MSIE 5.0")!=-1))?true:false
		this.ns4=(this.ns && parseInt(this.ver) == 4)?true:false;
		this.ns6=((this.name=="Netscape")&&(parseInt(this.ver)==5))?true:false
		this.standards=document.getElementById?true:false;
		this.dhtml=this.standards||this.ie4||this.ns4;
	}

	function showMe(eID){
		myFloater=new makeLayerObj(eID)
		myFloater.css.visibility="visible";
	}

	function hideMe(eID){
		myFloater=new makeLayerObj(eID)
		myFloater.css.visibility="hidden";
	}
