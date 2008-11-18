<?php # $Id: serendipity_event_contentrewrite.php 235 2005-07-08 13:29:39Z garvinhicking $

        @define('PLUGIN_EVENT_CONTENTREWRITE_FROM', 'quelle');
        @define('PLUGIN_EVENT_CONTENTREWRITE_TO', 'ziel');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NAME', 'Wort-Ersetzer');
        @define('PLUGIN_EVENT_CONTENTREWRITE_DESCRIPTION', 'Ersetzt ein Wort mit einem neuen Inhalt, z.B. für Akronyme');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NEWTITLE', 'Neuer Titel');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NEWTDESCRIPTION', 'Der Akronym-Titel des neuen Eintrages ({quelle})');
        @define('PLUGIN_EVENT_CONTENTREWRITE_OLDTITLE', 'Titel #%d');
        @define('PLUGIN_EVENT_CONTENTREWRITE_OLDTDESCRIPTION', 'Das Akronym ({quelle})');
        @define('PLUGIN_EVENT_CONTENTREWRITE_PTITLE', 'Plugin Titel');
        @define('PLUGIN_EVENT_CONTENTREWRITE_PDESCRIPTION', 'Der Name dieses Pligins');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NEWDESCRIPTION', 'Neue Beschreibung');
        @define('PLUGIN_EVENT_CONTENTREWRITE_NEWDDESCRIPTION', 'Die Beschreibung des neuen Eintrages ({ziel})');
        @define('PLUGIN_EVENT_CONTENTREWRITE_OLDDESCRIPTION', 'Beschreibung #%s');
        @define('PLUGIN_EVENT_CONTENTREWRITE_OLDDDESCRIPTION', 'Die Beschreibung des Eintrages ({ziel})');
        @define('PLUGIN_EVENT_CONTENTREWRITE_REWRITESTRING', 'Umformungsmaske');
        @define('PLUGIN_EVENT_CONTENTREWRITE_REWRITESTRINGDESC', 'Ein beliebiger Text der zur Ersetzung verwendet werden soll. Fügen Sie {quelle} und {ziel} irgendwo in diesem Text ein.' . "\n" . 'Beispiel: <acronym title="{quelle}">{ziel}</acronym>');
        @define('PLUGIN_EVENT_CONTENTREWRITE_REWRITECHAR', 'Rewrite Zeichen');
        @define('PLUGIN_EVENT_CONTENTREWRITE_REWRITECHARDESC', 'Falls es ein besonderes Zeichen geben soll, was die Wort-Ersetzung ausführt, geben Sie es hier an. Falls z.B. nur \'serendipity*\' damit ersetzt werden soll, was Sie als Akronym für \'serendipity\' definiert haben, und das \'*\' soll entfernt werden, dann geben Sie dieses Zeichen an.');
