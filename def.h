/*	$OpenBSD: def.h,v 1.8 1996/06/08 19:48:18 christos Exp $	*/
/*	$NetBSD: def.h,v 1.8 1996/06/08 19:48:18 christos Exp $	*/
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
 *
 *	@(#)def.h	8.2 (Berkeley) 3/21/94
 *	$NetBSD: def.h,v 1.8 1996/06/08 19:48:18 christos Exp $
 */

/*
 * Mail -- a mail program
 *
 * Author: Kurt Shoens (UCB) March 25, 1978
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "pathnames.h"

#define	APPEND				/* New mail goes to end of mailbox */

#define	ESCAPE		'~'		/* Default escape for sending */
#define	NMLSIZE		1024		/* max names in a message list */
#define	PATHSIZE	MAXPATHLEN	/* Size of pathnames throughout */
#define	HSHSIZE		59		/* Hash size for aliases and vars */
#define	LINESIZE	BUFSIZ		/* max readable line width */
#define	STRINGSIZE	((unsigned) 128)/* Dynamic allocation units */
#define	MAXARGC		1024		/* Maximum list of raw strings */
#define	NOSTR		((char *) 0)	/* Null string pointer */
#define	MAXEXP		25		/* Maximum expansion of aliases */

#define	equal(a, b)	(strncmp(a,b,BUFSIZ)==0)/* A nice function to string compare */

struct message {
	short	m_flag;			/* flags, see below */
	/* block, offset and lines should not be shorts (as they were
	/* in the past) since those are too easy to overflow with large
	/* mail messages.					Elijah
        /* block is on-disk block, used for optimizing reads to nice
         * boundaries.
	 */
	long	m_block;		/* block number of this message */
	long	m_offset;		/* offset in block of message */
	long	m_size;			/* Bytes in the message */
	long	m_lines;		/* Lines in the message */
};

/*
 * flag bits: type short, so 16 bits 1 to 32768
 */

#define MUSED             1      /* entry is used, but this bit isn't */
#define MDELETED          2      /* entry has been deleted */
#define MSAVED            4      /* entry has been saved */
#define MTOUCH            8      /* entry has been noticed */
#define MPRESERVE        16      /* keep entry in sys mailbox */
#define MMARK            32      /* message is marked! */
#define MODIFY           64      /* message has been modified */
#define MNEW            128      /* message has never been seen */
#define MREAD           256      /* message has been read sometime. */
#define MSTATUS         512      /* message status has changed */
#define MBOX           1024      /* Send this to mbox, regardless */
#define MLASTMARK      2048      /* Message mark history */
#define MREMEMBER      4096      /* unsaved user flag */
#define MFLAG          8192      /* saved user flag */
/*unused Mname        16384 */
/*unused Mname        32768 */
#define MBOTHUSER	(MREMEMBER|MFLAG)

/* operation for flag() */
#define FLAGUNSET         0
#define FLAGSET           1

/*
 * Given a file address, determine the block number it represents.
 * 8k is probably a good size in 2017, better than the old 4k.
 */
#define BLOCK_SIZE			8192
#define blockof(off)			((int) ((off) / (BLOCK_SIZE)))
#define offsetof(off)			((int) ((off) % (BLOCK_SIZE)))
#define positionof(block, offset)	((off_t)(block) * (BLOCK_SIZE) + (offset))

/*
 * Format of the command description table.
 * The actual table is declared and initialized
 * in lex.c
 */
struct cmd {
	char	*c_name;		/* Name of command */
	int	(*c_func) __P((void *));/* Implementor of the command */
	short	c_argtype;		/* Type of arglist (see below) */
	short	c_msgflag;		/* Required flags of messages */
	short	c_msgmask;		/* Relevant flags of messages */
};

/* Yechh, can't initialize unions */

#define	c_minargs c_msgflag		/* Minimum argcount for RAWLIST */
#define	c_maxargs c_msgmask		/* Max argcount for RAWLIST */

/*
 * Argument types.
 */

#define	MSGLIST	 0		/* Message list type */
#define	STRLIST	 1		/* A pure string */
#define	RAWLIST	 2		/* Shell string list */
#define	NOLIST	 3		/* Just plain 0 */
#define	NDMLIST	 4		/* Message list, no defaults */

#define	P	040		/* Autoprint dot after command */
#define	I	0100		/* Interactive command bit */
#define	M	0200		/* Legal from send mode bit */
#define	W	0400		/* Illegal when read only bit */
#define	F	01000		/* Is a conditional command */
#define	T	02000		/* Is a transparent command */
#define	R	04000		/* Cannot be called from collect */

/*
 * Oft-used mask values
 */

#define	MMNORM		(MDELETED|MSAVED)/* Look at both save and delete bits */
#define	MMNDEL		MDELETED	/* Look only at deleted bit */

/*
 * Structure used to return a break down of a head
 * line (hats off to Bill Joy!)
 */

struct headline {
	char	*l_from;	/* The name of the sender */
	char	*l_tty;		/* The tty string (if any) */
	char	*l_date;	/* The (possibly shortened) date string */
};

#define	GTO	1		/* Grab To: line */
#define	GSUBJECT 2		/* Likewise, Subject: line */
#define	GCC	4		/* And the Cc: line */
#define	GBCC	8		/* And also the Bcc: line */
#define	GMASK	(GTO|GSUBJECT|GCC|GBCC)
				/* Mask of places from whence */

#define	GNL	16		/* Print blank line after */
#define	GDEL	32		/* Entity removed from list */
#define	GCOMMA	64		/* detract puts in commas */

/*
 * Structure used to pass about the current
 * state of the user-typed message header.
 */

struct header {
	struct name *h_to;		/* Dynamic "To:" string */
	char *h_subject;		/* Subject string */
	struct name *h_cc;		/* Carbon copies string */
	struct name *h_bcc;		/* Blind carbon copies */
	struct name *h_smopts;		/* Sendmail options */
};

/*
 * Structure of namelist nodes used in processing
 * the recipients of mail and aliases and all that
 * kind of stuff.
 */

struct name {
	struct	name *n_flink;		/* Forward link in list. */
	struct	name *n_blink;		/* Backward list link */
	short	n_type;			/* From which list it came */
	char	*n_name;		/* This fella's name */
};

/*
 * Structure of a variable node.  All variables are
 * kept on a singly-linked list of these, rooted by
 * "variables"
 */

struct var {
	struct	var *v_link;		/* Forward link to next variable */
	char	*v_name;		/* The variable's name */
	char	*v_value;		/* And it's current value */
};

struct group {
	struct	group *ge_link;		/* Next person in this group */
	char	*ge_name;		/* This person's user name */
};

struct grouphead {
	struct	grouphead *g_link;	/* Next grouphead in list */
	char	*g_name;		/* Name of this group */
	struct	group *g_list;		/* Users in group. */
};

#define	NIL	((struct name *) 0)	/* The nil pointer for namelists */
#define	NONE	((struct cmd *) 0)	/* The nil pointer to command tab */
#define	NOVAR	((struct var *) 0)	/* The nil pointer to variables */
#define	NOGRP	((struct grouphead *) 0)/* The nil grouphead pointer */
#define	NOGE	((struct group *) 0)	/* The nil group pointer */
#define NOMVEC  ((int) 0)		/* Empty msgvec entry */

/*
 * Structure of the hash table of ignored header fields
 * (also used for interesting / highlighted fields.)
 */
struct ignoretab {
	int i_count;			/* Number of entries */
	struct ignore {
		struct ignore *i_link;	/* Next ignored field in bucket */
		char *i_field;		/* This ignored field */
	} *i_head[HSHSIZE];
};

/*
 * Token values returned by the scanner used for argument lists.
 * Also, sizes of scanner-related things.
 */

#define	TEOL	0			/* End of the command line */
#define	TNUMBER	1			/* A message number */
#define	TDASH	2			/* A simple dash */
#define	TSTRING	3			/* A string (possibly containing -) */
#define	TDOT	4			/* A "." */
#define	TUP	5			/* An "^" */
#define	TDOLLAR	6			/* A "$" */
#define	TSTAR	7			/* A "*" */
#define	TOPEN	8			/* An '(' */
#define	TCLOSE	9			/* A ')' */
#define TPLUS	10			/* A '+' */
#define TERROR	11			/* A lexical error */
#define TBANG	12			/* A '!' */
#define TOVER	13			/* A '>' */
#define TUNDER	14			/* A '<' */
#define TEQUAL	15			/* A '=' */

#define	REGDEP	2			/* Maximum regret depth. */
#define	STRINGLEN	1024		/* Maximum length of string token */

/*
 * Constants for conditional commands.  These describe whether
 * we should be executing stuff or not.
 */

#define	CANY		0		/* Execute in send or receive mode */
#define	CRCV		1		/* Execute in receive mode only */
#define	CSEND		2		/* Execute in send mode only */

/*
 * Kludges to handle the change from setexit / reset to setjmp / longjmp
 */

#define	setexit()	setjmp(srbuf)
#define	reset(x)	longjmp(srbuf, x)

/*
 * Truncate a file to the last character written. This is
 * useful just before closing an old file that was opened
 * for read/write.
 */
#define trunc(stream) {							\
	(void)fflush(stream); 						\
	(void)ftruncate(fileno(stream), (off_t)ftell(stream));		\
}

/* ctime is broken on SuSE 10. Use ctime_r. */
#define DONT_TRUST_CTIME

/* what to print in place of a missing Subject: header value */
#define NO_SUBJECT_PLACEHOLDER "(no subject)"

/* Used for deciding the display strategy when showing message list.
 * ... very narrow ->59 |60 <- narrow -> 69|70 <- normal ->89|90<- wide ...
 *
 * As of 2017 I find myself most often using very narrow (phone) and wide.
 * And having a screen too narrow to fit any subject was triggering the
 * no subject placeholder, which then made lines too long again.
 */ 
#define VERY_NARROW_SCREEN		(60)
#define NARROW_SCREEN			(70)
/* normal */
#define WIDE_SCREEN			(90)

/* Originally in list.c, but I wanted M_ALL in cmdtab.c, too. */
/*
 * Bit values for colon modifiers.
 */

#define	CMNEW		 01		/* New messages */
#define	CMOLD		 02		/* Old messages */
#define	CMUNREAD	 04		/* Unread messages */
#define	CMDELETED	010		/* Deleted messages */
#define	CMREAD		020		/* Read messages */
#define CMREMEMBER     0100		/* user session-only flagged messages */
#define CMFLAG         0200		/* user saved flaged messages */
#define CMNOUSER       0400		/* neither user saved flagged messages */

#define CMBOTHUSER     0300		/* either user flagged messages */
#define M_ALL          0337		/* All possible messages */

/* size check values */
#define SC_LINES	  1		/* check m_lines */
#define SC_CLINES	  2		/* check of cent (100) m_lines */
#define SC_BYTES	  3		/* check of m_size */
#define SC_KBYTES	  4		/* check of kilo (1024) m_size */
#define SC_MBYTES	  5		/* check of mega (1024*1024) m_size */

#define SC_LINES_CHAR	  'l'		/* check m_lines */
#define SC_LINES_ALT	  'L'
#define SC_CLINES_CHAR	  'c'		/* chcck of cent (100) m_lines */
#define SC_CLINES_ALT	  'C'
#define SC_BYTES_CHAR	  'b'		/* check of m_size */
#define SC_BYTES_ALT 	  'B'		
#define SC_KBYTES_CHAR	  'k'		/* check of kilo (1024) m_size */
#define SC_KBYTES_ALT     'K'
#define SC_MBYTES_CHAR	  'm'		/* check of mega (1024*1024) m_size */
#define SC_MBYTES_ALT     'M'

#define TO_CENT(x) (((x) + 99) / 100)
#define TO_KILO(x) (((x) + 1023) / 1024)
#define TO_MEGA(x) (((x) + 1048575) / 1048576)

/* for flags on the Status: header */
#define STATUS_SIZE	8

/* longest entry in coltab.co_name is currently 22 */
#define CO_NAMESIZE	24

/* column tab entries that are not used as disposition characters */
#define COLT_NEW	'n'
#define COLT_OLD	'o'
#define COLT_DEL	'd'
#define COLT_NONE	'U'	/* no mark, no flag */

/* disposition characters used in from summaries and some in column tab */
#define DISP_NEW	' '
#define DISP_READ	'r'
#define DISP_SAVE	'*'
#define DISP_PRES	'P'
#define DISP_UNRD	'u'
#define DISP_MBOX	'M'
#define DISP_FLAG	'f'
#define DISP_REMB	'm'	/* aka mark */
#define DISP_BOTH	'B'	/* mark and flag */

