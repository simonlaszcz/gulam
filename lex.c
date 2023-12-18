/*
	lex.c of gulam					10/10/86

	copyright (c) 1986 pm@Case
*/

/* Given a string (may be multi-line long), this module produces a
 WS-style sequence of stringlets, each stringlet being a lexeme.
*/

#include "ue.h"


uchar DELIMS[256];						/* these are inited in lex()    */
uchar SUBDELIMS[256];
uchar COMMA[256];
uchar BTRNQD[256];
uchar TKN2[256];
uchar EMPTY2[256];
uchar WHITEC[256];						/* white chars      */
uchar WHITEDELIMS[256];

/*
Note 1: WHITEC[0] must be 0, or else the first loop in nextlexeme()
	won't terminate.
Note 2:	DELIMS[0] = SUBDELIMS[0] = COMMA[0] = BTRNQD[0] must be set to 1,
	or else the second loop of nextlexeme() won't terminate.
*/

static WS *ws = NULL;
static int nlxm;							/* # stringlets in ws       */
static int clxm;							/* # current lxm        */

static uchar *lxnextp;					/* ptr to next word in the line being parsed  */
static uchar *bgnp;
static uchar prevc;						/* char that was overwritten by terminating null of previous lexeme */

static uchar *delim;
static uchar *lxm2b;
static uchar *lxm2s;
static uchar tkn2s[] = ">>>=<===!=/==~/~";

/* Return p + (0, 1 or 2)==length of the delim-based lexeme.
*/
static uchar *stdlxm(uchar *p)
{
	uchar *q;
	unsigned char c;
	unsigned char c1;

	if (lxm2b[c = *p])
		for (c1 = p[1], q = lxm2s; (q = strchr(q, c)) != NULL; q += 2)
			if (q[1] == c1)
				return p + 2;
	return ++p;
}

/* *p is either ' or "; find a matching quote   */
/* and terminate it with \0         */
static uchar *quotedword(uchar *p)
{
	uchar c;
	uchar sdq;

	sdq = *p;							/* single or double qoute   */
  matchsdq:
	while ((c = *++p) != 0 && (c != sdq))
		;
	if (c != sdq)
		emsg = "unmatched quote";
	else
	{
		if (p[-1] == '\021')
			goto matchsdq;				/* ^Q */
		++p;
	}
	return p;
}

/* Determine the extent of the next lexeme, and null-terminate it,
 return ptr to the 1st non-space uchar of the lexeme. [bgnp .. lxnextp-1]
 is the lexeme.  If bgnp >= lxnextp implies that there are no more lexemes
 on this line.
*/
static uchar *nextlexeme(void)
{
	uchar *p;
	uchar *q;
	int tlx;					/* length of token  */

	p = lxnextp;
	q = delim;
	*p = prevc;
	if (*p == '\0')
	{
		bgnp = p;
		return NULL;
	}
	while (WHITEC[(unsigned char)*p])
		p++;							/* Note 1 */
	bgnp = p;
	while (!q[(unsigned char)*p])
		p++;							/* Note 2 */
	tlx = (int)(p - bgnp);
	if (!tlx)							/* lexeme length == 0; so, that was a delimiter */
	{
		if ((*p == '\'') || (*p == '"'))
			p = quotedword(p);
		else if (*p)
			p = stdlxm(p);
	}
	lxnextp = p;
	prevc = *p;
	*p = '\0';
	return bgnp;
}


/* Tokenize the given string p, and return a ws-format string.  Each stringlet
 in ws will be a lexeme. dlm is the string of delimiters; t2 is a string 2-char
 long special lexemes that begin with a non-white delimiter.  dlm may or
 may not include white chars.
*/
WS *lex(uchar *p, uchar *dlm, uchar *t2)
{
	static int lxinited = 0;

	if (!lxinited)
	{
		lxinited = 1;
		charset(WHITEC, " \t\r\n", 1);
		charset(WHITEDELIMS, " \t\r\n", 1);
		charset(DELIMS, "; \t\r\n`'\"", 1);
		charset(SUBDELIMS, "!@#$%^&-=+`{}:;'\"\\|,.<>/", 1);
		charset(COMMA, ",", 1);
		charset(BTRNQD, " \t\r\n'\"", 1);
		/* Note 2 */
		WHITEDELIMS[0] = DELIMS[0] = SUBDELIMS[0] = COMMA[0] = BTRNQD[0] = (uchar) 1;
		charset(TKN2, ">=<!/~", 1);
		charset(EMPTY2, ES, 1);
	}
	delim = dlm;
	lxm2b = t2;
	lxm2s = t2 == TKN2 ? tkn2s : ES;
	if (p == NULL)
		p = ES;
	prevc = *p;
	lxnextp = p;
	freews(ws);
	ws = initws();
	for (nlxm = clxm = 0; (p = nextlexeme()) != NULL; nlxm++)
		strwcat(ws, p, 1);
	return ws;
}

/* Use up lex ws: used in doing args for $n substitutions
*/
WS *useuplexws(void)
{
	WS *w;

	w = ws;
	ws = NULL;
	nlxm = clxm = 0;
	return w;
}

uchar *lexgetword(void)
{
	return ws && clxm < nlxm ? nthstr(ws->ps, clxm++) : ES;
}

uchar *lexlastword(void)
{
	return ws && clxm < nlxm ? nthstr(ws->ps, --nlxm) : ES;
}

uchar *lextail(void)
{
	return catall(ws, clxm);
}

void appendlextail(WS *w)
{
	appendws(w, ws, clxm);
}

/* cat all stringlets numbered 0..nlxm-1, return result */
uchar *lexhead(void)
{
	if (ws)
		ws->ns--;
	return catall(ws, 0);
}

/* If there is a ';' stringlet among those in ws, null it, and return
 the remainder as a regular (ie., one \0, non-WSPS) string.
*/
uchar *lexsemicol(void)
{
	int i;
	int n;
	uchar *p;
	uchar *q;

	p = ws ? ws->ps : NULL;
	n = p ? ws->ns : 0;
	for (i = 0; (i < n); i++)
	{
		if (*p == ';' && p[1] == '\0')
		{
			q = catall(ws, i + 1);
			nlxm = ws->ns = i;
			*p = '\0';
			return q;					/* <=== */
		}
		p += strlen(p) + 1;
	}
	return NULL;
}

void lexaccept(WS *w)
{
	freews(ws);
	ws = w;
	clxm = 0;
	nlxm = (w ? w->ns : 0);
}

typedef struct SE
{
	int cx;
	int nx;
	WS *ws;
	struct SE *next;

} SE;

static SE *sp = NULL;

void lexpush(void)
{
	SE *tp;

	tp = (SE *) gmalloc(((uint) sizeof(SE)));
	if (tp == NULL)
		return;
	tp->cx = clxm;
	tp->nx = nlxm;
	tp->ws = ws;
	tp->next = sp;
	sp = tp;
	useuplexws();
}


void lexpop(void)
{
	SE *tp;

	tp = sp;
	if (tp == NULL)
		return;
	freews(ws);
	clxm = tp->cx;
	nlxm = tp->nx;
	ws = tp->ws;
	sp = tp->next;
	gfree(tp);
}

WS *lexdup(void)
{
	return dupws(ws);
}
