
spec2def(msi.dll msi.spec)

if (0)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

add_library(msi SHARED
            action.c
            alter.c
            appsearch.c
            automation.c
            classes.c
            cond.tab.c
            create.c</file>
            custom.c</file>
            database.c</file>
            delete.c</file>
            dialog.c</file>
            distinct.c</file>
            drop.c</file>
            events.c</file>
            files.c</file>
            font.c</file>
            format.c</file>
            handle.c</file>
            helpers.c</file>
            insert.c</file>
            install.c</file>
            join.c</file>
            media.c</file>
            msi.c</file>
            msi_main.c</file>
            msiquery.c</file>
            package.c</file>
            preview.c</file>
            record.c</file>
            registry.c</file>
            regsvr.c</file>
            script.c</file>
            select.c</file>
            source.c</file>
            sql.tab.c</file>
            storages.c</file>
            streams.c</file>
            string.c</file>
            suminfo.c</file>
            table.c</file>
            tokenize.c</file>
            update.c</file>
            upgrade.c</file>
            where.c</file>
            msi.rc</file>
            ${CMAKE_CURRENT_BINARY_DIR}/msi.def)

set_module_type(msi win32dll)

target_link_libraries(msi wine uuid PSEH_LIB)

add_importlibs(msi
               kernel32
               urlmon
               wininet
               comctl32
               shell32
               shlwapi
               cabinet
               oleaut32
               ole32
               version
               user32
               gdi32
               advapi32
               odbccp32
               ntdll)

#	<library>msiserver_interface</library>
#	<dependency>msiserver</dependency>
#	<dependency>msiheader</dependency>

add_cab_target(msi 1)
endif()
add_importlib_target(msi.spec)
