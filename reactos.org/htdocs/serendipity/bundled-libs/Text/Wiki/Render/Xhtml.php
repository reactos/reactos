<?php

class Text_Wiki_Render_Xhtml extends Text_Wiki_Render {
    
    var $conf = array('translate' => HTML_ENTITIES);
    
    function pre()
    {
        // attempt to translate HTML entities in the source before continuing.
        $type = $this->getConf('translate', null);
        
        // are we translating html?
        if ($type !== false && $type !== null) {
        
            // yes! get the translation table.
            $xlate = get_html_translation_table($type);
            
            // remove the delimiter character it doesn't get translated
            unset($xlate[$this->wiki->delim]);
            
            // translate!
            $this->wiki->source = strtr($this->wiki->source, $xlate);
        }
        
    }
    
    function post()
    {
        return;
    }
    
}
?>