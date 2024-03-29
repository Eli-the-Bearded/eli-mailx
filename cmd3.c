/*	$OpenBSD: cmd3.c,v 1.5 1996/06/08 19:48:14 christos Exp $	*/
/*	$NetBSD: cmd3.c,v 1.5 1996/06/08 19:48:14 christos Exp $	*/

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
static char sccsid[] = "@(#)cmd3.c	8.1 (Berkeley) 6/6/93";
#else
static char rcsid[] = "$OpenBSD: cmd3.c,v 1.5 1996/06/08 19:48:14 christos Exp $";
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"

/*
 * Mail -- a mail program
 *
 * Still more user commands.
 */
static int diction __P((const void *, const void *));

/*
 * Process a shell escape by saving signals, ignoring signals,
 * and forking a sh -c
 */
int
shell(v)
	void *v;
{
	char *str = v;
	sig_t sigint = signal(SIGINT, SIG_IGN);
	char *shell;
	char cmd[BUFSIZ];

	(void) strncpy(cmd, str, BUFSIZ);
	cmd[BUFSIZ-1]='\0';
	if (bangexp(cmd, BUFSIZ) < 0)
		return 1;
	if ((shell = value("SHELL")) == NOSTR)
		shell = _PATH_CSHELL;
	(void) run_command(shell, 0, -1, -1, "-c", cmd, NOSTR);
	(void) signal(SIGINT, sigint);
	printf("!\n");
	return 0;
}

/*
 * Fork an interactive shell.
 */
/*ARGSUSED*/
int
dosh(v)
	void *v;
{
	sig_t sigint = signal(SIGINT, SIG_IGN);
	char *shell;

	if ((shell = value("SHELL")) == NOSTR)
		shell = _PATH_CSHELL;
	(void) run_command(shell, 0, -1, -1, NOSTR, NOSTR, NOSTR);
	(void) signal(SIGINT, sigint);
	putchar('\n');
	return 0;
}

/*
 * Expand the shell escape by expanding unescaped !'s into the
 * last issued command where possible.
 */

char	lastbang[128];

int
bangexp(str, size)
	char *str;
	int size;
{
	char bangbuf[BUFSIZ];
	register char *cp, *cp2;
	register int n;
	int changed = 0;

	cp = str;
	cp2 = bangbuf;
	n = BUFSIZ;
	while (*cp) {
		if (*cp == '!') {
			if (n < strlen(lastbang)) {
overf:
				printf("Command buffer overflow\n");
				return(-1);
			}
			changed++;
			strcpy(cp2, lastbang);
			cp2 += strlen(lastbang);
			n -= strlen(lastbang);
			cp++;
			continue;
		}
		if (*cp == '\\' && cp[1] == '!') {
			if (--n <= 1)
				goto overf;
			*cp2++ = '!';
			cp += 2;
			changed++;
		}
		if (--n <= 1)
			goto overf;
		*cp2++ = *cp++;
	}
	*cp2 = 0;
	if (changed) {
		printf("!%s\n", bangbuf);
		fflush(stdout);
	}
	strncpy(str, bangbuf, size);
	str[size-1]='\0';
	strncpy(lastbang, bangbuf, 128);
	lastbang[127] = 0;
	return(0);
}

/*
 * Figure out which file has the requested help.
 */
static char *
find_help(topic)
	const char *topic;
{
	char *helpfile, *n, *stopic;
	char hname[PATHSIZE];
	int usedhf = 0;
	struct stat sbuf;

	if ((helpfile = value("helpfile")) == NOSTR) {
		helpfile = savestr(_PATH_HELP);
	} else {
		helpfile = savestr(helpfile);
		usedhf = 1;
	}

	if (topic == NULL) 
		return helpfile;

	stopic = savestr(topic);
	/* avoid bad user input */
	for (n = stopic; *n != 0; n++) {
		if ((*n >= 'a') && (*n <= 'z')) { continue; }
		if ((*n >= 'A') && (*n <= 'Z')) { continue; }
		if ((*n >= '0') && (*n <= '9')) { continue; }
		if (*n == '!') { continue; }
		if (*n == '#') { continue; }
		if (*n == '=') { continue; }
		if (*n == '?') { continue; }
		if (*n == '-') { continue; }
		return NOSTR;
	}
	
	snprintf(hname, PATHSIZE, "%s.%s", helpfile, stopic);
	if (0 == stat(hname, &sbuf)) {
		helpfile = savestr(hname);
		return helpfile;
	}

	if(usedhf) {
		snprintf(hname, PATHSIZE, "%s.%s", _PATH_HELP, stopic);
		if (0 == stat(hname, &sbuf)) {
			helpfile = savestr(hname);
			return helpfile;
		}
	}

	return NOSTR;
}

/*
 * Print out a nice help message from some file or another.
 */

jmp_buf	pagerstop;
int
help(v)
	void *v;
{
	FILE *f, *pager;
	char *use_help, **arglist, *topic;
	char helpbuf[LINESIZE];
	int s = 0;

	arglist = v;
	topic = *arglist;

	if (topic != NOSTR) {
		const struct cmd *command = lex(topic);
		if(command != NONE) {
			/* What can we say about this based on cmdtab? */
			s = snprintf(helpbuf, LINESIZE, 
				"\"%s\" is a built-in command", command->c_name);

			if((command->c_argtype & 07) == 0)
				s += snprintf(&helpbuf[s], LINESIZE,
					" which takes a message-list.");
			if((command->c_argtype & 07) == 1)
				s += snprintf(&helpbuf[s], LINESIZE,
					" which takes a message-list plus a string.");
			if((command->c_argtype & 07) == 2)
				s += snprintf(&helpbuf[s], LINESIZE,
					" which takes %d to %d parameters.",
					command->c_minargs, command->c_maxargs);
			if((command->c_argtype & 07) == 3)
				s += snprintf(&helpbuf[s], LINESIZE,
					" which takes no parameters.");
			if((command->c_argtype & 07) == 4)
				s += snprintf(&helpbuf[s], LINESIZE,
					" which takes a message-list without defaults.");
			s += snprintf(&helpbuf[s], LINESIZE, "\n");

			if(command->c_argtype & I)
				s += snprintf(&helpbuf[s], LINESIZE,
					"It is only valid in interactive mode.\n");
			if((command->c_argtype & M) == 0)
				s += snprintf(&helpbuf[s], LINESIZE,
					"It cannot be used in mail-send mode.\n");

			use_help = find_help(command->c_name);
		} else {
			use_help = find_help(topic);
		}
		if (use_help == NOSTR) {
			if (s)
				puts(helpbuf);
			printf("No%s help on %s\n", (s ? " other" : ""), topic);
			return 1;
		}
	} else {
		use_help = find_help(NULL);
	}

	
	if ((f = Fopen(use_help, "r")) == NULL) {
		perror(use_help);
		return(1);
	}
	if (!setjmp(pagerstop)) {
		pager = Pageropen();
		if (s)
			fputs(helpbuf, pager);
		while (readline(f, helpbuf, LINESIZE) >= 0) {
			fputs(helpbuf, pager);
			/* readline() eats the new lines */
			putc('\n', pager);
		}
	}
	Pagerclose(pager);
	Fclose(f);
	return(0);
}

/*
 * Change user's working directory.
 */
int
schdir(v)
	void *v;
{
	char **arglist = v;
	char *cp;

	if (*arglist == NOSTR)
		cp = homedir;
	else
		if ((cp = expand(*arglist)) == NOSTR)
			return(1);
	if (chdir(cp) < 0) {
		perror(cp);
		return(1);
	}
	return 0;
}

int
respond(v)
	void *v;
{
	int *msgvec = v;
	if (value("Replyall") == NOSTR)
		return (_respond(msgvec));
	else
		return (_Respond(msgvec));
}

/*
 * Reply to a list of messages.  Extract each name from the
 * message header and send them off to mail1()
 */
int
_respond(msgvec)
	int *msgvec;
{
	struct message *mp;
	char *cp, *rcv, *replyto;
	char **ap;
	struct name *np;
	struct header head;

	if (msgvec[1] != 0) {
		printf("Sorry, can't reply to multiple messages at once\n");
		return(1);
	}
	mp = &message[msgvec[0] - 1];
	touch(mp);
	dot = mp;
	if ((rcv = skin(hfield("from", mp))) == NOSTR)
		rcv = skin(nameof(mp, 1));
	if ((replyto = skin(hfield("reply-to", mp))) != NOSTR)
		np = extract(replyto, GTO);
	else if ((cp = skin(hfield("to", mp))) != NOSTR)
		np = extract(cp, GTO);
	else
		np = NIL;
	np = elide(np);
	/*
	 * Delete my name from the reply list,
	 * and with it, all my alternate names.
	 */
	np = delname(np, myname);
	if (altnames)
		for (ap = altnames; *ap; ap++)
			np = delname(np, *ap);
	if (np != NIL && replyto == NOSTR)
		np = cat(np, extract(rcv, GTO));
	else if (np == NIL) {
		if (replyto != NOSTR)
			printf("Empty reply-to field -- replying to author\n");
		np = extract(rcv, GTO);
	}
	head.h_to = np;
	if ((head.h_subject = hfield("subject", mp)) == NOSTR)
		head.h_subject = hfield("subj", mp);
	head.h_subject = reedit(head.h_subject);
	if (replyto == NOSTR && (cp = skin(hfield("cc", mp))) != NOSTR) {
		np = elide(extract(cp, GCC));
		np = delname(np, myname);
		if (altnames != 0)
			for (ap = altnames; *ap; ap++)
				np = delname(np, *ap);
		head.h_cc = np;
	} else
		head.h_cc = NIL;
	head.h_bcc = NIL;
	head.h_smopts = NIL;
	mail1(&head, 1);
	return(0);
}

/*
 * Modify the subject we are replying to to begin with Re: if
 * it does not already.
 */
char *
reedit(subj)
	register char *subj;
{
	char *newsubj;

	if (subj == NOSTR)
		return NOSTR;
	if ((subj[0] == 'r' || subj[0] == 'R') &&
	    (subj[1] == 'e' || subj[1] == 'E') &&
	    subj[2] == ':')
		return subj;
	newsubj = salloc(strlen(subj) + 5);
	strcpy(newsubj, "Re: ");
	strcpy(newsubj + 4, subj);
	return newsubj;
}

/*
 * Preserve the named messages, so that they will be sent
 * back to the system mailbox.
 */
int
preserve(v)
	void *v;
{
	int *msgvec = v;
	register struct message *mp;
	register int *ip, mesg;

	if (edit) {
		printf("Cannot \"preserve\" in edit mode\n");
		return(1);
	}
	for (ip = msgvec; *ip != NOMVEC; ip++) {
		mesg = *ip;
		mp = &message[mesg-1];
		mp->m_flag |= MPRESERVE;
		mp->m_flag &= ~MBOX;
		dot = mp;
	}
	return(0);
}

/*
 * Mark all given messages as unread.
 */
int
unread(v)
	void *v;
{
	int	*msgvec = v;
	register int *ip;

	for (ip = msgvec; *ip != NOMVEC; ip++) {
		dot = &message[*ip-1];
		dot->m_flag &= ~(MREAD|MTOUCH);
		dot->m_flag |= MSTATUS;
	}
	return(0);
}

/*
 * Print the size of each message.
 */
int
messize(v)
	void *v;
{
	int *msgvec = v;
	register struct message *mp;
	register int *ip, mesg;

	for (ip = msgvec; *ip != NOMVEC; ip++) {
		mesg = *ip;
		mp = &message[mesg-1];
		printf("%d: %ld/%ld\n", mesg, mp->m_lines, mp->m_size);
	}
	return(0);
}

/*
 * Quit quickly.  If we are sourcing, just pop the input level
 * by returning an error.
 */
int
rexit(v)
	void *v;
{
	if (sourcing)
		return(1);
	exit(0);
	/*NOTREACHED*/
}

/*
 * Set or display a variable value.  Syntax is similar to that
 * of csh.
 */
int
set(v)
	void *v;
{
	char **arglist = v;
	register struct var *vp;
	register char *cp, *cp2;
	char varbuf[BUFSIZ], **ap, **p;
	char shellout[BUFSIZ], fmt[BUFSIZ];
	int bterr, space = 1;
	int errs, h, s;

	if (*arglist == NOSTR) {
		for (h = 0, s = 1; h < HSHSIZE; h++)
			for (vp = variables[h]; vp != NOVAR; vp = vp->v_link)
				s++;
		ap = (char **) salloc(s * sizeof *ap);
		for (h = 0, p = ap; h < HSHSIZE; h++)
			for (vp = variables[h]; vp != NOVAR; vp = vp->v_link) {
				int slen = strlen(vp->v_name);
				if (slen > space) space = slen;
				*p++ = vp->v_name;
			}
		*p = NOSTR;
		sort(ap);
		/* nicely formatted */
		sprintf(fmt, "%%%d-s = %%s\n", space);
		for (p = ap; *p != NOSTR; p++)
			printf(fmt, *p, value(*p));
		/* if highlighting was turned on, make sure it is off now */
		endhl(stdout);
		return(0);
	}
	errs = 0;
	for (ap = arglist; *ap != NOSTR; ap++) {
		cp = *ap;
		cp2 = varbuf;
		while (*cp != '=' && *cp != '\0')
			*cp2++ = *cp++;
		*cp2 = '\0';
		if (*cp == '\0')
			cp = "";
		else
			cp++;
		if (equal(varbuf, "")) {
			printf("Non-null variable name required\n");
			errs++;
			continue;
		}

		if(*cp == '`') {
			char *endp = ++cp;
			int done = 0;
			while(*endp != '\0') {
			  if(*endp == '`') {
			    *endp = '\0';
			    done = 1;
			    break;
			  }
			  endp++;
			}

			if(!done) {
				printf("Unmatched backtick\n");
				errs++;
				continue;
			} 

			shellout[0] = '\0';
			bterr = backtick(cp, shellout, BUFSIZ);

			if(bterr) {
				printf("Backtick error\n");
				errs++;
				continue;
			}
			assign(varbuf, shellout);
		} else {
			assign(varbuf, cp);
		}
	}
	return(errs);
}

/*
 * Unset a bunch of variable values.
 */
int
unset(v)
	void *v;
{
	char **arglist = v;
	register struct var *vp, *vp2;
	int errs, h;
	char **ap;

	errs = 0;
	for (ap = arglist; *ap != NOSTR; ap++) {
		if ((vp2 = lookup(*ap)) == NOVAR) {
			if (!sourcing) {
				printf("\"%s\": undefined variable\n", *ap);
				errs++;
			}
			continue;
		}
		h = hash(*ap);
		if (vp2 == variables[h]) {
			variables[h] = variables[h]->v_link;
			vfree(vp2->v_name);
			vfree(vp2->v_value);
			free((char *)vp2);
			continue;
		}
		for (vp = variables[h]; vp->v_link != vp2; vp = vp->v_link)
			;
		vp->v_link = vp2->v_link;
		vfree(vp2->v_name);
		vfree(vp2->v_value);
		free((char *) vp2);
	}
	return(errs);
}

/*
 * Put add users to a group.
 */
int
group(v)
	void *v;
{
	char **argv = v;
	register struct grouphead *gh;
	register struct group *gp;
	register int h;
	int s;
	char **ap, *gname, **p;

	if (*argv == NOSTR) {
		for (h = 0, s = 1; h < HSHSIZE; h++)
			for (gh = groups[h]; gh != NOGRP; gh = gh->g_link)
				s++;
		ap = (char **) salloc(s * sizeof *ap);
		for (h = 0, p = ap; h < HSHSIZE; h++)
			for (gh = groups[h]; gh != NOGRP; gh = gh->g_link)
				*p++ = gh->g_name;
		*p = NOSTR;
		sort(ap);
		for (p = ap; *p != NOSTR; p++)
			printgroup(*p);
		return(0);
	}
	if (argv[1] == NOSTR) {
		printgroup(*argv);
		return(0);
	}
	gname = *argv;
	h = hash(gname);
	if ((gh = findgroup(gname)) == NOGRP) {
		gh = (struct grouphead *) calloc(sizeof *gh, 1);
		gh->g_name = vcopy(gname);
		gh->g_list = NOGE;
		gh->g_link = groups[h];
		groups[h] = gh;
	}

	/*
	 * Insert names from the command list into the group.
	 * Who cares if there are duplicates?  They get tossed
	 * later anyway.
	 */

	for (ap = argv+1; *ap != NOSTR; ap++) {
		gp = (struct group *) calloc(sizeof *gp, 1);
		gp->ge_name = vcopy(*ap);
		gp->ge_link = gh->g_list;
		gh->g_list = gp;
	}
	return(0);
}

/*
 * Sort the passed string vecotor into ascending dictionary
 * order.
 */
void
sort(list)
	char **list;
{
	register char **ap;

	for (ap = list; *ap != NOSTR; ap++)
		;
	if (ap-list < 2)
		return;
	qsort(list, ap-list, sizeof(*list), diction);
}

/*
 * Do a dictionary order comparison of the arguments from
 * qsort.
 */
static int
diction(a, b)
	const void *a, *b;
{
	return(strcmp(*(char **)a, *(char **)b));
}

/*
 * The do nothing command for comments.
 */

/*ARGSUSED*/
int
null(v)
	void *v;
{
	return 0;
}

/*
 * Change to another file.  With no argument, print information about
 * the current file.
 */
int
file(v)
	void *v;
{
	char **argv = v;

	if (argv[0] == NOSTR) {
		newfileinfo(SHOW_PREVIOUS);
		return 0;
	}
	if ((!shown_eof) && (value("confirmquit") != NOSTR)) {
	  int n = 0;
	  char *cp;
	  char linebuf[LINESIZE];

	  /* Switch files is much more difficult to to accidentally
	   * than quitting, so reverse the default from quit().
	   * (Switching files is just as dangerous to state in the
	   * file, hence prompting at all.)
	   */
	  printf("Really switch to '%s'? [y] ", *argv);
	  fflush(stdout);
	  if (readline(input, &linebuf[n], LINESIZE) < 0) {
	    /* EOF */
            return 1;
          } else
	  if ((n = strlen(linebuf)) > 0) {
            for (cp = linebuf; isspace(*cp); cp++, n--)
	  	1;
	    if ((*cp == 'n') || (*cp == 'N')) return 0;
	  }
	}

	if (setfile(*argv) < 0)
		return 1;
	announce(HIDE_PREVIOUS);
	shown_eof = 0;
	return 0;
}

/*
 * Just like file, but uses a variable for the filename.
 */
int
File(v)
	void *v;
{
	char **argv = v;
	char *filename;

	if (argv[0] != NOSTR) {
		filename = value(argv[0]);
		if (filename == NOSTR) {
			printf("Could not expand %s\n", argv[0]);
			return 1;
		}
		return file(&filename);
	}
	return file(v);
}

/*
 * Show variables or expand file names like echo
 */
int
echo(v)
	void *v;
{
	char **argv = v;
	char **ap;
	char *cp;
	char *vp;

	for (ap = argv; *ap != NOSTR; ap++) {
		cp = *ap;
    		if ((vp = value(cp)) != NOSTR) {
			if (ap != argv)
				putchar(' ');
			printf("%s", vp);
		}
		else if ((cp = expand(cp)) != NOSTR) {
			if (ap != argv)
				putchar(' ');
			printf("%s", cp);
		}
	}
	putchar('\n');
	return 0;
}

int
Respond(v)
	void *v;
{
	int *msgvec = v;
	if (value("Replyall") == NOSTR)
		return (_Respond(msgvec));
	else
		return (_respond(msgvec));
}

/*
 * Reply to a series of messages by simply mailing to the senders
 * and not messing around with the To: and Cc: lists as in normal
 * reply.
 */
int
_Respond(msgvec)
	int msgvec[];
{
	struct header head;
	struct message *mp;
	register int *ap;
	register char *cp;

	head.h_to = NIL;
	for (ap = msgvec; *ap != 0; ap++) {
		mp = &message[*ap - 1];
		touch(mp);
		dot = mp;
		if ((cp = skin(hfield("from", mp))) == NOSTR)
			cp = skin(nameof(mp, 2));
		head.h_to = cat(head.h_to, extract(cp, GTO));
	}
	if (head.h_to == NIL)
		return 0;
	mp = &message[msgvec[0] - 1];
	if ((head.h_subject = hfield("subject", mp)) == NOSTR)
		head.h_subject = hfield("subj", mp);
	head.h_subject = reedit(head.h_subject);
	head.h_cc = NIL;
	head.h_bcc = NIL;
	head.h_smopts = NIL;
	mail1(&head, 1);
	return 0;
}

/*
 * Conditional commands.  These allow one to parameterize one's
 * .mailrc and do some things if sending, others if receiving.
 */
int
ifcmd(v)
	void *v;
{
	char **argv = v;
	register char *cp;

	if (cond != CANY) {
		printf("Illegal nested \"if\"\n");
		return(1);
	}
	cond = CANY;
	cp = argv[0];
	switch (*cp) {

	/* if etbmail
	 *	stuff nail will ignore
	 * endif
         */
	case 'e': case 'E':
		cond = CETBMAIL;
		break;

	/* if heirloom
	 *	stuff we will ignore
	 *	but my "heirloom" mailx (nail 12+) will process
	 * endif
         */
	case 'h': case 'H':
		cond = CHEIRLOOM;
		break;

	/* if reading
	 *	read mode options
	 * else
	 *      ...
	 * endif
         */
	case 'r': case 'R':
		cond = CRCV;
		break;

	/* if sending
	 *	send mode options
	 * else
	 *      ...
	 * endif
         */
	case 's': case 'S':
		cond = CSEND;
		break;

	default:
		printf("Unrecognized if-keyword: \"%s\"\n", cp);
		return(1);
	}
	return(0);
}

/*
 * Implement 'else'.  This is pretty simple -- we just
 * flip over the conditional flag.
 */
int
elsecmd(v)
	void *v;
{

	switch (cond) {
	case CANY:
		/* no 'if' at all */
		printf("\"Else\" outside of suitable \"if\"\n");
		return(1);

	case CETBMAIL:
		cond = CHEIRLOOM;
		break;

	case CHEIRLOOM:
		cond = CETBMAIL;
		break;

	case CSEND:
		cond = CRCV;
		break;

	case CRCV:
		cond = CSEND;
		break;

	default:
		printf("Mail's idea of conditions is screwed up\n");
		cond = CANY;
		break;
	}
	return(0);
}

/*
 * End of if statement.  Just set cond back to anything.
 */
int
endifcmd(v)
	void *v;
{

	/* used to error out on cond == CANY, but accept that now. */
	cond = CANY;
	return(0);
}

/*
 * Set the list of alternate names.
 */
int
alternates(v)
	void *v;
{
	char **namelist = v;
	register int c;
	register char **ap, **ap2, *cp;

	c = argcount(namelist) + 1;
	if (c == 1) {
		if (altnames == 0)
			return(0);
		for (ap = altnames; *ap; ap++)
			printf("%s ", *ap);
		printf("\n");
		return(0);
	}
	if (altnames != 0)
		free((char *) altnames);
	altnames = (char **) calloc((unsigned) c, sizeof (char *));
	for (ap = namelist, ap2 = altnames; *ap; ap++, ap2++) {
		cp = (char *) calloc((unsigned) strlen(*ap) + 1, sizeof (char));
		strcpy(cp, *ap);
		*ap2 = cp;
	}
	*ap2 = 0;
	return(0);
}
