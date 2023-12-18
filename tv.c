/*
 * $Header: d:\home\projects\gulam\RCS\tv.c,v 1.3 2023/12/12 21:06:50 slaszcz Exp $ $Locker:  $
 * ======================================================================
 * $Log: tv.c,v $
 * Revision 1.3  2023/12/12  21:06:50  slaszcz
 * The env_style 'mw' has been renamed 'argv', reflecting that it is an
 * implementation of the official protocol.
 * The argv env_style is enabled by default.
 * The argv protocol is used only when enabled and necessary, i.e. the
 * cmdln exceeds 124 bytes. This improves compatibility with progs that
 * don't understand it; the cmdln length was always set to 127 which caused
 * malformed arguments.
 *
 * Revision 1.2  2023/12/07  10:05:30  slaszcz
 * New shell variable: restore_cwd_after_exec. When true, the cwd
 * is stored before an external cmd is executed. The cwd is then restored
 * when the cmd completes.
 *
 * Revision 1.1  2023/12/06  23:33:56  slaszcz
 * initial
 *
 * Revision 1.1  1991/09/10  01:02:04  apratt
 * First CI of AKP
 *
 * Revision: 1.5 90.02.13.14.33.14 apratt 
 * Added Mdmport[] and setmdmport() in variable table - see gioatari.c.
 * 
 * Revision: 1.4 90.02.05.14.24.10 apratt 
 * Added variable Font[] = "font" with handler font().
 * 
 * Revision: 1.3 89.06.16.17.24.06 apratt 
 * Header style change.
 * 
 * Revision: 1.2 89.06.02.13.42.00 Author: apratt
 * Compiler generated incorrect code for tbldelete when it
 * had register variables.  Don't ask my why; I just ripped them out.
 * 
 * Revision: 1.1 88.06.03.15.38.46 Author: apratt
 * Initial Revision
 * 
 */

/*sep 9, 86

	tbl.c 	of gulam -- a simple table mechanism
	alias.c	of gulam -- alias module
	env.c 	of gulam -- environment maintainer
	var.c	of gulam -- shell var maintainer
	hlp.c   of gulam -- help, what little there is

	copyright (c) 1986 pm@Case

	These modules are together here for no good reason;
	they were just too short!
*/

#include "ue.h"

/* Notes: This table module can create and maintain simple tables of two
columns; the first column has to be a string, the second column is whatever.
The first node in the TBLE list is used as a header.  The key-, and elm-fields
(except those of hdr) point to malloc'd areas; key pts to a string, elm can
be arbitrary.
*/


TBLE *tblfind(TBLE *t, uchar *k)
{
	TBLE *p;

	p = NULL;
	if (k && (p = t) != NULL)
		do
		{
			p = p->next;
		} while (p && (strcmp(p->key, k)) != 0);
	return p;
}

void tbldelete(TBLE *t, uchar *k)
{
	TBLE *p;
	TBLE *q;

	if ((k == NULL) || ((p = t) == NULL))
		return;
	do
	{
		q = p;
		p = p->next;
	} while (p && (strcmp(p->key, k)) != 0);
	if (p)
	{
		q->next = p->next;
		gfree(p->key);
		gfree(p->elm);
		gfree(p);
	}
}


void tblinsert(TBLE *t, uchar *k, uchar *e)
{
	TBLE *p;

	if ((t == NULL) || (k == NULL) || (e == NULL))
		return;
	tbldelete(t, k);
	if ((p = (TBLE *) gmalloc((uint) sizeof(TBLE))) != NULL)
	{
		p->key = k;
		p->elm = e;
		p->next = t->next;
		t->next = p;
	}
}

/* Create a new table. */

TBLE *tblcreate(void)
{
	TBLE *p;

	if ((p = (TBLE *) gmalloc(((uint) sizeof(TBLE)))) != NULL)
	{
		p->key = NULL;
		p->elm = NULL;
		p->next = NULL;
	}
	return p;
}


void tblfree(TBLE *t)
{
	TBLE *p;
	TBLE *q;

	for (p = t; p; p = q)
	{
		q = p->next;
		gfree(p->key);
		gfree(p->elm);
		gfree(p);
	}
}

/* Make a WS out of table's contents, in the requested style.
 0 => columnar format (a->key's only)	1 => key \t elm \r\n
 2 => key = elm \r\n			3 => key = elm \0
 4 => key =\0 elm \0
*/
WS *tblws(TBLE *t, int style)
{
	TBLE *a;
	int i;
	WS *ws;
	uchar seps[2];

	if (t == NULL)
		return NULL;
	ws = initws();
	if (ws == NULL)
		return NULL;
	seps[0] = (style < 2 ? '\t' : '=');	/* separator uchar */
	seps[1] = '\0';
	i = (style == 0 ? 1 : 0);
	for (a = t->next; a; a = a->next)
	{
		strwcat(ws, a->key, i);
		if (style)
		{
			strwcat(ws, seps, 0);
			if (style == 4)
				strwcat(ws, ES, 1);
			strwcat(ws, a->elm, 0);
			if (style < 3)
				strwcat(ws, CRLF, 0);
			else
				strwcat(ws, ES, 1);
		}
	}
	return ws;
}


uchar *tblstr(TBLE *t, int style)
{
	WS *ws;
	uchar *p;

	ws = tblws(t, style);
	if (ws == NULL)
		return NULL;
	p = ws->ps;
	if (style == 0)
	{
		p = pscolumnize(p, -1, 0);
		gfree(ws->ps);
	}
	gfree(ws);
	return p;
}


static TBLE *alip = NULL;

/* show/set alias(es)   */
void alias(uchar *arg)
{
	uchar *p;
	uchar *q;

	UNUSED(arg);
	p = lexgetword();
	if (*p)
	{
		q = lextail();
		if (*q)
		{
			if (alip == NULL)
				alip = tblcreate();
			tblinsert(alip, gstrdup(p), gstrdup(q));
		} else
			strg = gstrdup(getalias(p));
	} else
		strg = tblstr(alip, 1);
}


void unalias(uchar *arg)
{
	UNUSED(arg);
	tbldelete(alip, lexgetword());
}

/* return the alias'd string, if any, or itself */
uchar *getalias(uchar *p)
{
	TBLE *a;

	a = tblfind(alip, p);
	return a ? a->elm : p;
}

/* eof alias.c	*/
/* env.c of shell -- environment maintainer			*/

static TBLE *envp = NULL;

/* get string value of env var whose    */
/* name appears in *p...        */
uchar *ggetenv(uchar *p)
{
	TBLE *a;

	a = tblfind(envp, p);
	return a ? a->elm : NULL;
}

/* insert name p with string value q    */
void gputenv(const char *p, const char *q)
{
	if (envp == NULL)
		envp = tblcreate();
	tblinsert(envp, gstrdup(p), gstrdup(q));
}

/* duplicate the env string; if flag, put in \r\n */
WS *dupenvws(int flag)
{
	flag = (flag ? 2 : 3);
	if (flag == 3 && strcmp(varstr(EnvStyle), ENVSTYLE_BM) == 0)
		flag = 4;
	return tblws(envp, flag);
}

void printenv(uchar *arg)
{
	WS *ws;

	UNUSED(arg);
	ws = dupenvws(1);
	strg = ws->ps;
	gfree(ws);
}

void gsetenv(uchar *arg)
{
	uchar *p;
	uchar *q;

	UNUSED(arg);
	p = lexgetword();
	q = lexgetword();
	if (*p)
		gputenv(p, q);
	else
		printenv(NULL);
}

void gunsetenv(uchar *arg)
{
	UNUSED(arg);
	tbldelete(envp, lexgetword());
}

void readinenv(uchar *p)
{
	uchar *q;
	uchar *r;

	if (p == NULL)
		return;
	for (; *p; p = r + strlen(r) + 1)
	{
		q = p;
		p = strchr(p, '=');
		if (p == NULL)
			break;
		*p = '\0';
		r = p + 1;
		if (*r == '\0')
			r++;
		gputenv(q, r);
		*p = '=';
	}
}

/* var.c == shell variables and their operations	*/

uchar Gulam[] = "Gulam";
uchar Shell[] = "shell";
uchar GulamHelp[] = "gulam_help_file";
uchar Nrows[] = "nrows";
uchar OwnFonts[] = "own_fonts";
uchar History[] = "history";
uchar Cwd[] = "cwd";
uchar Home[] = "home";
uchar Rgb[] = "rgb";
uchar Path[] = "path";
uchar Status[] = "status";
uchar Verbosity[] = "verbosity";
uchar Mscursor[] = "mscursor";
uchar Ncmd[] = "ncmd";
uchar RestoreCwdAfterExec[] = "restore_cwd_after_exec";
uchar EnvStyle[] = "env_style";

#if	AtariST
uchar Font[] = "font";					/* extension by AKP */
uchar Mdmport[] = "mdmport";			/* extension by AKP */
#endif

struct SETVAR
{
	void (*fun)(void);					/* execute after updating the setvar    */
	const char *coupled;				/* iff coupled to  env var      */
	uchar *varname;						/* name of the set variable     */
};

static void rehashv(void)
{
	rehash(NULL);
}

static struct SETVAR stv[] = {
	{ NULL, "PWD", Cwd },
	{ rehashv, "PATH", Path },
	{ histinit, NULL, History },
	{ NULL, "HOME", Home },
	{ NULL, "SHELL", Shell },
	{ rdhelpfile, NULL, GulamHelp },
#if AtariST
	{ pallete, "RGB", Rgb },
	{ nrow2550, "LINES", Nrows },
	{ font, NULL, Font },					/* extension by AKP */
	{ setmdmport, NULL, Mdmport },			/* extension by AKP */
	{ mousecursor, NULL, Mscursor }
#endif
};

#define	Nstv	((uint)(sizeof(stv)/sizeof(struct SETVAR)))


static TBLE *varp = NULL;


/* return the string value of a var */
uchar *varstr(uchar *p)
{
	TBLE *a;

	a = tblfind(varp, p);
	return a ? a->elm : ES;
}

int varnum(uchar *p)
{
	uchar *q;

	q = varstr(p);
	return atoi(q);
}

void showvars(void)
{
	TBLE *p;

	if (varp == NULL)
		return;
	for (p = varp->next; p; p = p->next)
	{
		gputs(sprintp("var %D %s=%s %D\r\n", p->key, p->key, p->elm, p->elm));
	}
	gputs("--\r\n");
}


void insertvar(uchar *p, uchar *q)
{
	int i;
	void (*f)(void);

	if (varp == NULL)
		varp = tblcreate();
	q = gstrdup(q);
	unquote(q, q);
	tblinsert(varp, p = gstrdup(p), q);
	for (i = 0; i < Nstv; i++)
	{
		if (strcmp(p, stv[i].varname) == 0)
		{
			if (stv[i].coupled)
			{
				gputenv(stv[i].coupled, q);
			}
			if ((f = stv[i].fun) != 0)
				(*f) ();
			break;
		}
	}
}

void setvarnum(uchar *p, int n)
{
	insertvar(p, itoal((long) n));
}

void setvar(uchar *arg)
{
	uchar *p;
	uchar *dummy;

	UNUSED(arg);
	p = lexgetword();
	if (*p)
		insertvar(p, csexp(lexgetword(), &dummy));
	else
		strg = tblstr(varp, 1);
}

void unsetvar(uchar *arg)
{
	UNUSED(arg);
	tbldelete(varp, lexgetword());
}

/* -eof var.c-	*/
/* hlp.c	*/
static int nstate = -1;
static TBLE *atbl[3] = { NULL, NULL, NULL };

#define	XKCDNM	0						/* key code to key name     */
#define XFCDNM	1						/* fn code to fn name       */
#define	XGCMDD	2						/* Gulam cmd descriptive name   */
#define	XUNSHI	3						/* key codes for unshifted keys */

static void rdhelpln(uchar *q, int nb)
{
	uchar *p;

	lnn++;
	ntotal += nb;
	if (q[0] == '#')
	{
		nstate = q[1] - '1';
		return;
	}
	if (XKCDNM > nstate || nstate > XGCMDD)
		return;
	p = strchr(q, ' ');
	if (p == NULL || p == q)
		return;
	*p = '\0';
	tblinsert(atbl[nstate], gstrdup(q), gstrdup(p + 1));
	*p = ' ';
}

/* Read the Gulam help file.  Called from setvar(). */

void rdhelpfile(void)
{
	uchar *p;
	int i;

	nstate = -1;
	p = varstr(GulamHelp);
	if (*p == '\0')
		return;
	for (i = XKCDNM; i <= XGCMDD; i++)
	{
		tblfree(atbl[i]);
		atbl[i] = tblcreate();
	}
	frdapply(p, rdhelpln);
	gputs(CRLF);
}

/* Transform a key code into a name, using the above table.
*/
uchar *findname(int x, int k)
{
	uchar *kp;
	TBLE *t;

	kp = itoar(0x1000L + (long) k, 16) + 1;	/* skip the leading '1' */
	t = tblfind(atbl[x], kp);
	return t ? t->elm : ES;
}
