/* A tool for reading a file and verifying the contents are consistent
 * with a few well-known charsets, in particular UTF-8, US-ASCII, and
 * ISO-8859-1.
 *
 * October 2018, Benjamin Elijah Griffin / Eli the Bearded
 */

/*
 * Copyright (c) 2018
 *	Benjamin Elijah Griffin / Eli the Bearded
 *
 * Distributed uner the MIT License:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

extern int check_utf8(unsigned char*, int);
extern int check_8859_1(unsigned char*, int);
extern int check_ascii(unsigned char*, int);
extern int find_start_utf8(unsigned char*, int);

#define VERSION		"2018.10.17"

#define ASCII		0
#define UTF8		8
#define UTF		16	/* UTF-8 */
/*      8859		8859 */

#define LINE		80	/* small buffer size, about a line */
#define BLOCK		0x4000	/* large buffer size, 16kb */

int check_mode = 0;
int quiet = 0;
int verbose = 0;
int multiline = 0;
int nullokay = 0;
char progname[LINE];
char *use_file = NULL;

void
usage(void)
{
  printf("Usage for %s\n", progname);
  puts("Check valid encoding for a charset.\n");
  puts("     checkutf8  [ options ] [ filename ]  : defaults to UTF-8");
  puts("     check8859  [ options ] [ filename ]  : defaults to ISO-8859-1");
  puts("     checkascii [ options ] [ filename ]  : defaults to ASCII");
  puts("");
  puts("Charset options:");
  puts("     -8    --iso-8859-1              use ISO-8859-1 rules");
  puts("     -a    --ascii                   use ASCII rules");
  puts("     -u    --utf-8                   use UTF-8 rules");
  puts("");
  puts("Other options:");
  puts("  -c    --continue                keep checking after first error");
#if 0 /* not implemented yet */
  puts("  -n    --nullokay                changes how lines are processed");
#endif
  puts("  -q    --quiet                   no output, just error code set");
  puts("  -v    --verbose                 show where on line issue lies");
  puts("        --                        end of option flags");
  puts("");
  puts("Non-testing flags:");
  puts("        --help                    no testing, just show usage");
  puts("        --version                 no testing, just show version");

}
void
version(void)
{
  printf("%s: %s\n", progname, VERSION);
}

/* Parse the command line, setting globals. Calls exit() on error. */
void
parseargs(ac, av)
  int ac;
  char **av;
{
  int i;
  if(ac < 1) {
    fprintf(stderr, "No args? How is that possible?\n");
    exit(1);
  }

  i=0;

  /* set default charset based on program name */
  if(strstr(av[i], "checkutf8")) {
    check_mode = UTF8;
    strcpy(progname, "checkutf8");
  } else 
  if(strstr(av[i], "check8859")) {
    check_mode = 8859;
    strcpy(progname, "check8859");
  } else {
    check_mode = ASCII;
    if(strstr(av[i], "checkascii")) {
      strcpy(progname, "checkascii");
    } else {
      strncpy(progname, av[i], LINE);
    }
  }

  i++;
  while(i < ac) {
    if('-' == av[i][0]) {
      int c = 1;
      if('-' == av[i][c]) {
         /* long arg parse */
	 int u = i;
	 i++;

	 if(0 == strcmp("--", av[u])) {
	   /* got us an end of args */
	   if((i) == ac) {
	     /* nothing more */
	     break;
	   }

	   if(use_file) {
	     fprintf(stderr, "%s: too many filenames, have %s already\n", progname, use_file);
	     exit(1);
	   }

	   if((i+1) == ac) {
	     /* one more */
	     use_file = av[i];
	     break;
	   }

	   fprintf(stderr, "%s: too many filenames, have %s already\n", progname, av[i]);
	   exit(1);
	 } /* -- */

	 if(0 == strcasecmp("--iso-8859-1", av[u])) {
	   check_mode = 8859;
	   continue;
	 } /* --ascii */

	 if(0 == strcasecmp("--ascii", av[u])) {
	   check_mode = ASCII;
	   continue;
	 } /* --ascii */

	 if(0 == strcasecmp("--continue", av[u])) {
	   multiline = 1;
	   continue;
	 } /* --continue */

#if 0 /* not implemented yet */
	 if(0 == strcasecmp("--nullokay", av[u])) {
	   nullokay = 1;
	   continue;
	 } /* --nullokay */
#endif

	 if(0 == strcasecmp("--quiet", av[u])) {
	   quiet = 1;
	   verbose = 0;
	   continue;
	 } /* --quiet */

	 if(0 == strcasecmp("--utf-8", av[u])) {
	   check_mode = UTF8;
	   continue;
	 } /* --utf-8 */

	 if(0 == strcasecmp("--verbose", av[u])) {
	   quiet = 0;
	   verbose = 1;
	   continue;
	 } /* --verbose */


	 if(0 == strcasecmp("--help", av[u])) {
	   usage();
	   exit(0);
	 }

	 if(0 == strcasecmp("--version", av[u])) {
	   version();
	   exit(0);
	 }

	 fprintf(stderr, "%s: unrecognized option: %s\n", progname, av[i]);
	 exit(1);
      } else {
         /* short arg parse */
	 while(av[i][c]) {
	   switch(av[i][c]) {
	     case '8':
	       check_mode = 8859;
	       break;
	     case 'a':
	       check_mode = ASCII;
	       break;
	     case 'c':
	       multiline = 1;
	       break;
#if 0 /* not implemented yet */
	     case 'n':
	       nullokay = 1;
	       break;
#endif
	     case 'q':
	       quiet = 1;
	       verbose = 0;
	       break;
	     case 'u':
	       check_mode = UTF-8;
	       break;
	     case 'v':
	       quiet = 0;
	       verbose = 1;
	       break;

	     default:
	       fprintf(stderr, "%s: unrecognized flag: %c\n", progname, av[i][c]);
	       exit(1);
	   }
	   c++;
	 }
      }
    } /* have a leading - */
    else {
      if(use_file) {
	fprintf(stderr, "%s: too many filenames, have %s already\n", progname, use_file);
	exit(1);
      } else {
        use_file = av[i];
      }
    }
    i++;
  }

  if(use_file) {
    if(!freopen(use_file, "r", stdin)) {
      fprintf(stderr, "%s: failed to open %s\n", progname, use_file);
      exit(1);
    }
  }
} /* parseargs() */

int
fread_check(void)
{
  int rc = 0;
  unsigned char checkspace[BLOCK], hold[LINE];
 
  /* not implented yet */

  /* Not sure there's value here, except maybe making offsets into
   * the file as the output instead of the line and offset.
   */

  return rc;
} /* fread_check() */

int
fgets_check(void)
{
  int rc = 0;
  int where, size, held, line, held_err;
  unsigned char checkspace[BLOCK], hold[LINE];

  line = held = held_err = 0;
  hold[held] = 0;

  while(fgets(checkspace, BLOCK, stdin)) {
    if(strchr(checkspace, '\n')) {
      line ++;
    }
    size = strnlen(checkspace, BLOCK);

    if(held) {
      /* can only happen for UTF-8 */
      strncpy(&hold[held], checkspace, 4);
      where = check_utf8(hold, LINE);
      if((-1 == where) || (where > 4)) {
        /* no issues in the line overlap area */

	where = find_start_utf8(checkspace, LINE);
	while(where > 0) {
	  checkspace[where] = ' ';
	  where --;
	}
      } else {
        where += BLOCK;
	held_err = 1;
      }
      held = 0;
      hold[held] = 0;
    } /* deal with held over stuff */

    if ((!held_err) && (UTF8 == check_mode)) {
      where = check_utf8(checkspace, size);
      if(where > -1) {
	int len = strnlen(checkspace, size);
	if(where + 4 > len) {
	  /* might have a character split across read blocks. Urg. */
	  strncpy(hold, &checkspace[where], LINE);
	  held = strnlen(hold, LINE);
	  continue;
	}
      }
    } else if (8859 == check_mode) {
      where = check_8859_1(checkspace, size);
    } else {
      where = check_ascii(checkspace, size);
    }

    if(-1 != where) {
      rc = 1;
      if(!quiet) {
	printf("Line %d offset %d invalid\n", line, where+1);

	if(verbose) {
	  /* show where on that line */
	  printf("%s\n", checkspace);
	  while(where) { putchar('.'); where--; }
	  puts("^");
	}
	
        if(!multiline) { break; }
      }
      held_err = 0;
    }
  } /* while reading lines */

  return rc;
} /* fgets_check() */

int
main(ac, av)
  int ac;
  char **av;
{
  int rc;
  parseargs(ac, av);

#ifdef _TEST_PARSE_ARGS
  printf("The checkmode is %d\n", check_mode);
  printf("Will be quiet? %s\n", quiet?"yes":"no");
  printf("Will be verbose? %s\n", verbose?"yes":"no");
  printf("Using multiline? %s\n", multiline?"yes":"no");
  printf("Reading from: %s\n", use_file?use_file:"(stdin)");
#endif

  if(nullokay) {
    rc = fread_check();
  } else {
    rc = fgets_check();
  }
  return rc;
}

