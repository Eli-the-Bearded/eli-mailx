/*	$OpenBSD: head.c,v 1.5 1996/06/08 19:48:26 christos Exp $	*/
/*	$NetBSD: head.c,v 1.5 1996/06/08 19:48:26 christos Exp $	*/

/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
#if 0
static char sccsid[] = "@(#)head.c	8.1 (Berkeley) 6/6/93";
#else
static char rcsid[] = "$OpenBSD: head.c,v 1.5 1996/06/08 19:48:26 christos Exp $";
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"

/*
 * Mail -- a mail program
 *
 * Routines for processing and detecting headlines.
 */

/*
 * See if the passed line buffer is a mail header.
 * Return true if yes.  Note the extreme pains to
 * accomodate all funny formats.
 */
int
ishead(linebuf)
	char linebuf[];
{
	register char *cp;
	struct headline hl;
	char parbuf[BUFSIZ];

	cp = linebuf;
	if (*cp++ != 'F' || *cp++ != 'r' || *cp++ != 'o' || *cp++ != 'm' ||
	    *cp++ != ' ')
		return (0);
	parse(linebuf, &hl, parbuf);
	if (hl.l_from == NOSTR || hl.l_date == NOSTR) {
		fail(linebuf, "No from or date field");
		return (0);
	}
        /* be very tolerant about the date */
#if 0
	if (!isdate(hl.l_date)) {
		fail(linebuf, "Date field not legal date");
		return (0);
	}
#endif
	/*
	 * I guess we got it!
	 */

	return (1);
}

/*ARGSUSED*/
void
fail(linebuf, reason)
	char linebuf[], reason[];
{

	/*
	if (value("debug") == NOSTR)
		return;
	fprintf(stderr, "\"%s\"\nnot a header because %s\n", linebuf, reason);
	*/
}

/*
 * Split a headline into its useful components.
 * Copy the line into dynamic string space, then set
 * pointers into the copied line in the passed headline
 * structure.  Actually, it scans.
 */
void
parse(line, hl, pbuf)
	char line[], pbuf[];
	register struct headline *hl;
{
	register char *cp;
	char *sp;
	char word[LINESIZE];

	hl->l_from = NOSTR;
	hl->l_tty = NOSTR;
	hl->l_date = NOSTR;
	cp = line;
	sp = pbuf;

	/*
	 * Skip over "From" first.
	 * if line started as:
	 * From username@example.com  Fri Jul 29 17:50:02 2011
	 */
	cp = nextword(cp, word);
	/* word is now "From" and cp is
	 * username@example.com  Fri Jul 29 17:50:02 2011
	 */

	cp = nextword(cp, word);
	/* word is now "username@example.com" and cp is
	 * Fri Jul 29 17:50:02 2011
	 */
	if (*word)
		hl->l_from = copyin(word, &sp);

	/* for the rare
	 * From username ttyN Date Here
	 * form from line.
	 */
	if (cp != NOSTR && cp[0] == 't' && cp[1] == 't' && cp[2] == 'y') {
		cp = nextword(cp, word);
		hl->l_tty = copyin(word, &sp);
	}

	if (cp != NOSTR)
		hl->l_date = copyin(cp, &sp);

	/* some formats can be tweaked to a more compact form */
        if(isdate_tweak(hl->l_date))
		tweak(hl->l_date, 0);
}

/*
 * Copy the string on the left into the string on the right
 * and bump the right (reference) string pointer by the length.
 * Thus, dynamically allocate space in the right string, copying
 * the left string into it.
 */
char *
copyin(src, space)
	register char *src;
	char **space;
{
	register char *cp;
	char *top;

	top = cp = *space;
	while ((*cp++ = *src++) != '\0')
		;
	*space = cp;
	return (top);
}


/* Macros for tweak()ing */
#define datesep		('/')
#define ADD(a,b)	((a)*256+(b))
#define cur		(date[src])
#define cursp		(cur==' ')
#define curnsp		(cur!=' ')
#define next		(date[src+1])
#define nnext		(date[src+2])
#define setnext		(date[dest++])
#define pad		(setnext=' ')
#define skip		(src++)
#define dcp		(setnext=date[skip])
#define finish		if(src==dest) {return;} else {\
			  while(cur){dcp;} \
			  setnext='\0'; \
			}

/* Simple rewriting of the date from BEG's myfrm.c (where isshort is used) */
void
tweak(date,isshort)char*date;int isshort;{
  int month,tp=0,src=0,dest=0;
  char time[8];

  if (cursp) {
    if (cur==next) {
      if (isshort) {pad;}
      skip;
    }
    pad;
    skip;
  }
  /* Day of week */
  switch(ADD(cur,next)) {
    case ADD('S','u'):
    case ADD('M','o'):
    case ADD('T','u'):
    case ADD('W','e'):
    case ADD('T','h'):
    case ADD('F','r'):
    case ADD('S','a'):
      dcp; dcp;
      if (curnsp) { skip; } else { finish; }
      break;
    default:
      finish;
  }
  if (cursp) { dcp; } else { finish; }
  /* Month of year */
  switch(ADD(cur,next)) {
    case ADD('J','a'):
      month=1;
      break;
    case ADD('F','e'):
      month=2;
      break;
    case ADD('M','a'): /* March or May */
      if (nnext=='r') {
	month=3;
      } else
      if (nnext=='y') {
	month=5;
      } else
        finish;
      break;
    case ADD('A','p'):
      month=4;
      break;
    case ADD('J','u'): /* June or July */
      if (nnext=='n') {
	month=6;
      } else
      if (nnext=='l') {
	month=7;
      } else
        finish;
      break;
    case ADD('A','u'):
      month=8;
      break;
    case ADD('S','e'):
      month=9;
      break;
    case ADD('O','c'):
      month=10;
      break;
    case ADD('N','o'):
      month=11;
      break;
    case ADD('D','e'):
      month=12;
      break;
    default:
      finish;
  }
  if (month>9) {
    setnext='1';
    setnext=('0'+month-10);
    skip; skip;
  } else {
    setnext='0';
    setnext=('0'+month);
    skip; skip;
  }
  /* Third day in month abbr. */
  if (curnsp) { skip; } else { finish; }
  /* Skip space in date, and set seperator. */
  if (cursp) { setnext=datesep; skip; } else { finish; }
  /* deal with space for first digit in date case */
  if (cursp) { setnext='0'; skip; }
  /* Day of month */
  while(curnsp) {dcp;}
  skip; /* space */
  /* Time of day (yank) */
  while((curnsp)&&(tp<8)) {time[tp++]=cur;skip;}
  if (tp!=8) { finish; }
  if (cursp) { setnext=datesep; skip; } else { finish; }
  /* Build in some Y2K problems. Yay! */
  skip; skip; /* century */
  dcp; dcp;   /* year    */
  /* Time of day (put) */
  setnext=' ';
  for(tp=0;tp<8;setnext=time[tp++]);
  /* newline */
  dcp;
  /* null */
  dcp;
}


/*
 * Test to see if the passed string is a ctime(3) generated
 * date string as documented in the manual.  The template
 * below is used as the criterion of correctness.
 * Also, we check for a possible trailing time zone using
 * the tmztype template.
 */

/*
 * 'A'	An upper case char
 * 'a'	A lower case char
 * ' '	A space
 * '0'	A digit
 * 'O'	An optional digit or space
 * ':'	A colon
 * 'N'	A new line
 */
char ctype[] = "Aaa Aaa O0 00:00:00 0000";
char ctype_without_secs[] = "Aaa Aaa O0 00:00 0000";
char tmztype[] = "Aaa Aaa O0 00:00:00 AAA 0000";
char tmztype_without_secs[] = "Aaa Aaa O0 00:00 AAA 0000";

int
isdate(date)
	char date[];
{

	return cmatch(date, ctype_without_secs) || 
	       cmatch(date, tmztype_without_secs) || 
	       cmatch(date, ctype) || cmatch(date, tmztype);
}

/* check for tweak compatible date format */
int
isdate_tweak(date)
	char date[];
{

	return cmatch(date, ctype);
}

/*
 * Match the given string (cp) against the given template (tp).
 * Return 1 if they match, 0 if they don't
 */
int
cmatch(cp, tp)
	register char *cp, *tp;
{

	while (*cp && *tp)
		switch (*tp++) {
		case 'a':
			if (!islower(*cp++))
				return 0;
			break;
		case 'A':
			if (!isupper(*cp++))
				return 0;
			break;
		case ' ':
			if (*cp++ != ' ')
				return 0;
			break;
		case '0':
			if (!isdigit(*cp++))
				return 0;
			break;
		case 'O':
			if (*cp != ' ' && !isdigit(*cp))
				return 0;
			cp++;
			break;
		case ':':
			if (*cp++ != ':')
				return 0;
			break;
		case 'N':
			if (*cp++ != '\n')
				return 0;
			break;
		}
	if (*cp || *tp)
		return 0;
	return (1);
}

/*
 * Collect a liberal (space, tab delimited) word into the word buffer
 * passed.  Also, return a pointer to the next word following that,
 * or NOSTR if none follow.
 */
char *
nextword(wp, wbuf)
	register char *wp, *wbuf;
{
	register int c;

	if (wp == NOSTR) {
		*wbuf = 0;
		return (NOSTR);
	}
	while ((c = *wp++) && c != ' ' && c != '\t') {
		*wbuf++ = c;
		if (c == '"') {
 			while ((c = *wp++) && c != '"')
 				*wbuf++ = c;
 			if (c == '"')
 				*wbuf++ = c;
			else
				wp--;
 		}
	}
	*wbuf = '\0';
	for (; c == ' ' || c == '\t'; c = *wp++)
		;
	if (c == 0)
		return (NOSTR);
	return (wp - 1);
}
