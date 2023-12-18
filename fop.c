/*
 * $Header: d:\home\src\gulam\RCS\fop.c,v 1.2 2023/12/06 00:19:12 slaszcz Exp $ $Locker:  $
 * ======================================================================
 * $Log: fop.c,v $
 * Revision 1.2  2023/12/06  00:19:12  slaszcz
 * cwdvar() is no longer static. Now we can call this function
 * anywhere to update the cwd variable, e.g. after cd()
 *
 * Revision 1.1  1991/09/10  01:02:04  apratt
 * First CI of AKP
 *
 * Revision: 1.9 90.11.07.12.05.34 apratt 
 * Added a check in copyfio for negopts['f'] (that is, '-f').
 * If you said cp -f, it will call rm() to remove a stubborn
 * (e.g. read-only) destination file.
 * 
 * Revision: 1.8 90.10.22.11.17.30 apratt 
 * Added a message for errors other than EWRITF... I wish this code
 * wasn't so bad: if there was some error in a copy other than "no room"
 * the dest wouldn't have been removed and no error message would be
 * given.
 * 
 * Revision: 1.7 90.10.22.10.49.24 apratt 
 * Kbad wants userfeedback for mv, not just cp.  This involved just
 * moving the userfeedback call so it's before the mv check.
 * 
 * Revision: 1.6 90.10.11.12.38.00 apratt 
 * Moved Fattrib call for "cp -a" and made it -a, not +a.
 * 
 * Revision: 1.5 90.05.08.16.15.50 apratt 
 * Added a flag to cp: -a means "preserve attributes" and causes cp
 * to call Fattrib (#if TOS).
 * 
 * Revision: 1.4 90.02.01.12.18.04 apratt 
 * Added rm -f and rm -r processing.
 * 
 * Revision: 1.3 89.06.16.17.23.30 apratt 
 * Header style change.
 * 
 * Revision: 1.2 89.03.09.16.07.34 Author: apratt
 * Two things: first, check existence of the source file before deleting
 * the dest file in cpfileio or whatever; impacts mv and cp.
 * Second, don't ADD valu's when removing many files; valu will hold
 * the first nonzero value it gets.
 * 
 * Revision: 1.1 89.01.11.14.37.40 Author: apratt
 * Initial Revision
 * 
 */

/*
	fop.c -- file copy, move, remove, cd, mkdir etc	 08/02/86

	copyright (c) 1986, 1987 pm@Case

*/

/* 890111 kbad	fixed silent destination failure on copy/move
*/

#include "ue.h"

struct de								/* dir entry in the dirstk  */
{
	char *namep;
	struct de *nextp;
};

#define	MVFLAG	0x1122
static char *targetdir;					/* destination dir */
static int mvflag;						/* == MVFLAG iff mv operation */

static struct de *dirstkp;
static char DSE[] = "dir stack is empty";

void pwd(uchar *arg)
{
	UNUSED(arg);
	strg = gfgetcwd();
}

void cwdvar(void)
{
	char *p;

	p = gfgetcwd();
	insertvar("cwd", p);
	gfree(p);
}

void cdcmd(uchar *arg)
{
	char *p;

	UNUSED(arg);
	p = lexgetword();
	if (*p == '\0')
		p = varstr("home");
	cd(p);
	cwdvar();
}


/* Push onto the dir stk the given gmalloc'd str p which is the name
of a dir */
static void pushdir(uchar *p)
{
	struct de *dp;

	dp = (struct de *) gmalloc(((uint) sizeof(struct de)));
	if (dp)
	{
		dp->namep = p;
		dp->nextp = dirstkp;
		dirstkp = dp;
	}
}

static char *popdir(void)
{
	struct de *dp;
	char *dir;

	if (dirstkp == NULL)
		return NULL;
	dir = dirstkp->namep;
	dp = dirstkp->nextp;
	gfree(dirstkp);
	dirstkp = dp;
	return dir;
}


/* list the directories on the stack into strg      */
void dirs(uchar *arg)
{
	struct de *dp;
	WS *ws;
	char *cwd;

	UNUSED(arg);
	ws = initws();
	strwcat(ws, cwd = gfgetcwd(), 0);
	gfree(cwd);
	for (dp = dirstkp; dp; dp = dp->nextp)
	{
		strwcat(ws, " ", 0);
		strwcat(ws, dp->namep, 0);
	}
	if (ws)
	{
		strg = ws->ps;
		gfree(ws);
	}
}


/* push the given dir onto the stack */
void pushd(uchar *arg)
{
	char *p;
	char *q;

	UNUSED(arg);
	p = lexgetword();
	if (*p)
	{
		q = gfgetcwd();
		cd(p);
		if (valu >= 0)
		{
			cwdvar();
			if (q)
				pushdir(q);
		} else
			gfree(q);
	} else								/* no arg; so exch top elm with cwd */
	{
		p = gfgetcwd();
		q = popdir();
		if (q)
		{
			pushdir(p);
			cd(q);
			cwdvar();
		} else
		{
			gfree(p);
			emsg = DSE;
			valu = -1;
		}
	}
	dirs(NULL);								/* display the stack */
}


void popd(uchar *arg)
{
	char *p;

	UNUSED(arg);
	if ((p = popdir()) != NULL)
	{
		cd(p);
		cwdvar();
		gfree(p);
	} else
	{
		emsg = DSE;
		valu = -1;
		return;
	}
	dirs(NULL);
}

/* isdir()		-- see ls.c	*/

/* Given two pathnames p and q, return TRUE iff they both name the
same parent dir.  */

#if 00
int samedir(char *p, char *q)
{
	char *r;
	char *s;
	int n;

	r = strrchr(p, DSC);
	s = strrchr(q, DSC);
	if (r == s)
		return TRUE;					/* includes: r == s == NULL */
	if (r && s)
	{
		*r = *s = '\0';
		n = (strcmp(p, q) == 0);
		*r = *s = DSC;
		return n;
	} else
		return FALSE;					/* r == NULL or s == NULL */
}
#endif

static int samedevice(uchar *p, uchar *q)
{
	if (p[0] == q[0] && p[1] == ':' && q[1] == ':')
		return TRUE;
	return p[1] != ':' && q[1] != ':';
}


void grename(uchar *arg)
{
	char *p;
	char *q;

	UNUSED(arg);
	p = lexgetword();
	q = lexgetword();
	valu = (*q ? gfrename(p, q) : -1);
}


/*
 * rm -r handler.  Called by 'rm' in fop.c, this routine sets valu
 * to the first nonzero return code it gets.  It carries on, though,
 * deleting as much as possible before returning with the error code.
 * If the arg has wildcards in it I don't know what will happen.
 *
 * Written by Allan Pratt, with half-understood code borrowed from cp -r.
 */

static void rm_recurse(uchar *dir)
{
	char *p;
	WS *ws;
	long vtemp;

	if (isdir(dir))
	{
		p = dir + strlen(dir) - 1;
		if (*p == DSC)
			p = ES;
		else
			p = DS0;
		p = str3cat(dir, p, "*");
		ws = metaexpand(p);
		gfree(p);
		if (ws == NULL || ws->ps == NULL)
			goto ret;
		for (p = ws->ps; *p; p += strlen(p) + 1)
		{
			if (useraborted())
				break;
			rm_recurse(p);
		}

	  ret:
		freews(ws);
		if (valu == 0)
			valu = gfrmdir(dir);

	} else
	{
		/* not a directory; try just removing it */
		vtemp = gfunlink(dir);
		if (vtemp == -36 && negopts['f'])
		{
			/* access denied; maybe read-only */
			/* add writability and try again */
			posopts['w'] = 1;
			gchmod(dir);
			vtemp = gfunlink(dir);
		}
		if (valu == 0)
			valu = vtemp;
	}
}


void rm(uchar *p)
{
	/* set valu to error code if it's zero, or leave it otherwise AKP */
	long vtemp;

	/* function rm_recurse is below, by akp */
	if (negopts['r'])
	{
		rm_recurse(p);
		return;
	}
	vtemp = gfunlink(p);
	if (vtemp == -36 && negopts['f'])
	{
		/* access denied: maybe the file's read-only; if -f then force */
		/* perform "chmod +w file" (no harm if it's a dir or something) */
		posopts['w'] = 1;
		gchmod(p);

		/* retry the unlink */
		vtemp = gfunlink(p);
	}
	if (valu == 0)
		valu = vtemp;
}


static uchar *mkfullname(uchar *p)
{
	uchar *q;

#define NFILEN	80

	q = (p ? gmalloc(NFILEN) : NULL);
	if (q)
	{
		if (strlen(p) < NFILEN)
		{
			strcpy(q, p);
			fullname(q);
		} else
			*q = '\0';
	}
	return q;
}

/* copy one file to another; both names are pathnames.  */

static void copyfio(uchar *in, uchar *ou, int iscat)
{
	uchar *s;
	int fi;
	int fo;
	int foflag;
	int temp;
	long m;
	long n;
	long lsz;
	DOSTIME td;

	in = mkfullname(in);
	ou = mkfullname(ou);
	if ((in == NULL) || (ou == NULL))
		goto ret;

	/* AKP: check for existence of source file before deleting dest */
	if (flnotexists(in))
	{
		valu = EFILNF;
		goto ret;
	}

	if (iscat)
	{
		foflag = 0;
		fo = 1;
	} else
	{
		foflag = 1;						/* so we can read the opened file right away */
		if (strcmp(in, ou) == 0)
		{
			valu = -1;
			goto ret;
		}

		/* kbad wants userfeedback on moves, too... */
		userfeedback(sprintp("%s -> %s", in, ou), 2);
		if ((mvflag == MVFLAG) && samedevice(in, ou))
		{
			rm(ou);
			if (valu == 0 || valu == EFILNF)
				valu = gfrename(in, ou);
			goto ret;
		}
	}

	lsz = iscat ? 0x1000L : 0x0FFFFE02L;
	s = maxalloc(&lsz);

	lsz -= 2;							/* room for '\000' for iscat */
	lsz &= 0xFFFFFE00L;					/* better to read multiples of 512 */
	fi = (int)gfopen(in, 0);
	if (fi <= 0)
	{
		emsg = sprintp("cp: %s not found", in);
		goto frees;
	}
	for (;;)
	{
		n = gfread(fi, s, lsz);
		if (n < 0)
		{
			/* added by AKP: read-error detection */
			emsg = "read error on source file";
			valu = n;
			goto closefi;
		}
		if (foflag)
		{
			fo = (int)gfcreate(ou, 0);
			if (fo < MINFH && negopts['f'])
			{
				/* try "rm -f" on the dest file */
				rm(ou);
				if (valu == 0)
					fo = (int)gfcreate(ou, 0);
			}
			if (fo < MINFH)
			{
				emsg = "could not create dest file";
				valu = fo;
				goto closefi;
			}
			foflag = 0;
		}
		if (n <= 0L)
			break;
		if (iscat)
		{
			s[n] = '\0';
			outstr(s);
			m = n;
		} else
			m = gfwrite(fo, s, n);
		if (m < 0)
		{
			valu = m;
			break;
		}								/* AKP: write error */
		if (m != n)
		{
			valu = EWRITF;
			break;
		}
	}
	if (foflag == 0)
	{
		if (fo != 1)
		{
			temp = gfclose(fo);
			if (valu == 0)
				valu = temp;
		}
		if (posopts['t'])
		{
			gfdatime(&td, fi, 0);
			tch(ou, &td, TRUE);
		}
		/* didn't work for ST: Fdatime(td,fo,1); Fclose(fo);    */

	}
  closefi:temp = gfclose(fi);
	if (valu == 0)
		valu = temp;
#if TOS
	if (negopts['a'])
	{
		int attr = Fattrib(in, 0, 0);

		(void) Fattrib(ou, 1, attr);
	}
#endif
  frees:maxfree(s);
	if ((valu == 0) && (mvflag == MVFLAG))
		rm(in);
	else if (valu == EWRITF)
	{
		emsg = sprintp("no room for %s", ou);
		rm(ou);
	} else if (valu != 0)
	{
		/* generalized msg for other write errors */
		emsg = sprintp("copy failure for %s", ou);
		rm(ou);
	}
  ret:gfree(in);
	gfree(ou);
}


static void setupdir(char *q)
{
	char *r;

	targetdir = r = gmalloc(((uint) (strlen(q) + 2)));	/* may need to append DSC */
	if (r == NULL)
	{
		valu = ENSMEM;
		return;
	}
	strcpy(r, q);
	r += strlen(r) - 1;
	if (*r != DSC)
	{
		*++r = DSC;
		*++r = '\0';
	}
}


static void cpdirtodir(char *ind, char *oud);

static void cpftodir(char *p)
{
	char *q;
	char *r;
	int dirflag;

	dirflag = isdir(p);

	r = ((p[0] != '\0') && (p[1] == ':') ? p + 2 : p);
	q = strrchr(r, DSC);
	q = (q == NULL ? r : q + 1);

	/* q now points to leaf name */
	r = str3cat(targetdir, q, ES);
	if (dirflag)
	{
		if (negopts['R'] || negopts['r'])
			cpdirtodir(p, r);
	} else
		copyfio(p, r, 0);
	gfree(r);
}


/* Copy all the files in directory ind to the dest directory given in oud.
 Dir oud may or may not be an existing one.
*/
static void cpdirtodir(char *ind, char *oud)
{
	char *p;
	char *olddir;
	WS *ws;

	olddir = targetdir;
	ws = NULL;
	(void) gfmkdir(oud);
	/* create dir oud, if it did not exist before; no harm, ow */
	p = ind + strlen(ind) - 1;
	p = (*p == DSC ? ES : DS0);
	p = str3cat(ind, p, "*");
	ws = metaexpand(p);
	gfree(p);
	if (ws == NULL || (p = ws->ps) == NULL)
		goto ret;
	setupdir(oud);
	for (; *p; p += strlen(p) + 1)
	{
		if (useraborted())
			break;
		cpftodir(p);
	}
	gfree(targetdir);

  ret:freews(ws);
	targetdir = olddir;
	if ((valu == 0) && (mvflag == MVFLAG))
		valu = gfrmdir(ind);
}


void cp(uchar *arg)
{
	char *p;
	char *q;
	char *op;
	char *r;
	char c;
	int i;

	UNUSED(arg);
	op = (++mvflag == MVFLAG ? "mv" : "cp");

	q = lexlastword();
	if (isdir(q))
	{
		setupdir(q);
		doforeach(op, cpftodir);
		gfree(targetdir);
	} else
	{
		for (i = 0, r = NULL;;)
		{
			p = lexgetword();
			c = *p;
			if (c == '\0')
				break;
			else
			{
				r = p;
				i++;
			}
		}
		if (i == 0)
			emsg = "destination ?";
		if (i > 1)
			emsg = sprintp("last arg '%s' must be a dir", q);
		else if (asktodoop(op, r) == 1)
			copyfio(r, q, 0);
	}
	mvflag = 0;
}


/* Move files to destination.  We do this by copying to dest and
deleting the source.  The mvflag is set to 1-less than MVFLAG, so that
after ++ in cp() it will equal MVFLAG; this takes care of the
situation when mv is interrupted by user, so that the next cp does not
work as if a mv was requested.  */

void mv(uchar *arg)
{
	mvflag = MVFLAG - 1;
	cp(arg);
}


static void outstrn(uchar *text, int n)
{
	UNUSED(n);
	outstr(text);
}


void cat(uchar *p)
{
	int fi;

	fi = (int)gfopen(p, 0);
	if (fi <= 0)
		emsg = sprintp("cat: %s not found", p);
	else
		eachline(fi, outstrn);
}
