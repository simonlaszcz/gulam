/*
	util.c of Gulam -- simple utility routines

	Copyright (c) 1987 pm@Case
*/

/* 890111 kbad	Made aborting things a LOT easier by checking inside "eachline"
*/

#include "ue.h"
#include <stdarg.h>

uchar hex[17] = "0123456789abcdef";
uchar CRLF[] = "\r\n";
uchar ES[] = { '\0', '\0' };			/* two zero-bytes; empty string */


void panic(const char *s)
{
	gputs("fatal situation in Gulam:");
	gputs(s);
	ggetchar();
	exit(-1);
}

/* Set the 256-char long array a to all v (which is either 0 or 1) for
the chars in s; the rest to 1-v.  */

void charset(uchar *a, uchar *s, uint v)
{
	uchar *p;
	uchar u;
	int i;

	u = (uchar) (v ? 0 : 1);			/* more robust than 1 - v   */
	for (p = a + 256; p > a;)
		*--p = u;
	while ((i = (uchar)*s++) != 0)
		a[i] = (uchar) v;
}


/* Copy n bytes from s to d; these are either disjoint areas, or d is < s.
*/
void cpymem(void *_d, void *_s, int n)
{
	char *d = _d;
	const char *s = _s;
	if (d && s && n)
	{
		do
			*d++ = *s++;
		while (--n != 0);
	}
}


/* Growing arrays: #elements increases, can't predict the max, and
want to use as little space as possible.  */

GA *initga(int z, int i)
{
	GA *a;

	if (z <= 0 || i <= 0)
		return NULL;
	if ((a = (GA *) gmalloc(((uint) sizeof(GA)) + i * z)) != NULL)
	{
		a->sz = z;
		a->ne = 0;
		a->na = a->ni = i;
	}
	return a;
}

GA *addelga(GA *ga, void *e)
{
	GA *a;
	int n;
	int z;

	if (ga && e)
	{
		z = ga->sz;
		if (ga->ne == ga->na)
		{
			n = (ga->na) * z + ((uint) sizeof(GA));
			if ((a = (GA *) gmalloc(n + (ga->ni) * z)) != NULL)
			{
				cpymem((void *)a, (void *)ga, n);
				gfree(ga);
				ga = a;
				ga->na += ga->ni;
			} else
				return ga;
		}
		cpymem(ga->el + (ga->ne) * z, e, z);
		ga->ne++;
	}
	return ga;
}

/******    WS related functions.
ws->ns == #stringlets;
ws->ps == pt to the beginning of the block of mem that contains these;
ws->nc == length of this block;
ws->sz == allocated size of mem for such use.
Stringlets are numbered 0 to ws->ns - 1.
*/

WS *initws(void)
{
	WS *ws;

	if ((ws = (WS *) gmalloc((uint) sizeof(WS))) != NULL)
	{
		if ((ws->ps = gmalloc(Iwssz)) != NULL)
		{
			ws->sz = Iwssz;
			ws->nc = ws->ns = 0;
			ws->ps[0] = ws->ps[1] = '\0';
		} else
		{
			gfree(ws);
			ws = NULL;
		}
	}
	return ws;
}

void freews(WS *ws)
{
	if (ws)
	{
		gfree(ws->ps);
		gfree(ws);
	}
}

WS *dupws(WS *ws)
{
	WS *w2;

	if (ws == NULL)
		return NULL;
	w2 = (WS *) gmalloc(((uint) sizeof(WS)));
	if (w2)
	{
		if ((w2->ps = gmalloc(ws->nc)) != NULL)
		{
			w2->sz = w2->nc = ws->nc;
			w2->ns = ws->ns;
			cpymem(w2->ps, ws->ps, ws->nc);
		} else
		{
			gfree(w2);
			w2 = NULL;
		}
	}
	return w2;
}

int findstr(WSPS q, uchar *p)
{
	int n;

	if (p && q)
	{
		for (n = 0; *q; q += strlen(q) + 1)
		{
			if (strcmp(p, q) == 0)
				return n;
			n++;
		}
	}
	return -1;
}

/* Return ptr to n-th stringlet from p.  Stringlets are numbered 0 to
ws->ns - 1.  */

uchar *nthstr(WSPS p, int n)
{
	if ((n < 0) || (p == NULL))
		return ES;
	for (n++; --n && *p;)
		p += strlen(p) + 1;
	return p;
}

/* Cat the stringlets in p, starting from the m-th, with ' ' as
separator,and return the result.  (Just replace all but the last '\0'
with ' '.) */

uchar *catall(WS *ws, int m)
{
	uchar *p;
	uchar *q;
	int n;

	if (ws == NULL || 0 > m || m >= (n = ws->ns))
		return ES;
	q = p = nthstr(ws->ps, m);			/* m < n */
	while (++m < n)
	{
		p += strlen(p);
		*p++ = ' ';
	}
	return q;
}

/* Append to ws->ps the string p, and then one more '\0' if i == 1.
The value of i is either 1, or 0.  */

void strwcat(WS *ws, const uchar *p, int i)
{
	int n;
	uchar *q;

	if (p == NULL || ws == NULL)
		return;
	if (i)
		i = 1;
	n = (int)strlen(p) + ws->nc;
	if (ws->sz <= n + i)
	{
		if ((q = gmalloc(n + Iwssz)) != NULL)
		{
			cpymem(q, ws->ps, ws->nc);
			gfree(ws->ps);
			ws->ps = q;
			ws->sz = n + Iwssz;
		} else
			return;
	}
	if ((q = ws->ps) != NULL)
	{
		strcpy(q + ws->nc, p);
		q[n + i] = '\0';
		ws->nc = n + i;
		if (i)
			ws->ns++;
	}
}

/* Columnize the 'string' r.  If ns >= 0, r is in argv[] format, ns ==
#stringlets in r, cw == length of longest stringlet in r.  If ns < 0,
these values are to be computed by this routine, and r is in WSPS
format; we don't set cw initially to 0 because this permits the caller
to set it at a minimum.  pf is a ptr to a function that computes
one-char based on its argument string; this char can be '\0'.  */

uchar *pscolumnize(uchar *r, int ns, int cw)
{
	uchar *p;
	uchar *q;
	uchar *qq;
	uchar **aa;
	int i;
	int m;
	int n;
	int nc;
	int nr;								/* #col per line, #rows */

	if (r == NULL)
		return NULL;
	if (ns < 0)
	{
		ns = 0;
		aa = NULL;						/* compute cw */
		for (p = r; *p; p += n + 1)
		{
			n = (int)strlen(p);
			if (n > cw)
				cw = n;
			ns++;
		}
		cw++;							/* at least one blank as a separator */
	} else
		aa = (uchar **) r;

	nc = (screenwidth - 2) / cw;
	if (nc <= 0)
		nc = 1;
	nr = (ns + nc - 1) / nc;

	/* allocates more than nec, but this is ok; we free this soon   */
	q = qq = gmalloc(1 + ns * (cw + 2));
	if (q)
	{
		for (n = 0; n < nr; n++)
		{
			for (m = 0; m < nc; m++)
			{
				i = m * nr + n;
				if (i >= ns)
					break;
				p = (aa ? aa[i] : nthstr(r, i));
				for (i = 0; (*q++ = *p++) != 0;)
					i++;
				q[-1] = ' ';
				if (m < nc - 1)
					while (++i < cw)
						*q++ = ' ';
			}
			*q++ = '\r';
			*q++ = '\n';
		}
		*q = '\0';
	}
	return qq;
}

/* Make a new ws which has the stringletes of WSPS q each prefixed with p
*/
WS *prefixws(uchar *p, WSPS q)
{
	WS *w;

	w = initws();
	if (w && q)
	{
		while (*q)
		{
			strwcat(w, p, 0);
			strwcat(w, q, 1);
			q += strlen(q) + 1;
		}
	}
	return w;
}

/* Append stringlets m..  upwards of v to u */

void appendws(WS *u, WS *v, int m)
{
	int n;
	uchar *p;

	if (u == NULL || v == NULL)
		return;
	n = v->ns;
	p = v->ps;
	if (m < 0 || n < m || p == NULL)
		return;
	p = nthstr(p, m);
	while (m++ < n)
	{
		strwcat(u, p, 1);
		p += strlen(p) + 1;
	}
}

/* Shift the stringlets left by losing the (m-1)-th one.  */

void shiftws(WS *ws, int m)
{
	uchar *q;
	int n;
	int k;

	if (ws == NULL || m > ws->ns || m <= 0)
		return;
	q = nthstr(ws->ps, m - 1);
	n = (int)strlen(q) + 1;
	k = ws->nc - (int) (q - ws->ps);
	cpymem(q, q + n, k);
	ws->ns--;
	ws->nc -= n;
}

#ifdef DEBUG

/* Show the wsps p */

void showwsps(WSPS p)
{
	uchar *q;

	if (p == NULL)
	{
		outstr("showwsps arg is NULL");
		return;
	}
	q = pscolumnize(p, -1, 0);
	if (q)
	{
		outstr(q);
		gfree(q);
	}
}
#endif

static uchar bschar[256] = {
	'\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
	'\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
	'\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
	'\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
	'\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
	'\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
	'\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
	'\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
	'\100', '\101', '\b', '\103', '\104', '\033', '\014', '\107',
	'\110', '\111', '\112', '\113', '\114', '\115', '\n', '\117',
	'\120', '\121', '\r', '\123', '\t', '\125', '\v', '\127',
	'\130', '\131', '\132', '\133', '\134', '\135', '\136', '\137',
	'\140', '\141', '\b', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\n', '\157',
	'\160', '\161', '\r', '\163', '\t', '\165', '\v', '\167',
	'\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
	'\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
	'\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
	'\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
	'\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
	'\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
	'\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
	'\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
	'\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
	'\300', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
	'\310', '\311', '\312', '\313', '\314', '\315', '\316', '\317',
	'\320', '\321', '\322', '\323', '\324', '\325', '\326', '\327',
	'\330', '\331', '\332', '\333', '\334', '\335', '\336', '\337',
	'\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377'
};

static uchar *escapedchar(uchar *p, int *ip)
{
	int ic;
	int n;

	ic = p[1];
	if ('0' <= ic && ic <= '9')
	{
		for (ic = 0, n = 3; n; n--)
			if ('0' <= *++p && *p <= '9')
				ic = ic * 8 + (*p - '0');
	} else if (ic)
	{
		ic = bschar[ic & 0xff];
		p++;
	}
	*ip = ic;
	return p;
}

/* If the string p begins with either ' or " remove them, and convert
^Q-quoted-chars that it has, if any.  The unquoted string is stored
beginning at q.  Often called with q == p; however, if q points to the
middle of p, the src string will be clobbered -- obviously.  Return
ptr to new eos.  */

uchar *unquote(uchar *q, uchar *p)
{
	uchar c;
	uchar sdq;
	int i;

	if (p && q)
	{
		if ((sdq = *p) != 0 && (sdq == '"' || sdq == '\''))
		{
			while ((c = *++p) != 0)
			{
				if (c == sdq)
					break;
				if (c == '\021')
				{
					p = escapedchar(p, &i);
					c = i;
				}
				*q++ = c;
			}
			*q = '\0'; /** if (c!=sdq) emsg = "unmatched quote"; **/
		} else
		{
			i = (int)strlen(p);
			if (p != q)
				cpymem(q, p, i + 1);
			q += i;
		}
	}
	return q;
}

uchar *str3cat(uchar *p, uchar *q, uchar *r)					/* return the catenation p|q|r  */
{
	uchar *a;
	uchar *b;
	uchar *s;

	s = a = ((p && q && r) ? gmalloc(((uint) (strlen(p) + strlen(q) + strlen(r) + 1))) : NULL);
	if (s)
	{
		b = p;
		while ((*a++ = *b++) != 0)
			;
		a--;
		b = q;
		while ((*a++ = *b++) != 0)
			;
		a--;
		b = r;
		while ((*a++ = *b++) != 0)
			;
	}
	return s;
}

/* Return ptr to the leftmost occurrence of q in p.  Return NULL if no
match.  */

uchar *strsub(uchar *p, uchar *q)
{
	int n;

	if (p && q)
	{
		for (n = (int)strlen(q); (p = strchr(p, *q)) != NULL; p++)
			if (strncmp(p, q, ((size_t) n)) == 0)
				break;
	} else
		p = NULL;
	return p;
}


/*
 * dont use the lib strdup() as it calls malloc with size_t,
 * and does not handle NULL arguments
 */
char *gstrdup(const char *p)
{
	char *q;

	q = p ? gmalloc(((uint)strlen(p)) + 1) : NULL;
	if (q)
		strcpy(q, p);
	return q;
}


/* Change case: i == 0 => to lower; i != 0 => upper */

uchar *chcase(uchar *p, int i)
{
	uchar *q;
	uchar c;
	int lb;
	int ub;
	int d;

	if ((q = p) != NULL)
	{
		if (i)
		{
			lb = 'a';
			ub = 'z';
			d = 'a' - 'A';
		} else
		{
			lb = 'A';
			ub = 'Z';
			d = 'A' - 'a';
		}
		while ((c = *p) != 0)
		{
			if (lb <= c && c <= ub)
				c -= d;
			*p++ = c;
		}
	}
	return q;
}


int atoi(const char *p)
{
	return (int) atoir(p, 10);
}

/* Convert string p of digits given in radix r into a long integer */

long atoir(const char *p, int r)
{
	long x;
	long d;
	char c;
	char sign;

	x = 0;
	if (p == NULL)
		return 0;
	if (*p == '-')
	{
		sign = '-';
		p++;
	} else
	{
		sign = '+';
	}
	while ((c = *p++) != 0)
	{
		d = r;
		if (r == 16)
		{
			if ('A' <= c && c <= 'F')
				d = (long) (c + 10 - 'A');
			if ('a' <= c && c <= 'f')
				d = (long) (c + 10 - 'a');
		}
		if ('0' <= c && c <= '9')
			d = (long) (c - '0');
		if (d >= r)
			break;
		x = x * r + d;
	}
	if (sign == '-')
		x = -x;
	return x;
}

/* Convert the given long (32-bit) integer into a digit string in
radix r.  The radix r is one of 2, 8, 10, or 16.  Return a ptr to the
static string.  */

uchar *itoar(long i, int r)
{
	static uchar s[33];
	uchar *p;
	uchar c;

	p = &s[32];
	*p-- = '\0';
	c = '+';
	if (i < 0)
	{
		c = '-';
		i = -i;
	}
	do
	{
		*p-- = hex[i % r];
		i /= r;
	} while (i > 0);
	if (c == '-')
		*p = c;
	else
		p++;
	return p;
}

uchar *itoal(long i)
{
	return itoar(i, 10);
}

/* A small class of printf like format items is handled.  Assumes that
the stack grows down (see ap += ...  in the argument scan loop).
(Should merge mlwrite() of display.c with this.) */

uchar *sprintp(uchar *fmt, ...)
{
	int c, r;
	va_list ap;
	uchar *p;
	static uchar ms[1024];
	long v;
	
	va_start(ap, fmt);
	p = ms;
	while ((c = *fmt++) != 0)
	{
		if (c != '%')
		{
			*p++ = c;
			continue;
		}
		c = *fmt++;
		r = 0;
		switch (c)
		{
		case 'x':
			r += 6;
		case 'd':
			r += 2;
		case 'o':
			r += 8;
			v = va_arg(ap, int);
			strcpy(p, itoar(v, r));
			p += strlen(p);
			break;
		case 'X':
			r += 6;
		case 'D':
			r += 2;
		case 'O':
			r += 8;
			v = va_arg(ap, long);
			strcpy(p, itoar(v, r));
			p += strlen(p);
			break;
		case 's':
			strcpy(p, va_arg(ap, char *));
			p += strlen(p);
			break;
		case 'c':
			*p++ = va_arg(ap, int);
			break;
		default:
			*p++ = c;
			break;
		}
	}
	va_end(ap);
	*p = '\0';
	return ms;
}

int stackoverflown(int n)
{
#if 0
/* this section removed because it doesn't work on */
/* TT with two disjoint memory spaces. */
	if ((long) &dummy < _stksize + (long) n)
	{
		emsg = "run-time stack is about to overflow";
		return -1;
	}
#endif
	UNUSED(n);
	return 0;
}

/* Read the already Fopen'd file fd into a large buffer, and invoke
the given function on each line of the file.  */


void eachline(int fd, void (*fn)(uchar *q, int n))
{
	uchar *q;
	uchar *r;
	uchar *be;
	uchar *bb;
	uchar *p;
	long sz;
	long n;
	int i;
	long asz;

	valu = 0;
	emsg = NULL;
	asz = 19 * 1024 + 2;
	bb = maxalloc(&asz);
	if (bb == NULL)
	{
		valu = -39;
		return;
	}
	sz = asz - 2;						/* leave 1 byte for '\n' sentinel; make it even 19k */
	n = 0;
	for (q = r = bb; valu == 0 && (n = gfread(fd, r, (long) (bb - r + sz))) > 0; q = bb)
	{
		be = r + n;
		be[0] = '\n';					/* sentinels */
		while ((valu = useraborted() ? -1 : 0) == 0)
		{
			for (r = q; *r != '\n';)
				r++;
			if (r == be)
				break;					/* <== */
			/* between q and r is a whole line */
			if (r[-1] == '\r')
			{
				*--r = '\0';
				i = 2;
			} else
			{
				*r = '\0';
				i = 1;
				if ((p = strchr(q, '\r')) != NULL)
				{
					*r = '\n';
					r = p;
					*r = '\0';
				}
			}
			(*fn) (q, (int) (r - q));
			q = r + i;
		}
		if (!valu)
		{
			if (be - q >= sz)
			{
				emsg = sprintp("a line with >= %D chars has been split", sz);
				(*fn) (q, (int) (be - q));
				q = be;
			}
			/* move leftover part of ln to bgn of buf */
			for (r = bb; q < be;)
				*r++ = *q++;
		}
	}
	if (n < 0)
	{
		emsg = "File read error";
		valu = n;
	}
	if (n == 0 && bb != r)
	{
		emsg = "The last line ended without \\n";
		*r = '\0';
		(*fn) (q, (int) (r - q));
	}
	gfclose(fd);
	maxfree(bb);
}

/* Text contains 0 or more lines.  Invoke the given function on each
line.  */

void streachline(uchar *text, int (*fn)(void *ap, uchar *p), void *ap)
{
	uchar *p;
	uchar *q;
	uchar *et;
	uchar cr;
	KEY d;

	if (text == NULL)
		return;
	et = text + strlen(text);
	*et = '\n';							/* sentinel */
	for (p = text; (p < et) && (q = strchr(p, '\n')) != NULL; p = q + 1)
	{
		d = usertyped();
		if (d == CTRLS)
			ggetchar();					/* xon/xoff */
		else if (d == CTRLC)
			break;

		*q = '\0';
		cr = q[-1];
		if (cr == '\r')
			q[-1] = '\0';
		(*fn) (ap, p);
		q[-1] = cr;
		*q = '\n';						/* restore the line */
	}
	*et = '\0';
}
