/* The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Bugzilla Bug Tracking System.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributor(s): Dennis Melentyev <dennis.melentyev@infopulse.com.ua>
 */

 /* This file provides JavaScript functions to be included once one wish
  * to add a hide/reveal/collapse per-class functionality
  *
  * 
  * This file contains hide/reveal API for customizable page views
  * TUI stands for Tweak UI.
  *
  * See bug 262592 for usage examples.
  * 
  * Note: this interface is experimental and under development.
  * We may and probably will make breaking changes to it in the future.
  */

  var TUIClasses = new Array;
  var TUICookiesEnabled = -1;

  // Internal function to demangle cookies
  function TUI_demangle(value) {
    var pair;
    var pairs = value.split(",");
    for (i = 0; i < pairs.length; i++) {
      pair = pairs[i].split(":");
      if (pair[0] != null && pair[1] != null)
        TUIClasses[pair[0]] = pair[1];
    }
  }

 /*  TUI_tweak: Function to redraw whole document. 
  *  Also, initialize TUIClasses array with defaults, then override it 
  *  with values from cookie
  */

  function TUI_tweak( cookiesuffix, classes  ) {
    var dc = document.cookie;
    var begin = -1;
    var end = 0;

    // Register classes and their defaults
    TUI_demangle(classes);

    if (TUICookiesEnabled > 0) {
      // If cookies enabled, process them
      TUI_demangle(TUI_getCookie(cookiesuffix));
    }
    else if (TUICookiesEnabled == -1) {
      // If cookies availability not checked yet since browser does 
      // not has navigator.cookieEnabled property, let's check it manualy
      var cookie = TUI_getCookie(cookiesuffix);
      if (cookie.length == 0)
      {
        TUI_setCookie(cookiesuffix);
	// Cookies are definitely disabled for JS.
        if (TUI_getCookie(cookiesuffix).length == 0)
          TUICookiesEnabled = 0;
        else
          TUICookiesEnabled = 1;
      }
      else {
        // Have cookie set, pretend to be able to reset them later on
        TUI_demangle(cookie);
        TUICookiesEnabled = 1;
      }
    }
      
    if (TUICookiesEnabled > 0) {
      var els = document.getElementsByTagName('*');
      for (i = 0; i < els.length; i++) {
        if (null != TUIClasses[els[i].className]) {
          TUI_apply(els[i], TUIClasses[els[i].className]);
        }
      }
    }
    return;
  }

  // TUI_apply: Function to draw certain element. 
  // Receives element itself and style value: hide, reveal or collapse

  function TUI_apply(element, value) {
    if (TUICookiesEnabled > 0 && element != null) {
      switch (value)
      {
        case 'hide':
          element.style.visibility="hidden";
          break;
        case 'collapse':
          element.style.visibility="hidden";
          element.style.display="none";
          break;
        case 'reveal': // Shown item must expand
        default:     // The default is to show & expand
          element.style.visibility="visible";
          element.style.display="";
          break;
      }
    }
  }

  // TUI_change: Function to process class. 
  // Usualy called from onclick event of button

  function TUI_change(cookiesuffix, clsname, action) {
    if (TUICookiesEnabled > 0) {
      var els, i;
      els = document.getElementsByTagName('*');
      for (i=0; i<els.length; i++) {
        if (els[i].className.match(clsname)) {
          TUI_apply(els[i], action);
        }
      }
      TUIClasses[clsname]=action;
      TUI_setCookie(cookiesuffix);
    }
  }
  
  // TUI_setCookie: Function to set TUI cookie. 
  // Used internally

  function TUI_setCookie(cookiesuffix) {
    var cookieval = "";
    var expireOn = new Date();
    expireOn.setYear(expireOn.getFullYear() + 25); 
    for (clsname in TUIClasses) {
      if (cookieval.length > 0)
        cookieval += ",";
      cookieval += clsname+":"+TUIClasses[clsname];
    }
    document.cookie="Bugzilla_TUI_"+cookiesuffix+"="+cookieval+"; expires="+expireOn.toString();
  }

  // TUI_getCookie: Function to get TUI cookie. 
  // Used internally

  function TUI_getCookie(cookiesuffix) {
    var dc = document.cookie;
    var begin, end;
    var cookiePrefix = "Bugzilla_TUI_"+cookiesuffix+"="; 
    begin = dc.indexOf(cookiePrefix, end);
    if (begin != -1) {
      begin += cookiePrefix.length;
      end = dc.indexOf(";", begin);
      if (end == -1) {
        end = dc.length;
      }
      return unescape(dc.substring(begin, end));
    }
    return "";
  }
