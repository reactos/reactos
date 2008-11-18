<?php # $Id: lang_ja.inc.php,v 1.4 2005/05/17 11:37:42 garvinhicking Exp $

##########################################################################
# serendipity - another blogger...                                       #
##########################################################################
#                                                                        #
# (c) 2003 Jannis Hermanns <J@hacked.it>                                 #
# http://www.jannis.to/programming/serendipity.html                      #
#                                                                        #
# Translated by                                                          #
# João P. Matos <jmatos@math.ist.utl.pt>                                 #
#                                                                        #
##########################################################################

@define('PLUGIN_EVENT_WEBLOGPING_PING', 'Anunciar as entradas (por ping XML) a:');
@define('PLUGIN_EVENT_WEBLOGPING_SENDINGPING', 'Enviar o ping XML-RPC à máquina %s');
@define('PLUGIN_EVENT_WEBLOGPING_TITLE', 'Anunciar as entradas');
@define('PLUGIN_EVENT_WEBLOGPING_DESC', 'Enviar uma actualização de novas entradas aos serviços de indexação');
@define('PLUGIN_EVENT_WEBLOGPING_SUPERSEDES', '(substitui %s)');
@define('PLUGIN_EVENT_WEBLOGPING_CUSTOM', 'Serviços ping adicionais');
@define('PLUGIN_EVENT_WEBLOGPING_CUSTOM_BLAHBLA', 'Um ou mais serviços ping adicionais, separados por ",". As entradas precisam de ser formatadas como: "máquina.domínio/caminho". Se um "*" for introduzido no princípio do nome de uma máquina, as opções XML-RPC adicionais serão enviadas para essa máquina (se suportadas por essa máquina).');
@define('PLUGIN_EVENT_WEBLOGPING_SEND_FAILURE', 'Falhanço(Razão: %s)');
@define('PLUGIN_EVENT_WEBLOGPING_SEND_SUCCESS', 'Sucesso!!');

/* vim: set sts=4 ts=4 expandtab : */
?>