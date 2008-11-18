<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Klemens Friedl <frik85@reactos.org>

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


//
// The format of this file is ---> $roscms_langres['message'] = 'text';
//

/*
	Ersetzen Sie die folgenden Zeichen durch die entsprechende Zeichenfolge:
	ä => &auml;
	Ä => &Auml;
	ö => &ouml;
	Ö => &Ouml;
	ü => &uuml;
	Ü => &Uuml;
	ß => &szlig;
*/

/*$roscms_langres = array(
"Mainmenu_label1" => "Startseite",
"Mainmenu_label2" => "Medien",
"Mainmenu_label3" => "Support",
"Mainmenu_label4" => "Entwickler",
"Mainmenu_label5" => "meinReactOS"
);*/

// Language settings
$roscms_langres['charset'] = 'utf-8';
//$roscms_langres['charset'] = 'iso-8859-1';
$roscms_langres['lang_code'] = 'DE';


// main menu
$roscms_langres['Navigation'] = 'Navigation';
$roscms_langres['Home'] = 'Startseite';
$roscms_langres['Dev'] = 'Entwicklung';
$roscms_langres['Community'] = 'Community';
$roscms_langres['Support'] = 'Support';
$roscms_langres['myReactOS'] = 'meinReactOS';

// Login bar:
$roscms_langres['Account'] = 'Benutzerkonto';
$roscms_langres['Login'] = 'Login';
$roscms_langres['Logout'] = 'Logout';
$roscms_langres['Global_Login_System'] = 'Login System';
$roscms_langres['Register_Account'] = 'Registrieren';
$roscms_langres['Login_Nick'] = '&nbsp;Nick:&nbsp;';
$roscms_langres['LoginPwd'] = '&nbsp;Pwd:&nbsp;';


// myReactOS menu
$roscms_langres['User_Profil_Overview'] = 'Benutzerprofil &Uuml;bersicht';
$roscms_langres['User_Profil'] = 'Benutzerprofil';
$roscms_langres['Overview'] = '&Uuml;bersicht';
$roscms_langres['Login_System'] = 'Login System';
$roscms_langres['Messages'] = 'Nachrichten';
$roscms_langres['Contacts'] = 'Kontake';
$roscms_langres['Favorites'] = 'Favoriten';
$roscms_langres['Documentation'] = 'Dokumentation';
$roscms_langres['Account_Edit'] = 'Einstellungen';
$roscms_langres['Accountlist'] = 'Benutzerliste';


// myReactOS Content
$roscms_langres['myReactOS_description'] = 'Eine einfach zu bedienende und m&auml;chtige Benutzeroberfl&auml;che, mit der Sie ihr ReactOS Benutzerkonto verwalten k&ouml;nnen.';
$roscms_langres['Account_Information'] = 'Account Informationen';
$roscms_langres['Account_Information_description'] = 'Eine Zusammenfassung ... &lt;placeholder&gt;';

// Right side bar:
$roscms_langres['Latest_Release'] = 'Neueste Ver&ouml;ffentlichung';
$roscms_langres['myReactOS_Developer_Quotes'] = 'Entwickler Zitat';

// Error 404
$roscms_langres['Page_not_found'] = 'Seite nicht gefunden';




// Admin/Dev/Translator Interface
$roscms_langres['Dev_Interface'] = 'Entw Oberfl&auml;che';
$roscms_langres['Dev_Overview'] = '&Uuml;bersicht';
$roscms_langres['Dev_Pages'] = 'Seiten';
$roscms_langres['Dev_Contents'] = 'Inhalte';
$roscms_langres['Dev_Help'] = 'Hilfe';

$roscms_langres['Dev_DeveloperInterfaceOverview'] = 'Entwickler Oberfl&auml;che - &Uuml;bersicht';



$roscms_langres['ContTrans_Interface_Content'] = 'Entwickler Oberfl&auml;che - Inhalte';
$roscms_langres['ContTrans_Contents'] = 'Inhalte';
$roscms_langres['ContTrans_Interface_Pages'] = 'Entwickler Oberfl&auml;che - Seiten';
$roscms_langres['ContTrans_Pages'] = 'Seiten';

$roscms_langres['ContTrans_Profile'] = 'Profil';

$roscms_langres['ContTrans_Action'] = 'Aktion';
$roscms_langres['ContTrans_NewContent'] = 'Neuer Inhalt';
$roscms_langres['ContTrans_NewPage'] = 'Neue Seite';

$roscms_langres['ContTrans_resetfilters'] = 'Filter &amp; Sortierung zur&uuml;cksetzen';
$roscms_langres['ContTrans_Info'] = 'Info';
$roscms_langres['ContTrans_ContentID'] = 'Inhalt ID';
$roscms_langres['ContTrans_PageID'] = 'Seiten ID';
$roscms_langres['ContTrans_Content'] = 'Inhalt';
$roscms_langres['ContTrans_Rev'] = 'Rev.';
$roscms_langres['ContTrans_Date'] = 'Datum';
$roscms_langres['ContTrans_Time'] = 'Zeit';
$roscms_langres['ContTrans_User'] = 'User';
$roscms_langres['ContTrans_Language'] = 'Sprache';
$roscms_langres['ContTrans_International'] = 'Original (English)';
$roscms_langres['ContTrans_All'] = 'Alle';
$roscms_langres['ContTrans_Filter'] = 'Filter';
$roscms_langres['ContTrans_activecontent'] = 'aktive Inhalte';
$roscms_langres['ContTrans_activepages'] = 'aktive Seiten';
$roscms_langres['ContTrans_allcontent'] = 'alle Inhalte';
$roscms_langres['ContTrans_allpages'] = 'alle Seiten';
$roscms_langres['ContTrans_activeandvisiblecontent'] = 'aktive and sichtbare Inhalte';
$roscms_langres['ContTrans_currentuser'] = 'derzeitiger Benutzer';

$roscms_langres['ContTrans_normalpages'] = 'normale Seiten';
$roscms_langres['ContTrans_dynamicpages'] = 'dynamische Seiten';

$roscms_langres['ContTrans_history'] = 'Verlauf';
$roscms_langres['ContTrans_Sortedby'] = 'Sortieren nach';
$roscms_langres['ContTrans_contentid'] = 'inhalt id';
$roscms_langres['ContTrans_pageid'] = 'seiten id';
$roscms_langres['ContTrans_date'] = 'datum';
$roscms_langres['ContTrans_user'] = 'benutzer';
$roscms_langres['ContTrans_active'] = 'aktive';
$roscms_langres['ContTrans_Active'] = 'Aktive';
$roscms_langres['ContTrans_visible'] = 'sichtbar';
$roscms_langres['ContTrans_Visible'] = 'Sichtbar';
$roscms_langres['ContTrans_version'] = 'version';
$roscms_langres['ContTrans_extra'] = 'extra';
$roscms_langres['ContTrans_Extra'] = 'Extra';
$roscms_langres['ContTrans_language'] = 'sprache';
$roscms_langres['ContTrans_editor'] = 'editor';
$roscms_langres['ContTrans_Editor'] = 'Editor';
$roscms_langres['ContTrans_Translate'] = '&Uuml;bersetzen';
$roscms_langres['ContTrans_Edit'] = 'Ansehen / Bearbeiten';
$roscms_langres['ContTrans_Delete'] = 'L&ouml;schen';
$roscms_langres['ContTrans_filter_history'] = 'Filter: Verlauf';

$roscms_langres['ContTrans_ViewEditContent'] = 'Inhalt ansehen / bearbeiten';
$roscms_langres['ContTrans_ViewEditpage'] = 'View / Edit page';
$roscms_langres['ContTrans_SaveContent'] = 'Inhalt speichern';
$roscms_langres['ContTrans_Type'] = 'Type';
$roscms_langres['ContTrans_Description'] = 'Beschreibung';
$roscms_langres['ContTrans_Username'] = 'Benutzername';
$roscms_langres['ContTrans_egAbout'] = 'e.g. &quot;&uuml;ber&quot;';
$roscms_langres['ContTrans_checklang'] = 'Bitte &uuml;berpr&uuml;fen Sie, ob Sie die richtige Sprache ausgew&auml;hlt haben bevor Sie "Submit" klicken!';
$roscms_langres['ContTrans_SAVE'] = '<b>Speichern</b> (neue Version und neuer Datenbank Eintrag)';
$roscms_langres['ContTrans_UPDATE'] = '<b>Aktualisieren</b>  (&uuml;berschreibt den aktuelle Eintrag)';
$roscms_langres['ContTrans_Save1'] = 'Eine neue Version des Inhalts';
$roscms_langres['ContTrans_Save2'] = 'wurde angelegt/gespeichert!';
$roscms_langres['ContTrans_Save3'] = 'Wechsle zum \'Inhalt Bearbeiten\' Dialog';
$roscms_langres['ContTrans_Save4'] = 'Zur&uuml;ck zum \'Inhalt Bearbeiten\' Dialog';
$roscms_langres['ContTrans_Save5'] = 'Diese Seite';
$roscms_langres['ContTrans_Save6'] = 'Seite ansehen (dynamische Vorschau; neues Fenster)';
$roscms_langres['ContTrans_Save7'] = 'Zur&uuml;ck zum \'Seite Bearbeiten\' Dialog';

$roscms_langres['ContTrans_Preview'] = 'Vorschau';
$roscms_langres['ContTrans_PageTitle'] = 'Seiten-Titel';
$roscms_langres['ContTrans_Extention'] = 'Endung';


$roscms_langres['ContTrans_DevPageGenerator'] = 'Entwickler Oberfl&auml;che - Seiten Generator';
$roscms_langres['Dev_PageGenerator'] = 'Seiten Generator';
$roscms_langres['Dev_PageGeneratorOverview'] = 'Seiten Generator &Uuml;bersicht';
$roscms_langres['ContTrans_Change'] = '&Auml;nderung';
$roscms_langres['ContTrans_LatestGen'] = 'Letzte Gen.';
$roscms_langres['ContTrans_UserGen'] = 'Benutzer (Generator)';

$roscms_langres['ContTrans_Gen_desc'] = 'Der Seiten Generator kann statische HTML Seiten generieren wahlweise als Vorschau (Browser) oder als Datei (Server).';
$roscms_langres['ContTrans_Gen_menu1'] = 'Generiert alle GE&Auml;NDERTEN statischen Seiten';
$roscms_langres['ContTrans_Gen_menu2'] = 'Generiert alle statischen Seiten';
$roscms_langres['ContTrans_Gen_menu3'] = 'Generiert/zeigt eine einzelne statische Seite';
$roscms_langres['ContTrans_Gen_menu4'] = 'Seiten Vorschau (dynamisch von der Datenbank)';
$roscms_langres['ContTrans_Gen_menu3_desc'] = 'Generiert eine einzelne statische Seite; z.B.: um eine eine einzelne Seite zu aktualisieren';
$roscms_langres['ContTrans_allactivepages'] = 'all (active) pages';


$roscms_langres['ContTrans_yes'] = 'ja';
$roscms_langres['ContTrans_no'] = 'nein';


?>
