<?php # $Id: lang_es.inc.php,v 1.0 2005/08/20 11:37:42 garvinhicking Exp $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details
# Translation (c) by  Rodrigo Lazo <rlazo.paz@gmail.com>
/* vim: set sts=4 ts=4 expandtab : */

@define('PLUGIN_EVENT_SPARTACUS_NAME', 'Spartacus');
@define('PLUGIN_EVENT_SPARTACUS_DESC', '[S]erendipity [P]lugin [A]ccess [R]epository [T]ool [A]nd [C]ustomization/[U]nification [S]ystem - Te permite descargar extensiones desde nuestro repositorio en linea');
@define('PLUGIN_EVENT_SPARTACUS_FETCH', 'Haz click aquí para descargar un nuevo %s desde el Respositorio En-linea Serendipity');
@define('PLUGIN_EVENT_SPARTACUS_FETCHERROR', 'El URL %s no pudo ser abierto. Quizás Serendipity o el servidor en SourceForge.net están desconectados - lo lamentamos, intenta de nuevo más tarde.');
@define('PLUGIN_EVENT_SPARTACUS_FETCHING', 'Intentando acceder al URL %s...');
@define('PLUGIN_EVENT_SPARTACUS_FETCHED_BYTES_URL', 'Descargados %s bytes desde la URL. Guardando el archivo como %s...');
@define('PLUGIN_EVENT_SPARTACUS_FETCHED_BYTES_CACHE', 'Descargandos %s bytes desde un archivo ya existente en tu servidor. Guardando el archivo como %s...');
@define('PLUGIN_EVENT_SPARTACUS_FETCHED_DONE', 'Data descargada con éxito.');
@define('PLUGIN_EVENT_SPARTACUS_MIRROR_XML', 'Ubicación del archivo/réplica (XML metadata)');
@define('PLUGIN_EVENT_SPARTACUS_MIRROR_FILES', 'Ubicación del archivo/réplica (files)');
@define('PLUGIN_EVENT_SPARTACUS_MIRROR_DESC', 'Escoge una ubicación de descarga. NO alteres este valor a menos que sepas lo que estás haciendo y si el servidor se desactualiza. Esta opción está disponible principalmente para compatibilidad con forward.');
@define('PLUGIN_EVENT_SPARTACUS_CHOWN', 'Dueño de los archivos descargados');
@define('PLUGIN_EVENT_SPARTACUS_CHOWN_DESC', 'Aquí puedes ingresar el (FTP/Shell) dueño (por ejemplo "nobody") de los archivos descargados por Spartacus. Si lo dejas en blanco, no se realizarán cambios en la propiedad.');
@define('PLUGIN_EVENT_SPARTACUS_CHMOD', 'Permisos de los archivos descargados');
@define('PLUGIN_EVENT_SPARTACUS_CHMOD_DESC', 'Aquí puedes ingresar los permisos de los archivos (FTP/Shell) descargados por Spartacus en modo octal (por ejemplo "0777"). Si lo dejas vacío,  los permisos por defecto del sistema serán utilizados. Nota que no todos los servidores permiten definir/cambiar permisos. Presta atención que los permisos aplicados permiten la lectura y escritura por parte del usuario del webserver. Fuera de eso spartacus/Serendipity no puede sobreescribir archivos existentes.');
@define('PLUGIN_EVENT_SPARTACUS_CHMOD_DIR', 'Permisos de los directorios descargados');
@define('PLUGIN_EVENT_SPARTACUS_CHMOD_DIR_DESC', 'Aquí puedes ingresar los permisos de los directorios (FTP/Shell) descargados por Spartacus en modo octal (por ejemplo "0777"). Si lo dejas vacío,  los permisos por defecto del sistema serán utilizados. Nota que no todos los servidores permiten definir/cambiar permisos. Presta atención que los permisos aplicados permiten la lectura y escritura por parte del usuario del webserver. Fuera de eso spartacus/Serendipity no puede sobreescribir directorios existentes.');

?>
