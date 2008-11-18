<?php # $Id: serendipity_plugin_history.php 235 2005-07-08 13:29:39Z garvinhicking $

        @define('PLUGIN_HISTORY_NAME', 'Geschichte');
        @define('PLUGIN_HISTORY_DESC', 'Zeigt Einträge eines einstellbaren Alters an.');
        @define('PLUGIN_HISTORY_MIN_AGE', 'Mindestalter');
        @define('PLUGIN_HISTORY_MIN_AGE_DESC', 'Mindestalter der Einträge (in Tagen).');
        @define('PLUGIN_HISTORY_MAX_AGE', 'Höchstalter');
        @define('PLUGIN_HISTORY_MAX_AGE_DESC','Höchstalter der Einträge (in Tagen).');
        @define('PLUGIN_HISTORY_MAX_ENTRIES', 'Anzahl');
        @define('PLUGIN_HISTORY_MAX_ENTRIES_DESC', 'Wieviele Einträge sollen maximal angezeigt werden?');
        @define('PLUGIN_HISTORY_SHOWFULL', 'Ganze Einträge');
        @define('PLUGIN_HISTORY_SHOWFULL_DESC', 'Nicht nur Überschriften, sondern ganze Einträge anzeigen.');
        @define('PLUGIN_HISTORY_INTRO', 'Intro');
        @define('PLUGIN_HISTORY_INTRO_DESC', 'Text, der vor den Einträgen angezeigt werden soll');
        @define('PLUGIN_HISTORY_OUTRO', 'Outro');
        @define('PLUGIN_HISTORY_OUTRO_DESC', 'Text, der hinter den Einträgen angezeigt werden soll');
        @define('PLUGIN_HISTORY_DISPLAYDATE', 'Datum anzeigen');
        @define('PLUGIN_HISTORY_DISPLAYDATE_DESC', 'Vor jedem Eintrag das Datum anzeigen?');
        @define('PLUGIN_HISTORY_MAXLENGTH', 'Überschriftenlänge');
        @define('PLUGIN_HISTORY_MAXLENGTH_DESC', 'Nach wievielen Zeichen sollen die Überschriften abgeschnitten werden (0 für garnicht)?');
        @define('PLUGIN_HISTORY_SPECIALAGE', 'Vorgefertigter Zeitrahmen');
        @define('PLUGIN_HISTORY_SPECIALAGE_DESC', 'Wenn Sie statt einem vorgefertigten lieber einen eigenen Zeitraum einstellen möchten, wählen Sie \'Anderer\' aus und füllen die unteren beiden Felder.');
        @define('PLUGIN_HISTORY_SPECIALAGE_YEAR', 'Zeigt Einträge vom selben Datum des letzten Jahres an.');
        @define('PLUGIN_HISTORY_CUSTOMAGE', 'Zeitrahmen selbst einstellen');
        @define('PLUGIN_HISTORY_OYA', 'Heute vor einem Jahr');
        @define('PLUGIN_HISTORY_MYSELF', 'Anderer');
