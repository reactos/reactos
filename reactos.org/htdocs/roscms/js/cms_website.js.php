<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    */

  define('ROSCMS_PATH', '../');
  require('../lib/RosCMS_Autoloader.class.php');
  require('../login.php');
  global $roscms_intern_account_id;
?>
										function filtpopulatehelper(objidval, objidval2, filterid) {
											var filtentryselstr = '';
											var filtentryselstrs1 = '';
											var filtentryselstrs2 = '';
											
											//alert('objidval: '+objidval);
											
											if (objidval2 == 0 && roscms_access_level == 1) { // hidden filter entries don't need a combobox (only for SecLev = 1 user) 
												filtentryselstrs1 = '<input type="hidden" name="sfb'+filterid+'" id="sfb'+filterid+'" value="" />';
												filtentryselstrs2 = '<input type="hidden" name="sfc'+filterid+'" id="sfc'+filterid+'" value="" />';
											}
											else {
												switch(objidval) {
													case 'k': /* kind */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option>';
                            if (roscms_access_level > 1) {
                              filtentryselstrs1 += '<option value="no"'+roscms_cbm_hide+'>is not</option>';
                            }
                            filtentryselstrs1 += '</select>';
														filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="stable">Stable</option><option value="new">New</option><option value="draft">Draft</option><option value="unknown">Unknown or no status</option>';
                            if (roscms_access_level > 1) {
                              filtentryselstrs2 += '<option value="archive"+roscms_cbm_hide+>Archive</option>';
                            }
                            filtentryselstrs2 += '</select>';
														break;
													case 'y': /* type */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option>';
                            if (roscms_access_level > 1) {
                              filtentryselstrs1 += '<option value="no"+roscms_cbm_hide+>is not</option>';
                            }
                            filtentryselstrs1 += '</select>';
														filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="page">Page</option><option value="content">Content</option><option value="template">Template</option><option value="script">Script</option><option value="system">System</option></select>';
														break;
													case 's': /* starred */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option></select>';
														filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="true">on</option><option value="false">off</option></select>';
														break;
													case 'd': /* date */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><option value="no">is not</option><option value="sm">is smaller</option><option value="la">is larger</option></select>';
														filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="20" maxlength="50" />&nbsp;&nbsp;(e.g. 2007-02-22)';
														break;
													case 't': /* time */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><option value="no">is not</option><option value="sm">is smaller</option><option value="la">is larger</option></select>';
														filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="20" maxlength="50" />&nbsp;&nbsp;(e.g. 15:30)';
														break;
													case 'l': /* language */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><option value="no">is not</option></select>';
														filtentryselstrs2 = '<select id="sfc'+filterid+'"><?php

  $user_lang = ROSUser::getLanguage($roscms_intern_account_id, true);

  $stmt=DBConnection::getInstance()->prepare("SELECT lang_id, lang_name FROM languages WHERE lang_level > '0' ORDER BY lang_name ASC");
  $stmt->execute();
  while($language=$stmt->fetch()) {
    echo '<option value="'.$language['lang_id'].'"';

    if ($language['lang_id'] == $user_lang) {
      echo ' selected="selected"';
    }

    echo '>'.$language['lang_name'].'</option>';
  }
 ?></select>';
														break;
													case 'r': /* translate */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">to</option></select>';
														filtentryselstrs2 = '<select id="sfc'+filterid+'"><?php
  $stmt=DBConnection::getInstance()->prepare("SELECT lang_id, lang_level, lang_name FROM languages WHERE lang_level > '0'ORDER BY lang_name ASC");
  $stmt->execute();
  while($language=$stmt->fetch()) {
    if ($language['lang_level'] != '10') {
      echo '<option value="'.$language['lang_id'].'"';

      if ($language['lang_id'] == $user_lang) {
        echo ' selected="selected"';
      }
      echo '>'.$language['lang_name'].'</option>';
    }
  }
 ?></select>';
														break;
													case 'u': /* user */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option>';
                            if (roscms_access_level > 1) {
                              filtentryselstrs1 += '<option value="no"+roscms_cbm_hide+>is not</option>';
                            }
                            filtentryselstrs1 += '</select>';
														filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="20" maxlength="50" />&nbsp;&nbsp;(e.g. John Doe)';
														break;
													case 'v': /* version */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><option value="no">is not</option><option value="sm">is smaller</option><option value="la">is larger</option></select>';
														filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="5" maxlength="10" />&nbsp;&nbsp;(e.g. 12)';
														break;
													case 'c': /* column */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option></select>';
														filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="language">Language</option><option value="user">User</option><option value="type">Type</option><option value="version">Version</option>';
                            if (roscms_access_level > 1) {
                              filtentryselstrs2 += '<option value="security"+roscms_cbm_hide+>Security</option><option value="rights"+roscms_cbm_hide+>Rights</option>';
                            }
                            filtentryselstrs2 += '</select>';
														break;
													case 'o': /* order by */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="asc">Ascending</option><option value="desc">Descending</option></select>';
														filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="datetime">Date &amp; Time</option><option value="name">Name</option><option value="lang">Language</option><option value="usr">User</option><option value="type">Type</option><option value="ver">Version</option><option value="nbr">Number ("dynamic" entry)</option>';
                            if (roscms_access_level > 1) {
                              filtentryselstrs2 += '<option value="security"+roscms_cbm_hide+>Security</option><option value="revid"+roscms_cbm_hide+>RevID</option><option value="ext"+roscms_cbm_hide+>Extension</option><option value="status"+roscms_cbm_hide+>Status</option><option value="kind"+roscms_cbm_hide+>Kind</option>';
                            }
                            filtentryselstrs2 += '</select>';
														break;
													case 'i': /* security (ACL) */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><option value="no">is not</option></select>';
														filtentryselstrs2 = '<select id="sfc'+filterid+'"><?php
  $stmt=DBConnection::getInstance()->prepare("SELECT sec_name, sec_fullname FROM data_security ORDER BY sec_fullname ASC");
  $stmt->execute();
  while($ACL=$stmt->fetch()) {
    echo '<option value="'. $ACL['sec_name'] .'">'. $ACL['sec_fullname'] .'</option>';
  }
 ?></select>';
														break;
													case 'm': /* metadata */
														filtentryselstrs1 = '<input id="sfb'+filterid+'" type="text" value="" size="10" maxlength="50" />';
														filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="20" maxlength="50" />&nbsp;&nbsp;(entry: value)';
														break;
													case 'n': /* name */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option>';
                            if (roscms_access_level > 1) {
                              filtentryselstrs1 += '<option value="no"+roscms_cbm_hide+>is not</option><option value="likea"+roscms_cbm_hide+>is like *...*</option>';
                            }
                            filtentryselstrs1 += '<option value="likeb">is like ...*</option></select>';
														filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="20" maxlength="50" />&nbsp;&nbsp;(e.g. about)';
														break;
													case 'a': /* tag */
														filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option>';
                            if (roscms_access_level > 1) {
                              filtentryselstrs1 += '<option value="no"+roscms_cbm_hide+>is not</option>';
                            }
                            filtentryselstrs1 +='</select>';
														filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="15" maxlength="30" />&nbsp;&nbsp;(e.g. todo)';
														break;
														case 'e': /* system */
                              if (roscms_access_level == 3) {
                                filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="dataid">Data-ID</option><option value="revid">Rev-ID</option><option value="usrid">User-ID</option><option value="langid">Lang-ID</option></select>';
                                filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="15" maxlength="30" />';
                              }
															break;
												}
											}
												
											filtentryselstr += '<span id="sfz'+filterid+'">';
											filtentryselstr += filtentryselstrs1;
											filtentryselstr += '</span>&nbsp;';
											filtentryselstr += '<span id="sfy'+filterid+'">';
											filtentryselstr += filtentryselstrs2;
											filtentryselstr += '</span>';
											filtentryselstr += '<span id="sfx'+filterid+'">';
											filtentryselstr += '<input name="sfd'+filterid+'" type="hidden" value="'+objidval2+'" />';
											filtentryselstr += '</span>';
																					
											return filtentryselstr;
										}
										
										function filtpopulate(filtpopstr) {
											var lstfilterstr = '';
											var lstfilterstr2 = '';

											document.getElementById('filtersct').innerHTML = '';
											filtercounter = 0;
											filterid = 0;
											
											if (filtpopstr != '') {
												//alert(filtpopstr);
												var filtpopstr2 = '';
												var indexid = '';
												var filtvisibility = '';
												filtpopstr2 = filtpopstr.split('|');
												//alert(filtpopstr2[0]);
												
												for (var i=0; i < filtpopstr2.length; i++) {
													lstfilterstr2 = '';
													lstfilterstr2 = filtpopstr2[i].split('_');	
													
													if (lstfilterstr2[3] == 0) {
														filtvisibility = 0;
														if (roscms_access_level > 1) {
                              lstfilterstr +=  '<span style="font-style: italic;">';
														} else {
                              lstfilterstr +=  '<span style="display: none">';
														}
													}
													else {
														filtvisibility = 1;
														lstfilterstr +=  '<span style="font-style: normal;">';
													}
													
													indexid = i + 1;


													lstfilterstr +=  '<div id="filt'+indexid+'" class="filterbar2">and&nbsp;';

													if (lstfilterstr2[3] == 0 && roscms_access_level == 1) { // hidden filter entries don't need a combobox (only for SecLev = 1 user) 
														lstfilterstr +=  '<input type="hidden" name="sfa'+indexid+'" id="sfa'+indexid+'" value="" />';
													}
													else {
														lstfilterstr +=  '<select id="sfa'+indexid+'" onchange="filtentryselect(this.id)">';
																if (roscms_access_level > 1) { 
																	lstfilterstr += '<option value="k"+roscms_cbm_hide+>Status</option>';
																	lstfilterstr += '<option value="y"+roscms_cbm_hide+>Type</option>';
																} 
																lstfilterstr += '<option value="n">Name</option>';
																lstfilterstr += '<option value="v">Version</option>';
																lstfilterstr += '<option value="s">Starred</option>';
																lstfilterstr += '<option value="a">Tag</option>';
																lstfilterstr += '<option value="l">Language</option>';
																if (roscms_access_level > 1) {
																	lstfilterstr += '<option value="r"+roscms_cbm_hide+>Translate</option>';
																	lstfilterstr += '<option value="i"+roscms_cbm_hide+>Security</option>';
																	lstfilterstr += '<option value="m"+roscms_cbm_hide+>Metadata</option>';
																	lstfilterstr += '<option value="u"+roscms_cbm_hide+>User</option>';
																}
																if (roscms_access_level == 3) {
																	lstfilterstr += '<option value="e"+roscms_cbm_hide+>System</option>';
																}
																lstfilterstr += '<option value="d">Date</option>';
																lstfilterstr += '<option value="t">Time</option>';
																lstfilterstr += '<option value="c">Column</option>';
																lstfilterstr += '<option value="o">Order</option>';
														lstfilterstr += '</select>&nbsp;';
													}

													lstfilterstr +=  filtpopulatehelper(lstfilterstr2[0], lstfilterstr2[3], indexid);
													lstfilterstr +=  '&nbsp;&nbsp;&nbsp;<span id="fdel'+indexid+'" class="filterbutton" onclick="filtentrydel(this.id)"><img src="images/remove.gif" alt="" style="width:11px; height:11px; border:0px;" />&nbsp;Delete</span>';
													lstfilterstr +=  '</div>';

													
													if (lstfilterstr2[3] == 0) {
														lstfilterstr +=  '<span id="sfv'+indexid+'" class="filthidden"></span>'; // store visibility-status
													}
													else {
														lstfilterstr +=  '<span id="sfv'+indexid+'" class="filtvisible"></span>'; // store visibility-status
													}
													lstfilterstr +=  '</span>';
												}
											
												document.getElementById('filtersct').innerHTML = lstfilterstr;	
												
												
												for (var i=0; i < filtpopstr2.length; i++) {
													lstfilterstr2 = '';
													lstfilterstr2 = filtpopstr2[i].split('_');	
													
													//alert(lstfilterstr2[0]);
													
													indexid = i + 1;
													
													document.getElementById('sfa'+indexid).value = lstfilterstr2[0];
													if (lstfilterstr2[1] != '') {
														document.getElementById('sfb'+indexid).value = lstfilterstr2[1];
													}
													if (lstfilterstr2[2] != '') {
														document.getElementById('sfc'+indexid).value = lstfilterstr2[2];
													}
												}
												filtercounter = filtpopstr2.length;
												
												//alert('populate_counter: '+filtercounter);
											}
										}