/*	
 *	Gulam support routines for GCC
 *	  ++jrb
 */
#include "ue.h"

#ifdef __MINT__
#include <sys/ioctl.h>
extern int __mint;
#endif

#ifdef __PUREC__
#include <linea.h>
/* map Pure-C names to GNU-C ones */
#  define linea0() linea_init()
#  define lineaa() hide_mouse()
#  define linea9() show_mouse(0)
#  define __FONT FONT_HDR
#  define __fonts Fonts->font
#  define __aline Linea
#  define off_table ch_ofst
#  define dat_table fnt_dta
#  define form_height frm_hgt
#  define V_OFF_AD Vdiesc->v_off_ad
#  define V_FNT_AD Vdiesc->v_fnt_ad
#  define V_CEL_HT Vdiesc->v_cel_ht
#  define V_CEL_MX Vdiesc->v_cel_mx
#  define V_CEL_MY Vdiesc->v_cel_my
#  define V_CEL_WR Vdiesc->v_cel_wr
#  define V_BYTES_LIN Vdiesc->bytes_lin
#  define V_Y_MAX Vdiesc->v_rez_vt
#  define CONTRL Linea->contrl
#  define INTIN Linea->intin
#  define VPLANES Linea->v_planes
#else
#include <mint/linea.h>
#endif

/*
 * return base addr of lineA vars
 */
short *lineA(void)
{
	(void) linea0();
	return (short *) __aline;
}


/*
 * Make hi rez screen bios handle 50 lines of 8x8 characters.  Adapted
 * from original asm posting. look ma, no asm!
 */
/* ++ers: if under MiNT, don't do this */
void hi50(void)
{
	__FONT *f8x8;

#ifdef __MINT__
	if (__mint)
		return;
#endif
	lineA();
	f8x8 = __fonts[1];					/* 8x8 font header          */
	V_OFF_AD = (void *) f8x8->off_table;	/* v_off_ad <- 8x8 offset tab addr */
	V_FNT_AD = (void *) f8x8->dat_table;			/* v_fnt_ad <- 8x8 font data addr   */
	V_CEL_HT = 8;						/* v_cel_ht <- 8    8x8 cell height */
	V_CEL_MX = (V_BYTES_LIN / VPLANES) - 1;
	V_CEL_MY = (V_Y_MAX / 8) - 1;		/* v_cel_my <- 49   maximum cell "Y"    */
	V_CEL_WR = V_BYTES_LIN * 8;;		/* v_cel_wr <- 640  offset to cell Y+1  */
}


static unsigned long fnt_8x10[640];

/*
 * Make hi rez screen bios handle 40 lines of 8x8 characters.  Adapted
 * from original asm posting.
 */
void hi40(void)
{
	__FONT *f8x8;

#ifdef __MINT__
	if (__mint)
		return;
#endif

	/*  make 8x8 font into 8x10 font */
	memset(fnt_8x10, 0, sizeof(fnt_8x10));	/* 640 = size of 8x10 fnt; longs */

	lineA();
	f8x8 = __fonts[1];					/* 8x8 font header          */
	memcpy(&fnt_8x10[64], f8x8->dat_table, 512 * 4L);	/* copy 8x8 fnt data
														   starting at second line of 8x10 fnt */

	V_OFF_AD = (void *) f8x8->off_table;	/* v_off_ad <- 8x8 offset tab addr */
	V_FNT_AD = (void *) fnt_8x10;		/* v_fnt_ad <- 8x10 font data addr    */
	V_CEL_HT = 10;						/* v_cel_ht <- 10   8x10 cell height    */

	V_CEL_MX = (V_BYTES_LIN / VPLANES) - 1;
	V_CEL_MY = (V_Y_MAX / 10) - 1;						/* v_cel_my <- 39   maximum cell "Y"    */
	V_CEL_WR = V_CEL_WR = V_BYTES_LIN * 10;;			/* v_cel_wr <- 800  offset to cell Y+1  */
}


/*
 * Make hi rez screen bios handle 25 lines of 8x16 characters
 */
void hi25(void)
{
	__FONT *f8x16;

#ifdef __MINT__
	if (__mint)
		return;
#endif

	lineA();
	f8x16 = __fonts[2];					/* 8x16 font header             */
	V_OFF_AD = (void *) f8x16->off_table;	/* v_off_ad <- 8x16 offset tab addr */
	V_FNT_AD = (void *) f8x16->dat_table;		/* v_fnt_ad <- 8x16 font data addr     */
	V_CEL_HT = 16;						/* v_cel_ht <- 16    8x16 cell height   */
	V_CEL_MX = (V_BYTES_LIN / VPLANES) - 1;
	V_CEL_MY = (V_Y_MAX / 16) - 1;		/* v_cel_my <- 24   maximum cell "Y"    */
	V_CEL_WR = V_BYTES_LIN * 16;		/* v_cel_wr <- 1280  offset to cell Y+1 */
}

/*
 * show/activate the mouse
 */
void mouseon(uchar *arg)
{
	UNUSED(arg);
#ifdef __MINT__
	if (__mint)
		return;
#endif
	lineA();
	if (!CONTRL)
		return;
	CONTRL[1] = 0;
	CONTRL[3] = 1;
	INTIN[0] = 0;
	(void) linea9();
	mouseregular();
}


void mouseoff(uchar *arg)
{
	UNUSED(arg);
#ifdef __MINT__
	if (__mint)
		return;
#endif
	lineA();
	(void) lineaa();					/* mouse lineA call */
	mousecursor();
}


/*
 * getnrow() -- get number of rows.
 */

int getnrow(void)
{
#ifdef __MINT__
	struct winsize ws;

	ioctl(-1, TIOCGWINSZ, &ws);
	return ws.ws_row;
#else
	lineA();
	return V_CEL_MY + 1;
#endif
}

/*
 * getncol() -- get number of columns.
 */
int getncol(void)
{
#ifdef __MINT__
	struct winsize ws;

	ioctl(-1, TIOCGWINSZ, &ws);
	return ws.ws_col;
#else
	lineA();
	return V_CEL_MX + 1;
#endif
}


/*
 * Extension by AKP: "set font 8" now means "use the 8x8 font"
 * and RETURNS whatever number of lines that yields (for setting nrows)
 *
 * "set font 16" means "use the 8x16 font," and "set font 10" means
 * "use the 8x8 font with two extra blank lines."
 * Again, these return the actual number of rows you get.
 */

int font8(void)
{
	__FONT *f8x8;

#ifdef __MINT__
	if (__mint)
		return getnrow();
#endif

	lineA();
	f8x8 = __fonts[1];					/* 8x8 font header          */
	V_OFF_AD = (void *) f8x8->off_table;	/* v_off_ad <- 8x8 offset tab addr */
	V_FNT_AD = (void *) f8x8->dat_table;			/* v_fnt_ad <- 8x8 font data addr   */
	V_CEL_HT = 8;						/* v_cel_ht <- 8    8x8 cell height */

	/* new v_cel_wr (byte disp to next vertical alpha cell) =
	 * bytes_lin * (v_cel_ht = v_bytes_lin * 8)
	 */
	V_CEL_WR = V_BYTES_LIN * 8;

	/* new v_cel_mx = (bytes_lin / nplanes) - 1 */
	V_CEL_MX = (V_BYTES_LIN / VPLANES) - 1;

	/* new v_cel_my = (v_rez_vt / 8) - 1 */
	V_CEL_MY = (V_Y_MAX / 8) - 1;
	return V_CEL_MY + 1;
}

/*
 *  make 8x8 font into 8x10 font and make it current; return new nrows
 */
int font10(void)
{
	__FONT *f8x8;

#ifdef __MINT__
	if (__mint)
		return getnrow();
#endif

	/*  make 8x8 font into 8x10 font */
	memset(fnt_8x10, 0, sizeof(fnt_8x10));	/* 640 = size of 8x10 fnt; longs */

	lineA();
	f8x8 = __fonts[1];					/* 8x8 font header          */
	memcpy(&fnt_8x10[64], f8x8->dat_table, 512 * 4L);	/* copy 8x8 fnt data
														   starting at second line of 8x10 fnt */

	V_OFF_AD = (void *) f8x8->off_table;	/* v_off_ad <- 8x8 offset tab addr */
	V_FNT_AD = (void *) fnt_8x10;		/* v_fnt_ad <- 8x10 font data addr      */
	V_CEL_HT = 10;						/* v_cel_ht <- 10   8x10 cell height    */

	/* new v_cel_wr (byte disp to next vertical alpha cell) =
	 * bytes_lin * (v_cel_ht = v_bytes_lin * 10)
	 */
	V_CEL_WR = V_BYTES_LIN * 10;

	/* new v_cel_mx = (bytes_lin / nplanes) - 1 */
	V_CEL_MX = (V_BYTES_LIN / VPLANES) - 1;

	/* new v_cel_my = (v_rez_vt / 10) - 1 */
	V_CEL_MY = (V_Y_MAX / 10) - 1;
	return V_CEL_MY + 1;
}


/*
 * Switch to 8x16 font and return new nrows
 */
int font16(void)
{
	__FONT *f8x16;

#ifdef __MINT__
	if (__mint)
		return getnrow();
#endif

	lineA();
	f8x16 = __fonts[2];					/* 8x16 font header             */
	V_OFF_AD = (void *) f8x16->off_table;	/* v_off_ad <- 8x16 offset tab addr */
	V_FNT_AD = (void *) f8x16->dat_table;		/* v_fnt_ad <- 8x16 font data addr     */
	V_CEL_HT = 16;						/* v_cel_ht <- 16    8x16 cell height   */

	/* new v_cel_wr (byte disp to next vertical alpha cell) =
	 * bytes_lin * (v_cel_ht = v_bytes_lin * 16)
	 */
	V_CEL_WR = V_BYTES_LIN * 16;

	/* new v_cel_mx = (bytes_lin / nplanes) - 1 */
	V_CEL_MX = (V_BYTES_LIN / VPLANES) - 1;

	/* new v_cel_my = (v_rez_vt / 16) - 1 */
	V_CEL_MY = (V_Y_MAX / 16) - 1;
	return V_CEL_MY + 1;
}

#if 0
/*
/ Revector trap 1 and 13 to do stdout capture into Gulam buffer
/
/	.prvd
/savea7:	.long	0
/
/	.shri
/	.globl	gconout_, gcconws_, gfwrite_, oldp1_, oldp13_
/	.globl	mytrap1_, mytrap13_
/
// revector trap 13 to our own to handle Bconout(2, x)
//
/mytrap13_:
/	move.l	a7,savea7
/	move.l	usp, a7
/        move.w	(a7)+, d0	/ Bconout(2, c) == trap13(3, 2, c)
/	cmpi.w	$3,d0
/	bne	1f
/3:	move.w	(a7)+,d0
/	cmpi.w	$2, d0
/	bne	1f
/	jsr	gconout_	/ c is still on stack
/ 	movea.l	savea7,a7
/	rte
/1:	movea.l	savea7,a7
/	movea.l	oldp13_,a0
/ 	jmp	(a0)
/
// Revector trap 1 to handle Cconout, Cconws, and Fwrite(1,..) calls
//
/mytrap1_:
/	move.l	a7,savea7
/	move.l	usp, a7
/        move.w	(a7)+, d0
/	cmpi.w	$2,d0		/ Cconout(c) == trap1(0x2, c)
/	beq	2f
/	cmpi.w	$9,d0		/ Cconws(s) == trap1(0x9, s)
/	beq	9f
/	cmpi.w	$0x40,d0	/ Fwrite(1, ll, bf) == trap1(0x40, 1, ll, bf)
/ 	beq	4f
/1:	movea.l	savea7,a7
/	movea.l	oldp1_,a0
/	jmp	(a0)
/9:	jsr	gcconws_	/ s is still on stack
/	bra	0f
/4:	move.w	(a7)+,d0	/ d0 == 1 ?
/	cmpi.w	$1,d0
/	bne	1b
/	jsr	gfwrite_	/ gfwrite(ll, bf) long ll; char *bf;
/	bra	0f
/2:	jsr	gconout_
/0: 	movea.l	savea7,a7
/	rte
/
*/
#endif
