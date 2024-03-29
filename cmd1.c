/*	$OpenBSD: cmd1.c,v 1.5 1996/06/08 19:48:11 christos Exp $	*/
/*	$NetBSD: cmd1.c,v 1.5 1996/06/08 19:48:11 christos Exp $	*/

/*-
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
static char sccsid[] = "@(#)cmd1.c	8.1 (Berkeley) 6/6/93";
#else
static char rcsid[] = "$OpenBSD: cmd1.c,v 1.5 1996/06/08 19:48:11 christos Exp $";
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"

/*
 * Mail -- a mail program
 *
 * User commands.
 */

/*
 * Print the current active headings.
 * Don't change dot if invoker didn't give an argument.
 *
 * Takes a vector, uses the first thing in that vector
 * to find the start of a screen-ful, then ignores the
 * rest of the vector. Use from() for printing an actual
 * vector.
 */

static int screen;

int
headers(v)
	void *v;
{
	int *msgvec = v;
	register int n, mesg, flag;
	register struct message *mp;
	int size;

	size = screensize();
	n = msgvec[0];
	if (n != 0)
		screen = (n-1)/size;
	if (screen < 0)
		screen = 0;
	mp = &message[screen * size];
	if (mp >= &message[msgCount])
		mp = &message[msgCount - size];
	if (mp < &message[0])
		mp = &message[0];
	flag = 0;
	mesg = mp - &message[0];
	if (dot != &message[n-1])
		dot = mp;
	for (; mp < &message[msgCount]; mp++) {
		mesg++;
		if (mp->m_flag & MDELETED)
			continue;
		if (flag++ >= size)
			break;
		printhead(mesg);
	}
	if (flag == 0) {
		printf("No more mail.\n");
		shown_eof = 1;
		return(1);
	}
	return(0);
}

/*
 * Scroll to the next/previous screen
 */
int
scroll(v)
	void *v;
{
	char *arg = v;
	register int s, size;
	int cur[1];

	/* screen is the global index of the current page of messages; it will
	 * be changed if scroll() is successful, then headers is called with
	 * an empty vector to print current screen.
	 */

	cur[0] = 0;
	size = screensize();
	s = screen;
	switch (*arg) {
	case 0:
	case '+':
		s++;
		if (s * size > msgCount) {
			printf("On last screenful of messages\n");
			return(0);
		}
		screen = s;
		break;

	case '-':
		if (--s < 0) {
			printf("On first screenful of messages\n");
			return(0);
		}
		screen = s;
		break;

	default:
		printf("Unrecognized scrolling command \"%s\"\n", arg);
		return(1);
	}
	return(headers(cur));
}

/*
 * Compute screen size.
 */
int
screensize()
{
	int s;
	char *cp;

	if ((cp = value("screen")) != NOSTR && (s = atoi(cp)) > 0)
		return s;
	return screenheight - 4;
}

/*
 * Print out the headlines for each message
 * in the passed message list.
 */
int
from(v)
	void *v;
{
	int *msgvec = v;
	register int *ip;

	for (ip = msgvec; *ip != NOMVEC; ip++)
		printhead(*ip);
	if (--ip >= msgvec)
		dot = &message[*ip - 1];
	return(0);
}

extern int decode_header(char*, char*, int);	/* GREP move to .h */
extern int check_utf8(char*, int);	/* GREP move to .h */
extern int display_length(char*, int);	/* GREP move to .h */
/*
 * Print out the header of a specific message.
 * This is a slight improvement to the standard one.
 */
void
printhead(mesg)
	int mesg;
{
	struct message *mp;
	char headline[LINESIZE], wcount[LINESIZE], *subjline, dispc, curind;
	char pbuf[BUFSIZ], *subj7line, bytechar, *charset;
	struct headline hl;
	int subjlen,k,wcl,size_adj;
	char *name;

	size_adj = 0;
	mp = &message[mesg-1];
	(void) readline(setinput(mp), headline, LINESIZE);
	if ((subjline = hfield("subject", mp)) == NOSTR)
		subjline = hfield("subj", mp);
	/*
	 * Bletch!
	 */
	curind = dot == mp ? '>' : ' ';
        /* So many dispc symbols now!
         * Quick summary:
         *    r: already marked as read
         *    *: saved to a file this session
         *    P: flagged to preserve
         *    u: not marked as read
         *    M: flagged to save to mbox
         *    f: user specified message flag (overrider and saved on Status:)
         *    m: user specified message flag (overrider, but only this session)
         *    B: BOTH previous user specified message flags
	 * At least for now; see def.h for changes
         */
	dispc = DISP_READ;
	if (mp->m_flag & MSAVED)
		dispc = DISP_SAVE;
	if (mp->m_flag & MPRESERVE)
		dispc = DISP_PRES;
	if ((mp->m_flag & (MREAD|MNEW)) == MNEW)
		dispc = DISP_NEW;
	if ((mp->m_flag & (MREAD|MNEW)) == 0)
		dispc = DISP_UNRD;
	if (mp->m_flag & MBOX)
		dispc = DISP_MBOX;
	if (mp->m_flag & MFLAG)
		dispc = DISP_FLAG;
	if (mp->m_flag & MREMEMBER)
		dispc = DISP_REMB;
	if ((mp->m_flag & MFLAG) && (mp->m_flag & MREMEMBER))
		dispc = DISP_BOTH;
	parse(headline, &hl, pbuf);

	subj7line = malloc(LINESIZE);	/* fix to be static GREPgrep */
	if(!subj7line) {
	   (void)fprintf(stderr,"\nOut of memory!\n");
	   exit(2);
	}

	if (screenwidth < VERY_NARROW_SCREEN) {
		/* too small, skip message size altogether */
		wcount[0] = '\0';
		bytechar = '\0';
		size_adj = 15;
	} else if (screenwidth < NARROW_SCREEN) { 
		/* small, just show lines */
		sprintf(wcount, "%4ld", mp->m_lines);
		bytechar = '\0';
		size_adj = 5;
	} else {
		/* full size or close enough */
		/* make the "lines/bytes" bit */
		bytechar = ' ';
		k = TO_KILO(mp->m_size);
		if(k < 3500) {
		  if(mp->m_lines > 9999) {
			  sprintf(wcount, "%3ldc/%-3d ", TO_CENT(mp->m_lines), k);
		  } else {
			  sprintf(wcount, "%4ld/%-3d ", mp->m_lines, k);
		  }
		  bytechar = 'k';
		} else {
		  k = TO_MEGA(mp->m_size);
		  sprintf(wcount, "%3ldc/%-3d ", TO_CENT(mp->m_lines), k);
		  bytechar = 'M';
		}
		
		if(screenwidth >= WIDE_SCREEN) {
			size_adj = -1;
		}
	}

	/* paste in the right k/M multiplier, and shorten space
	 * for subject if wcount went longer than 9 chars
	 */
	wcl = strlen(wcount);
	subjlen = screenwidth - 48 - wcl + size_adj;
	/* "Why 48?" I ask myself in 2017. I'm not sure.
	 * Probably 55 (normal presub width) - 9 (normal wcount width) = 46
	 * + 2 char fudge for extra wide msg num case == 48
	 * size_adj is for 2017 screen size adjustments */

	/* put the bytecount char somewhere in wcount, if we have one */
	if(bytechar) {
		while(wcount[wcl - 1] == ' ') { wcl --; }
		if(wcount[wcl] == ' ') { wcount[wcl] = bytechar; }
	}

	name = value("show-rcpt") != NOSTR ?
		skin(hfield("to", mp)) : nameof(mp, 0);

	if ((charset = value("charset")) != NOSTR) { /* GREPgrep fix document setting */
		if (subjline == NOSTR || subjlen < 1)
			/* doesn't actually do much */
			copy(NO_SUBJECT_PLACEHOLDER, subj7line);
		else {
			int bad_at, cut_at;
			copy(subjline, subj7line);
			/* decodes in place when decoding anything */
			decode_header(subj7line, charset, LINESIZE);
			if(strcasecmp("UTF-8", charset) == 0){

				/* clean up bad UTF-8 ... */
				while(-1 != (bad_at = check_utf8(subj7line, LINESIZE))) {
				  if(bad_at > (4 * subjlen)) {
				    /* we're past the end of the subject we
				     * may display, even if all four-octet
				     * UTF-8.
				     */
				     break;
				  }
				  /* GREPgrep uses ~ as in to7strcpy, maybe make setting? */
				  subj7line[bad_at] = '~';
				}
				
				/* ... because display_length() only works on
				 * valid UTF-8 and 
				 *    printf("%.*s", subjlen, subj7line)
				 * will use a naive truncation.
				 */
				cut_at = display_length(subj7line, subjlen);
				subj7line[cut_at] = 0;
			} else {
				/* charset naive truncation */
				subj7line[subjlen] = 0;
			}

		}
	} else {
		/* go for old 7 bit clean code path */

		/* no subject or no space for subject */
		if (subjline == NOSTR || subjlen < 1)
			/* hopefully the placeholder is 7bit already, but... */
			to7strcpy(subj7line, NO_SUBJECT_PLACEHOLDER, subjlen);
		else {
			to7strcpy(subj7line, subjline, subjlen);
		}
		subj7line[subjlen] = 0;
	}

		 
	/*                current message pointer
	 *           msg num |             terse date (8 char)
	 *        msg flag | |  sender email  |
	 *              |  | |    (15 char)  / 
	 *              |  | |       |      /  
	 *              v  v v       v     v    subject (subjlen char) */
#define VERY_N_HEADER "%c%4d%c%-15.15s %8.8s %s\n"
	/*              ^  ^ ^       ^^    ^
	 *              1  4 1     15 1    8      = 28 pre-subject  */

	/*                current message pointer
	 *           msg num |            date (17 char)
	 *        msg flag | |  sender email |  message count string
	 *              |  | |    (18 char)  |  /     (4 char)
	 *              |  | |       |       |  |     
	 *              v  v v       v       v  v  subject (subjlen char) */
#define NARROW_HEADER "%c%4d%c%-18.18s %14.14s %s %s\n"
	/*              ^  ^ ^       ^^      ^^ ^^
	 *              1  4 1     18 1    14 1 4 1= 48 pre-subject  */

	/*                current message pointer
	 *           msg num |            date (17 char)
	 *        msg flag | |  sender email |  message count string
	 *              |  | |    (20 char)  |  /     (9 char)
	 *              |  | |       |       |  |     
	 *              v  v v       v       v  v  subject (subjlen char) */
#define NORMAL_HEADER "%c%4d%c%-20.20s %17.17s %s %s\n"
	/*              ^  ^ ^       ^^      ^^ ^^
	 *              1  4 1     20 1    17 1 9 1= 55 pre-subject  */

	/*               current message pointer
	 *          msg num |            date (17 char)
	 *       msg flag | |  sender email |  message count string
	 *            |   | |    (20 char)  |  /     (9 char)
	 *            |   | |       |       |  |     
	 *            v   v v       v       v  v  subject (subjlen char) */
#define WIDE_HEADER "%c %4d%c%-20.20s %17.17s %s %s\n"
	/*            ^^  ^ ^       ^^      ^^ ^^
	 *           1 1  4 1     20 1    17 1 9 1= 56 pre-subject  */

	if(screenwidth >= VERY_NARROW_SCREEN) {
		/* Narrow, normal, and wide screen all have the same
		 * columns, but might have different sizes for them. */

		char *use_date;

		if(screenwidth < NARROW_SCREEN) {
			/* skip leading day of week */
			use_date = 3 + hl.l_date;
		} else {
			use_date = hl.l_date;
		}
		printf((screenwidth >= WIDE_SCREEN)?   WIDE_HEADER :(
		       (screenwidth <  NARROW_SCREEN)? NARROW_HEADER :
						       NORMAL_HEADER),
			dispc, mesg, curind, name, use_date, wcount,
			subj7line);
	} else {
		/* Very narrow */
		char tersedate[10];
		int  cp, wp = 0;

		/* hl.l_date looks like "Da Mm/Dd/Yy Hh:Mm", copy out the
		 * "Mm/Dd/Yy" bit. */
		if(strlen(hl.l_date) > 11) {
			for(cp = 3, wp = 0; cp < 11; cp ++, wp ++) {
			  tersedate[wp] = hl.l_date[cp];
			}
		}
		tersedate[wp] = '\0';
                
		printf(VERY_N_HEADER,
			dispc, mesg, curind, name, tersedate,
			subj7line);
	}

	free(subj7line);
}

/*
 * Print out the value of dot.
 */
int
pdot(v)
	void *v;
{
	printf("At message %d of %d (%d of those deleted).\n",
			(int)(dot - &message[0] + 1),
			msgCount, delCount);
	 
	return(0);
}

/*
 * Touch the EOF flag to silence confirmquit option
 */
int
toucheof(v)
	void *v;
{
	shown_eof = 1;
	 
	return(0);
}

/*
 * Print out all the possible commands.
 */
int
pcmdlist(v)
	void *v;
{
	extern const struct cmd cmdtab[];
	register const struct cmd *cp;
	register int cc;

	printf("Commands are:\n");
	for (cc = 0, cp = cmdtab; cp->c_name != NULL; cp++) {
		cc += strlen(cp->c_name) + 2;
		if (cc > screenwidth - 2) {
			printf("\n");
			cc = strlen(cp->c_name) + 2;
		}
		if ((cp+1)->c_name != NOSTR)
			printf("%s, ", cp->c_name);
		else
			printf("%s\n", cp->c_name);
	}
	return(0);
}

/*
 * Paginate messages, honor ignored fields.
 */
int
more(v)
	void *v;
{
	int *msgvec = v;
	return (type1(msgvec, USE_IGNORES, USE_PAGER));
}

/*
 * Paginate messages, even printing ignored fields.
 */
int
More(v)
	void *v;
{
	int *msgvec = v;

	return (type1(msgvec, NO_IGNORES, USE_PAGER));
}

/*
 * Type out messages, honor ignored fields.
 */
int
type(v)
	void *v;
{
	int *msgvec = v;

	return(type1(msgvec, USE_IGNORES, NO_PAGER));
}

/*
 * Type out messages, even printing ignored fields.
 */
int
Type(v)
	void *v;
{
	int *msgvec = v;

	return(type1(msgvec, NO_IGNORES, NO_PAGER));
}

/*
 * Type out the messages requested.
 */
jmp_buf	pipestop;
int
type1(msgvec, doign, page)
	int *msgvec;
	int doign, page;
{
	register int *ip;
	struct message *mp;
	char *cp;
	int nlines;
	FILE *obuf;
#if __GNUC__
	/* Avoid longjmp clobbering */
	(void) &cp;
	(void) &obuf;
#endif

	obuf = stdout;
	if (setjmp(pipestop))
		goto close_pipe;
	if (value("interactive") != NOSTR &&
	    (page || (cp = value("crt")) != NOSTR)) {
		nlines = 0;
		if (!page) {
			for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++)
				nlines += message[*ip - 1].m_lines;
		}
		if (page || nlines > (*cp ? atoi(cp) : realscreenheight)) {
			obuf = Pageropen();
		}
	}
	for (ip = msgvec; *ip && ip - msgvec < msgCount; ip++) {
		mp = &message[*ip - 1];
		touch(mp);
		dot = mp;
		if (value("quiet") == NOSTR) {
			if(delCount) {
				fprintf(obuf, "Message %d of %d (%d of those deleted):\n", 
					*ip, msgCount, delCount);
			} else {
				fprintf(obuf, "Message %d of %d:\n", *ip, msgCount);
			}
		}
		(void) send(mp, obuf, doign ? ignore : 0, NOSTR);
	}
close_pipe:
	if (obuf != stdout) {
		Pagerclose(obuf);
	}
	return(0);
}

/*
 * Respond to a broken pipe signal --
 * probably caused by quitting more.
 */
void
brokpipe(signo)
	int signo;
{
	longjmp(pipestop, 1);
}

/*
 * Print a few selected headers from each desired message.
 * The headers are those listed in the interesting list,
 * an ignoretab structure. If no headers on on the list,
 * default to To, From, CC, and Subject (or whatever is
 * hardcoded in definter() in some future time).
 */
int
headersum(v)
	void *v;
{
	int *msgvec = v;
	register int *ip;
	register struct message *mp;
	int count, lines, int_cache, infld;
	char *cp, *cp2, c, linebuf[LINESIZE];
	FILE *ibuf;

	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		/* start out like top() */
		mp = &message[*ip - 1];
		dot = mp;
		if (value("quiet") == NOSTR) {
			/* use less verbose message count, without
			 * noting the deleted.
			 */
			starthl(stdout);
			printf("Message %d of %d", *ip, msgCount);
			endhl(stdout);
			puts(":");
		}
		ibuf = setinput(mp);
		count = mp->m_lines;
		infld = int_cache = 0;
		for (lines = 0; lines < count; lines++) {
			/* break on read error */
			if (readline(ibuf, linebuf, LINESIZE) < 0)
				break;

			/* skip "From " line */
			if(lines == 0) 
				continue;

			/* break on end of headers */
			if(linebuf[0] == '\0')
				break;

			if (infld && (linebuf[0] == ' ' || linebuf[0] == '\t')) {
				/* multiline header */
				if(int_cache) {
					puts(linebuf);
				}
				continue;
			}

			/* Now like send() but without ancient uucp
			 * headerless mail case
			 */

			/* Find a header field. Recall "Foo : bar" is allowed */
			for (cp = linebuf; (c = *cp++) && c != ':' && !isspace(c);)
				;

			cp2 = --cp;
			*cp2 = 0;	/* temporarily null terminate */
			int_cache = 0;
			infld = 1;
			if(isinter(linebuf, highlight)) {
				int_cache = 1;
				*cp2 = c;	/* restore */
				puts(linebuf);
			}
		}
	}
	return(0);
}

/*
 * Print the top so many lines of each desired message.
 * The number of lines is taken from the variable "toplines"
 * and defaults to 5.
 */
int
top(v)
	void *v;
{
	int *msgvec = v;
	register int *ip;
	register struct message *mp;
	int c, topl, lines, lineb;
	char *valtop, linebuf[LINESIZE];
	FILE *ibuf;

	topl = 5;
	valtop = value("toplines");
	if (valtop != NOSTR) {
		topl = atoi(valtop);
		if (topl < 0 || topl > 10000)
			topl = 5;
	}
	lineb = 1;
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mp = &message[*ip - 1];
		dot = mp;
		if (value("quiet") == NOSTR) {
			/* in 'top' use less verbose message count, without
			 * noting the deleted.
			 */
			starthl(stdout);
			printf("Message %d of %d", *ip, msgCount);
			endhl(stdout);
			puts(":");
		}
		ibuf = setinput(mp);
		c = mp->m_lines;
		if (!lineb)
			printf("\n");
		for (lines = 0; lines < c && lines <= topl; lines++) {
			if (readline(ibuf, linebuf, LINESIZE) < 0)
				break;
			puts(linebuf);
			lineb = blankline(linebuf);
		}
	}
	return(0);
}

/*
 * Touch all the given messages so that they will
 * get mboxed.
 */
int
stouch(v)
	void *v;
{
	int *msgvec = v;
	register int *ip;

	for (ip = msgvec; *ip != 0; ip++) {
		dot = &message[*ip-1];
		/* the touch() used by type1() */
		touch(dot);
		dot->m_flag &= ~MPRESERVE;
	}
	return(0);
}

/*
 * Set the short-term user flag on a message vector.
 * Called "mark" to the user, but "mark" is used internally for
 * messages to operate on during a particular action.
 */
int
sremember(v)
	void *v;
{
	int *msgvec = v;
	register int *ip;

	for (ip = msgvec; *ip != 0; ip++) {
		dot = &message[*ip-1];
		flag(dot,FLAGSET,MREMEMBER);
	}
	return(0);
}

/*
 * Clear the short-term user flag on a message vector.
 */
int
cremember(v)
	void *v;
{
	int *msgvec = v;
	register int *ip;

	for (ip = msgvec; *ip != 0; ip++) {
		dot = &message[*ip-1];
		flag(dot,FLAGUNSET,MREMEMBER);
	}
	return(0);
}

/*
 * Set the saved user flag on a message vector.
 */
int
sflag(v)
	void *v;
{
	int *msgvec = v;
	register int *ip;

	for (ip = msgvec; *ip != 0; ip++) {
		dot = &message[*ip-1];
                /* STATUS so it will be saved */
		flag(dot,FLAGSET,(MFLAG|MSTATUS));
	}
	return(0);
}

/*
 * Clear the saved user flag on a message vector.
 */
int
cflag(v)
	void *v;
{
	int *msgvec = v;
	register int *ip;

	for (ip = msgvec; *ip != 0; ip++) {
		dot = &message[*ip-1];
                /* STATUS so it will be saved */
		flag(dot,FLAGUNSET,(MFLAG|MSTATUS));
	}
	return(0);
}

/*
 * Make sure all passed messages get mboxed.
 */
int
mboxit(v)
	void *v;
{
	int *msgvec = v;
	register int *ip;

	for (ip = msgvec; *ip != 0; ip++) {
		dot = &message[*ip-1];
		dot->m_flag |= MTOUCH|MBOX;
		dot->m_flag &= ~MPRESERVE;
	}
	return(0);
}

/*
 * List the folders the user currently has.
 */
int
folders(v)
	void *v;
{
	char dirname[BUFSIZ];
	char *cmd;

	if (getfold(dirname, BUFSIZ) < 0) {
		printf("No value set for \"folder\"\n");
		return 1;
	}
	if ((cmd = value("LISTER")) == NOSTR)
		cmd = "ls";
	(void) run_command(cmd, 0, -1, -1, dirname, NOSTR, NOSTR);
	return 0;
}
