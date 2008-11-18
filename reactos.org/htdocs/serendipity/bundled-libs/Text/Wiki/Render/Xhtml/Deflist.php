<?php

class Text_Wiki_Render_Xhtml_Deflist extends Text_Wiki_Render {
    
    var $conf = array(
        'css_dl' => null,
        'css_dt' => null,
        'css_dd' => null
    );
    
    /**
    * 
    * Renders a token into text matching the requested format.
    * 
    * @access public
    * 
    * @param array $options The "options" portion of the token (second
    * element).
    * 
    * @return string The text rendered from the token options.
    * 
    */
    
    function token($options)
    {
        $type = $options['type'];
        $pad = "    ";
        
        switch ($type) {
        
        case 'list_start':
            $css = $this->formatConf(' class="%s"', 'css_dl');
            return "<dl$css>\n";
            break;
        
        case 'list_end':
            return "</dl>\n\n";
            break;
        
        case 'term_start':
            $css = $this->formatConf(' class="%s"', 'css_dt');
            return $pad . "<dt$css>";
            break;
        
        case 'term_end':
            return "</dt>\n";
            break;
        
        case 'narr_start':
            $css = $this->formatConf(' class="%s"', 'css_dd');
            return $pad . $pad . "<dd$css>";
            break;
        
        case 'narr_end':
            return "</dd>\n";
            break;
        
        default:
            return '';
        
        }
    }
}
?>