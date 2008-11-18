<?php # $Id:$

##########################################################################
# serendipity - another blogger...                                       #
##########################################################################
#                                                                        #
# (c) 2003 Jannis Hermanns <J@hacked.it>                                 #
# http://www.jannis.to/programming/serendipity.html                      #
#                                                                        #
# Translated by                                                          #
# João P Matos <jmatos@math.ist.utl.pt>                                  #
#                                                                        #
##########################################################################

@define('PLUGIN_EVENT_SPARTACUS_NAME', 'Spartacus');
@define('PLUGIN_EVENT_SPARTACUS_DESC', '[S]erendipity [P]lugin [A]ccess [R]epository [T]ool [A]nd [C]ustomization/[U]nification [S]ystem - Permite obter plugins directamente dos arquivos oficiais do Serendipity.');
@define('PLUGIN_EVENT_SPARTACUS_FETCH', 'Prima aqui para carregar um novo %s do arquivo oficial do Serendipity');
@define('PLUGIN_EVENT_SPARTACUS_FETCHERROR', 'Impossível aceder ao endereço %s. Pode ser que o servidor do Serendipity ou de SourceForge.net esteja temporariamente inacessível. Tente por favor mais tarde.');
@define('PLUGIN_EVENT_SPARTACUS_FETCHING', 'Tentando aceder ao endereço %s...');
@define('PLUGIN_EVENT_SPARTACUS_FETCHED_BYTES_URL', 'Obteve %s bytes da URL acima. Guardando o ficheiro como %s...');
@define('PLUGIN_EVENT_SPARTACUS_FETCHED_BYTES_CACHE', 'Obteve %s bytes dum ficheiro já existente no seu servidor. Guardando o ficheiro como %s...');
@define('PLUGIN_EVENT_SPARTACUS_FETCHED_DONE', 'Dados descarregados com sucesso.');
@define('PLUGIN_EVENT_SPARTACUS_MIRROR_XML', 'Localização de Ficheiro/Mirror (metadata XML)');
@define('PLUGIN_EVENT_SPARTACUS_MIRROR_FILES', 'Localização de Ficheiro/Mirror (ficheiros)');
@define('PLUGIN_EVENT_SPARTACUS_MIRROR_DESC', 'Escolha a localização do arquivo. NÃO mude este valor a não ser que saiba o que está a fazer e os servidores estiverem desactualizados. Esta opção foi disponibilizada principalmente para compatibilidade futura.');
@define('PLUGIN_EVENT_SPARTACUS_CHOWN', 'Proprietário dos ficheiros descarregados');
@define('PLUGIN_EVENT_SPARTACUS_CHOWN_DESC', 'Aqui pode mudar o (FTP/Shell) proprietário (por exemplo "nobody") de ficheiros descarregados pelo Spartacus. Se vazio, não são são feitas alterações.');
@define('PLUGIN_EVENT_SPARTACUS_CHMOD', 'Permissões de ficheiros descarregados');
@define('PLUGIN_EVENT_SPARTACUS_CHMOD_DESC', 'Aqui pode introduzir o modo octal (por exemplo "0777") das permissões de ficheiros (FTP/Shell) descarregados pelo Spartacus. Se vazio, a máscara de permissões por omissão do sistema é usada. Note que nem todos os servidores permitem definir ou alterar permissões. Note que as permissões aplicadas devem permitir leitura e escrita por parte do utilizador do servidor web. Além disso o spartacus/Serendipity não pode escrever sobre ficheiros existentes.');
@define('PLUGIN_EVENT_SPARTACUS_CHMOD_DIR', 'Permissões das directorias descarregadas');
@define('PLUGIN_EVENT_SPARTACUS_CHMOD_DIR_DESC', 'Aqui pode introduzir o modo octal (por exemplo "0777") das permissões de directorias (FTP/Shell) downloaded by Spartacus. descarregados pelo Spartacus. Se vazio, a máscara de permissões por omissão do sistema é usada. Note que nem todos os servidores permitem definir ou alterar permissões. Note que as permissões aplicadas devem permitir leitura e escrita por parte do utilizador do servidor web. Além disso o spartacus/Serendipity não pode escrever sobre ficheiros existentes.');

/* vim: set sts=4 ts=4 expandtab : */
?>