/*
	sysdpend.h of Gulam/uE

	All system dependent decls are either collected or
	documented here.

	AtariST/TOS version
*/

				/* OS defines: #def exactly one		*/
#define	TOS	1		/* AtariST GEMDOS/TOS			*/
#undef	USG			/* Unix System V			*/
#undef	BSD			/* Unix BSD 4.x				*/


#if	!TOS
/* 	Gulam's #include file for sys dependencies (TOS version) */
#endif

#ifdef __PUREC__
#include <tos.h>
#else
#include <osbind.h>
#define DTA _DTA
#define d_fname dta_name
#define d_attrib dta_attribute
#define d_date dta_date
#define d_time dta_time
#define d_length dta_size
#define DOSTIME _DOSTIME
#define DISKINFO _DISKINFO
#define cdecl __attribute__((__cdecl__))
#endif

#define	AtariST		1	/* => definite machine specificity	*/
#define	PMMALLOC	1	/* pm's own malloc			*/
				/* NB:	Reqd: In all versions, gfree(p)	*/
				/* should just return -40 if p == NULL.	*/
/* disable terminal emulator */
#define	XMDM		0
#define	LPRINT		1
#define	MINELM		0	/* used in array[MINELM]; make it 0	*/
				/* preferrably, 2 otherwise		*/

#define	Selfinserting(c) ((c>=0x20 && c<=0x7E) || (c>=0x80 && c<=0xFE))
#define	kbddisplayinit()	/* empty */
#define	topen()			/* empty */
#define	execfile(g, cmdln, envp) Pexec(0, g, cmdln, envp)
#define ALL_EXE_BRE			"*.(app|gtp|tos|ttp|prg|g)"
#define	executables(q)		matchednms(q, ALL_EXE_BRE, 1)
#define	deleteext(nm, p)	{if ((p = strrchr(nm, '.')) != NULL) *p = '\000';}
#define	isgulamfile(p)		(strcmp(p, ".g") == 0)


#define	OSFNML	12
#define MINFH	(-3)		/* min number valid as a file handle	*/
#define	MX_PATH_LENGTH	1024

#define	opencwdandread(a)	(Fsfirst("*.*", a) == 0L)
#define	readfromcwd()		(Fsnext() == 0L)
#define	closecwd()		/* empty */
#define	flnotexists(p)		Fsfirst(p, 0xFF)
#define	userid(x, p)		strcpy(p, "u         ")
#define gfrmdir(path)		Ddelete(path)	
#define gfmkdir(path)		Dcreate(path)	
#define	gfcreate(path, md)	Fcreate(path, md)
#define	gfopen(path, md)	Fopen(path, md)
#define	gfclose(fd)		Fclose(fd)
#define gfread(fd, buf, n)	Fread(fd, (long) n, buf)	
#define gfwrite(fd, buf, n)	Fwrite(fd, (long) n, buf)
#define	gfrename(ofnm, nfnm)	Frename(0, ofnm, nfnm)
#define gfunlink(path)		Fdelete(path)	
#define	gfdatime(td, fd, n)	Fdatime(td, fd, n)
#define	gfsfirst(pat, attr)	Fsfirst(pat, attr)
#define	gfsnext()		Fsnext()
#define	gfgetdta()		Fgetdta()

#define	inmdmrdy()		Bconstat(1)	/* input from modem rdy? */
#define sendchar(c)		Bconout(1, c)	/* send to mdm port */
#define	offflowcontrol()	Rsconf(-1,0,-1,-1,-1,-1)
#define	onflowcontrol()		Rsconf(-1,1,-1,-1,-1,-1)

#define	inkbdrdy()		Cconis()
#define	ggetchar()		Crawcin()		/* Bconin(2) */
#define	gputchar(c)		Bconout(2, c)
#define	onreversevideo()	gputs("\033p")
#define	offreversevideo()	gputs("\033q")
#define	toeolerase()		gputs("\033K")
#define	screenerase()		gputs("\033Y  \033J")
#define	invisiblecursor()	gputs("\033f")
#define	visiblecursor()		gputs("\033e")

/* for each compiler that does Ansi C define STANC below.
   note we cannot use __STDC__ because mwc and some others
   define it. Also, we cannot depend of #if __STDC__ because
   some pre-processors dont handle it correctly.
   Note: now always assumed.
*/


#include <stddef.h>
#include <string.h>
#include <stdlib.h>

				/* scan|ascii codes of some ST keys */
#define	ESCKEY	0x00010000L	/* Escape key */
#define	UNDOKEY	0x00610000L	/* Undo key */
#define	HELPKEY	0x00620000L	/* Help key */

/* format of ls -l line:
 0         1         2         3         4         5
 012345678901234567890123456789012345678901234567890123456 7 8
 drwx---... 1 useridxxx size0000 mon dd hh:mm filename.ext\r\n\0
*/

/* used in ls.c			*/
typedef	struct	GSTAT 		/*  similar to disk transfer area	*/
{/*	char	dos[21];*/
	char	dummy;		/*  jst for even-bdry alignment		*/
	char	attr;
	short	time;
	short	date;
	long	size;
/*	char	name[14];*/
}	GSTAT;

#define	bgnGSTAT 20

#define	SZcmd	256	/* size of cmd buffer; can be whatever	*/
#define	LDFLNSEP	2		/* length of DFLNSEP */

#define	DSC	'\\'	/* dir separator char		*/
#define	BMCGUL	'\372'	/* \372 is a little disc	*/
#define	BMCREG	'-'
#define	BMCTEM	'\360'
#define	BMCTMP	'.'
			/* BIOS level errors */

#define	E_OK	  0L	/* OK, no error			*/
#define	ERROR	 -1L	/* basic, fundamental error	*/
#define	EDRVNR	 -2L	/* drive not ready		*/
#define	EUNCMD	 -3L	/* unknown command		*/
#define	E_CRC	 -4L	/* CRC error			*/
#define	EBADRQ	 -5L	/* bad request			*/
#define	E_SEEK	 -6L	/* seek error			*/
#define	EMEDIA	 -7L	/* unknown media		*/
#define	ESECNF	 -8L	/* sector not found		*/
#define	EPAPER	 -9L	/* no paper			*/
#define	EWRITF	-10L	/* write fault			*/
#define	EREADF	-11L	/* read fault			*/
#define	EGENRL	-12L	/* general error		*/
#define	EWRPRO	-13L	/* write protect		*/
#define	E_CHNG	-14L	/* media change			*/
#define	EUNDEV	-15L	/* unknown device		*/
#define	EBADSF	-16L	/* bad sectors on format	*/
#define	EOTHER	-17L	/* insert other disk		*/

			/* BDOS level errors */

#define	EINVFN	-32L	/* invalid function number			 1 */
#define	EFILNF	-33L	/* file not found				 2 */
#define	EPTHNF	-34L	/* path not found				 3 */
#define	ENHNDL	-35L	/* too many open files (no handles left)	 4 */
#define	EACCDN	-36L	/* access denied				 5 */
#define	EIHNDL	-37L	/* invalid handle				 6 */
#define	ENSMEM	-39L	/* insufficient memory				 8 */
#define EIMBA	-40L	/* invalid memory block address			 9 */
#define	EDRIVE	-46L	/* invalid drive was specified			15 */
#define	ENMFIL	-49L	/* no more files				18 */

			/* our own inventions */

#define	ERANGE	-64L	/* range error					33 */
#define	EINTRN	-65L	/* internal error				34 */
#define EPLFMT  -66L	/* invalid program load format			35 */
#define EGSBF	-67L	/* setblock failure due to growth restrictions  36 */

/*	Other machine/system specific/dependent files: */

#if 00
	gcoatari.c	/* terminal emulator rtns for Atari ST */
	gasmmwc.s	/* asm code for Atari ST */
	gmcatari.c	/* atari specific rtns and related vars	*/
	gfsatari.s	/* file sys interface */
	pmalloc.c  	/* malloc/free package */
#endif
