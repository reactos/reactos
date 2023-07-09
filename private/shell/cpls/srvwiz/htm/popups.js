//client area=460x345px

   function popup(flag){
   var bs=body.scrollTop
   //set mouse coordinate variables - where click event occurred on document
   var oX=event.clientX;
   var oY=((event.clientY)+body.scrollTop);
   
   //check to see if cursor is in the client area (user is invoking popup with a keyboard)
   if (((event.clientY)>345)||((event.clientY)<0)){
       oY=(175+body.scrollTop)
	   }
   if (((event.clientX)>444)||((event.clientX)<0)){
       oX=(230)
	   }
   
   //set variables for popup object
   var oDiv=document.all("popup"+flag);
   var oDef=document.all("def"+flag);
   		
   //set variable for definition length
   var dLength=oDef.innerHTML.length

   //determine width of the popup based on length of definition
   if ((dLength)>=650){
       oDiv.style.width="400px";
	   }
   else if ((dLength)>=500){
       oDiv.style.width="350px";
	   }
   else if ((dLength)>=350){
       oDiv.style.width="300px";
	   }
   else if ((dLength)>=200){
       oDiv.style.width="250px";
	   }
   else if ((dLength)>=150){
       oDiv.style.width="225px";
	   }
   else if ((dLength)>=100){
       oDiv.style.width="200px";
	   }
   else if ((dLength)>=60){
       oDiv.style.width="175px";
	   }
   else {
       oDiv.style.width="150px";
	   }

   //set variables for the object's height and width
   var oDivW=oDef.offsetWidth;
   var oDivH=oDiv.offsetHeight;
   
   //set variables for document width (changes depending on whether page scrolls)
   var docW=document.body.clientWidth;
   
   //set variables for distance to center and middle of the object
   var oDivC=(oDivW /2);
   var oDivM=(oDivH /2);
   
   //position object over mouse click
 
   	   //ensure top margin of 2 pixels
       if (((oY - oDivM)- 2)<=0){
		   oDiv.style.top=2
		   }
	   else {
		   oDiv.style.top=(oY - oDivM)
		   }
	 
	   //ensure left margin of 10 pixels
       if (((oX-oDivC)-10)<=0){
		   oDiv.style.left=10
		   }
	   //ensure right margin of 10 pixels
	   else if ((oX+oDivC)>=(docW-12)){
		   oDiv.style.left=((docW-12)-oDivW);
		   }
	   else {
		   oDiv.style.left=(oX-oDivC);
		   }
	   
   	   //ensure top of popup is visible - scroll if necessary
	   if ((body.scrollTop+oDivM)>=oY){
		   bs=((oY-2)-oDivM);
		   body.scrollTop=bs;
		   }
		   
	//ensure bottom of popup is visible - scroll if necessary
	   if (((oY+oDivM+10)-(body.scrollTop))>=345){
		   bs=((oY+oDivM+10)-345);
		   body.scrollTop=bs;
		   }

   //set variables for the object's position
   var oDivT=oDiv.style.pixelTop
   var oDivL=oDiv.style.pixelLeft

   //z-layer the popup over the drop shadow
   oDiv.style.zIndex=2
   shadow.style.zIndex=1
   
   //set the shadow size exactly the same as the definition
   shadow.style.width=(oDivW+2);
   shadow.style.height=oDivH;
   
   //position the drop shadow relative to the definition   
   shadow.style.top=(oDivT+8);
   shadow.style.left=(oDivL+7);
   
   //show the definition
   oDiv.style.visibility="visible";
   
  //show the drop shadow
   shadow.style.visibility="visible";
   	   
   //give the popup focus
   oDef.focus();
   
   //scroll appropriate amount
   body.scrollTop=bs;
  
   }
   
   
   function popOff(flag){
   var oDiv=document.all("popup"+flag);
   oDiv.style.visibility="hidden";
   shadow.style.visibility="hidden"
   }
   
   
   function nothing(){}

