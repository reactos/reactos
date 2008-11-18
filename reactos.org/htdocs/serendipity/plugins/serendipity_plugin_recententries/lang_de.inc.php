<?php # $Id: serendipity_plugin_shoutbox.php 235 2005-07-08 13:29:39Z garvinhicking $

        @define('PLUGIN_RECENTENTRIES_TITLE', 'Aktuelle Einträge');
        @define('PLUGIN_RECENTENTRIES_BLAHBLAH', 'Zeigt die Titel der aktuellsten Einträge mit Datum');
        @define('PLUGIN_RECENTENTRIES_NUMBER', 'Anzahl der Einträge');
        @define('PLUGIN_RECENTENTRIES_NUMBER_BLAHBLAH', 'Wieviele Einträge sollen angezeigt werden? (Standard: 10)');
        @define('PLUGIN_RECENTENTRIES_NUMBER_FROM', 'Angezeigte Einträge überspringen');
        @define('PLUGIN_RECENTENTRIES_NUMBER_FROM_DESC', 'Nur die neuesten Einträge, die nicht schon auf der Hauptseite zu sehen sind, werden angezeigt. (Default: die neuesten ' . $serendipity['fetchLimit'] . ' werden übersprungen)');
        @define('PLUGIN_RECENTENTRIES_NUMBER_FROM_RADIO_ALL', 'Alle anzeigen');
        @define('PLUGIN_RECENTENTRIES_NUMBER_FROM_RADIO_RECENT', 'Einträge auf der Hauptseite überspringen');
