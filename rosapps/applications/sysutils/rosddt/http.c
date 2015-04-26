#include <windows.h>
#include <wininet.h>
#include <urlmon.h>
#include <stdio.h>
#include "http.h"

#define INTERNET_FLAGS (INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_NO_COOKIES )

static char *http_receive(HINTERNET h_req, u_long *d_size)
{
	u_long bytes  = sizeof(u_long);
	u_long qsize  = 0;
	u_long readed = 0;
	char  *data   = NULL;
	char   buff[4096];

	if (HttpQueryInfo(h_req, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &qsize, &bytes, NULL) != 0) {
		data = malloc(qsize + 1);
	}

	do
	{
		if (InternetReadFile(h_req, buff, sizeof(buff), &bytes) == 0) {
			break;
		}
		if ( (readed + bytes) > qsize) {
			data = realloc(data, readed + bytes + 1);
			if (data == NULL) break;
			qsize += bytes;
		}
		memcpy(data + readed, buff, bytes); readed += bytes;
	} while (bytes != 0);

	if ( (data != NULL) && (readed != qsize) ) {
		free(data); data = NULL;
	} else {
		if (d_size != NULL) *d_size = readed;
		data[readed] = 0;
	}	
	return data;
}

void *http_get(wchar_t *url, u_long *d_size)
{
	HINTERNET h_inet = NULL;
	HINTERNET h_req  = NULL;
	char     *replay = NULL;
	
	do
	{
		h_inet = InternetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if (h_inet == NULL) break;
		
		h_req = InternetOpenUrl(h_inet, url, NULL, 0, INTERNET_FLAGS, 0);
		if (h_req == NULL) break;

		replay = http_receive(h_req, d_size);
	} while (0);

	if (h_req != NULL) {
		InternetCloseHandle(h_req);
	}
	if (h_inet != NULL) {
		InternetCloseHandle(h_inet);
	}
	return replay;
}

void *http_post(wchar_t *url, void *data, int size, u_long *d_size)
{
	URL_COMPONENTS url_cm = {0};
	HINTERNET      h_inet = NULL;
	HINTERNET      h_conn = NULL;
	HINTERNET      h_req  = NULL;
	char          *q_data = NULL;
	char          *replay = NULL;
	wchar_t        host[MAX_PATH];
	wchar_t        path[MAX_PATH];
	char *p = NULL;
    char *d = data;

	do
	{
		if ( (q_data = malloc(size * 3 + 10)) == NULL ) {
			break;
		}
		strcpy(q_data, "data="); p = q_data + 5;

		while (size--) {
			p += sprintf(p, "%%%.2x", (u_int)*d++);
		}

		url_cm.dwStructSize     = sizeof(url_cm);
		url_cm.lpszHostName     = host;
		url_cm.dwHostNameLength = sizeof(host);
		url_cm.lpszUrlPath      = path;
		url_cm.dwUrlPathLength  = sizeof(path);

		if (InternetCrackUrl(url, 0, 0, &url_cm) == 0) {
			break;
		}

		h_inet = InternetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if (h_inet == NULL) break;

		h_conn = InternetConnect(h_inet, host, url_cm.nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
		if (h_conn == NULL) break;

		h_req = HttpOpenRequest(h_conn, L"POST", path, NULL, NULL, NULL, INTERNET_FLAGS, 0);
		if (h_req == NULL) break;

		HttpAddRequestHeaders(
			h_req, L"Content-Type: application/x-www-form-urlencoded", 47*2, HTTP_ADDREQ_FLAG_ADD);

		if (HttpSendRequest(h_req, NULL, 0, q_data, strlen(q_data)) == 0) {
			break;
		}
		replay = http_receive(h_req, d_size);
	} while (0);

	if (h_req != NULL) {
		InternetCloseHandle(h_req);
	}
	if (h_conn != NULL) {
		InternetCloseHandle(h_conn);
	}
	if (h_inet != NULL) {
		InternetCloseHandle(h_inet);
	}
	if (q_data != NULL) free(q_data);
	return replay;
}
