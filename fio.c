/*	$OpenBSD: fio.c,v 1.5 1996/06/08 19:48:22 christos Exp $	*/
/*	$NetBSD: fio.c,v 1.5 1996/06/08 19:48:22 christos Exp $	*/

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
static char sccsid[] = "@(#)fio.c	8.1 (Berkeley) 6/6/93";
#else
static char rcsid[] = "$OpenBSD: fio.c,v 1.5 1996/06/08 19:48:22 christos Exp $";
#endif
#endif /* not lint */

#include "rcv.h"
#include <sys/file.h>
#include <sys/wait.h>

#include <unistd.h>
#include <paths.h>
#include <errno.h>
#include "extern.h"

/*
 * Mail -- a mail program
 *
 * File I/O.
 */

/*
 * Set up the input pointers while copying the mail file into /tmp.
 * mestmp file contains all the message pointers until we reach end of
 * mbox, then makemessage() allocates memory for the whole array at once.
 */
void
setptr(ibuf)
	register FILE *ibuf;
{
	extern char *tmpdir;
	register int c, count;
	register char *cp, *cp2;
	struct message this;
	FILE *mestmp;
	off_t offset;
	int maybe, inhead, alllines;
	char linebuf[LINESIZE];

	/* Get temporary file. */
	(void)snprintf(linebuf,LINESIZE,"%s/mail.XXXXXX", tmpdir);
	if ((c = mkstemp(linebuf)) == -1 ||
	    (mestmp = Fdopen(c, "r+")) == NULL) {
		(void)fprintf(stderr, "mail: can't open %s\n", linebuf);
		exit(1);
	}
	(void)unlink(linebuf);

	msgCount = 0;
	delCount = 0;
	maybe = 1;
	inhead = 0;
	offset = 0;
	alllines = 0;
	this.m_flag = MUSED|MNEW;
	this.m_size = 0;
	this.m_lines = 0;
	this.m_block = 0;
	this.m_offset = 0;
	this.m_loffset = 0;
	for (;;) {
		if (fgets(linebuf, LINESIZE, ibuf) == NULL) {
			if (append(&this, mestmp)) {
				perror("temporary file");
				exit(1);
			}
			makemessage(mestmp);
			return;
		}
		count = strlen(linebuf);
		(void) fwrite(linebuf, sizeof *linebuf, count, otf);
		if (ferror(otf)) {
			perror("/tmp");
			exit(1);
		}
		linebuf[count - 1] = 0;
		if (maybe && linebuf[0] == 'F' && ishead(linebuf)) {
			msgCount++;
			if (append(&this, mestmp)) {
				perror("temporary file");
				exit(1);
			}
			this.m_flag = MUSED|MNEW;
			this.m_size = 0;
			this.m_lines = 0;
			/* block number in file, and how far into that block */
			this.m_block = blockof(offset);
			this.m_offset = offsetof(offset);
			this.m_loffset = alllines;
			inhead = 1;
		} else if (linebuf[0] == 0) {
			inhead = 0;
		} else if (inhead) {
			for (cp = linebuf, cp2 = "status";; cp++) {
				if ((c = *cp2++) == 0) {
					while (isspace(*cp++))
						;
					if (cp[-1] != ':')
						break;
					while ((c = *cp++) != '\0') {
						/* Unknown characters on Status: 
						 * get ignored.
						 */
						if (c == 'R')
							this.m_flag |= MREAD;
						else if (c == 'O')
							this.m_flag &= ~MNEW;
						else if (c == '+')
							this.m_flag |= MFLAG;
					}
					inhead = 0;
					break;
				}
				if (*cp != c && *cp != toupper(c))
					break;
			}
		}
		offset += count;
		this.m_size += count;
		this.m_lines++;
		alllines++;
		maybe = linebuf[0] == 0;
	}
}

/*
 * Drop the passed line onto the passed output buffer.
 * If a write error occurs, return -1, else the count of
 * characters written, including the newline.
 */
int
putline(obuf, linebuf)
	FILE *obuf;
	char *linebuf;
{
	register int c;

	c = strlen(linebuf);
	(void) fwrite(linebuf, sizeof *linebuf, c, obuf);
	(void) putc('\n', obuf);
	if (ferror(obuf))
		return (-1);
	return (c + 1);
}

/*
 * Read up a line from the specified input into the line
 * buffer.  Return the number of characters read.  Do not
 * include the newline at the end.
 */
int
readline(ibuf, linebuf, linesize)
	FILE *ibuf;
	char *linebuf;
	int linesize;
{
	register int n,oldfl;
        char *res;
	clearerr(ibuf);
#ifdef IOSAFE
        /* we want to be able to get interrupts while waiting user-input
           we cannot to safely inside a stdio call, so we first ensure there  
           is now data in the stdio buffer by doing the stdio call with the descriptor 
           in non-blocking state and then do a select. 
           Hope it is safe (the libc should not break on a EAGAIN) 
           lprylli@graville.fdn.fr*/ 
        n = 0; /* number of caracters already read */
        while (n < linesize - 1) {
          errno = 0;
          oldfl = fcntl(fileno(ibuf),F_GETFL);
          fcntl(fileno(ibuf),F_SETFL,oldfl | O_NONBLOCK);
          res = fgets(linebuf + n, linesize-n, ibuf);
          fcntl(fileno(ibuf),F_SETFL,oldfl);
          if (res != NULL) {
            n = strlen(linebuf);
            if (n > 0 && linebuf[n-1] == '\n')
              break;
          } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
	    clearerr(ibuf);
	  } else {
            /* probably EOF one the file descriptors */
            if (n > 0)
              break;
            else
              return -1;
            
          }{
            extern int got_interrupt;
            fd_set rds;
            FD_ZERO(&rds);
            FD_SET(fileno(ibuf),&rds);
            select(fileno(ibuf)+1,&rds,NULL,NULL,NULL);
            /* if an interrupt occur drops the current line and returns */
            if (got_interrupt)
              return -1;
          }
        }
#else
        if (fgets(linebuf, linesize, ibuf) == NULL)
          return -1;
#endif
	n = strlen(linebuf);
	if (n > 0 && linebuf[n - 1] == '\n')
          linebuf[--n] = '\0';
	return n;
}

/*
 * Return a file buffer all ready to read up the
 * passed message pointer.
 */
FILE *
setinput(mp)
	register struct message *mp;
{

	fflush(otf);
	if (fseek(itf, (long)positionof(mp->m_block, mp->m_offset), 0) < 0) {
		perror("fseek");
		panic("temporary file seek");
	}
	return (itf);
}

/*
 * Take the data out of the passed ghost file and toss it into
 * a dynamically allocated message structure.
 */
void
makemessage(f)
	FILE *f;
{
	register size_t size = (msgCount + 1) * sizeof (struct message);

	if (message != 0)
		free((char *) message);
	if ((message = (struct message *) malloc((unsigned) size)) == 0)
		panic("Insufficient memory for %d messages", msgCount);
	dot = message;
	size -= sizeof (struct message);
	fflush(f);
	(void) lseek(fileno(f), (off_t)sizeof *message, 0);
	if (read(fileno(f), (char *) message, size) != size)
		panic("Message temporary file corrupted");
	message[msgCount].m_size = 0;
	message[msgCount].m_lines = 0;
	Fclose(f);
}

/*
 * Append the passed message descriptor onto the temp file.
 * If the write fails, return 1, else 0
 */
int
append(mp, f)
	struct message *mp;
	FILE *f;
{
	return fwrite((char *) mp, sizeof *mp, 1, f) != 1;
}

/*
 * Delete a file, but only if the file is a plain file.
 */
int
rm(name)
	char *name;
{
	struct stat sb;

	if (stat(name, &sb) < 0)
		return(-1);
	if (!S_ISREG(sb.st_mode)) {
		errno = EISDIR;
		return(-1);
	}
	return(unlink(name));
}

static int sigdepth;		/* depth of holdsigs() */
static sigset_t nset, oset;
/*
 * Hold signals SIGHUP, SIGINT, and SIGQUIT.
 */
void
holdsigs()
{

	if (sigdepth++ == 0) {
		sigemptyset(&nset);
		sigaddset(&nset, SIGHUP);
		sigaddset(&nset, SIGINT);
		sigaddset(&nset, SIGQUIT);
		sigprocmask(SIG_BLOCK, &nset, &oset);
	}
}

/*
 * Release signals SIGHUP, SIGINT, and SIGQUIT.
 */
void
relsesigs()
{

	if (--sigdepth == 0)
		sigprocmask(SIG_SETMASK, &oset, NULL);
}

/*
 * Determine the size of the file possessed by
 * the passed buffer.
 */
off_t
fsize(iob)
	FILE *iob;
{
	struct stat sbuf;

	if (fstat(fileno(iob), &sbuf) < 0)
		return 0;
	return sbuf.st_size;
}

/*
 * Evaluate the string given as a new mailbox name.
 * Supported meta characters:
 *	%	for my system mail box
 *	%user	for user's system mail box
 *	#	for previous file
 *	&	invoker's mbox file
 *	+file	file in folder directory
 *	any shell meta character
 * Return the file name as a dynamic string.
 */
char *
expand(name)
	register char *name;
{
	char xname[PATHSIZE];
	char cmdbuf[PATHSIZE];		/* also used for file names */
	register int pid, l;
	register char *cp, *shell;
	int pivec[2];
	struct stat sbuf;
	extern union wait wait_status;

	/*
	 * The order of evaluation is "%" and "#" expand into constants.
	 * "&" can expand into "+".  "+" can expand into shell meta characters.
	 * Shell meta characters expand into constants.
	 * This way, we make no recursive expansion.
	 */
	switch (*name) {
	case '%':
		findmail(name[1] ? name + 1 : myname, xname, PATHSIZE);
		return savestr(xname);
	case '#':
		if (name[1] != 0)
			break;
		if (prevfile[0] == 0) {
			printf("No previous file\n");
			return NOSTR;
		}
		return savestr(prevfile);
	case '&':
		if (name[1] == 0 && (name = value("MBOX")) == NOSTR)
			name = "~/mbox";
		/* fall through */
	}
	if (name[0] == '+' && getfold(cmdbuf, PATHSIZE) >= 0) {
		snprintf(xname, PATHSIZE, "%s/%s", cmdbuf, name + 1);
		name = savestr(xname);
	}
	/* catch the most common shell meta character */
	/* homedir is set to $HOME or . in tinit() */
	if (name[0] == '~' && (name[1] == '/' || name[1] == '\0')) {
		snprintf(xname, PATHSIZE, "%s%s", homedir, name + 1);
		name = savestr(xname);
	}
	if (!anyof(name, "~{[*?$`'\"\\"))
		return name;
	if (pipe(pivec) < 0) {
		perror("pipe");
		return name;
	}
	/* expand any shell globs left (~/ handled already above) */
	snprintf(cmdbuf, PATHSIZE, "echo %s", name);
	if ((shell = value("SHELL")) == NOSTR)
		shell = _PATH_CSHELL;
	pid = start_command(shell, 0, -1, pivec[1], "-c", cmdbuf, NOSTR);
	if (pid < 0) {
		close(pivec[0]);
		close(pivec[1]);
		return NOSTR;
	}
	close(pivec[1]);
	l = read(pivec[0], xname, PATHSIZE);
	if (l < 0) {
		perror("read");
		close(pivec[0]);
		return NOSTR;
	}
	close(pivec[0]);
	if (wait_child(pid) < 0 && wait_status.w_termsig != SIGPIPE) {
		fprintf(stderr, "\"%s\": Expansion failed.\n", name);
		return NOSTR;
	}
	if (l == 0) {
		fprintf(stderr, "\"%s\": No match.\n", name);
		return NOSTR;
	}
	if (l == PATHSIZE) {
		fprintf(stderr, "\"%s\": Expansion buffer overflow.\n", name);
		return NOSTR;
	}
	xname[l] = 0;
	for (cp = &xname[l-1]; *cp == '\n' && cp > xname; cp--)
		;
	cp[1] = '\0';
	if (index(xname, ' ') && stat(xname, &sbuf) < 0) {
		fprintf(stderr, "\"%s\": Ambiguous.\n", name);
		return NOSTR;
	}
	return savestr(xname);
}

/*
 * Determine the current folder directory name.
 */
int
getfold(name, size)
	char *name;
	int size;
{
	char *folder;

	if ((folder = value("folder")) == NOSTR)
		return (-1);
	if (*folder == '/') {
		strncpy(name, folder, size);
		name[size-1]='\0';
	} else {
		snprintf(name, size, "%s/%s", homedir, folder);
	}
	return (0);
}

/*
 * Return the name of the dead.letter file.
 */
char *
getdeadletter()
{
	register char *cp;

	if ((cp = value("DEAD")) == NOSTR || (cp = expand(cp)) == NOSTR)
		cp = expand("~/dead.letter");
	else if (*cp != '/') {
		char buf[PATHSIZE];

		(void) snprintf(buf, PATHSIZE, "~/%s", cp);
		cp = expand(buf);
	}
	return cp;
}
