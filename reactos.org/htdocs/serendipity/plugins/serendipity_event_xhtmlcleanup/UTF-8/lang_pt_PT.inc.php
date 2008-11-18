<?php # $Id: lang_ja.inc.php,v 1.4 2005/05/17 11:37:42 garvinhicking Exp $

##########################################################################
# serendipity - another blogger...                                       #
##########################################################################
#                                                                        #
# (c) 2003 Jannis Hermanns <J@hacked.it>                                 #
# http://www.jannis.to/programming/serendipity.html                      #
#                                                                        #
# Translated by                                                          #
# JOão P. Matos <jmatos@math.ist.utl.pt>                                 #
#                                                                        #
##########################################################################

@define('PLUGIN_EVENT_XHTMLCLEANUP_NAME', 'Correcção de erros XHTML frequentes');
@define('PLUGIN_EVENT_XHTMLCLEANUP_DESC', 'Permite de corrigir automaticamente uma boa parte dos erros de XHTML em entradas. Ajuda a manter o seu blogue com XHTML correcto.');
@define('PLUGIN_EVENT_XHTMLCLEANUP_XHTML', 'Codificar dados de XML interpretados?');
@define('PLUGIN_EVENT_XHTMLCLEANUP_XHTML_DESC', 'Usa um método de interpretação de XML para assegurar a validade XHTML do seu código. Esta interpretação de xml pode converter entradas já válidas em entidades não escapadas, de maneira que a codificação é posterior à interpretação. Ponha esta marca em OFF se isto introduz codificação dupla no seu caso!');
@define('PLUGIN_EVENT_XHTMLCLEANUP_UTF8', 'Limpar entidades UTF-8?');
@define('PLUGIN_EVENT_XHTMLCLEANUP_UTF8_DESC', 'Se activo, entidades HTML derivadas de caracteres UTF-8 são convertidas correctamente e não duplamente codificadas.');

/* vim: set sts=4 ts=4 expandtab : */
?>