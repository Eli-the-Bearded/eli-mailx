/* A smattering of UTF-8 smarts intended to be used for dealing display
 * content from unknown sources in a UTF-8 setting. For convience, has
 * a ISO-8859-1 validator, but not a general purpose UTF-8 or charset
 * library.
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

/* Bit patterns for legitimate UTF-8:
 *
 * non-highbit:
 *    0bbbbbbb
 * two octet highbit:
 *    110bbbbb 10bbbbbb
 * three octet highbit:
 *    1110bbbb 10bbbbbb 10bbbbbb
 * four octet highbit:
 *    11110bbb 10bbbbbb 10bbbbbb 10bbbbbb
 */

/* low bit (no highbit)
 *    0bbbbbbb
 * note that null is low bit
 */
#define UTF8_LOWBIT(oct)       (0x00 == ((oct) & 0x80))

/* any continuation octet
 *    10bbbbbb
 */
#define UTF8_CONTINUATION(oct) (0x80 == ((oct) & 0xC0))

/* start of two octet
 *    110bbbbb
 */
#define UTF8_SEQUENCE_2(oct)   (0xC0 == ((oct) & 0xE0))

/* start of three octet
 *    1110bbbb
 */
#define UTF8_SEQUENCE_3(oct)   (0xE0 == ((oct) & 0xF0))

/* start of four octet
 *    11110bbb
 */
#define UTF8_SEQUENCE_4(oct)   (0xF0 == ((oct) & 0xF8))

#define iequal(a,b,l)     (strncasecmp(a,b,l)==0)/* case-insensitive equal */


/* checks a string str of length len for legit UTF-8 bit patterns.
 * null will not terminate the string -- those are legit 7bit ASCII.
 * returns byte offset of first non-legit sequence or -1 if 100% okay.
 */
int
check_utf8(str, len)
  unsigned char* str;
  int len;
{
  int seq, pos, run, octet;
  run = 0;

  for(pos = 0; pos < len; pos++) {
    octet = str[pos];

    /* start of a sequence */
    if(run == 0) {
      seq = pos;
    }

    if(UTF8_LOWBIT(octet)) {
      if( run != 0 ) {
	/* whoops, wanted highbit there */
        return seq;
      }
      continue;
    }

    if(UTF8_CONTINUATION(octet)) {
      if( run ) {
	/* one of our expected run */
        run --;
	continue;
      }
      /* whoops, not the right spot for this */
      return seq;
    }

    if( run ) {
      /* whoops, should have had a continuation octet above */
      return seq;
    }

    if(UTF8_SEQUENCE_2(octet)) {
      run = 1; /* one more */
      continue;
    }

    if(UTF8_SEQUENCE_3(octet)) {
      run = 2; /* two more */
      continue;
    }

    if(UTF8_SEQUENCE_4(octet)) {
      run = 3; /* three more */
      continue;
    }

    /* yikes! fall through! */
    return seq;
  }

  return -1;
} /* check_utf8() */

/* Scan a string (up to len long) looking for a valid start of a UTF-8
 * sequence. (Does not check that it is a valid UTF-8 character, but
 * just where one could be.) Valid starts are low-bit or non-continuation.
 * Useful for figuring out where to start printing when dealing with a
 * stream entered somewhere besides the start.
 * Returns -1 if none are found or the starting offset.
 */
int
find_start_utf8(str, len)
  unsigned char* str;
  int len;
{
  int i = 0;
  while(i < len) {
    if(UTF8_LOWBIT(str[i])) {
      return i;
    }
    if(!UTF8_CONTINUATION(str[i])) {
      return i;
    }

    i++;
  }
  return -1;
} /* find_start_utf8() */

/* Checks a string str of length len for legit ASCII bit patterns.
 * Recall ASCII is a strict subset of UTF-8.
 * null will not terminate the string -- those are legit 7bit ASCII.
 * returns byte offset of first non-legit sequence or -1 if 100% okay.
 */
int
check_ascii(str, len)
  unsigned char* str;
  int len;
{
  int pos;

  for(pos = 0; pos < len; pos++) {

    if(!UTF8_LOWBIT(str[pos])) {
	/* Um, yeah, we wanted everything to be lowbit. */
        return pos;
    }
  }

  return -1;
} /* check_ascii() */

/* Checks a string str of length len for legit ISO-8859-1 bit patterns.
 * Legit UTF-8 can masquarade as ISO-8859-1, but some things that are
 * pawned off as ISO-8859-1 are really Windows-1252 (CP-1252).
 * null will not terminate the string -- those are legit 7bit ASCII.
 * returns byte offset of first non-legit sequence or -1 if 100% okay.
 */
int
check_8859_1(str, len)
  unsigned char* str;
  int len;
{
  int pos;

  for(pos = 0; pos < len; pos++) {

    if(!UTF8_LOWBIT(str[pos])) {
      /* is it in the control character range? */
      if(str[pos] < 160) {
        return pos;
      }
    }
  }

  return -1;
} /* check_8859_1() */

/* Checks a string str of length len for legit charset bit patterns.
 * The charset is a case-insenstive name for a MIME chacter encoding and
 * set of characters. The set part is not considered, only the encoding.
 * If the charset is one of UTF-8 or ISO-8859-1 (also called ISO-LATIN-1),
 * it will be tested against rules for those sets. Everything else is
 * checked against the ASCII rules.
 * null will not terminate the string -- those are legit 7bit ASCII.
 * returns byte offset of first non-legit sequence or -1 if 100% okay.
 */
int
check_charset(charset, str, len)
  unsigned char* charset;
  unsigned char* str;
  int len;
{

  /* use a length of one more than the fixed string for these iequal tests */
  if(iequal(charset, "UTF-8", 6) || iequal(charset, "UTF8", 5)) {
    return check_utf8(str,len);
  }
  if(iequal(charset, "ISO-8859-1", 11) || iequal(charset, "ISO-LATIN-1", 12)) {
    return check_8859_1(str,len);
  }
  return check_ascii(str,len);
} /* check_charset */

/*
 * for a valid utf-8 string str and a desired character count len,
 * return the number of bytes from the string to print.
 * expects a null terminated utf-8 string, and returns a value 
 *    betweeen len and 4 x len
 *    or strlen(str) if that's less than len
 *    or -1 if invalid null-terminated utf-8 is discovered
 */
int
display_length(str, len)
  unsigned char* str;
  int len;
{
  int to_emit = 0;
  while(len) {
    /* null is lowbit, get this out of the way */
    if(*str == 0) { return(to_emit); }

    if(UTF8_LOWBIT(*str)) {
      str ++;
      to_emit ++;
    }
    else if(UTF8_SEQUENCE_2(*str)) {
      str += 2;
      to_emit += 2;
    }
    else if(UTF8_SEQUENCE_3(*str)) {
      str += 3;
      to_emit += 3;
    }
    else if(UTF8_SEQUENCE_4(*str)) {
      str += 4;
      to_emit += 4;
    }
    else {
      /* WHOA! should never happen (if valid null-terminated UTF-8). */
      return(-1);
    }
    len --;
  }

  return(to_emit);
} /* display_length() */

/* for self tests:
 *    cc -o utf-8 -D_RUN_TESTS utf-8.c && ./utf-8
 */
#ifdef _RUN_TESTS

#include <stdio.h>
#include <strings.h>

struct tests {
	/* a test string */
	char    *input;
	int	len;		/* length, use 0 to just strlen() it */
				/* needed for embeded null tests */

	/* and expected return values from each of the checkers. */
	int	utf8;
	int	iso;
	int	ascii;
	/* check_charset will try the inputs with different charset settings */
};
struct tests cases[] = {
	/* test string					len utf iso ascii */
   {	"Plain ASCII",					 0, -1, -1, -1,   },
   /* with ellipsis */
   {	"windows/cp 1252\205",				 0, 15, 15, 15,   },
   /* with pound sign */
   {	"\302\243100, same as in UTF-8 town",		 0, -1, -1,  0,   },
   /* with degree sign */
   {	"98\260 in the shade of ISO-8859-1",		 0,  2, -1,  2,   },
   /* check past null */
   {	"\3\2\1\0\1\2\3",				 8, -1, -1, -1,   },
   /* check again, with a bad character after the null */
   {	"\3\2\1\0\1\2\3\213",				 9,  7,  7,  7,   },

   {	0, 0, 0, 0   },

};
struct tests *tcase;

int
main(argc, argv)
  int argc;
  char **argv;
{
  int i;
  /* measure all the simple test cases */
  for (i=0; cases[i].input; i++) {
    tcase = &cases[i];
    if(0 == tcase->len) {
      tcase->len = strlen(tcase->input);
    }
  }
  return ( test_check_ascii() ||
           test_check_8859_1() ||
	   test_check_utf8() ||
	   test_check_charset() ||
	   test_display_len() ||
	   test_find_start_utf8() );
} /* main() */

/* Note: each of g (got) and w (want) are evaluated twice. As a macro,
 * instead of a function, we can use this to automatically exit the
 * calling function on error.
 */
#define RESULT_CHECK(g, w) { \
      if(w == g) { \
	printf("  ... good:: expected %d == check output %d\n", w, g); \
      } else { \
	printf("FAILED: Expected %d but got %d\n", w, g); \
	return 1; \
      } \
    }

int
test_find_start_utf8()
{
  struct start_tests {
     char* fragment;
     int   where;
  };
  struct start_tests stcases[] = {
     { " \360\237\244\246 ", 0 },
     {  "\360\237\244\246 ", 0 },
     {      "\237\244\246 ", 3 },
     {          "\244\246 ", 2 },
     {              "\246 ", 1 },
     {                  " ", 0 },
     { 0, 0 }
  };
  struct start_tests *tcase;
  int i, r;

  for (i=0; stcases[i].fragment; i++) {
    tcase = &stcases[i];
    printf("Checking for start UTF-8: %s\n", tcase->fragment);
    r = find_start_utf8(tcase->fragment, 12);
    RESULT_CHECK(r, tcase->where);
  }
  return 0;
}

int
test_check_utf8()
{
  int i, r;
  for (i=0; cases[i].input; i++) {
    tcase = &cases[i];
    printf("Checking UTF-8 length(%d): %s\n", tcase->len, tcase->input);
    r = check_utf8(tcase->input, tcase->len);
    RESULT_CHECK(r, tcase->utf8);
  }
  return 0;
} /* test_check_utf8() */

int
test_check_8859_1()
{
  int i, r;
  for (i=0; cases[i].input; i++) {
    tcase = &cases[i];
    printf("Checking ISO-8859-1 length(%d): %s\n", tcase->len, tcase->input);
    r = check_8859_1(tcase->input, tcase->len);
    RESULT_CHECK(r, tcase->iso);
  }
  return 0;
} /* test_check_8859_1() */

int
test_check_ascii()
{
  int i, r;
  for (i=0; cases[i].input; i++) {
    tcase = &cases[i];
    printf("Checking ASCII length(%d): %s\n", tcase->len, tcase->input);
    r = check_ascii(tcase->input, tcase->len);
    RESULT_CHECK(r, tcase->ascii);
  }
  return 0;
} /* test_check_ascii() */


int
test_check_charset()
{
  int i, r;
  for (i=0; cases[i].input; i++) {
    tcase = &cases[i];

    printf("Checking as US-ASCII length(%d): %s\n", tcase->len, tcase->input);
    r = check_charset("US-ASCII", tcase->input, tcase->len);
    RESULT_CHECK(r, tcase->ascii);

    printf("Checking as ISO-LATIN-1 length(%d): %s\n", tcase->len, tcase->input);
    r = check_charset("ISO-LATIN-1", tcase->input, tcase->len);
    RESULT_CHECK(r, tcase->iso);

    printf("Checking as ISO-8859-1 length(%d): %s\n", tcase->len, tcase->input);
    r = check_charset("ISO-8859-1", tcase->input, tcase->len);
    RESULT_CHECK(r, tcase->iso);

    printf("Checking as UTF-8 length(%d): %s\n", tcase->len, tcase->input);
    r = check_charset("UTF-8", tcase->input, tcase->len);
    RESULT_CHECK(r, tcase->utf8);
  }
  return 0;
} /* test_check_charset() */

/* This test works a little differently. It takes a string and a display
 * length (in characters), then tries to truncate the string to that
 * length, and then verifies that the truncated string is still UTF-8.
 * So ideally you have a bunch of different length UTF-8 characters in
 * the test string, and a bunch of different lengths that would naively
 * cut-off mid character.
 */
int
test_display_len()
{
  int full, want, use, where, dots;

  /* ten characters of Hebrew after some simpler ASCII, then 
   * a space and U+1F926 FACE PALM
   * unsigned char jesus[] = "ASCII was good enough for \327\231\326\260\327\224\327\225\326\271\327\251\326\273\327\201\327\242\326\267 \360\237\244\246";
   * Actually that's a bad example since it has combining characters,
   * so the dot line doesn't match up.
   * Use small caps instead. A mix of 1, 2, 3 and 4 character UTF-8.
   */
   unsigned char jesus[] = "ASCII was Good Enough\302\256 for \341\264\212\341\264\207\352\234\261\341\264\234\352\234\261 \360\237\244\246";
#define ENOUGH 80
  unsigned char something[ENOUGH];

  /* naive string length */
  full = strlen(jesus);

  if(-1 != (where = check_utf8(jesus, full))) {
    printf("Test error source sentence is not UTF-8, at %d\n", where);
  }
  for(want = 20; want < full; want ++) {
    strcpy(something, jesus);

    use = display_length(something, want);
    if(use < ENOUGH) { 
      something[use] = 0;
    } else { 
      printf("Test error ENOUGH is not enough (%d not less than %d)\n", use, ENOUGH);
      return 2;
    }
    printf("Using %d of %d octets for %d characters.\n", use, full, want);

    if(-1 != (where = check_utf8(something, use))) {
      printf("Failed to validate as UTF-8, at %d\n", where);
      return(1);
    }
    puts(something);
    dots = want;
    while(dots) { putchar('.'); dots --; }
    putchar('\n');
    if (use >= full) {
      break;
    }
  }

  return(0);
} /* test_display_len() */
#endif /* _RUN_TESTS */
