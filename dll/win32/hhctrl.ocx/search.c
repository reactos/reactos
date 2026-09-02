/*
 * Copyright 2010 Erich Hoover
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "hhctrl.h"
#include "stream.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(htmlhelp);

static SearchItem *SearchCHM_Folder(SearchItem *item, IStorage *pStorage,
                                    const WCHAR *folder, const char *needle);

/* Allocate a ListView entry for a search result. */
static SearchItem *alloc_search_item(WCHAR *title, const WCHAR *filename)
{
    int filename_len = filename ? (lstrlenW(filename)+1)*sizeof(WCHAR) : 0;
    SearchItem *item;

    item = calloc(1, sizeof(SearchItem));
    if(filename)
    {
        item->filename = malloc(filename_len);
        memcpy(item->filename, filename, filename_len);
    }
    item->title = title; /* Already allocated */

    return item;
}

/* Fill the ListView object corresponding to the found Search tab items */
static void fill_search_tree(HWND hwndList, SearchItem *item)
{
    int index = 0;
    LVITEMW lvi;

    SendMessageW(hwndList, LVM_DELETEALLITEMS, 0, 0);
    while(item) {
        TRACE("list debug: %s\n", debugstr_w(item->filename));

        memset(&lvi, 0, sizeof(lvi));
        lvi.iItem = index++;
        lvi.mask = LVIF_TEXT|LVIF_PARAM;
        lvi.cchTextMax = lstrlenW(item->title)+1;
        lvi.pszText = item->title;
        lvi.lParam = (LPARAM)item;
        item->id = (HTREEITEM)SendMessageW(hwndList, LVM_INSERTITEMW, 0, (LPARAM)&lvi);
        item = item->next;
    }
}

/* Search the CHM storage stream (an HTML file) for the requested text.
 *
 * Before searching the HTML file all HTML tags are removed so that only
 * the content of the document is scanned.  If the search string is found
 * then the title of the document is returned.
 */
static WCHAR *SearchCHM_File(IStorage *pStorage, const WCHAR *file, const char *needle)
{
    char *buffer = NULL, *new_buffer;
    strbuf_t content, node, node_name;
    IStream *temp_stream = NULL;
    DWORD i, buffer_size = 0;
    WCHAR *title = NULL;
    BOOL found = FALSE;
    stream_t stream;
    HRESULT hres;

    hres = IStorage_OpenStream(pStorage, file, NULL, STGM_READ, 0, &temp_stream);
    if(FAILED(hres)) {
        FIXME("Could not open '%s' stream: %08lx\n", debugstr_w(file), hres);
        return NULL;
    }

    strbuf_init(&node);
    strbuf_init(&content);
    strbuf_init(&node_name);

    stream_init(&stream, temp_stream);

    /* Remove all HTML formatting and record the title */
    while(next_node(&stream, &node)) {
        get_node_name(&node, &node_name);

        if(next_content(&stream, &content) && content.len > 1)
        {
            char *text = &content.buf[1];
            int textlen = content.len-1;

            if(!stricmp(node_name.buf, "title"))
            {
                int wlen = MultiByteToWideChar(CP_ACP, 0, text, textlen, NULL, 0);
                title = malloc((wlen + 1) * sizeof(WCHAR));
                MultiByteToWideChar(CP_ACP, 0, text, textlen, title, wlen);
                title[wlen] = 0;
            }

            new_buffer = realloc(buffer, buffer_size + textlen + 1);
            if(!new_buffer) goto cleanup;
            buffer = new_buffer;
            memcpy(&buffer[buffer_size], text, textlen);
            buffer[buffer_size + textlen] = '\0';
            buffer_size += textlen;
        }

        strbuf_zero(&node);
        strbuf_zero(&content);
    }

    /* Convert the buffer to lower case for comparison against the
     * requested text (already in lower case).
     */
    for(i=0;i<buffer_size;i++)
        buffer[i] = tolower(buffer[i]);

    /* Search the decoded buffer for the requested text */
    if(strstr(buffer, needle))
        found = TRUE;

cleanup:
    strbuf_free(&node);
    strbuf_free(&content);
    strbuf_free(&node_name);

    free(buffer);
    IStream_Release(temp_stream);

    if(!found)
    {
        free(title);
        return NULL;
    }
    return title;
}

/* Search all children of a CHM storage object for the requested text and
 * return the last found search item.
 */
static SearchItem *SearchCHM_Storage(SearchItem *item, IStorage *pStorage,
                                     const char *needle)
{
    static const WCHAR szHTMext[] = {'.','h','t','m',0};
    IEnumSTATSTG *elem = NULL;
    WCHAR *filename = NULL;
    STATSTG entries;
    HRESULT hres;
    ULONG retr;

    hres = IStorage_EnumElements(pStorage, 0, NULL, 0, &elem);
    if(hres != S_OK)
    {
        FIXME("Could not enumerate '/' storage elements: %08lx\n", hres);
        return NULL;
    }
    while (IEnumSTATSTG_Next(elem, 1, &entries, &retr) == NOERROR)
    {
        filename = entries.pwcsName;
        while(wcschr(filename, '/'))
            filename = wcschr(filename, '/')+1;
        switch(entries.type) {
        case STGTY_STORAGE:
            item = SearchCHM_Folder(item, pStorage, filename, needle);
            break;
        case STGTY_STREAM:
            if(wcsstr(filename, szHTMext))
            {
                WCHAR *title = SearchCHM_File(pStorage, filename, needle);

                if(title)
                {
                    item->next = alloc_search_item(title, entries.pwcsName);
                    item = item->next;
                }
            }
            break;
        default:
            FIXME("Unhandled IStorage stream element.\n");
        }
    }
    IEnumSTATSTG_Release(elem);
    return item;
}

/* Open a CHM storage object (folder) by name and find all items with
 * the requested text.  The last found item is returned.
 */
static SearchItem *SearchCHM_Folder(SearchItem *item, IStorage *pStorage,
                                    const WCHAR *folder, const char *needle)
{
    IStorage *temp_storage = NULL;
    HRESULT hres;

    hres = IStorage_OpenStorage(pStorage, folder, NULL, STGM_READ, NULL, 0, &temp_storage);
    if(FAILED(hres))
    {
        FIXME("Could not open '%s' storage object: %08lx\n", debugstr_w(folder), hres);
        return NULL;
    }
    item = SearchCHM_Storage(item, temp_storage, needle);

    IStorage_Release(temp_storage);
    return item;
}

/* Search the entire CHM file for the requested text and add all of
 * the found items to a ListView for the user to choose the item
 * they want.
 */
void InitSearch(HHInfo *info, const char *needle)
{
    CHMInfo *chm = info->pCHMInfo;
    SearchItem *root_item = alloc_search_item(NULL, NULL);

    SearchCHM_Storage(root_item, chm->pStorage, needle);
    fill_search_tree(info->search.hwndList, root_item->next);
    if(info->search.root)
        ReleaseSearch(info);
    info->search.root = root_item;
}

/* Free all of the found Search items. */
void ReleaseSearch(HHInfo *info)
{
    SearchItem *item = info->search.root;

    info->search.root = NULL;
    while(item) {
        free(item->filename);
        item = item->next;
    }
}
