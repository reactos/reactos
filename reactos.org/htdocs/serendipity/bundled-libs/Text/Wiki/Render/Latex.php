<?php

/**
* 
* Formats parsed Text_Wiki for LaTeX rendering.
* 
* $Id: Latex.php,v 1.2 2004/09/25 19:05:13 pmjones Exp $
* 
* @author Jeremy Cowgar <jeremy@cowgar.com>
* 
* @package Text_Wiki
* 
* @todo [http://google.com] becomes 1 with a LaTeX footnote in subscript.
*       This should be a normal LaTeX footnote associated with the
*       previous word?
* 
* @todo parse "..." to be ``...''
* 
* @todo parse '...' to be `...'
* 
* @todo move escape_latex to a static function, move escaping to the
*       individual .php files they are associated with
* 
* @todo allow the user to add conf items to do things like
*       + A custom document header
*       + Custom page headings
*       + Include packages
*       + Set Title, Author, Date
*       + Include a title page
*       + Not output Document Head/Foot (maybe combinding many pages?)
* 
*/

class Text_Wiki_Render_Latex extends Text_Wiki_Render {
    function escape_latex ($txt) {
        $txt = str_replace("\\", "\\\\", $txt);
        $txt = str_replace('#', '\#', $txt);
        $txt = str_replace('$', '\$', $txt);
        $txt = str_replace('%', '\%', $txt);
        $txt = str_replace('^', '\^', $txt);
        $txt = str_replace('&', '\&', $txt);
        $txt = str_replace('_', '\_', $txt);
        $txt = str_replace('{', '\{', $txt);
        $txt = str_replace('}', '\}', $txt);
        
        // Typeset things a bit prettier than normas
        $txt = str_replace('~',   '$\sim$', $txt);
        $txt = str_replace('...', '\ldots', $txt);

        return $txt;
    }

    function escape($tok, $ele) {
        if (isset($tok[$ele])) {
            $tok[$ele] = $this->escape_latex($tok[$ele]);
        }

        return $tok;
    }
    
    function pre()
    {
        foreach ($this->wiki->tokens as $k => $tok) {
            if ($tok[0] == 'Code') {
                continue;
            }

            $tok[1] = $this->escape($tok[1], 'text');
            $tok[1] = $this->escape($tok[1], 'page');
            $tok[1] = $this->escape($tok[1], 'href');
            
            $this->wiki->tokens[$k] = $tok;
        }

        $this->wiki->source = $this->escape_latex($this->wiki->source);

        return
            "\\documentclass{article}\n".
            "\\usepackage{ulem}\n".
            "\\pagestyle{headings}\n".
            "\\begin{document}\n";
    }
    
    function post()
    {
        return "\\end{document}\n";
    }
    
}
?>