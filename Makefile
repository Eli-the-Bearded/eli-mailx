#	$OpenBSD: Makefile,v 1.8 1996/06/08 19:48:09 christos Exp $
#	$NetBSD: Makefile,v 1.8 1996/06/08 19:48:09 christos Exp $

PROG=	mail
CC=gcc

CPPFLAGS=-D_BSD_SOURCE 

# static build
#CFLAGS=-g -static
# profiling build
#CLFLAGS=-pg
CFLAGS=-g 

SRCS=	version.c aux.c cmd1.c cmd2.c cmd3.c cmdtab.c collect.c no_dot_lock.c \
	edit.c fio.c getname.c head.c v7.local.c lex.c list.c main.c names.c \
	popen.c quit.c send.c strings.c temp.c tty.c vars.c mime.c utf-8.c

OBJS=$(SRCS:%.c=%.o)
LIBS=

CHECKPROGS = checkutf8 check8859 checkascii
CHECKPROGS_SRCS = checkutf8.c utf-8.c
CHECKPROGS_OBJS=$(CHECKPROGS_SRCS:%.c=%.o)

TESTPROG = mimetest utf8test

ALL_PROG = $(PROG) $(CHECKPROGS) $(TESTPROG)

SFILES=	mail.help mail.tildehelp
EFILES=	mail.rc
LINKS=	${BINDIR}/mail ${BINDIR}/Mail ${BINDIR}/mail ${BINDIR}/mailx
MFILES=	mail.1

default: all

 all: $(PROG) $(CHECKPROGS)

 # limited test suite
 test: $(TESTPROG)
	@echo Ran tests for: $>
 
 $(PROG): $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) $(LIBS)
 
 checkutf8: $(CHECKPROGS_OBJS)
	rm -f $(CHECKPROGS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(CHECKPROGS_OBJS) $(LIBS)

 check8859: checkutf8
	ln $> $@

 checkascii: checkutf8
	ln $> $@

 .c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<
 
 .y.c:
	bison $<
	mv -f $*.tab.c $@
 
 clean:
	rm -f $(ALL_PROG) *.o *~
 
 install:
	install -c -m 2755 -o root -g mail -s $(PROG) $(DESTDIR)/usr/bin/
	install -c -m 755 -o root -g root -s $(CHECKPROGS) $(DESTDIR)/usr/bin/ 
	install -c -m 644 $(MFILES) $(DESTDIR)/usr/man/man1/
	cd misc && install -c -m 644 $(EFILES) $(DESTDIR)/etc/
	cd misc && install -c -m 644 $(SFILES) $(DESTDIR)/usr/lib/

# these two items have a built in test suite
mimetest: mime.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -D_RUN_TESTS -o $@ mime.c
	./$@ && echo $@ PASSED ALL TESTS

utf8test: utf-8.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -D_RUN_TESTS -o $@ utf-8.c
	./$@ && echo $@ PASSED ALL TESTS


aux.o: def.h extern.h glob.h rcv.h pathnames.h
cmd1.o: def.h extern.h glob.h rcv.h pathnames.h
cmd2.o: def.h extern.h glob.h rcv.h pathnames.h
cmd3.o: def.h extern.h glob.h rcv.h pathnames.h
cmdtab.o: def.h extern.h pathnames.h
collect.o: def.h extern.h glob.h rcv.h pathnames.h
dotlock.o: def.h extern.h glob.h rcv.h pathnames.h
edit.o: def.h extern.h glob.h rcv.h pathnames.h
fio.o: def.h extern.h glob.h rcv.h pathnames.h
getname.o: def.h extern.h glob.h rcv.h pathnames.h
head.o: def.h extern.h glob.h rcv.h pathnames.h
lex.o: def.h extern.h glob.h rcv.h pathnames.h
list.o: def.h extern.h glob.h rcv.h pathnames.h
main.o: def.h extern.h glob.h rcv.h pathnames.h
names.o: def.h extern.h glob.h rcv.h pathnames.h
# no_dot_lock.o has no dependencies on non-system includes
popen.o: def.h extern.h glob.h rcv.h pathnames.h
quit.o: def.h extern.h glob.h rcv.h pathnames.h
send.o: def.h extern.h glob.h rcv.h pathnames.h
strings.o: def.h extern.h glob.h rcv.h pathnames.h
temp.o: def.h extern.h glob.h rcv.h pathnames.h
tty.o: def.h extern.h glob.h rcv.h pathnames.h
v7.local.o: def.h extern.h glob.h rcv.h pathnames.h
vars.o: def.h extern.h glob.h rcv.h pathnames.h
# version.o has no dependencies on non-system includes

# checkutf8.o has no dependencies on non-system includes
# mime.o has no dependencies on non-system includes
# utf-8.o has no dependencies on non-system includes

