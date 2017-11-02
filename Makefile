#	$OpenBSD: Makefile,v 1.8 1996/06/08 19:48:09 christos Exp $
#	$NetBSD: Makefile,v 1.8 1996/06/08 19:48:09 christos Exp $

PROG=	mail
CC=gcc

# use second line starting from hamm release
#CPPFLAGS=-I/usr/include/bsd -D_BSD_SOURCE -DIOSAFE
CPPFLAGS=-D_BSD_SOURCE 

# static build
#CFLAGS=-g -static
# profiling build
#CLFLAGS=-pg
CFLAGS=-g 

SRCS=	version.c aux.c cmd1.c cmd2.c cmd3.c cmdtab.c collect.c no_dot_lock.c \
	edit.c fio.c getname.c head.c v7.local.c lex.c list.c main.c names.c \
	popen.c quit.c send.c strings.c temp.c tty.c vars.c

OBJS=$(SRCS:%.c=%.o)
LIBS=

SFILES=	mail.help mail.tildehelp
EFILES=	mail.rc
LINKS=	${BINDIR}/mail ${BINDIR}/Mail ${BINDIR}/mail ${BINDIR}/mailx
MFILES=	mail.1

default: all

 all: $(PROG)
 
 $(PROG): $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) $(LIBS)
 
 .c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<
 
 .y.c:
	bison $<
	mv -f $*.tab.c $@
 
 clean:
	rm -f $(PROG) *.o *~
 
 install:
	install -c -m 2755 -o root -g mail -s $(PROG) $(DESTDIR)/usr/bin/
	install -c -m 644 $(MFILES) $(DESTDIR)/usr/man/man1/
	cd misc && install -c -m 644 $(EFILES) $(DESTDIR)/etc/
	cd misc && install -c -m 644 $(SFILES) $(DESTDIR)/usr/lib/
