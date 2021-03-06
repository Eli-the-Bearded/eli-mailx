/*	$OpenBSD: list.c,v 1.4 1996/06/08 19:48:30 christos Exp $	*/
/*	$NetBSD: list.c,v 1.4 1996/06/08 19:48:30 christos Exp $	*/

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
static char sccsid[] = "@(#)list.c	8.2 (Berkeley) 4/19/94";
#else
static char rcsid[] = "$OpenBSD: list.c,v 1.4 1996/06/08 19:48:30 christos Exp $";
#endif
#endif /* not lint */

#include "rcv.h"
#include <ctype.h>
#include "extern.h"

/*
 * Mail -- a mail program
 *
 * Message list handling.
 */

/*
 * Convert the user string of message numbers and
 * store the numbers into vector.
 *
 * Returns the count of messages picked up or -1 on error.
 */
int
getmsglist(buf, vector, flags)
	char *buf;
	int *vector, flags;
{
	register int *ip;
	register struct message *mp;

	if (msgCount == 0) {
		*vector = 0;
		return 0;
	}
	if (markall(buf, flags) < 0) {
		return(-1);
	}
	ip = vector;
	for (mp = &message[0]; mp < &message[msgCount]; mp++)
		if (mp->m_flag & MMARK)
			*ip++ = mp - &message[0] + 1;
	*ip = 0;
	return(ip - vector);
}

/*
 * Mark all messages that the user wanted from the command
 * line in the message structure.  Return 0 on success, -1
 * on error.
 */

/*
 * The following table describes the letters which can follow
 * the colon and gives the corresponding modifier bit.
 */

struct coltab {
	char	co_char;		/* What filter name after : */
	int	co_bit;			/* Associated modifier bit */
	int	co_mask;		/* m_status bits to mask */
	int	co_equal;		/* ... must equal this */
	char    co_name[CO_NAMESIZE];	/* Name to print */
} coltab[] = {
    { COLT_NEW,		CMNEW,		MNEW,		MNEW,		"new" },
    { COLT_OLD,		CMOLD,		MNEW,		0,		"old" },
    { DISP_UNRD,	CMUNREAD,	MREAD,		0,		"unread" },
    { COLT_DEL,		CMDELETED,	MDELETED,	MDELETED,	"deleted" },
    { DISP_READ,	CMREAD,		MREAD,		MREAD,		"read" },
    { DISP_FLAG,	CMFLAG,		MFLAG,		MFLAG,		"flagged" },
    { DISP_REMB,	CMREMEMBER,	MREMEMBER,	MREMEMBER,	"marked" },
    { DISP_BOTH,	CMBOTHUSER,	MBOTHUSER,	MBOTHUSER,	"marked and flagged" },
    { COLT_NONE,	CMNOUSER,	MBOTHUSER,	0,		"no marks or flags" },
    { 0,		0,		0,		0,		"" }
};

static	int	lastcolmod;

/*
 * This does the main work of parsing the description of the messages
 * to be selected. buf is the command buffer from the user, f is a 
 * set of message flags that must also be met. delete() / undelete()
 * select things differently than print() for from(), f gives us the
 * rules to use.
 */
int
markall(buf, f)
	char buf[];
	int f;
{
	register char **np;
	register int i;
	register struct message *mp;
	char *namelist[NMLSIZE], *bufp;
	int tok, beg, mc, star, other, valdot, colmod, colresult, check_value;
	int offset, end_off;

	valdot = dot - &message[0] + 1;
	colmod = 0;
	for (i = 1; i <= msgCount; i++)
		saveunmark(i);
	bufp = buf;
	mc = 0;
	np = &namelist[0];
	scaninit();
	tok = scan(&bufp);
	star = 0;
	other = 0;
	beg = 0;
	check_value = 0;
	while (tok != TEOL) {
		switch (tok) {

		/* use last matching list
		 * eg: test a match with "f /something"
		 *     and if it looks right, operate on it with (eg) "d !"
                 */
		case TBANG:
		case TTILDE:
			if (other) {
#define CANT_MIX_EM		printf("Can't mix \"*\", \"!\", \"~\" with anything\n");
				return(-1);
			}
			other++;
			for (i = 1; i <= msgCount; i++)
				usesavemark(i, (TTILDE == tok));
			break;

		case TNUMBER:
number:
			if (star) {
				printf("No numbers mixed with *\n");
				return(-1);
			}
			mc++;
			other++;
			if (beg != 0) {
				if (check(lexnumber, f))
					return(-1);
				for (i = beg; i <= lexnumber; i++)
					if (f == MDELETED || f == M_ALL || (message[i - 1].m_flag & MDELETED) == 0)
						mark(i);
				beg = 0;
				break;
			}
			beg = lexnumber;
			if (check(beg, f))
				return(-1);
			tok = scan(&bufp);
			regret(tok);
			if (tok != TDASH) {
				mark(beg);
				beg = 0;
			}
			break;

		case TPLUS:
			if (beg != 0) {
				printf("Non-numeric second argument\n");
				return(-1);
			}
			i = valdot;
			do {
				i++;
				if (i > msgCount) {
					printf("Referencing beyond EOF\n");
					return(-1);
				}
			} while ((message[i - 1].m_flag & MDELETED) != f);
			mark(i);
			break;

		case TDASH:
			if (beg == 0) {
				i = valdot;
				do {
					i--;
					if (i <= 0) {
						printf("Referencing before 1\n");
						return(-1);
					}
				} while ((message[i - 1].m_flag & MDELETED) != f);
				mark(i);
			}
			break;

		case TSTRING:
			if (beg != 0) {
				printf("Non-numeric second argument\n");
				return(-1);
			}
			other++;
			if (lexstring[0] == ':') {
				colresult = evalcol(lexstring[1]);
				if (colresult == 0) {
					printf("Unknown colon modifier \"%s\"\n",
					    lexstring);
					return(-1);
				}
				colmod |= colresult;
			}
			else
				*np++ = savestr(lexstring);
			break;

		case THASH:
			if (lexsizecheck < 0) {
				puts("Need an offset parameter\n");
				return(-1);
			}
			switch (lexsizeflag) {
				case SC_LINES:
				case SC_BYTES:
					check_value = lexsizecheck;
					break;
				case SC_CLINES:
					check_value = FROM_CENT(lexsizecheck);
					lexsizeflag = SC_LINES;
					break;
				case SC_KBYTES:
					check_value = FROM_KILO(lexsizecheck);
					lexsizeflag = SC_BYTES;
					break;
				case SC_MBYTES:
					check_value = FROM_MEGA(lexsizecheck);
					lexsizeflag = SC_BYTES;
					break;
			}
			for (i = 0; i < msgCount; i++)
				if (f == M_ALL ||
				    (message[i].m_flag & MDELETED) == f) {
					if(SC_BYTES == lexsizeflag) {
						offset  = message[i].m_offset
							+ ( BLOCK_SIZE *
						  	message[i].m_block );
						end_off = offset +
							message[i].m_size;
					} else {
						/* lines */
						offset  = message[i].m_loffset;
						end_off = offset +
							message[i].m_lines;
					}

					if ((offset <= check_value) &&
					    (check_value <= end_off)
						   ) {
						mark(i+1);
						mc++;
					}
				}
			if (mc == 0) {
				printf("%s %d out of bounds (max %d) or deleted.\n",
				(SC_BYTES == lexsizeflag)? "Byte" : "Line",
				check_value, end_off);
				return(-1);
			}
			break;


		case TOVER:
		case TUNDER:
		case TEQUAL:
			if (lexsizecheck < 0) {
				puts("Need a size value\n");
				return(-1);
			}

			/* find matching messages */
			for (i = 0; i < msgCount; i++)
				if (f == M_ALL || (message[i].m_flag & MDELETED) == f) {
					int check_match = 0;

					switch (lexsizeflag) {
						case SC_LINES:
							check_value = message[i].m_lines;
							break;
						case SC_CLINES:
							check_value = TO_CENT(message[i].m_lines);
							break;
						case SC_BYTES:
							check_value = message[i].m_size;
							break;
						case SC_KBYTES:
							check_value = TO_KILO(message[i].m_size);
							break;
						case SC_MBYTES:
							check_value = TO_MEGA(message[i].m_size);
							break;

					}

					switch (tok) {
						case TEQUAL:
							check_match = (check_value == lexsizecheck);
							break;
						case TUNDER:
							check_match = (check_value < lexsizecheck);
							break;
						case TOVER:
							check_match = (check_value > lexsizecheck);
							break;
					}
					if(check_match) {
						mark(i+1);
						mc++;
					}
				}
			if (mc == 0) {
				printf("No applicable messages.\n");
				return(-1);
			}
			break;

		case TDOLLAR:
		case TUP:
		case TDOT:
			lexnumber = metamess(lexstring[0], f);
			if (lexnumber == -1)
				return(-1);
			goto number;

		case TSTAR:
			if (other) {
				CANT_MIX_EM;
				return(-1);
			}
			star++;
			other++;
			break;

		case TERROR:
			return -1;
		}
		tok = scan(&bufp);
	}
	lastcolmod = colmod;
	*np = NOSTR;
	if (star) {
		mc = 0;
		for (i = 0; i < msgCount; i++)
			if (f == M_ALL || (message[i].m_flag & MDELETED) == f) {
				mark(i+1);
				mc++;
			}
		if (mc == 0) {
			printf("No applicable messages.\n");
			return(-1);
		}
		return(0);
	}

	/*
	 * If no numbers were given (or messages selected by size), mark all
	 * of the messages, so that we can unmark any whose sender was not
	 * selected if any user names were given.
	 */

	if ((np > namelist || colmod != 0) && mc == 0)
		for (i = 1; i <= msgCount; i++)
			if (f == M_ALL || (message[i-1].m_flag & MDELETED) == f)
				mark(i);

	/*
	 * If any names were given, go through and eliminate any
	 * messages whose senders were not requested.
	 */

	if (np > namelist) {
		for (i = 1; i <= msgCount; i++) {
			for (mc = 0, np = &namelist[0]; *np != NOSTR; np++)
				if ((**np == '/') || (**np == '%')) {
					if (matchsubj(*np, i)) {
						mc++;
						break;
					}
				}
				else {
					if (matchsender(*np, i)) {
						mc++;
						break;
					}
				}
			if (mc == 0)
				unmark(i);
		}

		/*
		 * Make sure we got some decent messages.
		 */

		mc = 0;
		for (i = 1; i <= msgCount; i++)
			if (message[i-1].m_flag & MMARK) {
				mc++;
				break;
			}
		if (mc == 0) {
			printf("No messages matching {%s",
				namelist[0]);
			for (np = &namelist[1]; *np != NOSTR; np++)
				printf(", %s", *np);
			printf("}\n");
			return(-1);
		}
	}

	/*
	 * If any colon modifiers were given, go through and
	 * unmark any messages which do not satisfy the modifiers.
	 */

	if (colmod != 0) {
		for (i = 1; i <= msgCount; i++) {
			register struct coltab *colp;

			mp = &message[i - 1];
			for (colp = &coltab[0]; colp->co_char; colp++)
				/* & == to ensure all bits of multibit mods */
				if ((colp->co_bit & colmod) == colp->co_bit)
					if ((mp->m_flag & colp->co_mask)
					    != colp->co_equal)
						unmark(i);
			
		}
		for (mp = &message[0]; mp < &message[msgCount]; mp++)
			if (mp->m_flag & MMARK)
				break;
		if (mp >= &message[msgCount]) {
			register struct coltab *colp;

			printf("No messages satisfy");
			for (colp = &coltab[0]; colp->co_char; colp++)
				/* & == to ensure all bits of multibit mods */
				if ((colp->co_bit & colmod) == colp->co_bit)
					printf(" :%c (%s)", colp->co_char, colp->co_name);
			printf("\n");
			return(-1);
		}
	}
	return(0);
}

/*
 * Turn the character after a colon modifier into a bit
 * value.
 */
int
evalcol(col)
	int col;
{
	register struct coltab *colp;

	if (col == 0)
		return(lastcolmod);
	for (colp = &coltab[0]; colp->co_char; colp++)
		if (colp->co_char == col)
			return(colp->co_bit);
	return(0);
}

/*
 * Check the passed message number for legality and proper flags.
 * If f is MDELETED or M_ALL, then either kind will do.  Otherwise, the message
 * has to be undeleted.
 */
int
check(mesg, f)
	int mesg, f;
{
	register struct message *mp;

	if (mesg < 1 || mesg > msgCount) {
		printf("%d: Invalid message number\n", mesg);
		return(-1);
	}
	mp = &message[mesg-1];
	if (f == M_ALL) {
		return(0);
	}
	if (f != MDELETED && (mp->m_flag & MDELETED) != 0) {
		printf("%d: Inappropriate message\n", mesg);
		return(-1);
	}
	return(0);
}

/*
 * Scan out the list of string arguments, shell style
 * for a RAWLIST.
 */
int
getrawlist(line, argv, argc)
	char line[];
	char **argv;
	int  argc;
{
	register char c, *cp, *cp2, quotec;
	int argn;
	char linebuf[BUFSIZ];

	argn = 0;
	cp = line;
	for (;;) {
		for (; *cp == ' ' || *cp == '\t'; cp++)
			;
		if (*cp == '\0')
			break;
		if (argn >= argc - 1) {
			printf(
			"Too many elements in the list; excess discarded.\n");
			break;
		}
		cp2 = linebuf;
		quotec = '\0';
		while ((c = *cp) != '\0') {
			cp++;
			if (quotec != '\0') {
				if (c == quotec) {
					/* need to copy the close backtick */
					if(c == '`') {
						*cp2++ = c;
					}
					quotec = '\0';
				} else if (c == '\\') {
					switch (c = *cp++) {
					case '\0':
						*cp2++ = '\\';
						cp--;
						break;
					case '0': case '1': case '2': case '3':
					case '4': case '5': case '6': case '7':
						c -= '0';
						if (*cp >= '0' && *cp <= '7')
							c = c * 8 + *cp++ - '0';
						if (*cp >= '0' && *cp <= '7')
							c = c * 8 + *cp++ - '0';
						*cp2++ = c;
						break;
					case 'b':
						*cp2++ = '\b';
						break;
					case 'f':
						*cp2++ = '\f';
						break;
					case 'n':
						*cp2++ = '\n';
						break;
					case 'r':
						*cp2++ = '\r';
						break;
					case 't':
						*cp2++ = '\t';
						break;
					case 'v':
						*cp2++ = '\v';
						break;
					default:
						*cp2++ = c;
					}
				} else if (c == '^') {
					c = *cp++;
					if (c == '?')
						*cp2++ = '\177';
					/* null doesn't show up anyway */
					else if ((c >= 'A' && c <= '_') ||
						 (c >= 'a' && c <= 'z'))
						*cp2++ = c & 037;
					else {
						*cp2++ = '^';
						cp--;
					}
				} else
					*cp2++ = c;
			} else if (c == '`') {
				/* backticks are only expanded in set() */
				*cp2++ = c;
				quotec = c;
			} else if (c == '"' || c == '\'')
				quotec = c;
			else if (c == ' ' || c == '\t')
				break;
			else
				*cp2++ = c;
		}
		*cp2 = '\0';
		argv[argn++] = savestr(linebuf);
	}
	argv[argn] = NOSTR;
	return argn;
}

/*
 * scan out a single lexical item and return its token number,
 * updating the string pointer passed **p.  Also, store the value
 * of the number or string scanned in lexnumber or lexstring as
 * appropriate.  In any event, store the scanned `thing' in lexstring.
 */

struct lex {
	char	l_char;
	char	l_token;
} singles[] = {
	{ '$',	TDOLLAR },
	{ '.',	TDOT },
	{ '^',	TUP },
	{ '*',	TSTAR },
	{ '-',	TDASH },
	{ '+',	TPLUS },
	{ '(',	TOPEN },	/* not currently used in parser */
	{ ')',	TCLOSE },	/* not currently used in parser */
	{ '!',	TBANG },
	{ '~',  TTILDE },	/* the anti ! */
	{ 0,	0 }
};
struct lex sizechecks[] = {
	{ '=',	TEQUAL },
	{ '<',	TUNDER },
	{ '>',	TOVER },
	{ '#',  THASH },	/* finds offsets into mbox, not sizes */
	{ 0,	0 }
};

/* The pointer sp will be changed to point to the next thing to scan when
 * returning a token. That next thing may just be a null for end of line.
 */
int
scan(sp)
	char **sp;
{
	register char *cp, *cp2;
	register int c;
	register struct lex *lp;
	int quotec;

	if (regretp >= 0) {
		strncpy(lexstring, string_stack[regretp], STRINGLEN);
		lexstring[STRINGLEN-1]='\0';
		lexnumber = numberstack[regretp];
		return(regretstack[regretp--]);
	}
	cp = *sp;
	cp2 = lexstring;
	c = *cp++;

	/*
	 * strip away leading white space.
	 */

	while (c == ' ' || c == '\t')
		c = *cp++;

	/*
	 * If no characters remain, we are at end of line,
	 * so report that.
	 */

	if (c == '\0') {
		*sp = --cp;
		return(TEOL);
	}

	/*
	 * If the leading character is a digit, scan
	 * the number and convert it on the fly.
	 * Return TNUMBER when done.
	 */

	if (isdigit(c)) {
		lexnumber = 0;
		while (isdigit(c)) {
			lexnumber = lexnumber*10 + c - '0';
			*cp2++ = c;
			c = *cp++;
		}
		*cp2 = '\0';
		*sp = --cp;
		return(TNUMBER);
	}

	/*
	 * Check for single character tokens; return such
	 * if found.
	 */

	for (lp = &singles[0]; lp->l_char != 0; lp++)
		if (c == lp->l_char) {
			lexstring[0] = c;
			lexstring[1] = '\0';
			*sp = cp;
			return(lp->l_token);
		}

	/*
	 * The various size check operators
	 * Format: OPERATOR VALUE [ FLAG ]
	 * With optional whitespace in there.
	 */
	for (lp = &sizechecks[0]; lp->l_char != 0; lp++)
		if (c == lp->l_char) {
			lexsizecheck = -1;
			lexsizeflag  = 0;
			lexstring[0] = c;
			lexstring[1] = '\0';

			c = *cp++;

			/* skip whitespace and test for EOL */
			while (c == ' ' || c == '\t')
				c = *cp++;
			if (c == '\0') {
				*sp = --cp;
				fputs("Incomplete size check\n", stderr);
				return(TERROR);
			}

			if (isdigit(c)) {
				lexsizecheck = 0;
				while (isdigit(c)) {
					lexsizecheck = lexsizecheck*10 + c - '0';
					c = *cp++;
				}
			} else {
				fputs("Could not understand size check\n", stderr);
				return(TERROR);
			}


			/* skip whitespace and test for EOL */
			while (c == ' ' || c == '\t')
				c = *cp++;
			if (c == '\0') {
				*sp = --cp;
				
				/* default flag */
			  	lexsizeflag = SC_LINES;
				return(lp->l_token);
			}

			/* check for a flag */
			switch(c) {
			  case SC_LINES_CHAR:
			  case SC_LINES_ALT:
			  	lexsizeflag = SC_LINES;
				break;
			  case SC_CLINES_CHAR:
			  case SC_CLINES_ALT:
			  	lexsizeflag = SC_CLINES;
				break;
			  case SC_BYTES_CHAR:
			  case SC_BYTES_ALT:
			  	lexsizeflag = SC_BYTES;
				break;
			  case SC_KBYTES_CHAR:
			  case SC_KBYTES_ALT:
			  	lexsizeflag = SC_KBYTES;
				break;
			  case SC_MBYTES_CHAR:
			  case SC_MBYTES_ALT:
			  	lexsizeflag = SC_MBYTES;
				break;
			  default:
				fputs("Unknown size check flag\n", stderr);
				return(TERROR);
			}

			c = *cp++;
			*sp = cp;
			return(lp->l_token);
		} /* size check operator */

	/*
	 * We've got a string!  Copy all the characters
	 * of the string into lexstring, until we see
	 * a null, space, or tab.
	 * If the lead character is a " or ', save it
	 * and scan until you get another.
	 */

	quotec = 0;
	if (c == '\'' || c == '"') {
		quotec = c;
		c = *cp++;
	}
	while (c != '\0') {
		if (c == quotec) {
			cp++;
			break;
		}
		if (quotec == 0 && (c == ' ' || c == '\t'))
			break;
		if (cp2 - lexstring < STRINGLEN-1)
			*cp2++ = c;
		c = *cp++;
	}
	if (quotec && c == 0) {
		fprintf(stderr, "Missing %c\n", quotec);
		return TERROR;
	}
	*sp = --cp;
	*cp2 = '\0';
	return(TSTRING);
}

/*
 * Unscan the named token by pushing it onto the regret stack.
 */
void
regret(token)
	int token;
{
	if (++regretp >= REGDEP)
		panic("Too many regrets");
	regretstack[regretp] = token;
	lexstring[STRINGLEN-1] = '\0';
	string_stack[regretp] = savestr(lexstring);
	numberstack[regretp] = lexnumber;
}

/*
 * Reset all the scanner global variables.
 */
void
scaninit()
{
	regretp = -1;
}

/*
 * Find the first message whose flags & m == f  and return
 * its message number.
 */
int
first(f, m)
	int f, m;
{
	register struct message *mp;

	if (msgCount == 0)
		return 0;
	f &= MDELETED;
	m &= MDELETED;
	for (mp = dot; mp < &message[msgCount]; mp++)
		if ((mp->m_flag & m) == f)
			return mp - message + 1;
	for (mp = dot-1; mp >= &message[0]; mp--)
		if ((mp->m_flag & m) == f)
			return mp - message + 1;
	return 0;
}

/*
 * See if the passed name sent the passed message number.  Return true
 * if so.
 */
int
matchsender(str, mesg)
	char *str;
	int mesg;
{
	register char *cp, *cp2, *backup;

	if (!*str)	/* null string matches nothing instead of everything */
		return 0;
	backup = cp2 = nameof(&message[mesg - 1], 0);
	cp = str;
	while (*cp2) {
		if (*cp == 0)
			return(1);
		if (raise(*cp++) != raise(*cp2++)) {
			cp2 = ++backup;
			cp = str;
		}
	}
	return(*cp == 0);
}

/*
 * See if the given string matches inside the subject field of the
 * given message.  For the purpose of the scan, we ignore case differences.
 * If it does, return true.  The string search argument is assumed to
 * have the form "/search-string."  If it is of the form "/," we use the
 * previous search string.
 * A search of form '%foo' is just like "/foo" except that it will
 * act as if searchheaders is set.
 */

#define DEFAULT_MATCH 0
#define HEADERS_MATCH 1
char lastscan[128];
int
matchsubj(str, mesg)
	char *str;
	int mesg;
{
	register struct message *mp;
	register char *cp, *cp2, *backup;
	int stype;

	if(*str == '%') {
	       stype = HEADERS_MATCH;
	} else {
	       stype = DEFAULT_MATCH;
	}
	str++;
	if (strlen(str) == 0) {
                if (stype == HEADERS_MATCH) {
			/* special case: bare % matches everything */
			return(1);
		}
		str = lastscan;
	} else {
		strncpy(lastscan, str, 128);
		lastscan[127]='\0';
	}
	mp = &message[mesg-1];
	
	/*
	 * Now look, ignoring case, for the word in the string.
	 */

	if ((stype || value("searchheaders")) && (cp = index(str, ':'))) {
		*cp++ = '\0';
		cp2 = hfield(str, mp);
		cp[-1] = ':';
		str = cp;
	} else {
		cp = str;
		cp2 = hfield("subject", mp);
	}
	if (cp2 == NOSTR)
		return(0);
	backup = cp2;
	while (*cp2) {
		if (*cp == 0)
			return(1);
		if (raise(*cp++) != raise(*cp2++)) {
			cp2 = ++backup;
			cp = str;
		}
	}
	return(*cp == 0);
}

/*
 * Mark the named message by setting its mark bit.
 * This "mark" is used internally for messages to operate on during a
 * particular action and is completely different from the user command.
 */
void
mark(mesg)
	int mesg;
{
	register int i;

	i = mesg;
	if (i < 1 || i > msgCount)
		panic("Bad message number to mark");
	message[i-1].m_flag |= MMARK;
}

/*
 * Unmark the named message.
 */
void
unmark(mesg)
	int mesg;
{
	register int i;

	i = mesg;
	if (i < 1 || i > msgCount)
		panic("Bad message number to unmark");
	message[i-1].m_flag &= ~MMARK;
}

/*
 * Restore saved marks.
 * This "mark" is used internally for messages to operate on during a
 * particular action, and the saved marks are accessed via ! history.
 */
void
usesavemark(mesg,invert)
	int mesg;
	int invert;
{
	if (mesg < 1 || mesg > msgCount)
		panic("Bad message number to usesavemark");
        if (message[mesg-1].m_flag & MLASTMARK) {
		message[mesg-1].m_flag |= MMARK;
	}
	if (invert) {
		message[mesg-1].m_flag ^= MMARK;
	}
}

/*
 * Unmark the named message, but save the mark.
 */
void
saveunmark(mesg)
	int mesg;
{
	register int i;
	int for_history;

	i = mesg;
	if (i < 1 || i > msgCount)
		panic("Bad message number to saveunmark");
        for_history = message[i-1].m_flag & MMARK;
	message[i-1].m_flag &= ~(MMARK|MLASTMARK);
	if (for_history) 
		message[i-1].m_flag |= MLASTMARK;
}

/*
 * Return the message number corresponding to the passed meta character.
 */
int
metamess(meta, f)
	int meta, f;
{
	register int c, m;
	register struct message *mp;

	c = meta;
	switch (c) {
	case '^':
		/*
		 * First 'good' message left.
		 */
		for (mp = &message[0]; mp < &message[msgCount]; mp++)
			if ((f == M_ALL) || (mp->m_flag & MDELETED) == f)
				return(mp - &message[0] + 1);
		printf("No applicable messages\n");
		return(-1);

	case '$':
		/*
		 * Last 'good message left.
		 */
		for (mp = &message[msgCount-1]; mp >= &message[0]; mp--)
			if ((f == M_ALL) || (mp->m_flag & MDELETED) == f)
				return(mp - &message[0] + 1);
		printf("No applicable messages\n");
		return(-1);

	case '.':
		/* 
		 * Current message.
		 */
		m = dot - &message[0] + 1;
		if ((f == M_ALL) || (dot->m_flag & MDELETED) != f) {
			printf("%d: Inappropriate message\n", m);
			return(-1);
		}
		return(m);

	default:
		printf("Unknown metachar (%c)\n", c);
		return(-1);
	}
}
