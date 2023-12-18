/*
 * $Header: d:\home\projects\gulam\RCS\main.c,v 1.4 2023/12/15 22:44:04 slaszcz Exp $ $Locker:  $
 * ======================================================================
 * $Log: main.c,v $
 * Revision 1.4  2023/12/15  22:44:04  slaszcz
 * Added a startup message
 *
 * Revision 1.3  2023/12/12  21:06:50  slaszcz
 * The env_style 'mw' has been renamed 'argv', reflecting that it is an
 * implementation of the official protocol.
 * The argv env_style is enabled by default.
 * The argv protocol is used only when enabled and necessary, i.e. the
 * cmdln exceeds 124 bytes. This improves compatibility with progs that
 * don't understand it; the cmdln length was always set to 127 which caused
 * malformed arguments.
 *
 * Revision 1.2  2023/12/05  23:31:52  slaszcz
 * Show the cursor before displaying the prompt.
 *
 * Revision 1.1  1999/12/31  01:26:24  slaszcz
 * initial
 *
 * Revision 1.1  1991/09/10  01:02:04  apratt
 * First CI of AKP
 *
 * Revision: 1.8 90.10.22.11.01.20 apratt 
 * Changed the date to 10/22/90 because 10/11 was a pretty wide release
 * and this is different: in fop.c, moved userfeedback so it happens
 * for mv as well as cp.
 * 
 * Revision: 1.7 90.10.11.14.22.46 apratt 
 * Changed date after kbad/akp to "10/11/90"
 * 
 * Revision: 1.6 89.12.04.16.13.16 apratt 
 * Updated version number (added date after akp/kbad line).
 * 
 * Revision: 1.5 89.06.16.17.24.00 apratt 
 * Header style change.
 * 
 * Revision: 1.4 89.02.14.14.57.54 Author: apratt
 * Added masterdate, which is exported to gmcatari.c and used in tooold().
 * 
 * Revision: 1.3 89.02.07.12.23.30 Author: apratt
 * Changed usage of cmdfirst to cmdprobe, zero for "don't" and incremented
 * when tested so it's only zero once.  Maybe this will work...
 * 
 * Revision: 1.2 89.02.01.16.28.18 Author: apratt
 * Added cmdfirst variable: when zero, no change. When nonzero, inhibits
 * call to useraborted() in processcmd().  Zeroed by processcmd().
 * Set in read-and-execute procedure (I forget its name).  This should
 * mean that you can type a command name, hit return, and immediately
 * start typing ahead for that command.
 * 
 * Revision: 1.1 89.01.11.15.02.34 Author: apratt
 * Initial Revision
 * 
 */

/*
	main.c -- main file of a new shell, called gulam	10/11/86

	copyright (c) 1987 pm@cwru.edu
*/

/* 890111 kbad	Modified header slightly, made error messages inverse video
*/

#ifdef __GNUC__
#include "minimal.h"					/* use customized one, not the one in include */
#endif

#include "ue.h"




/* 
 * cmdprobe: (AKP) when zero, causes useraborted not to be called.
 * It's incremented when it's checked, so it only inhibits the call once.
 */

int cmdprobe;

uchar cnmcomplete = ESC;
uchar cnmshow = CTRLD;

static uchar ID[] =						/* id of this program       */
	"\r\n\
beta-test version 1.03.04.05 890111 of\r\n\
\tyet another shell for %s\r\n\r\n\
%s Copyright %s 1987 pm@cwru.edu\r\n\
(non-profit redistribution is permitted)\r\n\r\n\
some changes by ggf@js.uucp,\r\n\
\t{akp,kbad}@atari.uucp (10/22/90)\r\n\
more changes by tho (10/30/21)\r\n\
more changes by slz (Dec 2023)\r\n\
\r\n";

unsigned long masterdate;						/* AKP: extern exported to gmcatari.c */

/**	last alpha	0.09.09.15 090687 of\r\n\ **/

static void showid(void)
{
	outstr(sprintp(ID, OS, GulamLogo, Copyright));
}

void outstrg(void)
{
	outstr(strg);
	gfree(strg);
	strg = NULL;
}

static void outemsg(void)
{
	onreversevideo();
	outstr(emsg);
	offreversevideo();
	emsg = NULL;
}

/* Make a gmalloc'd copy of the prompt; when freeing, must be gfreed
*/
uchar *getprompt(void)
{
	uchar *p;
	uchar *q;
	WS *ws;

	p = varstr("prompt");
	if (*p == '\0')
		p = gstrdup(defaultprompt);
	else
	{
		ws = expand(gstrdup(p), 0);
		p = catall(ws, 0);				/* p now == ws->ps */
		gfree(ws);
	}
	q = str3cat(p, varstr("prompt_tail"), ES);
	gfree(p);
	return q;
}

void gulamhelp(uchar *arg)
{
	UNUSED(arg);
	showid();
	showbuiltins();
	outstr("\r\nprograms accessible through hash table:");
	which(0);
	if (strg == NULL || *strg == '\0')
		outstr("none! (your hash table is empty)");
	outstrg();							/* at least gfree(strg) */
	keysetup();							/* may have been clobbered by the Pexec'd progs */
	state = TRUE;
}

unsigned int userfnminput(uchar **p, int sz, void (*fn)(uchar *r), int pexp)
{
	uchar *q;
	uchar *r;
	uchar c;
	int n;

	for (;;)
	{
		c = getuserinput(*p, sz);
		if (c != cnmcomplete && c != cnmshow)
			break;
		n = fnmexpand(p, &q, pexp, 1);	/* 1 ==> TENEX */
		if (c == cnmshow && n > 1 && fn != NULL)
		{
			r = pscolumnize(q, -1, -1);
			if (r)
			{
				(*fn) (r);
				gfree(r);
			}
			mlmesg(NULL);
		}
		gfree(q);
	}
	return c;
}

static void showcompletes(uchar *r)
{
	gputs(CRLF);
	gputs(r);
}

/* Get one (cmd) line from user.  Include esc-name
completion. */

uchar *getoneline(void)
{
	char *p;
	unsigned int u;

	tominibuf();
	if ((p = gmalloc(SZcmd)) != NULL)
	{
		*p = '\0';
		u = userfnminput(&p, SZcmd, showcompletes, 0);
		if (u == '\007')
			*p = '\0';
	}
	return p;
}

/* Get one cmd, and do it.  Called from (1) within this file, (2) from
misc.c via spawn(), and (3) via _shell_p */

void getcmdanddoit(void)
{
	char *p;

	visiblecursor();
	p = getoneline();
	gputs(CRLF);
	cmdprobe = 0;						/* AKP: inhibit first call to useraborted() */
	processcmd(p, 1);					/* processcmd() frees p */
}

void gtime(uchar *arg)									/* called from docmd(); see do.c */
{
	WS *ws;

	UNUSED(arg);
	starttick = getticks();
	ws = useuplexws();
	shiftws(ws, 1);
	gfree(execcmd(ws));
}

uchar *execcmd(WS *ws)
{
	char *qq;

	qq = NULL;
	if (stackoverflown(256))
		goto ret;
	ws = aliasexp(ws);
	if (ws && ws->ps)
	{
		lexaccept(ws);
		if (ws->ps[0])
		{
			qq = gstrdup(lexsemicol());
			docmd();
		}
	} else
	  ret:freews(ws);
	/* do not freews(ws) ow because lex() will later on */
	return qq;
}


/* Process the given line as a shell cmd: store it in history (if
savehist != 0), expand it, and finally run it.  The globals strg,
emsg, valu are initialized in docmd(), and contain the results at the
end.  The semicolon counting is there to break possible recursion in
aliassed cmds containing semicolons.  */

void processcmd(char *qq, int savehist)
{
	char *p;
	WS *ws;
	int nsemi;
	int maxsemi;

	if (qq == NULL || *qq == '\0')
	{
		gfree(qq);
		return;
	}
	emsg = NULL;
	if (strchr(qq, '!') != NULL)
	{
		p = substhist(qq);				/* p != qq => qq got freed there */
		if (p != qq)
			userfeedback(p, 0);
		outemsg();
	} else
		p = qq;
	if (savehist)
		remember(p, 1);
	ws = expand(p, 0);					/* expand frees p */
	if (emsg)
	{
		outemsg();
		freews(ws);
		return;
	}
	maxsemi = varnum("semicolon_max");
	if (maxsemi <= 0)
		maxsemi = 20;
	for (nsemi = 0;;)
	{
		if (varnum("time"))
			starttick = getticks();
		if ((++nsemi > maxsemi) || (cmdprobe++ && useraborted()))
		{
			freews(ws);
			valu = -nsemi;
			qq = NULL;
		} else
		{
			qq = execcmd(ws);
			outstrg();
			undoredirections();
			outemsg();
		}
		if (valu)
			outstr(sprintp("cmd exit code %d", (int) valu));
		valu = 0L;
		computetime();
		if (qq == NULL)
			break;						/* <== */
		lex(qq, DELIMS, TKN2);
		gfree(qq);
		ws = useuplexws();
	}
}

/* main() of Gulam.  If argc > 1, the shell is being invoked for
non-interactive work.  */

int main(int argc, char **argv, char **envp)
{
	uchar *p;
	WS *ws;
	int m;
	int nco;

	ueinit();
	if (exitue == 3)
		exit(-1);
	setgulam();

	readinenv(*envp);					/* read in the supplied environment */
	if ((p = gfgetcwd()) != NULL)
	{
		insertvar(Home, p);
		insertvar(Cwd, p);
		gfree(p);
	}
	insertvar(Ncmd, itoal(1L));
	insertvar(Verbosity, "1");
	insertvar(EnvStyle, ENVSTYLE_ARGV);
	stamptime(&masterdate);			/* AKP */

	if (argc == 1)
		goto interactive;
	processcmd(gstrdup("gulam.g"), 0);
	processcmd(gstrdup("rehash"), 0);
	for (nco = 1; argc-- > 1;)
	{
		p = *++argv;
		if (nco && strcmp(p, "-c"))
			processcmd(gstrdup(p), 0);
		else if (nco)
		{
			nco = 0;
			ws = initws();
		} else
			strwcat(ws, p, 1);
	}
	if ((nco == 0) && ws)
	{
		p = catall(ws, 0);
		processcmd(p, 0);				/* ws->ps is freed here */
		gfree(ws);
	}
	return (int) valu;

interactive:
	tioinit();
	gputenv("LINES", itoal(getnrow()));
	gputenv("COLUMNS", itoal(getncol()));
	exitue = -1;
	update();
	mlerase();
	showid();
	processcmd(gstrdup("date"), 0);
	processcmd(gstrdup("gulam.g"), 0);
	readhistory();
	m = varnum("show_mem");

	for (;;)
	{
		getcmdanddoit();				/* see bexit() in do.c */
		if (m)
			processcmd(gstrdup("mem"), 0);
	}
}
