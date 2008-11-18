<?php

/*

This is Textile
A Humane Web Text Generator

Version 2.0 beta
8 July, 2003

Copyright (c) 2003, Dean Allen, www.textism.com
All rights reserved.

_______
LICENSE

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name Textile nor the names of its contributors may be used to
  endorse or promote products derived from this software without specific
  prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

_____________
USING TEXTILE

Block modifier syntax:

    Header: h(1-6).
    Paragraphs beginning with 'hn. ' (where n is 1-6) are wrapped in header tags.
    Example: h1. Header... -> <h1>Header...</h1>

    Paragraph: p. (also applied by default)
    Example: p. Text -> <p>Text</p>

    Blockquote: bq.
    Example: bq. Block quotation... -> <blockquote>Block quotation...</blockquote>

    Blockquote with citation: bq.:http://citation.url
    Example: bq.:http://textism.com/ Text...
    ->    <blockquote cite="http://textism.com">Text...</blockquote>

    Footnote: fn(1-100).
    Example: fn1. Footnote... -> <p id="fn1">Footnote...</p>

    Numeric list: #, ##
    Consecutive paragraphs beginning with # are wrapped in ordered list tags.
    Example: <ol><li>ordered list</li></ol>

    Bulleted list: *, **
    Consecutive paragraphs beginning with * are wrapped in unordered list tags.
    Example: <ul><li>unordered list</li></ul>

Phrase modifier syntax:

            _emphasis_   ->   <em>emphasis</em>
            __italic__   ->   <i>italic</i>
              *strong*   ->   <strong>strong</strong>
              **bold**   ->   <b>bold</b>
          ??citation??   ->   <cite>citation</cite>
        -deleted text-   ->   <del>deleted</del>
       +inserted text+   ->   <ins>inserted</ins>
         ^superscript^   ->   <sup>superscript</sup>
           ~subscript~   ->   <sub>subscript</sub>
                @code@   ->   <code>computer code</code>
           %(bob)span%   ->   <span class="bob">span</span>

         ==notextile==   ->   leave text alone (do not format)

        "linktext":url   ->   <a href="url">linktext</a>
 "linktext(title)":url   ->   <a href="url" title="title">linktext</a>

            !imageurl!   ->   <img src="imageurl" />
  !imageurl(alt text)!   ->   <img src="imageurl" alt="alt text" />
    !imageurl!:linkurl   ->   <a href="linkurl"><img src="imageurl" /></a>

ABC(Always Be Closing)   ->   <acronym title="Always Be Closing">ABC</acronym>


Table syntax:

    Simple tables:

        |a|simple|table|row|
        |And|Another|table|row|

        |_. A|_. table|_. header|_.row|
        |A|simple|table|row|

    Tables with attributes:

        table{border:1px solid black}.
        {background:#ddd;color:red}. |{}| | | |


Applying Attributes:

    Most anywhere Textile code is used, attributes such as arbitrary css style,
    css classes, and ids can be applied. The syntax is fairly consistent.

    The following characters quickly alter the alignment of block elements:

        <  ->  left align    ex. p<. left-aligned para
        >  ->  right align       h3>. right-aligned header 3
        =  ->  centred           h4=. centred header 4
        <> ->  justified         p<>. justified paragraph

    These will change vertical alignment in table cells:

        ^  ->  top          ex. |^. top-aligned table cell|
        -  ->  middle           |-. middle aligned|
        ~  ->  bottom           |~. bottom aligned cell|

    Plain (parentheses) inserted between block syntax and the closing dot-space
    indicate classes and ids:

        p(hector). paragraph -> <p class="hector">paragraph</p>

        p(#fluid). paragraph -> <p id="fluid">paragraph</p>

        (classes and ids can be combined)
        p(hector#fluid). paragraph -> <p class="hector" id="fluid">paragraph</p>

    Curly {brackets} insert arbitrary css style

        p{line-height:18px}. paragraph -> <p style="line-height:18px">paragraph</p>

        h3{color:red}. header 3 -> <h3 style="color:red">header 3</h3>

    Square [brackets] insert language attributes

        p[no]. paragraph -> <p lang="no">paragraph</p>

        %[fr]phrase% -> <span lang="fr">phrase</span>

    Usually Textile block element syntax requires a dot and space before the block
    begins, but since lists don't, they can be styled just using braces

        #{color:blue} one  ->  <ol style="color:blue">
        # big                   <li>one</li>
        # list                  <li>big</li>
                                <li>list</li>
                               </ol>

    Using the span tag to style a phrase

        It goes like this, %{color:red}the fourth the fifth%
        -> It goes like this, <span style="color:red">the fourth the fifth</span>

*/

        $textile_hlgn = "(?:\<(?!>)|(?<!<)\>|\<\>|\=|[()]+)";
        $textile_vlgn = "[\-^~]";
        $textile_clas = "(?:\([^)]+\))";
        $textile_lnge = "(?:\[[^]]+\])";
        $textile_styl = "(?:\{[^}]+\})";
        $textile_cspn = "(?:\\\\\d+)";
        $textile_rspn = "(?:\/\d+)";
        $textile_a = "(?:$textile_hlgn?$textile_vlgn?|$textile_vlgn?$textile_hlgn?)";
        $textile_s = "(?:$textile_cspn?$textile_rspn?|$textile_rspn?$textile_cspn?)";
        $textile_c = "(?:$textile_clas?$textile_styl?$textile_lnge?|$textile_styl?$textile_lnge?$textile_clas?|$textile_lnge?$textile_styl?$textile_clas?)";
        $textile_pnct = '[\!"#\$%&\'()\*\+,\-\./:;<=>\?@\[\\\]\^_`{\|}\~]';

    function textile($text,$lite='') {

        if (get_magic_quotes_gpc()==1)
            $text = stripslashes($text);

        $text = textile_incomingEntities($text);
        $text = textile_encodeEntities($text);
        $text = textile_fixEntities($text);
        $text = textile_cleanWhiteSpace($text);

        $text = textile_getRefs($text);

        $text = textile_noTextile($text);
        $text = textile_image($text);
        $text = textile_links($text);
        $text = textile_code($text);
        $text = textile_span($text);
        $text = textile_superscript($text);
        $text = textile_footnoteRef($text);
        $text = textile_glyphs($text);
        $text = textile_retrieve($text);

        if ($lite=='') {
            $text = textile_lists($text);
            $text = textile_table($text);
            $text = textile_block($text);
        }

            /* clean up <notextile> */
        $text = preg_replace('/<\/?notextile>/', "",$text);

            /* turn the temp char back to an ampersand entity */
        $text = str_replace("x%x%","&#38;",$text);

        $text = str_replace("<br />","<br />\n",$text);

        return trim($text);
    }

// -------------------------------------------------------------
    function textile_pba($in,$element="") // "parse block attributes"
    {
        global $textile_hlgn,$textile_vlgn,$textile_clas,$textile_styl,$textile_cspn,$textile_rspn,$textile_a,$textile_s,$textile_c;

        $textile_style=''; $textile_class=''; $language=''; $textile_colspan=''; $rowspan=''; $id=''; $textile_atts='';

        if (!empty($in)) {
            $matched = $in;
            if($element=='td'){
                if(preg_match("/\\\\(\d+)/",$matched,$textile_csp)) $textile_colspan=$textile_csp[1];
                if(preg_match("/\/(\d+)/",$matched,$rsp)) $rowspan=$rsp[1];

                if (preg_match("/($textile_vlgn)/",$matched,$vert))
                    $textile_style[] = "vertical-align:".textile_vAlign($vert[1]).";";
            }

            if(preg_match("/\{([^}]*)\}/",$matched,$textile_sty)) {
                $textile_style[]=$textile_sty[1].';';
                $matched = str_replace($textile_sty[0],'',$matched);
            }

            if(preg_match("/\[([^)]+)\]/U",$matched,$lng)) {
                $language=$lng[1];
                $matched = str_replace($lng[0],'',$matched);
            }

            if(preg_match("/\(([^()]+)\)/U",$matched,$textile_cls)) {
                $textile_class=$textile_cls[1];
                $matched = str_replace($textile_cls[0],'',$matched);
            }

            if(preg_match("/([(]+)/",$matched,$pl)) {
                $textile_style[] = "padding-left:".strlen($pl[1])."em;";
                $matched = str_replace($pl[0],'',$matched);
            }
            if(preg_match("/([)]+)/",$matched,$pr)) {
                dump($pr);
                $textile_style[] = "padding-right:".strlen($pr[1])."em;";
                $matched = str_replace($pr[0],'',$matched);
            }

            if (preg_match("/($textile_hlgn)/",$matched,$horiz))
                $textile_style[] = "text-align:".textile_hAlign($horiz[1]).";";

            if (preg_match("/^(.*)#(.*)$/",$textile_class,$ids)) {
                $id = $ids[2];
                $textile_class = $ids[1];
            }

            if($textile_style) $textile_atts.=' style="'.join("",$textile_style).'"';
            if($textile_class) $textile_atts.=' class="'.$textile_class.'"';
            if($language) $textile_atts.=' lang="'.$language.'"';
            if($id) $textile_atts.=' id="'.$id.'"';
            if($textile_colspan) $textile_atts.=' colspan="'.$textile_colspan.'"';
            if($rowspan) $textile_atts.=' rowspan="'.$rowspan.'"';

            return $textile_atts;
        } else {
            return '';
        }
    }

// -------------------------------------------------------------
    function textile_table($text)
    {
        global $textile_a,$textile_c,$textile_s;
        $text = $text."\n\n";
        return preg_replace_callback("/^(?:table(_?$textile_s$textile_a$textile_c)\. ?\n)?^($textile_a$textile_c\.? ?\|.*\|)\n\n/smU",
            "textile_fTable",$text);
    }

// -------------------------------------------------------------
    function textile_fTable($matches)
    {
        global $textile_s,$textile_a,$textile_c;
        $tatts = textile_pba($matches[1],'table');

           foreach(preg_split("/\|$/m",$matches[2],-1,PREG_SPLIT_NO_EMPTY) as $row){
            if (preg_match("/^($textile_a$textile_c\. )(.*)/m",$row,$rmtch)) {
                $ratts = pba($rmtch[1],'tr');
                $row = $rmtch[2];
            } else $ratts = '';

            foreach(explode("|",$row) as $textile_cell){
                $textile_ctyp = "d";
                if (preg_match("/^_/",$textile_cell)) $textile_ctyp = "h";
                if (preg_match("/^(_?$textile_s$textile_a$textile_c\. )(.*)/",$textile_cell,$textile_cmtch)) {
                    $textile_catts = pba($textile_cmtch[1],'td');
                    $textile_cell = $textile_cmtch[2];
                } else $textile_catts = '';

                if(trim($textile_cell)!='')
                    $textile_cells[] = "\t\t\t<t$textile_ctyp$textile_catts>$textile_cell</t$textile_ctyp>";
            }
            $rows[] = "\t\t<tr$ratts>\n".join("\n",$textile_cells)."\n\t\t</tr>";
            unset($textile_cells,$textile_catts);
        }
        return "\t<table$tatts>\n".join("\n",$rows)."\n\t</table>\n\n";
    }


// -------------------------------------------------------------
    function textile_lists($text)
    {
        global $textile_a,$textile_c;
        return preg_replace_callback("/^([#*]+$textile_c .*)$(?![^#*])/smU","textile_fList",$text);
    }

// -------------------------------------------------------------
    function textile_fList($m)
    {
        global $textile_a,$textile_c;
        $text = explode("\n",$m[0]);
        foreach($text as $line){
            $nextline = next($text);
            if(preg_match("/^([#*]+)($textile_a$textile_c) (.*)$/s",$line,$m)) {
                list(,$tl,$textile_atts,$textile_content) = $m;
                $nl = preg_replace("/^([#*]+)\s.*/","$1",$nextline);
                if(!isset($lists[$tl])){
                    $lists[$tl] = true;
                    $textile_atts = textile_pba($textile_atts);
                    $line = "\t<".textile_lT($tl)."l$textile_atts>\n\t<li>".$textile_content;
                } else {
                    $line = "\t\t<li>".$textile_content;
                }

                if ($nl===$tl){
                    $line .= "</li>";
                } elseif($nl=="*" or $nl=="#") {
                    $line .= "</li>\n\t</".textile_lT($tl)."l>\n\t</li>";
                    unset($lists[$tl]);
                }
                if (!$nl) {
                    foreach($lists as $k=>$v){
                        $line .= "</li>\n\t</".textile_lT($k)."l>";
                        unset($lists[$k]);
                    }
                }
            }
            $out[] = $line;
        }
        return join("\n",$out);
    }

// -------------------------------------------------------------
    function textile_lT($in)
    {
        return preg_match("/^#+/",$in) ? 'o' : 'u';
    }

// -------------------------------------------------------------
    function textile_block($text)
    {
        global $textile_a,$textile_c;

        $pre = false;
        $find = array('bq','h[1-6]','fn\d+','p');

        $text = preg_replace("/(.+)\n(?![#*\s|])/",
            "$1<br />", $text);

        $text = explode("\n",$text);
        array_push($text," ");

        foreach($text as $line) {
            if (preg_match('/<pre>/i',$line)) { $pre = true; }
            foreach($find as $tag){
                $line = ($pre==false)
                ?    preg_replace_callback("/^($tag)($textile_a$textile_c)\.(?::(\S+))? (.*)$/",
                        "textile_fBlock",$line)
                :    $line;
            }

            $line = preg_replace('/^(?!\t|<\/?pre|<\/?code|$| )(.*)/',"\t<p>$1</p>",$line);

            $line=($pre==true) ? str_replace("<br />","\n",$line):$line;
            if (preg_match('/<\/pre>/i',$line)) { $pre = false; }

            $out[] = $line;
        }
        return join("\n",$out);
    }

// -------------------------------------------------------------
    function textile_fBlock($m)
    {
#        dump($m);
        list(,$tag,$textile_atts,$textile_cite,$textile_content) = $m;

        $textile_atts = textile_pba($textile_atts);

        if(preg_match("/fn(\d+)/",$tag,$fns)){
            $tag = 'p';
            $textile_atts.= ' id="fn'.$fns[1].'"';
            $textile_content = '<sup>'.$fns[1].'</sup> '.$textile_content;
        }

        $textile_start = "\t<$tag";
        $end = "</$tag>";

        if ($tag=="bq") {
            $textile_cite = textile_checkRefs($textile_cite);
            $textile_cite = ($textile_cite!='') ? ' cite="'.$textile_cite.'"' : '';
            $textile_start = "\t<blockquote$textile_cite>\n\t\t<p";
            $end = "</p>\n\t</blockquote>";
        }

        return "$textile_start$textile_atts>$textile_content$end";
    }


// -------------------------------------------------------------
    function textile_span($text)
    {
        global $textile_c,$textile_pnct;
        $qtags = array('\*\*','\*','\?\?','-','__','_','%','\+','~');

        foreach($qtags as $f) {
            $text = preg_replace_callback(
                "/(?<=^|\s|\>|[[:punct:]]|[{(\[])
                ($f)
                ($textile_c)
                (?::(\S+))?
                (\w.+\w)
                ([[:punct:]]*)
                $f
                (?=[])}]|[[:punct:]]+|\s|$)
            /xmU","textile_fSpan",$text);
        }
        return $text;
    }

// -------------------------------------------------------------
    function textile_fSpan($m)
    {
#        dump($m);
        global $textile_c;
        $qtags = array(
            '*'   => 'b',
            '**'  => 'strong',
            '??'  => 'cite',
            '_'   => 'em',
            '__'  => 'i',
            '-'   => 'del',
            '%'   => 'span',
            '+'   => 'ins',
            '~'   => 'sub');

            list(,$tag,$textile_atts,$textile_cite,$textile_content,$end) = $m;
            $tag = $qtags[$tag];
            $textile_atts = textile_pba($textile_atts);
            $textile_atts.= ($textile_cite!='') ? 'cite="'.$textile_cite.'"' : '';

        return "<$tag$textile_atts>$textile_content$end</$tag>";
    }

// -------------------------------------------------------------
    function textile_links($text)
    {
        global $textile_c;
        return preg_replace_callback('/
            ([\s[{(]|[[:punct:]])?     # $pre
            "                          # start
            ('.$textile_c.')                   # $textile_atts
            ([^"]+)                  # $text
            \s?
            (?:\(([^)]+)\)(?="))?    # $title
            ":
            (\S+\b)                    # $url
            (\/)?                      # $textile_slash
            ([^\w\/;]*)                # $post
            (?=\s|$)
        /Ux',"textile_fLink",$text);
    }

// -------------------------------------------------------------
    function textile_fLink($m)
    {
        list(,$pre,$textile_atts,$text,$title,$url,$textile_slash,$post) = $m;

        $url = textile_checkRefs($url);

        $textile_atts = textile_pba($textile_atts);
        $textile_atts.= ($title!='') ? ' title="'.$title.'"' : '';

        $textile_atts = ($textile_atts!='') ? textile_shelve($textile_atts) : '';

        if ($text=='_') {
            $text = $url;
        }

        if (preg_match('/^http(s?):\/\//i', $url)) {
            $textile_atts.=' class="external"';
        }

        return $pre.'<a href="'.$url.$textile_slash.'"'.$textile_atts.'>'.$text.'</a>'.$post;

    }

// -------------------------------------------------------------
    function textile_getRefs($text)
    {
        return preg_replace_callback("/(?<=^|\s)\[(.+)\]((?:http:\/\/|\/)\S+)(?=\s|$)/U",
            "textile_refs",$text);
    }

// -------------------------------------------------------------
    function textile_refs($m)
    {
        list(,$flag,$url) = $m;
        $GLOBALS['urlrefs'][$flag] = $url;
        return '';
    }

// -------------------------------------------------------------
    function textile_checkRefs($text)
    {
        global $urlrefs;
        return (isset($urlrefs[$text])) ? $urlrefs[$text] : $text;
    }

// -------------------------------------------------------------
    function textile_image($text)
    {
        global $textile_c;
        return preg_replace_callback("/
            \!                   # opening
            (\<|\=|\>)?          # optional alignment atts
            ($textile_c)                 # optional style,class atts
            (?:\. )?             # optional dot-space
            ([^\s(!]+)           # presume this is the src
            \s?                  # optional space
            (?:\(([^\)]+)\))?    # optional title
            \!                   # closing
            (?::(\S+))?          # optional href
            (?=\s|$)             # lookahead: space or end of string
        /Ux","textile_fImage",$text);
    }

// -------------------------------------------------------------
    function textile_fImage($m)
    {
        list(,$textile_algn,$textile_atts,$url) = $m;
        $textile_atts = textile_pba($textile_atts);
        $textile_atts.= ($textile_algn!='') ? ' align="'.textile_iAlign($textile_algn).'"' : '';
        $textile_atts.= (isset($m[4])) ? ' title="'.$m[4].'"' : '';
        $textile_size = @getimagesize($url);
        if($textile_size) $textile_atts.= " $textile_size[3]";

        $href = (isset($m[5])) ? textile_checkRefs($m[5]) : '';
        $url = textile_checkRefs($url);

        $out = '';
        $out.= ($href!='') ? '<a href="'.$href.'">' : '';
        $out.= '<img src="'.$url.'"'.$textile_atts.' />';
        $out.= ($href!='') ? '</a>' : '';

        return $out;
    }

// -------------------------------------------------------------
    function textile_code($text)
    {
        global $textile_pnct;
        return preg_replace_callback("/
            (?:^|(?<=[\s\(])|([[{]))         # 1 open bracket?
            @                                # opening
            (?:\|(\w+)\|)?                   # 2 language
            (.+)                             # 3 code
            @                                # closing
            (?:$|([\]}])|
            (?=[[:punct:]]{1,2}|
            \s))                             # 4 closing bracket?
        /Ux","textile_fCode",$text);
    }

// -------------------------------------------------------------
    function textile_fCode($m)
    {
        list(,$before,$lang,$textile_code,$textile_after) = $m;
        $lang = ($lang!='') ? ' language="'.$lang.'"' : '';
        return $before.'<code'.$lang.'>'.$textile_code.'</code>'.$textile_after;
    }

// -------------------------------------------------------------
    function textile_shelve($val)
    {
        $GLOBALS['shelf'][] = $val;
        return ' <'.count($GLOBALS['shelf']).'>';
    }

// -------------------------------------------------------------
    function textile_retrieve($text)
    {
        global $textile_shelf;
          $i = 0;
        if(is_array($textile_shelf)) {
        foreach($textile_shelf as $r){
            $i++;
            $text = str_replace("<$i>",$r,$text);
        }
        }
            return $text;
    }

// -------------------------------------------------------------
    function textile_incomingEntities($text)
    {
        /*  turn any incoming ampersands into a dummy character for now.
            This uses a negative lookahead for alphanumerics followed by a semicolon,
            implying an incoming html entity, to be skipped */

        return preg_replace("/&(?![#a-z0-9]+;)/i","x%x%",$text);
    }

// -------------------------------------------------------------
    function textile_encodeEntities($text)
    {
        /*  Convert high and low ascii to entities. If multibyte string functions are
            available (on by default in php 4.3+), we convert using unicode mapping as
            defined in the function encode_high(). If not, we use php's nasty
            built-in htmlentities() */

        return (function_exists('mb_encode_numericentity'))
        ?    textile_encode_high($text)
        :    htmlentities($text,ENT_NOQUOTES,"utf-8");
    }

// -------------------------------------------------------------
    function textile_fixEntities($text)
    {
        /*  de-entify any remaining angle brackets or ampersands */
        return str_replace(array("&gt;", "&lt;", "&amp;"),
            array(">", "<", "&"), $text);
    }

// -------------------------------------------------------------
    function textile_cleanWhiteSpace($text)
    {
        $out = str_replace(array("\r\n","\t"), array("\n",''), $text);
        $out = preg_replace("/\n{3,}/","\n\n",$out);
        $out = preg_replace("/\n *\n/","\n\n",$out);
        $out = preg_replace('/"$/',"\" ", $out);
        return $out;
    }

// -------------------------------------------------------------
    function textile_noTextile($text)
    {
        return preg_replace('/(^|\s)==(.*)==(\s|$)?/msU',
            '$1<notextile>$2</notextile>$3',$text);
    }

// -------------------------------------------------------------
    function textile_superscript($text)
    {
        return preg_replace('/\^(.*)\^/mU','<sup>$1</sup>',$text);
    }

// -------------------------------------------------------------
    function textile_footnoteRef($text)
    {
        return preg_replace('/\b\[([0-9]+)\](\s)?/U',
            '<sup><a href="#fn$1">$1</a></sup>$2',$text);
    }

// -------------------------------------------------------------
    function textile_glyphs($text)
    {
            // fix: hackish
        $text = preg_replace('/"\z/',"\" ", $text);

        $glyph_search = array(
        '/([^\s[{(>])?\'(?(1)|(?=\s|s\b|[[:punct:]]))/',        //  single closing
        '/\'/',                                                 //  single opening
        '/([^\s[{(>])?"(?(1)|(?=\s|[[:punct:]]))/',             //  double closing
        '/"/',                                                  //  double opening
        '/\b( )?\.{3}/',                                        //  ellipsis
        '/\b([A-Z][A-Z0-9]{2,})\b(?:[(]([^)]*)[)])/',           //  3+ uppercase acronym
        '/(^|[^"][>\s])([A-Z][A-Z0-9 ]{2,})([^<a-z0-9]|$)/',    //  3+ uppercase caps
        '/\s?--\s?/',                                           //  em dash
        '/\s-\s/',                                              //  en dash
        '/(\d+) ?x ?(\d+)/',                                    //  dimension sign
        '/\b ?[([]TM[])]/i',                                    //  trademark
        '/\b ?[([]R[])]/i',                                     //  registered
        '/\b ?[([]C[])]/i');                                    //  copyright

    $glyph_replace = array(
        '$1&#8217;$2',                          //  single closing
        '&#8216;',                              //  single opening
        '$1&#8221;',                            //  double closing
        '&#8220;',                              //  double opening
        '$1&#8230;',                            //  ellipsis
        '<acronym title="$2">$1</acronym>',     //  3+ uppercase acronym
        '$1<span class="caps">$2</span>$3',     //  3+ uppercase caps
        '&#8212;',                              //  em dash
        ' &#8211; ',                            //  en dash
        '$1&#215;$2',                           //  dimension sign
        '&#8482;',                              //  trademark
        '&#174;',                               //  registered
        '&#169;');                              //  copyright


    $textile_codepre = false;
        /*  if no html, do a simple search and replace... */
    if (!preg_match("/<.*>/",$text)) {
        $text = preg_replace($glyph_search,$glyph_replace,$text);
        return $text;
    } else {
        $text = preg_split("/(<.*>)/U",$text,-1,PREG_SPLIT_DELIM_CAPTURE);
        foreach($text as $line) {
                $offtags = ('code|pre|kbd|notextile');

                    /*  matches are off if we're between <code>, <pre> etc. */
                if (preg_match('/<('.$offtags.')>/i',$line)) $textile_codepre = true;
                if (preg_match('/<\/('.$offtags.')>/i',$line)) $textile_codepre = false;

                if (!preg_match("/<.*>/",$line) && $textile_codepre == false) {
                    $line = preg_replace($glyph_search,$glyph_replace,$line);
                }

                    /* do htmlspecial if between <code> */
                if ($textile_codepre == true) {
                    $line = htmlspecialchars($line,ENT_NOQUOTES,"UTF-8");
                    $line = preg_replace('/&lt;(\/?'.$offtags.')&gt;/',"<$1>",$line);
                }

            $glyph_out[] = $line;
        }
        return join('',$glyph_out);
    }
    }

// -------------------------------------------------------------
    function textile_iAlign($in)
    {
        $vals = array(
            '<'=>'left',
            '='=>'center',
            '>'=>'right');
        return (isset($vals[$in])) ? $vals[$in] : '';
    }

// -------------------------------------------------------------
    function textile_hAlign($in)
    {
        $vals = array(
            '<'=>'left',
            '='=>'center',
            '>'=>'right',
            '<>'=>'justify');
        return (isset($vals[$in])) ? $vals[$in] : '';
    }

// -------------------------------------------------------------
    function textile_vAlign($in)
    {
        $vals = array(
            '^'=>'top',
            '-'=>'middle',
            '~'=>'bottom');
        return (isset($vals[$in])) ? $vals[$in] : '';
    }

// -------------------------------------------------------------
    function textile_encode_high($text,$textile_charset="UTF-8")
    {
        $textile_cmap = textile_cmap();
        return mb_encode_numericentity($text, $textile_cmap, $textile_charset);
    }

// -------------------------------------------------------------
    function textile_decode_high($text,$textile_charset="UTF-8")
    {
        $textile_cmap = textile_cmap();
        return mb_decode_numericentity($text, $textile_cmap, $textile_charset);
    }

// -------------------------------------------------------------
    function textile_cmap()
    {
        $f = 0xffff;
        $textile_cmap = array(
         160,  255,  0, $f,
         402,  402,  0, $f,
         913,  929,  0, $f,
         931,  937,  0, $f,
         945,  969,  0, $f,
         977,  978,  0, $f,
         982,  982,  0, $f,
         8226, 8226, 0, $f,
         8230, 8230, 0, $f,
         8242, 8243, 0, $f,
         8254, 8254, 0, $f,
         8260, 8260, 0, $f,
         8465, 8465, 0, $f,
         8472, 8472, 0, $f,
         8476, 8476, 0, $f,
         8482, 8482, 0, $f,
         8501, 8501, 0, $f,
         8592, 8596, 0, $f,
         8629, 8629, 0, $f,
         8656, 8660, 0, $f,
         8704, 8704, 0, $f,
         8706, 8707, 0, $f,
         8709, 8709, 0, $f,
         8711, 8713, 0, $f,
         8715, 8715, 0, $f,
         8719, 8719, 0, $f,
         8721, 8722, 0, $f,
         8727, 8727, 0, $f,
         8730, 8730, 0, $f,
         8733, 8734, 0, $f,
         8736, 8736, 0, $f,
         8743, 8747, 0, $f,
         8756, 8756, 0, $f,
         8764, 8764, 0, $f,
         8773, 8773, 0, $f,
         8776, 8776, 0, $f,
         8800, 8801, 0, $f,
         8804, 8805, 0, $f,
         8834, 8836, 0, $f,
         8838, 8839, 0, $f,
         8853, 8853, 0, $f,
         8855, 8855, 0, $f,
         8869, 8869, 0, $f,
         8901, 8901, 0, $f,
         8968, 8971, 0, $f,
         9001, 9002, 0, $f,
         9674, 9674, 0, $f,
         9824, 9824, 0, $f,
         9827, 9827, 0, $f,
         9829, 9830, 0, $f,
         338,  339,  0, $f,
         352,  353,  0, $f,
         376,  376,  0, $f,
         710,  710,  0, $f,
         732,  732,  0, $f,
         8194, 8195, 0, $f,
         8201, 8201, 0, $f,
         8204, 8207, 0, $f,
         8211, 8212, 0, $f,
         8216, 8218, 0, $f,
         8218, 8218, 0, $f,
         8220, 8222, 0, $f,
         8224, 8225, 0, $f,
         8240, 8240, 0, $f,
         8249, 8250, 0, $f,
         8364, 8364, 0, $f);
        return $textile_cmap;
    }


// -------------------------------------------------------------
    function textile_popup_help($name,$helpvar,$windowW,$windowH) {
        return ' <a target="_blank" href="http://www.textpattern.com/help/?item='.$helpvar.'" onclick="window.open(this.href, \'popupwindow\', \'width='.$windowW.',height='.$windowH.',scrollbars,resizable\'); return false;">'.$name.'</a><br />';

        return $out;
    }

// -------------------------------------------------------------
    function textile_txtgps($thing)
    {
        if (isset($_POST[$thing])){
            if (get_magic_quotes_gpc()==1){
                return stripslashes($_POST[$thing]);
            } else {
                return $_POST[$thing];
            }
        } else {
            return '';
        }
    }


// -------------------------------------------------------------
// The following functions are used to detextile html, a process
// still in development.


// -------------------------------------------------------------
    function textile_detextile($text) {

    $text = preg_replace("/<br \/>\s*/","\n",$text);

    $oktags = array('p','ol','ul','li','i','b','em','strong','span','a','h[1-6]',
        'table','tr','td','u','del','sup','sub','blockquote');

    foreach($oktags as $tag){
        $text = preg_replace_callback("/\t*<(".$tag.")\s*([^>]*)>(.*)<\/\\1>/Usi",
        "textile_processTag",$text);
    }

        $glyphs = array(
            '&#8217;'=>'\'',        # single closing
            '&#8216;'=>'\'',        # single opening
            '&#8221;'=>'"',         # double closing
            '&#8220;'=>'"',         # double opening
            '&#8212;'=>'--',        # em dash
            '&#8211;'=>' - ',       # en dash
            '&#215;' =>'x',         # dimension sign
            '&#8482;'=>'(TM)',      # trademark
            '&#174;' =>'(R)',       # registered
            '&#169;' =>'(C)',       # copyright
            '&#8230;'=>'...'        # ellipsis
        );

        foreach($glyphs as $f=>$r){
            $text = str_replace($f,$r,$text);
        }

        $list = false;

        $text = preg_split("/(<.*>)/U",$text,-1,PREG_SPLIT_DELIM_CAPTURE);
            foreach($text as $line){

            if ($list == false && preg_match('/<ol/',$line)){
                $line = "";
                $list = "o";
            } else if (preg_match('/<\/ol/',$line)){
                $line = "";
                $list = false;
            } else if ($list == false && preg_match('/<ul/',$line)){
                $line = "";
                $list = "u";
            } else if (preg_match('/<\/ul/',$line)){
                $line = "";
                $list = false;
            } else if ($list == 'o'){
                $line = preg_replace('/<li.*>/U','# ', $line);
            } else if ($list == 'u'){
                $line = preg_replace('/<li.*>/U','* ', $line);
            }
            $glyph_out[] = $line;
        }

        $text = implode('',$glyph_out);

        $text = preg_replace('/^\t* *p\. /m','',$text);

        return textile_decode_high($text);
    }


// -------------------------------------------------------------
    function textile_processTag($matches)
    {
        list($textile_all,$tag,$textile_atts,$textile_content) = $matches;
        $textile_a = textile_splat($textile_atts);
#        dump($tag); dump($textile_content); dump($textile_a);

        $phr = array(
        'em'=>'_',
        'i'=>'__',
        'b'=>'**',
        'strong'=>'*',
        'cite'=>'??',
        'del'=>'-',
        'ins'=>'+',
        'sup'=>'^',
        'sub'=>'~',
        'span'=>'%');

        $blk = array('p','h1','h2','h3','h4','h5','h6');

        if(isset($phr[$tag])) {
            return $phr[$tag].textile_sci($textile_a).$textile_content.$phr[$tag];
        } elseif($tag=='blockquote') {
            return 'bq.'.textile_sci($textile_a).' '.$textile_content;
        } elseif(in_array($tag,$blk)) {
            return $tag.textile_sci($textile_a).'. '.$textile_content;
        } elseif ($tag=='a') {
            $t = textile_filterAtts($textile_a,array('href','title'));
            $out = '"'.$textile_content;
            $out.= (isset($t['title'])) ? ' ('.$t['title'].')' : '';
            $out.= '":'.$t['href'];
            return $out;
        } else {
            return $textile_all;
        }
    }

// -------------------------------------------------------------
    function textile_filterAtts($textile_atts,$ok)
    {
        foreach($textile_atts as $textile_a) {
            if(in_array($textile_a['name'],$ok)) {
                if($textile_a['att']!='') {
                $out[$textile_a['name']] = $textile_a['att'];
                }
            }
        }
#        dump($out);
        return $out;
    }

// -------------------------------------------------------------
    function textile_sci($textile_a)
    {
        $out = '';
        foreach($textile_a as $t){
            $out.= ($t['name']=='class') ? '(='.$t['att'].')' : '';
            $out.= ($t['name']=='id') ? '[='.$t['att'].']' : '';
            $out.= ($t['name']=='style') ? '{='.$t['att'].'}' : '';
            $out.= ($t['name']=='cite') ? ':'.$t['att'] : '';
        }
        return $out;
    }

// -------------------------------------------------------------
    function textile_splat($textile_attr)  // returns attributes as an array
    {
        $textile_arr = array();
        $textile_atnm = '';
        $mode = 0;

        while (strlen($textile_attr) != 0){
            $ok = 0;
            switch ($mode) {
                case 0: // name
                    if (preg_match('/^([a-z]+)/i', $textile_attr, $match)) {
                        $textile_atnm = $match[1]; $ok = $mode = 1;
                        $textile_attr = preg_replace('/^[a-z]+/i', '', $textile_attr);
                    }
                break;

                case 1: // =
                    if (preg_match('/^\s*=\s*/', $textile_attr)) {
                        $ok = 1; $mode = 2;
                        $textile_attr = preg_replace('/^\s*=\s*/', '', $textile_attr);
                    break;
                    }
                    if (preg_match('/^\s+/', $textile_attr)) {
                        $ok = 1; $mode = 0;
                        $textile_arr[] = array('name'=>$textile_atnm,'whole'=>$textile_atnm,'att'=>$textile_atnm);
                        $textile_attr = preg_replace('/^\s+/', '', $textile_attr);
                    }
                break;

                case 2: // value
                    if (preg_match('/^("[^"]*")(\s+|$)/', $textile_attr, $match)) {
                        $textile_arr[]=array('name' =>$textile_atnm,'whole'=>$textile_atnm.'='.$match[1],
                                'att'=>str_replace('"','',$match[1]));
                        $ok = 1; $mode = 0;
                        $textile_attr = preg_replace('/^"[^"]*"(\s+|$)/', '', $textile_attr);
                    break;
                    }
                    if (preg_match("/^('[^']*')(\s+|$)/", $textile_attr, $match)) {
                        $textile_arr[]=array('name' =>$textile_atnm,'whole'=>$textile_atnm.'='.$match[1],
                                'att'=>str_replace("'",'',$match[1]));
                        $ok = 1; $mode = 0;
                        $textile_attr = preg_replace("/^'[^']*'(\s+|$)/", '', $textile_attr);
                    break;
                    }
                    if (preg_match("/^(\w+)(\s+|$)/", $textile_attr, $match)) {
                        $textile_arr[]=
                            array('name'=>$textile_atnm,'whole'=>$textile_atnm.'="'.$match[1].'"',
                                'att'=>$match[1]);
                        $ok = 1; $mode = 0;
                        $textile_attr = preg_replace("/^\w+(\s+|$)/", '', $textile_attr);
                    }
                break;
            }
            if ($ok == 0){
                $textile_attr = preg_replace('/^\S*\s*/', '', $textile_attr);
                $mode = 0;
            }
        }
        if ($mode == 1) $textile_arr[] =
                array ('name'=>$textile_atnm,'whole'=>$textile_atnm.'="'.$textile_atnm.'"','att'=>$textile_atnm);

        return $textile_arr;
    }
?>
