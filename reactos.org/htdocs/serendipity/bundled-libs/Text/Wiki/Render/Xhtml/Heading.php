<?php

class Text_Wiki_Render_Xhtml_Heading extends Text_Wiki_Render {
    
    var $conf = array(
        'css_h1' => null,
        'css_h2' => null,
        'css_h3' => null,
        'css_h4' => null,
        'css_h5' => null,
        'css_h6' => null
    );
    
    function token($options)
    {
        // get nice variable names (id, type, level)
        extract($options);
        
        if ($type == 'start') {
            $css = $this->formatConf(' class="%s"', "css_h$level");
            return "<h$level$css id=\"$id\">";
        }
        
        if ($type == 'end') {
            return "</h$level>\n";
        }
    }
}
?>