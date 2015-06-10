#	$OpenBSD: Makefile,v 1.1 1994/06/29 05:09:54 deraadt Exp $

DIR=	usd/07.mail
SRCS=	mail0.nr mail1.nr mail2.nr mail3.nr mail4.nr mail5.nr mail6.nr \
	mail7.nr mail8.nr mail9.nr maila.nr
MACROS=	-me
TBL=tbl
manual.ps: ${SRCS}
	${TBL} ${SRCS} | groff ${MACROS} -Tps > $@

clean :
	-rm manual.ps

