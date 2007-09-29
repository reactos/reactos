#include "fitz-base.h"

void
fz_warn(char *fmt, ...)
{
	va_list ap;
	fprintf(stderr, "warning: ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

fz_error *
fz_throw1(char *fmt, ...)
{
	va_list ap;
	fz_error *eo;

	eo = fz_malloc(sizeof(fz_error));
	if (!eo) return fz_outofmem;

	eo->refs = 1;
	strlcpy(eo->func, "unknown", sizeof eo->func);
	strlcpy(eo->file, "unknown", sizeof eo->file);
	eo->line = 0;

	va_start(ap, fmt);
	vsnprintf(eo->msg, sizeof eo->msg, fmt, ap);
	eo->msg[sizeof(eo->msg) - 1] = '\0';
	va_end(ap);

	return eo;
}

fz_error *
fz_throw0(const char *func, const char *file, int line, char *fmt, ...)
{
	va_list ap;
	fz_error *eo;

	eo = fz_malloc(sizeof(fz_error));
	if (!eo) return fz_outofmem;

	eo->refs = 1;
	strlcpy(eo->func, func, sizeof eo->func);
	strlcpy(eo->file, file, sizeof eo->file);
	eo->line = line;

	va_start(ap, fmt);
	vsnprintf(eo->msg, sizeof eo->msg, fmt, ap);
	eo->msg[sizeof(eo->msg) - 1] = '\0';
	va_end(ap);

	if (getenv("BOMB"))
	{
		fflush(stdout);
		fprintf(stderr, "%s:%d: %s(): %s\n", eo->file, eo->line, eo->func, eo->msg);
		fflush(stderr);
		abort();
	}

	return eo;
}

void
fz_droperror(fz_error *eo)
{
	if (eo->refs > 0)
		eo->refs--;
	if (eo->refs == 0)
		fz_free(eo);
}

