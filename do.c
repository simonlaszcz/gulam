/*
 * $Header: d:\home\projects\gulam\RCS\do.c,v 1.6 2023/12/15 22:45:36 slaszcz Exp $ $Locker:  $
 * ======================================================================
 * $Log: do.c,v $
 * Revision 1.6  2023/12/15  22:45:36  slaszcz
 * Don't unquote the cmdln of external commands, otherwise args would be
 * split on all spaces.
 *
 * Revision 1.5  2023/12/13  21:08:22  slaszcz
 * If env_style is not argv and the cmdln would be truncated, then prompt
 * the user whether to continue with cmd execution.
 *
 * Revision 1.4  2023/12/12  21:06:50  slaszcz
 * The env_style 'mw' has been renamed 'argv', reflecting that it is an
 * implementation of the official protocol.
 * The argv env_style is enabled by default.
 * The argv protocol is used only when enabled and necessary, i.e. the
 * cmdln exceeds 124 bytes. This improves compatibility with progs that
 * don't understand it; the cmdln length was always set to 127 which caused
 * malformed arguments.
 *
 * Revision 1.3  2023/12/07  10:05:30  slaszcz
 * New shell variable: restore_cwd_after_exec. When true, the cwd
 * is stored before an external cmd is executed. The cwd is then restored
 * when the cmd completes.
 *
 * Revision 1.2  2023/12/06  00:24:50  slaszcz
 * initial
 *
 * Revision 1.1  2023/12/03  23:04:06  slaszcz
 * initial
 *
 * Revision 1.1  1991/09/10  01:02:04  apratt
 * First CI of AKP
 *
 * Revision: 1.9 89.06.16.17.23.18 apratt 
 * Header style change.
 * 
 * Revision: 1.8 89.04.14.23.11.50 Author: apratt
 * Added use of strrchr to find .g in file name to see if it is
 * a batch file... Previously, used index(), so ..\foo.g wouldn't be
 * executed right.
 * 
 * Added command-line length byte = 0x7f if env_style == mw.
 * 
 * Revision: 1.7 89.03.10.17.13.16 Author: apratt
 * Fixed a REALLY STUPID bug: args were processed according to b->type
 * for the built-in command lexically after the command name,
 * if the command wasn't a built-in.  Now it's processed as a '1'
 * meaning it might change dir stuff, but no other strange handling.
 * 
 * Revision: 1.6 89.03.10.14.23.52 Author: apratt
 * Fixed range checking for command tail.  Maxes out at 7e as it should.
 * 
 * Revision: 1.5 89.03.10.13.56.06 Author: apratt
 * Stomped cmdline tail length to max 0x7e so it's not a lie.
 * 
 * Revision: 1.4 89.02.07.13.31.34 Author: apratt
 * Reinstated mem, but made it more innocuous
 * 
 * Revision: 1.3 89.02.01.15.06.26 Author: apratt
 * oops
 * 
 * Revision: 1.2 89.02.01.14.45.34 Author: apratt
 * Removed "mem" command -- doesn't work for TOS 1.4.
 * 
 * Revision: 1.1 88.06.03.15.39.00 Author: apratt
 * Initial Revision
 * 
 */

/*
	docmd.c of gulam --  builtin commands			7/20/86

	copyright (c) 1986 pm@Case
*/

#include "ue.h"

int state = TRUE;						/* == T => do the cmd; == F => skip it  */
long valu;								/* result of calling the function   */
uchar *strg;							/* result string, if any        */
uchar *emsg;							/* error msg string, if any errors  */
uchar negopts[256];						/* -ve options              */
uchar posopts[256];						/* +ve options              */
int fda[4] = { MINFH - 1, MINFH - 1, MINFH - 1, MINFH - 1 };
uchar *rednm[2] = { NULL, NULL };		/* names of redirections    */

int outappend = 0;


static uchar Sgulamend[] = "gulamend.g";

/* Collect options from the users command.  This is invoked only
if the cmd is a builtin. All non-option words are given back to the
lex module. */

void addoptions(uchar *p)
{
	uchar *q;
	int i;

	q = (*p == '+' ? posopts : negopts);
	while ((i = (int) *++p) != 0)
		q[i] = (uchar) 1;
}


static void setdate(uchar *arg)
{
	UNUSED(arg);
	gsdatetime(lexgetword());
}


/* "run" the file given by the pathname g */
static void run(uchar *g, uchar *cmdln, uchar *envp)
{
	uchar *p;
	uchar *cwd = NULL;

	if (varnum(RestoreCwdAfterExec))	
		cwd = gfgetcwd();
	emsg = NULL;
	p = strrchr(g, '.');
	if (p != NULL)
	{
		if (isgulamfile(p))
		{
			valu = batch(g, cmdln, envp);
			goto restore_cwd;
		}
	}

	keyreset(FALSE, 1);
	valu = execfile(g, cmdln, envp);
	keysetup();
	setgulam();							/* set _shell_p ptrs again  */
restore_cwd:
	if (cwd != NULL) {
		cd(cwd);
		cwdvar();
	}
}


/**
* Prepare command tail, and environment for Pexec() 
* flag=1 denotes .g file executed using 'source' cmd
*/
#if TOS
static void mkcmdenv(int flag, char *pgm, char **cmdp, char **envp)
{
	static uchar ARGVVAL[] = "ARGV=";
	uchar *p = NULL, *e = NULL;
	WS *ws = NULL, *cmd = NULL;
	UNUSED(pgm);
	*cmdp = *envp = NULL;
		
	/* duplicate the cmdln as it gets trashed by lextail and we
	might need it for ARGV */
	cmd = lexdup();
	
	ws = initws();
	strwcat(ws, ES, 1);				/* 1 byte for length */
	strwcat(ws, lextail(), 0);
	p = *cmdp = ws->ps;
	
	/* 'cmdln' of Pexec needs length */
	/* we only get a byte to store length and ws->nc is an int */
	if (ws->nc > 255)
		*p = 255;
	else
		*p = (uchar) (ws->nc - 1);		
	gfree(ws);
	ws = NULL;
	
	/* cmdline size check */
	/* only use the ARGV protocol if cmdline is too long */
	if (*p > 124) {
		*p = 124;
		if (flag == 0) {
			e = varstr(EnvStyle);
			if (e == NULL || *e == '\0')
				e = ENVSTYLE_GU;
			if (strcmp(e, ENVSTYLE_ARGV) == 0)
				*p = 127;	
		}
		if (*p == 124 && mlyesno("Command line will be truncated. Continue? [yn] ") != TRUE) {
			emsg = "Aborted";
			goto cleanup;
		}
	}
	
	ws = dupenvws(0);
	if (*p == 127) {
		strwcat(ws, ARGVVAL, 0);
		appendws(ws, cmd, 0);
	}
	*envp = ws->ps;
	
cleanup:
	gfree(ws);	
	gfree(cmd);
}
#else
static void mkcmdenv(int flag, char *pgm, char **cmdp, char **envp)
{
	uchar *p;
	WS *ws;

	ws = dupenvws(0);
	*envp = ws->ps;
	gfree(ws);

	ws = initws();
	strwcat(ws, lextail(), 0);
	if (ws == NULL)
		*cmdp = NULL;
	else
		p = *cmdp = ws->ps;
	gfree(ws);
}
#endif

/* Execute file p: We search the hash table first; then try as is;
then try appending .tos|.ttp|.ttp|.g.  If flag != 0, just see if p is
batch-able as is.  */

static void lexec(uchar *p, int flag)
{
	uchar *q;
	uchar *lst;
	int i, n;
	uchar *cmdln;
	uchar *envp;
	uchar *g;

	mkcmdenv(flag, p, &cmdln, &envp);
	if (emsg)
		goto freecmdln;
	if (flag)
	{
		valu = batch(p, cmdln, envp);
		goto freecmdln;
	}
	if ((q = hashlookup(0, p)) != NULL)
	{
		run(q, cmdln, envp);
		goto freecmdln;
	}

	q = fnmsinparent(p);
	lst = strg;
	strg = NULL;
	n = matchednms(lst, q, 0);			/* first try as-is in current dir    */
	if (n == 1)
	{
		run(p, cmdln, envp);
		if (valu != EFILNF)
			goto freelst;
	}

	valu = EFILNF;
	g = NULL;
	n = 0;

#if	TOS
	if ((q = str3cat(q, ALL_EXE_BRE, ES)) != NULL)
	{
		n = matchednms(lst, q, 1);
		gfree(q);
	}
	if ((g = str3cat(p, ".xxx", ES)) != NULL)
	{
		if (n > 0)
		{
			for (q = g + strlen(p) + 1, i = 0; i < SZext - 1; i += 4)
			{
				strcpy(q, &ext[i]);
				run(g, cmdln, envp);
				if (valu != EFILNF)
					break;
			}
		}
	} else
		valu = ENSMEM;
#endif
	if (valu == EFILNF && (q = hashlookup(1, p)) != NULL)
	{
		run(q, cmdln, envp);
		goto freelst;
	}
	if (valu == ENSMEM)
		emsg = "not enough memory";
	if (valu == EFILNF)
		emsg = (char *) sprintp("%s: not found", p);

	gfree(g);
  freelst:gfree(lst);
  freecmdln:gfree(cmdln);
	gfree(envp);
	insertvar(Status, itoal(valu));
}


void source(uchar *arg)
{
	UNUSED(arg);
	lexec(lexgetword(), 1);
}


void echo(uchar *arg)
{
	UNUSED(arg);
	strg = gstrdup(lextail());
}


static void bexit(uchar *arg)
{
	char *p;
	int n;

	UNUSED(arg);
	n = atoi(lexgetword());
	if (inbatchfile())
	{
		csexit(n);
		return;
	}

	if (quickexit(0, 1) != 1)
		return;
	mouseon(NULL);
	p = fnmpred('e', Sgulamend);
	if (*p == '1')
		processcmd(gstrdup(Sgulamend), 0);
	savehistory();
	keyreset(FALSE, 1);
	fontreset();
  	invisiblecursor();
  	cd(varstr("home"));
	exit(n);
}


/* Ask, if the -i option is present, if "op p?".  Return 1 if okayed,
0 if nayed, and ABORT (==2), if aborted.  */

int asktodoop(uchar *op, uchar *p)
{
	return negopts['i'] ? mlyesno(sprintp("%s %s?", op, p)) : TRUE;
}


/* do f() for each word in current cmd */
void doforeach(uchar *op, void (*f)(uchar *arg))
{
	uchar *p;
	int flag;

	for (;;)
	{
		p = lexgetword();
		if (*p == '\0')
			break;
		flag = asktodoop(op, p);
		if (flag == ABORT)
			break;
		if (flag)
			(*f) (p);
		if (useraborted())
			break;
	}
	mlmesg(ES);
}


void setuekey(uchar *arg)
{
	uchar *p;
	int n;

	UNUSED(arg);
	n = negopts['m'] ? MINIKB : negopts['g'] ? GUKB : REGKB;
	p = lexgetword();
	bindkey(n, p, lexgetword());		/* see ue's kb.c */
}


/* note redirected filename */
static int noteredfnm(int r, char *fnm)
{
	char c;

	c = *fnm;
	if (c == '>')
	{
		r = 1;
		outappend = 0;
		if (fnm[1] == '>')
		{
			fnm++;
			outappend = 1;
		}
	} else
	{
		if (c == '<')
			r = 0;
		else if (r <= 1)
			fnm--;
		else
			return 3;
	}

	if (fnm[1])
	{
		gfree(rednm[r]);
		unquote(fnm + 1, fnm + 1);
		rednm[r] = gstrdup(fnm + 1);
		r = 2;
	}
	return r;
}


/*
 Unquote the arguments to cmd (ie., the ws->ps stringlets) in place.
 Also, note redirections and options (if builtin cmd).
 As we handle the redirs, p will lag behind q;
 if there are no redirs, p will == q all the time.

  builtin commands have the form
                               cmd options args

  using /posopts arrays imply that options of all builtin commands
  are +/- single letter only.
  So we have, for example,
                               chmod +w file
  but we cannot have
                               chmod -r reference +w files

  we need to know which part of the command line a '+' or '-' appears so we can
  interpret it as an option or part of an expression

 input:  /cmd/ /-f/ /arg1/ ...
 output: /cmd/ /arg1/ ...
         & nopts[], popts[] set accordingly
*/
static void processargs(int builtin, int unqflag)
{
	WS *ws;
	uchar *p;
	uchar *q;
	int i;
	int k;
	int n;
	int redi;
	int brl;
	int finopt = -1;					/* -1 -> cmd part, 0 -> options, >0 -> args */

	ws = useuplexws();
	redi = 3;
	if (ws && (q = p = ws->ps) != NULL)
	{
		for (brl = 0, i = ws->ns; i-- > 0; q += n + 1)
		{
			n = (int)strlen(q);
			k = *q;
			if (k == '{')
				brl++;
			else if (k == '}')
				brl--;
			redi = (brl == 0 ? noteredfnm(redi, q) : 3);
			if (redi < 3)
			{
				ws->ns--;
			} else if (builtin && finopt == 0 && (k == '-' || k == '+'))
			{
				addoptions(q);
				ws->ns--;
			} else
			{
				finopt++;
				if (unqflag)
					p = unquote(p, q) + 1;
				else
				{
					if (p < q)
						cpymem(p, q, n);
					p += n + 1;
				}
			}
		}
		*p++ = '\0';
		ws->nc = (int) (p - ws->ps);
	}
	lexaccept(ws);
	doredirections();
}


#if XMDM
static void utemul(uchar *arg)
{
	UNUSED(arg);
	temul(FALSE, 1);
}
#endif


			/* b->type values */
#define	OWNWAY		2					/* cmd handles args own way */
#define	ARGREQ		4					/* 1=>0/1 arg reqd      */
#define	PREFIX		6					/* cmd is a prefix to others    */
#define	NOUNQU		8					/* don't do unquote for this    */
#define	DOEACH		10					/* do the cmd for each arg  */
#define	BATCHS		12					/* ok in batch files only   */
/* +1 	added 		if 	cmd may change dir contents	*/

/*   built-in commands      */
/* must list in alpha order */
static struct bi
{
	const char *name;						/* name of builtin cmd      */
	uchar len;							/* strlen of the cmd name   */
	uchar type;							/* cmd type; see below      */
	void (*rtnp)(uchar *);
} builtins[] = {
	{ "alias", 5, OWNWAY, alias },
	{ "cat", 3, DOEACH, cat },
	{ "cd", 2, ARGREQ, cdcmd },
	{ "chmod", 5, DOEACH + 1, gchmod },
	{ "copy", 4, OWNWAY + 1, cp },
	{ "cp", 2, OWNWAY + 1, cp },
	{ "date", 4, ARGREQ, setdate },
	{ "df", 2, DOEACH, df },
#if	TOS
	{ "dm", 2, OWNWAY, dm },
#endif
	{ "dirc", 4, OWNWAY, fnmtbl },
	{ "dirs", 4, OWNWAY, dirs },
	{ "echo", 4, OWNWAY, echo },
	{ "egrep", 5, OWNWAY, egrep },
	{ "ef", 2, BATCHS, cselse },			/* invoke even if state==F */
	{ "endfor", 6, BATCHS, csendfor },
	{ "endif", 5, BATCHS, csendif },		/* invoke even if state==F */
	{ "endwhile", 8, BATCHS, csendwhile },
	{ "exit", 4, OWNWAY, bexit },
	{ "fg", 2, OWNWAY, fg },
	{ "fgrep", 5, OWNWAY, fgrep },
	{ "foreach", 7, BATCHS, csforeach },
#if	TOS
	{ "format", 6, DOEACH + 1, format },
	{ "gem", 3, PREFIX, Gem },
#endif
	{ "grep", 4, OWNWAY, egrep },
	{ "help", 4, OWNWAY, gulamhelp },
	{ "history", 7, OWNWAY, history },
	{ "if", 2, BATCHS, csif },				/* invoke even if state==F */
	{ "kb", 2, OWNWAY, setuekey },
#if	LPRINT
	{ "lpr", 3, OWNWAY, lpr },
#endif
	{ "ls", 2, OWNWAY, ls },
#if	TOS
	{ "mem", 3, OWNWAY, mem },
#endif
	{ "mkdir", 5, DOEACH + 1, gmkdir },
	{ "more", 4, OWNWAY, moreue },
#if	TOS
	{ "mson", 4, OWNWAY, mouseon },
	{ "msoff", 5, OWNWAY, mouseoff },
#endif
	{ "mv", 2, OWNWAY + 1, mv },
#if	TOS
	{ "peekw", 5, OWNWAY, lpeekw },
	{ "pokew", 5, OWNWAY, lpokew },
#endif
	{ "popd", 4, OWNWAY, popd },
#if	LPRINT
	{ "print", 5, OWNWAY, print },
#endif
	{ "printenv", 8, OWNWAY, printenv },
	{ "pushd", 5, ARGREQ, pushd },
	{ "pwd", 3, OWNWAY, pwd },
	{ "rehash", 6, OWNWAY, rehash },
	{ "ren", 3, OWNWAY + 1, grename },
	{ "rm", 2, DOEACH + 1, rm },
	{ "rmdir", 5, DOEACH + 1, grmdir },
#if	XMDM
	{ "rx", 2, DOEACH + 1, rxmdm },
#endif
	{ "set", 3, NOUNQU, setvar },
	{ "setenv", 6, OWNWAY, gsetenv },
	{ "source", 6, OWNWAY, source },
#if	XMDM
	{ "sx", 2, DOEACH, sxmdm },
	{ "te", 2, OWNWAY, utemul },
	{ "teexit", 6, OWNWAY, teexit },
#endif
	{ "time", 4, PREFIX, gtime },
	{ "touch", 5, DOEACH, touch },
	{ "ue", 2, OWNWAY + 1, ue },
	{ "unalias", 7, OWNWAY, unalias },
	{ "unset", 5, OWNWAY, unsetvar },
	{ "unsetenv", 8, OWNWAY, gunsetenv },
	{ "which", 5, OWNWAY, cmdwhich },
	{ "while", 5, BATCHS, cswhile },
	{ NULL, 0, OWNWAY, NULL }
};

#define NBC	((int)(sizeof(builtins) / sizeof(struct bi)))
#define	MXL	8							/* length of longest built-in cmd   */
#define	MXC	8							/* #columns in the showbuiltin list */

void showbuiltins(void)
{
	struct bi *b;
	uchar *s;
	WS *ws;

	outstr(sprintp("%d built-in commands:", NBC - 1));

	ws = initws();
	if (ws == NULL)
		return;
	for (b = builtins; b->name != NULL; b++)
		strwcat(ws, b->name, 1);
	s = pscolumnize(ws->ps, -1, 0);
	outstr(s);
	gfree(s);
	freews(ws);
}


void docmd(void)
{
	struct bi *b;
	uchar *p;
	int lw;
	int i;
	int btype;

	valu = 0;
	strg = emsg = NULL;
	charset(negopts, ES, 1);
	charset(posopts, ES, 1);

	p = lexgetword();
	lw = (int)strlen(p);
	i = 1;
	if (lw <= MXL)
		for (b = builtins; b->name; b++)
		{
			if ((int) b->len != lw)
				continue;
			i = strcmp(p, b->name);
			if (i <= 0)
				break;
		}

	/*
	 * n.b. btype is compared against values, not bits, so you'd
	 * better not try to set more than one bit (except the +1 bit).
	 */
	 
	/* We shouldn't unquote the cmdln of external commands */
	if (i != 0)
		btype = NOUNQU;
	else
		btype = (int) b->type;

	lw = btype & 0x0001;
	btype &= 0x00FE;					/* i==0 => built-in command */
	if (i == 0 && btype == BATCHS)
	{
		if (!inbatchfile())
			goto skipit;
	} else if (state != TRUE)
		goto skipit;

	if (btype != PREFIX)
	{
		processargs(i == 0, btype != NOUNQU);
		p = lexgetword();				/* see above rtn ... */
	}
	if (i == 0)
	{
		if (btype == DOEACH)
			doforeach(p, b->rtnp);
		else
			(*(b->rtnp)) (p);
	} else
	{
		lexec(p, 0);
		lw = 1;
	}

  skipit:
	if (lw || varnum("dir_cache") == 0)
		freewtbl();
}
