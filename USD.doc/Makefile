#	$OpenBSD: Makefile,v 1.1 1994/06/29 05:09:54 deraadt Exp $

DIR=	usd/07.mail
SRCS=	mail0.nr mail1.nr mail2.nr mail3.nr mail4.nr mail5.nr mail6.nr \
	mail7.nr mail8.nr mail9.nr maila.nr
MACROS=	-me
TBL=tbl

.PHONY: all

all: manual.pdf manual.ps manual.html manual.txt

manual.pdf: ${SRCS}
	${TBL} ${SRCS} | groff ${MACROS} -Tpdf > $@

manual.ps: ${SRCS}
	${TBL} ${SRCS} | groff ${MACROS} -Tps > $@

manual.html: ${SRCS}
	${TBL} ${SRCS} | groff ${MACROS} -Thtml > $@

manual.txt: ${SRCS}
	${TBL} ${SRCS} | groff ${MACROS} -Tascii > $@

clean :
	-rm -f manual.pdf manual.ps manual.html manual.txt

