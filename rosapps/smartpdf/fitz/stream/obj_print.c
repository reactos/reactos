#include "fitz-base.h"
#include "fitz-stream.h"

struct fmt
{
	char *buf;
	int cap;
	int len;
	int indent;
	int tight;
	int col;
	int sep;
	int last;
};

static void fmtobj(struct fmt *fmt, fz_obj *obj);

static inline int iswhite(int ch)
{
	return
		ch == '\000' ||
		ch == '\011' ||
		ch == '\012' ||
		ch == '\014' ||
		ch == '\015' ||
		ch == '\040';
}

static inline int isdelim(int ch)
{
	return  ch == '(' || ch == ')' ||
			ch == '<' || ch == '>' ||
			ch == '[' || ch == ']' ||
			ch == '{' || ch == '}' ||
			ch == '/' ||
			ch == '%';
}

static inline void fmtputc(struct fmt *fmt, int c)
{
	if (fmt->sep && !isdelim(fmt->last) && !isdelim(c)) {
		fmt->sep = 0;
		fmtputc(fmt, ' ');
	}
	fmt->sep = 0;

	if (fmt->buf && fmt->len < fmt->cap)
		fmt->buf[fmt->len] = c;

	if (c == '\n')
		fmt->col = 0;
	else
		fmt->col ++;

	fmt->len ++;

	fmt->last = c;
}

static void fmtindent(struct fmt *fmt)
{
	int i = fmt->indent;
	while (i--) {
		fmtputc(fmt, ' ');
		fmtputc(fmt, ' ');
	}
}

static inline void fmtputs(struct fmt *fmt, char *s)
{
	while (*s)
		fmtputc(fmt, *s++);
}

static inline void fmtsep(struct fmt *fmt)
{
	fmt->sep = 1;
}

static void fmtstr(struct fmt *fmt, fz_obj *obj)
{
	int i;
	int c;

	fmtputc(fmt, '(');
	for (i = 0; i < obj->u.s.len; i++)
	{
		c = (unsigned char) obj->u.s.buf[i];
		if (c == '\n')
			fmtputs(fmt, "\\n");
		else if (c == '\r')
			fmtputs(fmt, "\\r");
		else if (c == '\t')
			fmtputs(fmt, "\\t");
		else if (c == '\b')
			fmtputs(fmt, "\\b");
		else if (c == '\f')
			fmtputs(fmt, "\\f");
		else if (c == '(')
			fmtputs(fmt, "\\(");
		else if (c == ')')
			fmtputs(fmt, "\\)");
		else if (c < 32 || c > 126) {
			char buf[16];
			fmtputc(fmt, '\\');
			sprintf(buf, "%o", c);
			fmtputs(fmt, buf);
		}
		else
			fmtputc(fmt, c);
	}
	fmtputc(fmt, ')');
}

static void fmthex(struct fmt *fmt, fz_obj *obj)
{
	int i;
	int b;
	int c;

	fmtputc(fmt, '<');
	for (i = 0; i < obj->u.s.len; i++) {
		b = (unsigned char) obj->u.s.buf[i];
		c = (b >> 4) & 0x0f;
		fmtputc(fmt, c < 0xA ? c + '0' : c + 'A' - 0xA);
		c = (b) & 0x0f;
		fmtputc(fmt, c < 0xA ? c + '0' : c + 'A' - 0xA);
	}
	fmtputc(fmt, '>');
}

static void fmtname(struct fmt *fmt, fz_obj *obj)
{
	char *s = fz_toname(obj);
	int i, c;

	fmtputc(fmt, '/');

	for (i = 0; s[i]; i++)
	{
		if (isdelim(s[i]) || iswhite(s[i]))
		{
			fmtputc(fmt, '#');
			c = (s[i] >> 4) & 0xf;
			fmtputc(fmt, c < 0xA ? c + '0' : c + 'A' - 0xA);
			c = s[i] & 0xf;
			fmtputc(fmt, c < 0xA ? c + '0' : c + 'A' - 0xA);
		}
		else
		{
			fmtputc(fmt, s[i]);
		}
	}
}

static void fmtarray(struct fmt *fmt, fz_obj *obj)
{
	int i;

	if (fmt->tight) {
		fmtputc(fmt, '[');
		for (i = 0; i < fz_arraylen(obj); i++) {
			fmtobj(fmt, fz_arrayget(obj, i));
			fmtsep(fmt);
		}
		fmtputc(fmt, ']');
	}
	else {
		fmtputs(fmt, "[ ");
		for (i = 0; i < fz_arraylen(obj); i++) {
			if (fmt->col > 60) {
				fmtputc(fmt, '\n');
				fmtindent(fmt);
			}
			fmtobj(fmt, fz_arrayget(obj, i));
			fmtputc(fmt, ' ');
		}
		fmtputc(fmt, ']');
		fmtsep(fmt);
	}
}

static void fmtdict(struct fmt *fmt, fz_obj *obj)
{
	int i;
	fz_obj *key, *val;

	if (fmt->tight) {
		fmtputs(fmt, "<<");
		for (i = 0; i < fz_dictlen(obj); i++) {
			fmtobj(fmt, fz_dictgetkey(obj, i));
			fmtsep(fmt);
			fmtobj(fmt, fz_dictgetval(obj, i));
			fmtsep(fmt);
		}
		fmtputs(fmt, ">>");
	}
	else {
		fmtputs(fmt, "<<\n");
		fmt->indent ++;
		for (i = 0; i < fz_dictlen(obj); i++) {
			key = fz_dictgetkey(obj, i);
			val = fz_dictgetval(obj, i);
			fmtindent(fmt);
			fmtobj(fmt, key);
			fmtputc(fmt, ' ');
			if (fz_isarray(val))
				fmt->indent ++;
			fmtobj(fmt, val);
			fmtputc(fmt, '\n');
			if (fz_isarray(val))
				fmt->indent --;
		}
		fmt->indent --;
		fmtindent(fmt);
		fmtputs(fmt, ">>");
	}
}

static void fmtobj(struct fmt *fmt, fz_obj *obj)
{
	char buf[256];

	if (!obj) {
		fmtputs(fmt, "<nil>");
		return;
	}

	switch (obj->kind)
	{
	case FZ_NULL:
		fmtputs(fmt, "null");
		break;
	case FZ_BOOL:
		fmtputs(fmt, fz_tobool(obj) ? "true" : "false");
		break;
	case FZ_INT:
		sprintf(buf, "%d", fz_toint(obj));
		fmtputs(fmt, buf);
		break;
	case FZ_REAL:
		sprintf(buf, "%g", fz_toreal(obj));
		if (strchr(buf, 'e')) /* bad news! */
			sprintf(buf, fabs(fz_toreal(obj)) > 1 ? "%1.1f" : "%1.8f", fz_toreal(obj));
		fmtputs(fmt, buf);
		break;
	case FZ_STRING:
		{
			int added = 0;
			int i, c;
			for (i = 0; i < obj->u.s.len; i++) {
				c = (unsigned char)obj->u.s.buf[i];
				if (strchr("()\\\n\r\t\b\f", c) != 0)
					added ++;
				else if (c < 8)
					added ++;
				else if (c < 32)
					added += 2;
				else if (c >= 127)
					added += 3;
			}
			if (added < obj->u.s.len)
				fmtstr(fmt, obj);
			else
				fmthex(fmt, obj);
		}
		break;
	case FZ_NAME:
		fmtname(fmt, obj);
		break;
	case FZ_ARRAY:
		fmtarray(fmt, obj);
		break;
	case FZ_DICT:
		fmtdict(fmt, obj);
		break;
	case FZ_INDIRECT:
		sprintf(buf, "%d %d R", obj->u.r.oid, obj->u.r.gid);
		fmtputs(fmt, buf);
		break;
	case FZ_POINTER:
		sprintf(buf, "$%p", obj->u.p);
		fmtputs(fmt, buf);
		break;
	default:
		sprintf(buf, "<unknown object type %d>", obj->kind);
		fmtputs(fmt, buf);
		break;
	}
}

int
fz_sprintobj(char *s, int n, fz_obj *obj, int tight)
{
	struct fmt fmt;

	fmt.indent = 0;
	fmt.col = 0;
	fmt.sep = 0;
	fmt.last = 0;

	fmt.tight = tight;
	fmt.buf = s;
	fmt.cap = n;
	fmt.len = 0;
	fmtobj(&fmt, obj);

	if (fmt.buf && fmt.len < fmt.cap)
		fmt.buf[fmt.len] = '\0';

	return fmt.len;
}

void
fz_debugobj(fz_obj *obj)
{
	char buf[1024];
	char *ptr;
	int n;

	n = fz_sprintobj(nil, 0, obj, 0);
	if (n < sizeof buf)
	{
		fz_sprintobj(buf, sizeof buf, obj, 0);
		fwrite(buf, 1, n, stdout);
	}
	else
	{
		ptr = fz_malloc(n);
		if (!ptr)
			return;
		fz_sprintobj(ptr, n, obj, 0);
		fwrite(ptr, 1, n, stdout);
		fz_free(ptr);
	}
}

