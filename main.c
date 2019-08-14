/*	$OpenBSD: main.c,v 1.5 1996/06/08 19:48:31 christos Exp $	*/
/*	$NetBSD: main.c,v 1.5 1996/06/08 19:48:31 christos Exp $	*/

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
static char copyright[] =
"@(#) Copyright (c) 1980, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)main.c	8.1 (Berkeley) 6/6/93";
#else
static char rcsid[] = "$OpenBSD: main.c,v 1.5 1996/06/08 19:48:31 christos Exp $";
#endif
#endif /* not lint */

/*
 * Most strcpy/sprintf functions have been changed to strncpy/snprintf to
 * correct several buffer overruns (at least one ot them was exploitable).
 * Sat Jun 20 04:58:09 CEST 1998 Alvaro Martinez Echevarria <alvaro@lander.es>
 */

#include "rcv.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include "extern.h"

/*
 * Mail -- a mail program
 *
 * Startup -- interface with user.
 */

jmp_buf	hdrjmp;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	register int i;
	struct name *to, *cc, *bcc, *smopts;
	char *subject;
	char *ef;
	char *ef_var;
	char *cmdoption;
	char *espot;
	char nosrc = 0;
	sig_t prevint;

        /*
         * Absolutely the first thing we do is save our egid
         * and set it to the rgid, so that we can safely run
         * setgid.  We use the sgid (saved set-gid) to allow ourselves
         * to revert to the egid if we want (temporarily) to become
         * priveliged.
         */
        effectivegid = getegid();
        realgid = getgid();
        if (setgid (realgid) < 0) {
          perror("setgid");
          exit(1);
 	}
 
	/*
	 * Set up a reasonable environment.
	 * Figure out whether we are being run interactively,
	 * start the SIGCHLD catcher, and so forth.
	 */
	(void) signal(SIGCHLD, sigchild);
	if (isatty(0))
		assign("interactive", "");
	image = -1;
	/*
	 * Now, determine how we are being used.
	 * We successively pick off - flags.
	 * If there is anything left, it is the base of the list
	 * of users to mail to.  Argp will be set to point to the
	 * first of these users.
	 */
	ef = NOSTR;
	ef_var = NOSTR;
	to = NIL;
	cc = NIL;
	bcc = NIL;
	cmdoption = NULL;
	smopts = NIL;
	subject = NOSTR;
	shown_eof = 0;
	while ((i = getopt(argc, argv, "INT:b:c:dF:finS:s:u:ve:")) != EOF) {
		switch (i) {
		case 'T':
			/*
			 * Next argument is temp file to write which
			 * articles have been read/deleted for netnews.
			 */
			Tflag = optarg;
			if ((i = creat(Tflag, 0600)) < 0) {
				perror(Tflag);
				exit(1);
			}
			close(i);
			break;
		case 'e':
			/*
			 * Next argument is first command to run.
			 */
			cmdoption = optarg;
			break;
		case 'u':
			/*
			 * Next argument is person to pretend to be.
			 */
			myname = optarg;
			break;
		case 'i':
			/*
			 * User wants to ignore interrupts.
			 * Set the variable "ignore"
			 */
			assign("ignore", "");
			break;
		case 'd':
			debug++;
			break;
		case 'S':
			/* copied from nail: enable 'set'tings on
			 * command line:
			 *    -S variable[=value]
			 */
			espot = strchr(optarg, '=');
			if (espot) {
			  *espot = 0;
			  espot++;
			  assign(optarg, espot);
			} else {
			  assign(optarg, "");
			}
			break;
		case 's':
			/*
			 * Give a subject field for sending from
			 * non terminal
			 */
			subject = optarg;
			break;
		case 'f':
			/*
			 * User is specifying file to "edit" with Mail,
			 * as opposed to reading system mailbox.
			 * If no argument is given after -f, we read his
			 * mbox file.
			 *
			 * getopt() can't handle optional arguments, so here
			 * is an ugly hack to get around it.
			 */
			if((ef != NOSTR) || (ef_var != NOSTR)) {
				fprintf(stderr, "Can't specify multiple files to read\n");
				exit(1);
			}
			if ((argv[optind]) && (argv[optind][0] != '-'))
				ef = argv[optind++];
			else
				ef = "&";
			break;
		case 'F':
			/*
			 * like -f, but the file is a variable set in a
			 * mailrc file (and we always want the arg)
			 */
			if((ef != NOSTR) || (ef_var != NOSTR)) {
				fprintf(stderr, "Can't specify multiple files to read\n");
				exit(1);
			}
			ef_var = optarg;
			break;
		case 'n':
			/*
			 * User doesn't want to source /usr/lib/Mail.rc
			 */
			nosrc++;
			break;
		case 'N':
			/*
			 * Avoid initial header printing.
			 */
			assign("noheader", "");
			break;
		case 'v':
			/*
			 * Send mailer verbose flag
			 */
			assign("verbose", "");
			break;
		case 'I':
			/*
			 * We're interactive
			 */
			assign("interactive", "");
			break;
		case 'c':
			/*
			 * Get Carbon Copy Recipient list
			 */
			cc = cat(cc, nalloc(optarg, GCC));
			break;
		case 'b':
			/*
			 * Get Blind Carbon Copy Recipient list
			 */
			bcc = cat(bcc, nalloc(optarg, GBCC));
			break;
		case '?':
			fputs("\
Usage: mail [-iInv] [-S var[=value]] [-s subject] [-c CCaddr] [-b BCCaddr]\n\
	    to-addr... [ - sendmail-options ]\n\
       mail [-iInNv] [-S var[=value]] [-e command] -f [name]\n\
       mail [-iInNv] [-S var[=value]] [-e command] [-F boxvar]\n\
       mail [-iInNv] [-S var[=value]] [-e command] [-u user]\n",
				stderr);
			exit(1);
		}
	}
	for (i = optind; (argv[i]) && (*argv[i] != '-'); i++)
		to = cat(to, nalloc(argv[i], GTO));
	for (; argv[i]; i++)
		smopts = cat(smopts, nalloc(argv[i], 0));
	/*
	 * Check for inconsistent arguments.
	 */
	if (to == NIL && (subject != NOSTR || cc != NIL || bcc != NIL)) {
		fputs("You must specify direct recipients with -s, -c, or -b.\n", stderr);
		exit(1);
	}
	if (((ef != NOSTR) || (ef_var != NOSTR)) && to != NIL) {
		fprintf(stderr, "Cannot give -f/-F and people to send to.\n");
		exit(1);
	}
	tinit();
	setscreensize();
	input = stdin;
	rcvmode = !to;
	spreserve();
	if (!nosrc)
		load(_PATH_MASTER_RC);
	/*
	 * Expand returns a savestr, but load only uses the file name
	 * for fopen, so it's safe to do this.
	 */
	load(expand("~/.mailrc"));
	if (!rcvmode) {
		mail(to, cc, bcc, smopts, subject);
		/*
		 * why wait?
		 */
		exit(senderr);
	}
	/*
	 * Ok, we are reading mail.
	 * Decide whether we are editing a mailbox or reading
	 * the system mailbox, and open up the right stuff.
	 */
	if (ef == NOSTR)
		ef = "%";
	/* value lookup needs to come after the load() of ~/.mailrc */
	if (ef_var != NOSTR) {
		ef = value(ef_var);
		if(ef == NOSTR) {
			fprintf(stderr, "No variable '%s' to expand.\n", ef_var);
			exit(1);
		}
	}
	if (setfile(ef) < 0)
		exit(1);		/* error already reported */
	if (setjmp(hdrjmp) == 0) {
		extern char *version;

		if ((prevint = signal(SIGINT, SIG_IGN)) != SIG_IGN)
			signal(SIGINT, hdrstop);
		if (value("quiet") == NOSTR)
			printf("Mail version %s.  Type ? for help.\n",
				version);
		announce(HIDE_PREVIOUS);
		fflush(stdout);
		signal(SIGINT, prevint);
	}
	commands(cmdoption);
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	quit();
	exit(0);
}

/*
 * Interrupt printing of the headers.
 */
void
hdrstop(signo)
	int signo;
{

	fflush(stdout);
	fprintf(stderr, "\nInterrupt\n");
	longjmp(hdrjmp, 1);
}

/*
 * Compute what the screen size for printing headers should be.
 * We use the following algorithm for the height:
 *	If baud rate < 1200, use  9
 *	If baud rate = 1200, use 14
 *	If baud rate > 1200, use 24 or ws_row
 * Width is either 80 or ws_col;
 */
void
setscreensize()
{
	struct termios tbuf;
	struct winsize ws;
	speed_t ospeed;

	if (ioctl(1, TIOCGWINSZ, (char *) &ws) < 0)
		ws.ws_col = ws.ws_row = 0;
	if (tcgetattr(1, &tbuf) < 0)
		ospeed = B9600;
	else
		ospeed = cfgetospeed(&tbuf);
	if (ospeed < B1200)
		screenheight = 9;
	else if (ospeed == B1200)
		screenheight = 14;
	else if (ws.ws_row != 0)
		screenheight = ws.ws_row;
	else
		screenheight = 24;
	if ((realscreenheight = ws.ws_row) == 0)
		realscreenheight = 24;
	if ((screenwidth = ws.ws_col) == 0)
		screenwidth = 80;
#if 0
#define SCREEN_RULER "123456789112345678921234567893123456789412345678951234567896123456789712345678981234567899123456789A123456789B"

	/* TESTING narrow, wide, etc screensizes */
	screenwidth=50;
	printf("%.*s\n", (screenwidth - 1), SCREEN_RULER);
#endif
}
