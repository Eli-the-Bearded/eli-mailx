/*	$OpenBSD: aux.c,v 1.4 1996/06/08 19:48:10 christos Exp $	*/
/*	$NetBSD: aux.c,v 1.4 1996/06/08 19:48:10 christos Exp $	*/

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
static char sccsid[] = "@(#)aux.c	8.1 (Berkeley) 6/6/93";
#else
static char rcsid[] = "$OpenBSD: aux.c,v 1.4 1996/06/08 19:48:10 christos Exp $";
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"

/*
 * Mail -- a mail program
 *
 * Auxiliary functions.
 */
static char *save2str __P((char *, char *));

/*
 * Return a pointer to a dynamic copy of the argument.
 * savestr() copies to malloc()ed space that will be
 * saved until end of program (if before spreserve())
 * or wiped before processing every new command.
 */
char *
savestr(str)
	const char *str;
{
	char *new;
	int size = strlen(str) + 1;

	if ((new = salloc(size)) != NOSTR)
		bcopy(str, new, size);
	return new;
}

/*
 * Make a copy of new argument incorporating old one.
 */
static char *
save2str(str, old)
	char *str, *old;
{
	char *new;
	int newsize = strlen(str) + 1;
	int oldsize = old ? strlen(old) + 1 : 0;

	if ((new = salloc(newsize + oldsize)) != NOSTR) {
		if (oldsize) {
			bcopy(old, new, oldsize);
			new[oldsize - 1] = ' ';
		}
		bcopy(str, new + oldsize, newsize);
	}
	return new;
}

/*
 * Announce a fatal error and die.
 */
#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

void
#if __STDC__
panic(const char *fmt, ...)
#else
panic(fmt, va_alist)
	char *fmt;
        va_dcl
#endif
{
	va_list ap;
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void)fprintf(stderr, "panic: ");
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
	fflush(stderr);
	abort();
}

/*
 * Touch the named message by setting its MTOUCH flag.
 * Touched messages have the effect of not being sent
 * back to the system mailbox on exit.
 */
void
touch(mp)
	register struct message *mp;
{

	mp->m_flag |= MTOUCH;
	if ((mp->m_flag & MREAD) == 0)
		mp->m_flag |= MREAD|MSTATUS;
}

/*
 * Set (op is true) or clear (op is false) a flag (or flags) on a message.
 */
void
flag(mp,op,flag)
	register struct message *mp;
	int op;
	short flag;
{
	if (op) {
	    mp->m_flag |= flag;
	} else {
	    mp->m_flag &= ~flag;
	}
}

/*
 * Echo a generic highlight sequence. No terminal type checking.
 * Cheesy, but no need to link to curses either. 		BEG
 * But backticks are now allowed in set assignments, so you can
 * set starthlstr=`tput smso`
 * set endhlstr=`tput rmso`					BEG 2017
 */
void
starthl(strm)
	FILE *strm;
{
    char *hlst;

    if ((hlst = value("starthlstr")) == NOSTR) {
      return;
    }
    /* encode% $(tput smul)	# set mode underline
     * %1b%5b%34%6d -> <ESC>[4m
     * (void)fprintf(strm,"%c[4m",0x1b);
     */
    (void)fprintf(strm,"%s",hlst);
}

/*
 * Echo a generic highlight stop sequence. No terminal type checking.
 * Cheesy, but no need to link to curses either. 		BEG
 */
void
endhl(strm)
	FILE *strm;
{
    char *hlst;

    if ((hlst = value("endhlstr")) == NOSTR) {
      return;
    }
    /* encode% $(tput rmul)	# remove mode underline 
     * %1b%5b%32%34%6d -> <ESC>[24m
     * (void)fprintf(strm,"%c[24m",0x1b);
     */
    (void)fprintf(strm,"%s",hlst);
}


/*
 * Test to see if the passed file name is a directory.
 * Return true if it is.
 */
int
isdir(name)
	char name[];
{
	struct stat sbuf;

	if (stat(name, &sbuf) < 0)
		return(0);
	return((sbuf.st_mode & S_IFMT) == S_IFDIR);
}

/*
 * Count the number of arguments in the given string raw list.
 */
int
argcount(argv)
	char **argv;
{
	register char **ap;

	for (ap = argv; *ap++ != NOSTR;)
		;	
	return ap - argv - 1;
}

/*
 * Return the desired header line from the passed message
 * pointer (or NOSTR if the desired header field is not available).
 */
char *
hfield(field, mp)
	char field[];
	struct message *mp;
{
	register FILE *ibuf;
	char linebuf[LINESIZE];
	register int lc;
	register char *hfield;
	char *colon, *oldhfield = NOSTR;

	ibuf = setinput(mp);
	if ((lc = mp->m_lines - 1) < 0)
		return NOSTR;
	if (readline(ibuf, linebuf, LINESIZE) < 0)
		return NOSTR;
	while (lc > 0) {
		if ((lc = gethfield(ibuf, linebuf, lc, &colon)) < 0)
			return oldhfield;
		if ((hfield = ishfield(linebuf, colon, field)) != NULL)
			oldhfield = save2str(hfield, oldhfield);
	}
	return oldhfield;
}

/*
 * Return the next header field found in the given message.
 * Return >= 0 if something found, < 0 elsewise.
 * "colon" is set to point to the colon in the header.
 * Must deal with \ continuations & other such fraud.
 */
int
gethfield(f, linebuf, rem, colon)
	register FILE *f;
	char linebuf[];
	register int rem;
	char **colon;
{
	char line2[LINESIZE];
	register char *cp, *cp2;
	register int c;

	for (;;) {
		if (--rem < 0)
			return -1;
		if ((c = readline(f, linebuf, LINESIZE)) <= 0)
			return -1;
		for (cp = linebuf; isprint(*cp) && *cp != ' ' && *cp != ':';
		     cp++)
			;
		if (*cp != ':' || cp == linebuf)
			continue;
		/*
		 * I guess we got a headline.
		 * Handle wraparounding
		 */
		*colon = cp;
		cp = linebuf + c;
		for (;;) {
			while (--cp >= linebuf && (*cp == ' ' || *cp == '\t'))
				;
			cp++;
			if (rem <= 0)
				break;
			ungetc(c = getc(f), f);
			if (c != ' ' && c != '\t')
				break;
			if ((c = readline(f, line2, LINESIZE)) < 0)
				break;
			rem--;
			for (cp2 = line2; *cp2 == ' ' || *cp2 == '\t'; cp2++)
				;
			c -= cp2 - line2;
			if (cp + c >= linebuf + LINESIZE - 2)
				break;
			*cp++ = ' ';
			bcopy(cp2, cp, c);
			cp += c;
		}
		*cp = 0;
		return rem;
	}
	/* NOTREACHED */
}

/*
 * Check whether the passed line is a header line of
 * the desired breed.  Return the field body, or 0.
 */

char*
ishfield(linebuf, colon, field)
	char linebuf[], field[];
	char *colon;
{
	register char *cp = colon;

	*cp = 0;
	if (strcasecmp(linebuf, field) != 0) {
		*cp = ':';
		return 0;
	}
	*cp = ':';
	for (cp++; *cp == ' ' || *cp == '\t'; cp++)
		;
	return cp;
}

/*
 * Copy a string, clobbering high-bits and control characters as we go
 */
void
to7strcpy(dest, src, size)
	register char *dest, *src;
	int size;
{
	register char *max;

	max=dest+size-1;
	while (dest<=max) {
		if((*src) < 0) {
		    /* signed char, less than 0 is highbit */
		    *dest++ = '~';
		} else
		if (((*src) == '\t') || ((*src) == '\r') || ((*src) == '\n')) {
		    /* whitespace substitution */
		    *dest++ = ' ';
		} else
		if (((*src) != 0) && (*src) < ' ') {
		    /* control-char substitution */
		    *dest++ = '~';
		} else {
		    *dest++ = *src;
		}

		if (*src == 0)
                  break;
		src++;
	}
}

/*
 * Copy a string, lowercasing it as we go.
 */
void
istrcpy(dest, src, size)
	register char *dest, *src;
	int size;
{
	register char *max;

	max=dest+size-1;
	while (dest<=max) {
		if (isupper(*src)) {
			*dest++ = tolower(*src);
		} else {
			*dest++ = *src;
		}
		if (*src++ == 0)
                  break;
	}
}

/*
 * The following code deals with input stacking to do source
 * commands.  All but the current file pointer are saved on
 * the stack.
 */

static	int	ssp;			/* Top of file stack */
struct sstack {
	FILE	*s_file;		/* File we were in. */
	int	s_cond;			/* Saved state of conditionals */
	int	s_loading;		/* Loading .mailrc, etc. */
} sstack[NOFILE];

/*
 * Pushdown current input file and switch to a new one.
 * Set the global flag "sourcing" so that others will realize
 * that they are no longer reading from a tty (in all probability).
 */
int
source(v)
	void *v;
{
	char **arglist = v;
	FILE *fi;
	char *cp;

	if ((cp = expand(*arglist)) == NOSTR)
		return(1);
	if ((fi = Fopen(cp, "r")) == NULL) {
		perror(cp);
		return(1);
	}
	if (ssp >= NOFILE - 1) {
		printf("Too much \"sourcing\" going on.\n");
		Fclose(fi);
		return(1);
	}
	sstack[ssp].s_file = input;
	sstack[ssp].s_cond = cond;
	sstack[ssp].s_loading = loading;
	ssp++;
	loading = 0;
	cond = CANY;
	input = fi;
	sourcing++;
	return(0);
}

/*
 * Pop the current input back to the previous level.
 * Update the "sourcing" flag as appropriate.
 */
int
unstack()
{
	if (ssp <= 0) {
		printf("\"Source\" stack over-pop.\n");
		sourcing = 0;
		return(1);
	}
	Fclose(input);
	if (cond != CANY)
		printf("Unmatched \"if\"\n");
	ssp--;
	cond = sstack[ssp].s_cond;
	loading = sstack[ssp].s_loading;
	input = sstack[ssp].s_file;
	if (ssp == 0)
		sourcing = loading;
	return(0);
}

/*
 * Touch the indicated file.
 * This is nifty for the shell.
 */
void
alter(name)
	char *name;
{
	struct stat sb;
	struct timeval tv[2];

	if (stat(name, &sb))
		return;
	tv[0].tv_sec = time((time_t *)0) + 1;
	tv[1].tv_sec = sb.st_mtime;
	tv[0].tv_usec = tv[1].tv_usec = 0;
	(void)utimes(name, tv);
}

/*
 * Examine the passed line buffer and
 * return true if it is all blanks and tabs.
 */
int
blankline(linebuf)
	char linebuf[];
{
	register char *cp;

	for (cp = linebuf; *cp; cp++)
		if (*cp != ' ' && *cp != '\t')
			return(0);
	return(1);
}

/*
 * Get sender's name from this message.  If the message has
 * a bunch of arpanet stuff in it, we may have to skin the name
 * before returning it.
 */
char *
nameof(mp, reptype)
	register struct message *mp;
	int reptype;
{
	register char *cp, *cp2;

	cp = skin(name1(mp, reptype));
	if (reptype != 0 || charcount(cp, '!') < 2)
		return(cp);
	cp2 = rindex(cp, '!');
	cp2--;
	while (cp2 > cp && *cp2 != '!')
		cp2--;
	if (*cp2 == '!')
		return(cp2 + 1);
	return(cp);
}

/*
 * Start of a "comment".
 * Ignore it.
 */
char *
skip_comment(cp)
	register char *cp;
{
	register int nesting = 1;

	for (; nesting > 0 && *cp; cp++) {
		switch (*cp) {
		case '\\':
			if (cp[1])
				cp++;
			break;
		case '(':
			nesting++;
			break;
		case ')':
			nesting--;
			break;
		}
	}
	return cp;
}

/*
 * Skin an arpa net address according to the RFC 822 interpretation
 * of "host-phrase."
 */
char *
skin(name)
	char *name;
{
	register int c;
	register char *cp, *cp2;
	char *bufend;
	int gotlt, lastsp;
	char nbuf[BUFSIZ];

	if (name == NOSTR)
		return(NOSTR);
	if (index(name, '(') == NOSTR && index(name, '<') == NOSTR
	    && index(name, ' ') == NOSTR)
		return(name);
	gotlt = 0;
	lastsp = 0;
	bufend = nbuf;
	for (cp = name, cp2 = bufend; (c = *cp++) != '\0'; ) {
		switch (c) {
		case '(':
			cp = skip_comment(cp);
			lastsp = 0;
			break;

		case '"':
			/*
			 * Start of a "quoted-string".
			 * Copy it in its entirety.
			 */
			while ((c = *cp) != '\0') {
				cp++;
				if (c == '"')
					break;
				if (c != '\\')
					*cp2++ = c;
				else if ((c = *cp) != '\0') {
					*cp2++ = c;
					cp++;
				}
			}
			lastsp = 0;
			break;

		case ' ':
			/* super super old fashioned addressing: foo at bar */
			if (cp[0] == 'a' && cp[1] == 't' && cp[2] == ' ')
				cp += 3, *cp2++ = '@';
			else
			if (cp[0] == '@' && cp[1] == ' ')
				cp += 2, *cp2++ = '@';
			else
				lastsp = 1;
			break;

		case '<':
			cp2 = bufend;
			gotlt++;
			lastsp = 0;
			break;

		case '>':
			if (gotlt) {
				gotlt = 0;
				while ((c = *cp) && c != ',') {
					cp++;
					if (c == '(')
						cp = skip_comment(cp);
					else if (c == '"')
						while ((c = *cp) != '\0') {
							cp++;
							if (c == '"')
								break;
							if (c == '\\' && *cp)
								cp++;
						}
				}
				lastsp = 0;
				break;
			}
			/* Fall into . . . */

		default:
                        /* use just one space, but not if inside <brackets> */
			if (lastsp && !gotlt) {
				lastsp = 0;
				*cp2++ = ' ';
			}
			*cp2++ = c;
			if (c == ',' && !gotlt) {
				*cp2++ = ' ';
				for (; *cp == ' '; cp++)
					;
				lastsp = 0;
				bufend = cp2;
			}
		}
	}
	*cp2 = 0;

	return(savestr(nbuf));
}

/*
 * Fetch the sender's name from the passed message.
 * Reptype can be
 *	0 -- get sender's name for display purposes
 *	1 -- get sender's name for reply
 *	2 -- get sender's name for Reply
 */
char *
name1(mp, reptype)
	register struct message *mp;
	int reptype;
{
	char namebuf[LINESIZE];
	char linebuf[LINESIZE];
	register char *cp, *cp2;
	register FILE *ibuf;
	int first = 1;

	if ((cp = hfield("from", mp)) != NOSTR)
		return cp;
	if (reptype == 0 && (cp = hfield("sender", mp)) != NOSTR)
		return cp;
	ibuf = setinput(mp);
	namebuf[0] = 0;
	if (readline(ibuf, linebuf, LINESIZE) < 0)
		return(savestr(namebuf));
newname:
	for (cp = linebuf; *cp && *cp != ' '; cp++)
		;
	for (; *cp == ' ' || *cp == '\t'; cp++)
		;
	for (cp2 = &namebuf[strlen(namebuf)];
	     *cp && *cp != ' ' && *cp != '\t' && cp2 < namebuf + LINESIZE - 1;)
		*cp2++ = *cp++;
	*cp2 = '\0';
	if (readline(ibuf, linebuf, LINESIZE) < 0)
		return(savestr(namebuf));
	if ((cp = index(linebuf, 'F')) == NULL)
		return(savestr(namebuf));
	if (strncmp(cp, "From", 4) != 0)
		return(savestr(namebuf));
	while ((cp = index(cp, 'r')) != NULL) {
		if (strncmp(cp, "remote", 6) == 0) {
			if ((cp = index(cp, 'f')) == NULL)
				break;
			if (strncmp(cp, "from", 4) != 0)
				break;
			if ((cp = index(cp, ' ')) == NULL)
				break;
			cp++;
			if (first) {
				strncpy(namebuf, cp, LINESIZE);
				first = 0;
			} else {
				cp2=rindex(namebuf, '!')+1;
				strncpy(cp2, cp, (namebuf+LINESIZE)-cp2);
			}
			namebuf[LINESIZE-2]='\0';
			strcat(namebuf, "!");
			goto newname;
		}
		cp++;
	}
	return(savestr(namebuf));
}

/*
 * Count the occurances of c in str
 */
int
charcount(str, c)
	char *str;
	int c;
{
	register char *cp;
	register int i;

	for (i = 0, cp = str; *cp; cp++)
		if (*cp == c)
			i++;
	return(i);
}

/*
 * Are any of the characters in the two strings the same?
 */
int
anyof(s1, s2)
	register char *s1, *s2;
{

	while (*s1)
		if (index(s2, *s1++))
			return 1;
	return 0;
}

/*
 * Convert c to upper case
 */
int
raise(c)
	register int c;
{

	if (islower(c))
		return toupper(c);
	return c;
}

/*
 * Copy s1 to s2, return pointer to null in s2.
 */
char *
copy(s1, s2)
	register char *s1, *s2;
{

	while ((*s2++ = *s1++) != '\0')
		;
	return s2 - 1;
}

/*
 * See if the given header field is supposed to be ignored.
 */
int
isign(field, ignore)
	char *field;
	struct ignoretab ignore[2];
{
	char realfld[BUFSIZ];

	if (ignore == ignoreall)
		return 1;
	/*
	 * Lower-case the string, so that "Status" and "status"
	 * will hash to the same place.
	 */
	istrcpy(realfld, field, BUFSIZ);
	realfld[BUFSIZ-1]='\0';
	/*
	 * If there are any headers in retain list, use that instead of
	 * the ignore list, just reversing the membership test.
	 */
	if (ignore[1].i_count > 0)
		return (!member(realfld, ignore + 1));
	else
		return (member(realfld, ignore));
}

int
member(realfield, table)
	register char *realfield;
	struct ignoretab *table;
{
	register struct ignore *igp;

	for (igp = table->i_head[hash(realfield)]; igp != 0; igp = igp->i_link)
		if (*igp->i_field == *realfield &&
		    equal(igp->i_field, realfield))
			return (1);
	return (0);
}

/*
 * See if the given header field is supposed to be highlighted
 */
int
ishl(field, highlight)
	char *field;
	struct ignoretab highlight[2];
{
	char realfld[BUFSIZ];

	/*
	 * Lower-case the string, so that "Status" and "status"
	 * will hash to the same place.
	 */
	istrcpy(realfld, field, BUFSIZ);
	realfld[BUFSIZ-1]='\0';
	return (member(realfld, highlight));
}

/*
 * A hardcoded default interesting header list check
 */
int
definter(field)
	char *field;
{
	if(*field == 't' && equal("to", field))
		return(1);
	if(*field == 'f' && equal("from", field))
		return(1);
	if(*field == 's' && equal("subject", field))
		return(1);
	if(*field == 'c' && equal("cc", field))
		return(1);
	return (0);
}

/*
 * See if the given header field is interesting
 */
int
isinter(field, highlight)
	char *field;
	struct ignoretab highlight[2];
{
	char realfld[BUFSIZ];

	/*
	 * Lower-case the string, so that "Status" and "status"
	 * will hash to the same place.
	 */
	istrcpy(realfld, field, BUFSIZ);
	realfld[BUFSIZ-1]='\0';
	if (highlight[1].i_count > 0)
		return (member(realfld, highlight+1));
	else
		return (definter(realfld));
}

