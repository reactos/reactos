<?php # $Id: serendipity_event_trackexits.php 308 2005-07-27 15:25:35Z wesley $

        @define('PLUGIN_EVENT_TRACKBACK_NAME', 'Textformatierung: Externe Links zählen');
        @define('PLUGIN_EVENT_TRACKBACK_DESC', 'Click auf Externe Links verfolgen');
        @define('PLUGIN_EVENT_TRACKBACK_COMMENTREDIRECTION', 'URL von Kommentatoren maskieren?');
        @define('PLUGIN_EVENT_TRACKBACK_COMMENTREDIRECTION_BLAHBLA', 'Verhindert Spam-Missbrauch aber auch positiven Nutzen von Verlinkungen innerhalb Blogs. Wenn der Wert auf "s9y" gesetzt wird, werden interne Routinen zur Weiterleitung verwendet. Bei dem Wert "google" wird Google verwendet. Ein leerer Wert schaltet die Weiterleitung aus(Standard).');
        @define('PLUGIN_EVENT_TRACKBACK_COMMENTREDIRECTION_NONE', 'Keine');
        @define('PLUGIN_EVENT_TRACKBACK_COMMENTREDIRECTION_S9Y', 'Serendipity Exit-Tracking Routine');
        @define('PLUGIN_EVENT_TRACKBACK_COMMENTREDIRECTION_GOOGLE', 'Google PageRank Deflector');
