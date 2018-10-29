/* A smattering of MIME smarts intended to be used for dealing with MIME
 * encoded words (RFC 2047) in mail headers. Not a general purpose MIME
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

#include <stdlib.h>
#include <strings.h>

/* This data structure is inspired by mime.c in trn4, the decoder using
 * it is my own. I have different ideas about error conditions.   -EtB-
 */

#define PP	127	/* padding (=, also used for ?) */
#define SS	128	/* skip (white space) */
#define XX	129	/* error */
			/* and 0 to 63 are place value for that character */
static unsigned char index_b64[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, SS,SS,SS,SS, XX,SS,XX,XX,  /*  00- 15 */
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,  /*  16- 31 */
    SS,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,62, XX,XX,XX,63,  /*  32- 47 */
    52,53,54,55, 56,57,58,59, 60,61,XX,XX, XX,PP,XX,PP,  /*  48- 63 */
    XX, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,  /*  64- 79 */
    15,16,17,18, 19,20,21,22, 23,24,25,XX, XX,XX,XX,XX,  /*  80- 95 */
    XX,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,  /*  96-111 */
    41,42,43,44, 45,46,47,48, 49,50,51,XX, XX,XX,XX,XX,  /* 112-127 */
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,  /* 128-... */
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,  
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, 
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,  /* ...-255 */
};

/* This is very specifically for the QP used in mime words.
 * It does double duty being a validator and base16 decode lookup,
 * plus storing the special _ to 0x20 remap.
 *
 * All of these "errors" need to be 128 or above.
 * Valid HEX characters should be 0 to 15, and other non-errors
 * between 16 and 127.
 */
#define QP	130	/* quoted printable char start (=) */
#define EE	131	/* end of block (?) */
#define SP	 32	/* underscore -> space map */
#define OK	 64	/* other */
#define LT	126	/* limited acceptability */
/* XX is used as above: error, which in this case are characters
 * not allowed in MW QP. The "must be encoded" specials are:
 *   especials = "(" / ")" / "<" / ">" / "@" / "," / ";" / ":" / "
 *               <"> / "/" / "[" / "]" / "?" / "." / "="
 * but recall that ? and = are used in the mime words themselves.
 * These especials are only *forbidden* in charset. In the content
 * part, ( ) and " are *sometimes* allowed and *sometimes* not
 * allowed.
 * And _ is explicitly defined as 0x20 (32) even if space is in a 
 * different place in the character set.
 */
static unsigned char index_qp[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,  /*  00- 15 */
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,  /*  16- 31 */
    XX,OK,LT,OK, OK,OK,OK,OK, LT,LT,OK,OK, LT,OK,LT,LT,  /*  32- 47 */
     0, 1, 2, 3,  4, 5, 6, 7,  8, 9,LT,LT, LT,QP,LT,EE,  /*  48- 63 */
    LT,10,11,12, 13,14,15,LT, OK,OK,OK,OK, OK,OK,OK,OK,  /*  64- 79 */
    OK,OK,OK,OK, OK,OK,OK,OK, OK,OK,OK,LT, OK,LT,OK,SP,  /*  80- 95 */
    OK,10,11,12, 13,14,15,OK, OK,OK,OK,OK, OK,OK,OK,OK,  /*  96-111 */
    OK,OK,OK,OK, OK,OK,OK,OK, OK,OK,OK,OK, OK,OK,OK,OK,  /* 112-127 */
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,  /* 128-... */
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,  
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, 
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,  /* ...-255 */
};


/* For a given string, calculate how much space the full decode would
 * require. If the value would exceed max_care, return max_care.
 */
int
decode_b64_size(from, max_care)
  char *from;
  int  max_care;
{
  int size = 1;	/* minimum size, for the null terminator */
  int b    = 0;

  while(*from) {
    unsigned char p = index_b64[(unsigned char)*from];

    from++;

    if (XX == p) { return size;       }
    if (SS == p) { continue;          }
    if (PP == p) { return (size + 3); }

    b++;
    if(4 == b) {
      b = 0;
      size += 3;
    }
    if(size > max_care) {
      return max_care;
    }
  }
  return size;
} /* decode_b64_size() */

/* In base64, three 8-bit values get encoded into four 6-bit values.
 * Bit view:
 *
 *  {   source octet one  }  {   source octet two  }  { source octet three  }
 *  a0 a1 a2 a3 a4 a5 a6 a7  b0 b1 b2 b3 b4 b5 b6 b7  c0 c1 c2 c3 c4 c5 c6 c7
 *  { out char one  } {  out char two  } { out char three } { out char four }
 *
 * If a message ends after SO1,
 *	 only two bits of OC2 are used. OC3 and OC4 are "=" padding.
 * If a message ends after SC2,
 *	 only four bits of OC3 are used. OC4 is "=" padding.
 *
 * Since I intend this for use with MIME encoded words:
 *        =?charset?B?base64content?=
 *                  ^B for base64
 * I'm taking ? as a padding string for input that lacks a correct pad.
 * Howver, this is also tolerant of whitespace, as in a regular base64
 * block (whitespace being disallowed in MIME words).
 */

/* decode_b64 helper: convert a block (OC1 to OC4 in block[0..3]) into
 * original bytes (SO1 to S02 in to[0-2]) then set to[3] to null.
 * Designed for clarity over speed.
 */
#define DECODEBLOCK()	{ \
	  to[o  ] = (block[0] << 2) | (block[1] >> 4); \
	  to[o+1] = (block[1] << 4) | (block[2] >> 2); \
	  to[o+2] = (block[2] << 6) | (block[3]); \
	  to[o+3] = '\0'; \
	}

/* Decode base64; *to string should be preallocated and sufficently large:
 * about  4 + ( strlen(from) / 4 ) * 3 (but ignoring whitespace in *from)
 * or use decode_b64_size().
 *
 * Blocks of four characters from source to be turned into three bytes in
 * destination). Guaranteed even on error the *to string will be null
 * terminated, although likely shorter than full message.
 *
 * The max value is interpreted as the the size of *to. If a block of 4
 * source characters would cause *to to exceed max, decoding will stop
 * prematurely (without an error).
 *
 * return value is: -1 error in input; 0 did nothing; 1 successful decode
 */
int
decode_b64(from, to, max)
  char *from;
  char *to;
  int max;
{
  unsigned char block[4];
  int i = 0;
  int o = 0;
  int did_something = 0;

  if(max <= 0) {
    return -1;
  }
  *to = '\0';
  while(*from) {
    unsigned char p = index_b64[(unsigned char)*from];

    from++;

    if (XX == p) { return -1; }
    if (SS == p) { continue;  }

    /* end input processing on padding */
    if (PP == p) { 
      /* zero out rest of block */
      while(i < 4) { block[i] = 0; i++; }
      DECODEBLOCK();
      did_something = 1;
      i = 0;
      break;
    }
    
    /* we have a value! */
    block[i] = p;
    i++;

    /* we have a full block! */
    if(4 == i) {
      DECODEBLOCK();
      did_something = 1;
      i = 0;
      o += 3;

      /* will next block exceed max? */
      if((o + 3) > max) { break; }
    }
  } /* while scanning from */

  if(i != 0 ) { return -1; }
  return did_something;
} /* decode_b64() */

/* For a given string, calculate how much space the full decode would
 * require. If the value would exceed max_care, return max_care.
 * Does not do a rigorous check on validity.
 */
int
decode_qp_size(from, max_care)
  char *from;
  int   max_care;
{
  int s = 1;	/* for final null */

  while((s < max_care) && (*from)) {
    if (index_qp[(unsigned char)*from] == EE) {
      /* take EE as end of source */
      break;
    }
    if (index_qp[(unsigned char)*from] == QP) {
      if(from[1] && from [2]) {
        s ++;
	from += 3;
	continue;
      }
      /* error: end of string in =XX  */
      break;
    }
    s++;
    from ++;
  }

  return s;
} /* decode_qp_size() */

/* Decode quoted printable; *to string should be preallocated and
 * sufficently large, output can range from something 1/3rd *from 
 * to the same size as *from.
 *
 * The max value is interpreted as the the size of *to. Decoding
 * will stop prematurely (without an error) before exceeding max
 * (while still null terminating the output).
 *
 * Knows to encode _ as space, and errors out on illegal characters
 * in QP, which includes whitespace in mime-word QP.
 *
 * return value is: -1 error in input; 0 did nothing; 1 successful decode
 */
int
decode_qp(from, to, max)
  char *from;
  char *to;
  int max;
{
  int o = 0;
  int did_something = 0;

  if(max <= 0) {
    return -1;
  }
  to[o] = '\0';
  while(*from) {
    unsigned char f = (unsigned char)*from;
    unsigned char p = index_qp[f];

    /* stop now if there's not enough space */
    if((o+1) >= max) {
      break;
    }

    from ++;

    if (EE == p) {
      break;
    }
    else if(SP == p) { 
      to[o] = p;
      did_something = 1;
    }
    else if (QP == p) {
      /* if *from is null, a will be XX */
      unsigned char a = index_qp[(unsigned char)*from];
      unsigned char b;

      if(a > 16) {
        return -1;
      }
      from ++;
      b = index_qp[(unsigned char)*from];
      if(b > 16) {
        return -1;
      }
      from ++;
      to[o] = (a*16) + b;
      did_something = 1;
    }
    else if (p > 127) {
      /* illegal in QP mime word */
      return -1;
    }
    else {
      to[o] = f;
      did_something = 1;
    }

    o++;
    to[o] = '\0';
  } /* while scanning *from */

  return did_something;
} /* decode_qp() */

/* Decode a "header" in-place. A header, here, is a null terminated
 * string with zero or more embeded MIME-words. No other aspects of
 * RFC-822 headers are enforced, so it can be a complete RFC-822 header,
 * just the value, or something else entirely. The MW in header is
 * decoded to plain text if the charset provided matches the one in the
 * word. For charsets ISO-8859-* and UTF-8, MW with charset US-ASCII
 * will also be decoded, since ASCII is a strict subset of those, and MW
 * with US-ASCII charsets exist in the wild.
 *
 * The format of a MW is:
 *   =?CHARSET?X?CONTENT?=
 * where CHARSET is a case *in*sensitive charset name,
 *          X    is a case *in*sensitive "B"ase64 or "Q"uoted-printable flag
 *       CONTENT is an encoded block with no whitespace
 *
 * When there are multiple MW in a row, the whitespace between them should
 * be deleted. If there's other stuff besides a MW, the whitespace will
 * be preserved:
 *    case 1:  =?CHARSET?X?CONTENT?= =?CHARSET?X?CONTENT?=
 *          -> {decoded CONTENT}{decoded CONTENT}
 *    case 2:  =?CHARSET?X?CONTENT?= and more =?CHARSET?X?CONTENT?=
 *          -> {decoded CONTENT} and more {decoded CONTENT}
 *
 * The output values are *not* wrapped to under 80 columns with RFC-822
 * continued header syntax. Nor are the output values limited to RFC-821
 * 1000 characters per line.
 *
 * A MW always takes up more space than the decoded content, so the
 * resulting decoded text will never be larger than the original. That
 * said, a max value is used to truncate the content if less than the
 * full header is desired. The truncation is NOT charset aware, and
 * the output should be validated and possibly retruncated. Set max to
 * allow for that; UTF-8 characters can require as a many as 4 bytes.
 * (Use case: displaying as much of a header as fits on a single
 * terminal line.)
 *
 * Twice max will be used in scratch space, so huge max is a bad idea;
 *
 * Returns the number of MIME words decoded (0 or more) on success,
 * and -1 on error. A successful run will always leave header a null
 * terminated string of max bytes or less (including null). An
 * error will always leave header unchanged.
 */
#define DECODE_STATE_PLAIN		1
#define DECODE_STATE_POSSIBLE_WORD	2
#define DECODE_STATE_Q_WORD		3
#define DECODE_STATE_B_WORD		4
#define DECODE_STATE_DONE_WORD		5
#define DECODE_STATE_COPY_NEW		6
#define DECODE_STATE_INTERWORD		7
#define DECODE_STATE_COMPLETE		8

int
decode_header(header, charset, max)
  char *header;
  char *charset;
  int   max;
{
  char *in, *out, *new, encoding;
  int  i, o, n, words, rc, ascii_okay, bookmark, contentstart;
  int state;
  int charsetok, charsetlen;

  /* sanity checks */
  if(max < 1) { return -1; }
  if(header == NULL) { return -1; }
  if(charset == NULL) { return -1; }
  if(*header == 0)  { return -1; }
  if(*charset == 0)  { return -1; }

  charsetlen = strnlen(charset, max);
  in = header;
  out = (char*)malloc(max);
  if(NULL == out) { return -1; }
  new = (char*)malloc(max + 4);
  if(NULL == new) { free(out); return -1; }

  /* length 9 to stop before terminating null */
  if(0 == strncasecmp("iso-8859-", charset, 9)) {
    ascii_okay = 1;
  /* length 6 to include checking terminating null */
  } else if(0 == strncasecmp("utf-8", charset, 6)) {
    ascii_okay = 1;
  } else {
    ascii_okay = 0;
  }

  i = o = 0;
  words = 0;
  encoding = 0;
  state = DECODE_STATE_PLAIN;
  /* complex state machine ahead
   *    in each state, copy at most one character to out[]
   *    copying to out[] can come from in[] or from new[]
   *    reading from in[] can advance significantly in one loop
   *		eg, when writing to new[]
   *    states that don't copy to out[] may fall through after changing state
   *		this can save a restart of the loop
   *
   *    bookmark is used as a copy restart position for in[]
   *    contentstart is used a copy position for encoded content in in[]
   */
  while(o < max) {
/* debugging giant state machines is fun.
    printf("DEBUG: in[%d]='%c'; o=%d, n=%d; state = %s\n", i, in[i], o, n,
	(       state == DECODE_STATE_PLAIN         ? "PLAIN"         :
	 (      state == DECODE_STATE_POSSIBLE_WORD ? "POSSIBLE_WORD" :
	  (     state == DECODE_STATE_Q_WORD        ? "Q_WORD"        :
	   (    state == DECODE_STATE_B_WORD        ? "B_WORD"        :
	    (   state == DECODE_STATE_DONE_WORD     ? "DONE_WORD"     :
	     (  state == DECODE_STATE_COPY_NEW      ? "COPY_NEW"      :
	      ( state == DECODE_STATE_INTERWORD     ? "INTERWORD"     :
	       (state == DECODE_STATE_COMPLETE      ? "COMPLETE"      : "--error--" 
	))))))))
    );
*/
    /* copy until start of a MW */
    if(DECODE_STATE_PLAIN == state) {
      if(in[i] == 0) {
        state = DECODE_STATE_COMPLETE;
        out[o] = 0;
	break;
      }
      if(in[i] == '=') {
        if(in[i+1] == '?') {
	  state = DECODE_STATE_POSSIBLE_WORD;
	  bookmark = i;
	  i += 2;
	  continue;
	}
      }

      out[o] = in[i];
      o++;
      i++;
      continue;
    } /* DECODE_STATE_PLAIN */

    if(DECODE_STATE_POSSIBLE_WORD == state) {
      /* We are at (in[i]):
       *     right here ------------v
       *      who knows who cares =?CHARSET?X?CONTENT?= does not matter
       * we are going to scan up to at most here -----^
       *
       * but if we decide *not* do decode, we just copy the first '='
       * restore i to bookmark+1, and restart in DECODE_STATE_PLAIN.
       * We won't decode if not right charset, if X flag is bogus, or if
       * not a complete word.
       */
      charsetok = 0;

      if(0 == strncasecmp(charset, &in[i], strnlen(charset, charsetlen))) {
        i += charsetlen;
	if('?' == in[i]) { charsetok = 1; }
      } else if (ascii_okay && (0 == strncasecmp("us-ascii", &in[i], 8))) {
        i += 8;
	if('?' == in[i]) { charsetok = 1; }
      }

#define LEAVE_DECODE_STATE_POSSIBLE_WORD() { \
        out[o] = in[bookmark]; \
	o++; \
	i = bookmark +1; \
	state = DECODE_STATE_PLAIN; \
	continue; \
      }

#define LEAVE_DECODE_STATE_INTERWORD() LEAVE_DECODE_STATE_POSSIBLE_WORD()

      if(!charsetok) {
        LEAVE_DECODE_STATE_POSSIBLE_WORD();
      }
      /* acceptable charset, let's move on */

      i++; /* was at second ? */
      if(('Q' == in[i]) || ('q' == in[i])) { encoding = 'q'; }
      if(('B' == in[i]) || ('b' == in[i])) { encoding = 'b'; }

      if(!encoding) {
        LEAVE_DECODE_STATE_POSSIBLE_WORD();
      }
      i++;
      if('?' != in[i]) { /* now checking third ? */
        LEAVE_DECODE_STATE_POSSIBLE_WORD();
      }
      i++;
      contentstart = i;

      /* scan for ?=, leaving if we get obvious bad content */
      while(in[i]) {
        if(state != DECODE_STATE_POSSIBLE_WORD) {
	   /* We hit a LEAVE_DECODE_STATE_POSSIBLE_WORD, so break the loop.
	    */
	  break;
	}

        if('?' == in[i]) {
	  if('=' == in[i+1]) {
	    /* we have a word! */
	    /* will process starting from contentstart */
	    if('q' == encoding) {
	      state = DECODE_STATE_Q_WORD;
	    } else {
	      state = DECODE_STATE_B_WORD;
	    }
	    /* next time we check i, start after the word */
	    i += 2;
	    continue;
	  }

	  /* shucks, so close. */
	  LEAVE_DECODE_STATE_POSSIBLE_WORD();
	} /* found ? in content area */

        if('q' == encoding) {
	  if(XX == index_qp[in[i]]) {
	    /* invalid content in qp */
	    LEAVE_DECODE_STATE_POSSIBLE_WORD();
	  }
	} else {
	  if(XX == index_b64[in[i]]) {
	    /* invalid content in b64 */
	    LEAVE_DECODE_STATE_POSSIBLE_WORD();
	  }
	}
	i++;
      } /* examining content part of possible word */

    } /* DECODE_STATE_POSSIBLE_WORD */

    if(DECODE_STATE_B_WORD == state) {
      /* max + 4 to ensure we don't prematurely truncate */
      rc = decode_b64(&in[contentstart], new, max + 4);
      state = DECODE_STATE_DONE_WORD;
      /* fall thru */
    } /* DECODE_STATE_B_WORD */

    if(DECODE_STATE_Q_WORD == state) {
      rc = decode_qp(&in[contentstart], new, max);
      state = DECODE_STATE_DONE_WORD;
      /* fall thru */
    } /* DECODE_STATE_Q_WORD */

    if(DECODE_STATE_DONE_WORD == state) {
      /* don't modify on error, just abandon it all */
      if(rc == -1) {
        free(new); free(out);
	return -1;
      }
      n = 0;
      words ++;
      state = DECODE_STATE_COPY_NEW;
      /* fall thru */
    } /* DECODE_STATE_DONE_WORD */

    if(DECODE_STATE_COPY_NEW == state) {
      if(new[n]) {
        out[o] = new[n];
	n ++;
	o ++;
	continue;
      }

      /* we've run out of new; decide next state */
      if(in[i]) {
	bookmark = i;
	state = DECODE_STATE_INTERWORD;
      } else {
	/* we've run out of input */
        out[o] = 0;
	state = DECODE_STATE_COMPLETE;
      }
    } /* DECODE_STATE_COPY_NEW */

    /* We've fininshed a word, but might have another. If we
     * do have another, we discard interword whitespace. If we
     * don't, we restore i to the bookmark and start copying again.
     */
    if(DECODE_STATE_INTERWORD == state) {
      if(0 == in[i]) {
        LEAVE_DECODE_STATE_INTERWORD();
      }
      /* reuse base64 whitespace check here */
      if(SS == index_b64[in[i]]) {
        i++;
	continue;
      }

      if('=' == in[i]) {
        if ('?' == in[i+1]) {
	  /* If not a real MIME word, we well end up checking that
	   * possible word twice. Once this time, once after copying the
	   * whitespace after the first word. That is a really rare
	   * case -- actual MIME word followed by false MIME word --
	   * so accept the risk.
	   */
	  i += 2; /* start at charset, we already have a bookmark */
	  state = DECODE_STATE_POSSIBLE_WORD;
	  continue;
	}
      }

      /* Uh-oh. Definitely not another word follow. */
      LEAVE_DECODE_STATE_INTERWORD();

    } /* DECODE_STATE_INTERWORD */

    if(DECODE_STATE_COMPLETE == state) {
      break;
    }
  } /* while not exceeding maximum output */

  /* we are done with this now */
  free(new);

  if(words != 0) {
    /* copy out to in */

    /* but where to put that null first */
    if (o < max) {
      o++;
    } else if (o == max) {
      o--;
    }
    out[o] = 0;

    strncpy(header, out, max);
  }
  free(out);
  return words;
} /* decode_header() */

/* for self tests:
 *    cc -o mimetests -D_RUN_TESTS mime.c && ./mimetests
 */
#ifdef _RUN_TESTS

#include <stdio.h>
#define DECODE_SIZE	80

/* run tests on decode_b64, return 0 if all work */
int
test_decode_b64()
{
  char decode_buffer[DECODE_SIZE];
  struct tests {
          char    *b64;
          char    *out;
	  int     bsize; /* always a multiple of 3 + 1 */
	  int     bad; 	 /* if set, consider error success */
  };
  struct tests cases[] = {
    /* regular text */
    {	"bWFudWFs",			"manual"	, 7 ,0	},
    /* white space skipping */
    {	"dHJh bnNt\taXNz\raW9u\n",	"transmission"	, 13,0	},
    /* odd binary */
    {   "AQID",				"\01\02\03"	, 4 ,0	},
    {   "////",				"\377\377\377"	, 4 ,0	},
    /* non-standard pad character */
    {	"YQ??",				"a"		, 4 ,0	},
    {	"YWI?",				"ab"		, 4 ,0	},
    /* increasing strings */
    {	"QQ==",				"A"		, 4 ,0	},
    {	"QUI=",				"AB"		, 4 ,0	},
    {	"QUJD",				"ABC"		, 4 ,0	},
    {	"QUJDRA==",			"ABCD"		, 7 ,0	},
    {	"QUJDREU=",			"ABCDE"		, 7 ,0	},
    {	"QUJDREVG",			"ABCDEF"	, 7 ,0	},
    {	"Q U J D R E V G R w = =",	"ABCDEFG"	, 10,0	},
    /* trigger a truncation (78 char output from src with 104 char) */
    /* expect to generate errors :^) */
    {	"Q2F1c2Ugc29tZSBlcnJvcnMuIFRoaXMgc3RyaW5nIGlzIHRvbyBsb25nIGZvciBvdXRwdXQgYnVmZmVyIGFuZCB3aWxsIGJlIHRydW5jYXRlZC4K",
    					"Cause some errors. This string is too long for output buffer and will be truncated.\n"
							, 85, 1 },
    {	"QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVphYmNkZWZnaGlqa2xtbm9wcXJzdHV2d3h5ekFCQ0RFRkdISUpLTE1OT1BRUlNUVVZXWFlaYWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXoK",
        				"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
							, 105, 1 },
    /* end */
    { 	0, 0, 1, 0  }
  };
  struct tests *tcase;
  int worked;
  int allworked = 0;
  int rc;
  int s;
  int i;

  for (i=0; cases[i].out; i++) {
    tcase = &cases[i];

    worked = decode_b64(tcase->b64, decode_buffer, DECODE_SIZE);
    rc = strncmp(decode_buffer, tcase->out, DECODE_SIZE);
    s  = decode_b64_size(tcase->b64, DECODE_SIZE);

    printf("Want: <%s> decode to <%s>\n", tcase->b64, tcase->out);
    printf("Buffer size needed: %d, expected %d, same? %s\n", s,
    		tcase->bsize, ((s==tcase->bsize)? "si" : "no"));

    printf("Got %d (%s): <%s> as expected? %s\n\n", worked, 
    		((worked == -1)? "decoder error" : "decoded  fine"),
		decode_buffer, (0 == rc ? "YES": "NO!") );

    rc += (s != tcase->bsize);
    if(0 != rc) { if(0 == tcase->bad) { allworked = 1; printf("UNEXPECTED RESULT\n"); } }
    else        { if(1 == tcase->bad) { allworked = 1; printf("UNEXPECTED RESULT\n"); } }
    if(allworked) { break; }
  }
  return allworked;
} /* test_decode_b64() */

/* run tests on decode_qp, return 0 if all work */
int
test_decode_qp()
{
  char decode_buffer[DECODE_SIZE];
  struct tests {
          char    *qp;
          char    *out;
	  int     msize;	/* minimum needed size, with null */
	  int     bad;		/* if set, consider error success */
  };
  struct tests cases[] = {
    {	"qazwsxedcrfvtgbyhnujmikolp",	"qazwsxedcrfvtgbyhnujmikolp"	, 27, 0},
    {	"This_is_MIME!",		"This is MIME!"			, 14, 0},
    {	"White=20space",		"White space"			, 12, 0},
    {	"=FF=01=3d=Ab",			"\377\01=\253"			,  5, 0},
    {	"Error. This won't work.",	"Error"				,  6, 1},
    /* actual QP word content from twitter, which contains "must encode"
     * characters, namely double quotes:
     * Subject: =?UTF-8?Q?"Storm_winds_force_waterfalls_to_?=
     *  =?UTF-8?Q?move_up_instead_of_down_=F0=9F=92=A8"_Moment?=
     */
    {	"\042Storm_winds_force_waterfalls_to_move_up_instead_of_down_=F0=9F=92=A8\042_Moment", "\042Storm winds force waterfalls to move up instead of down \360\237\222\250\042 Moment", 70, 0},

    {	0, 0, 1, 0 }
  };
  struct tests *tcase;
  int worked;
  int allworked = 0;
  int rc;
  int s;
  int i;

  for (i=0; cases[i].out; i++) {
    tcase = &cases[i];

    worked = decode_qp(tcase->qp, decode_buffer, DECODE_SIZE);
    rc = strncmp(decode_buffer, tcase->out, DECODE_SIZE);
    s  = decode_qp_size(tcase->qp, DECODE_SIZE);

    printf("Want: <%s> decode to <%s>\n", tcase->qp, tcase->out);
    printf("Buffer size needed: %d, expected %d, same? %s\n", s,
    		tcase->msize, ((s==tcase->msize)? "si" : "no"));

    printf("Got %d (%s): <%s> as expected? %s\n\n", worked, 
    		((worked == -1)? "decoder error" : "decoded  fine"),
		decode_buffer, (0 == rc ? "YES": "NO!") );

    rc += (worked < 0);
    if(0 != rc) { if(0 == tcase->bad) { allworked = 1; printf("UNEXPECTED RESULT\n"); } }
    else        { if(1 == tcase->bad) { allworked = 1; printf("UNEXPECTED RESULT\n"); } }
    if(allworked) { break; }
  }
  return allworked;
} /* test_decode_qp() */

/* run tests on decode_header, return 0 if all work */
int
test_decode_header()
{
  char header_buffer[DECODE_SIZE];
  int i;
  struct tests {
          char    *head;
          char    *charset;
          char    *out;
	  int     msize;	/* max size, for truncation tests */
  };
  struct tests cases[] = {
    /* things that should be changed */
    {	"=?CHARSET?Q?This_is_MIME!?=",				"charset",
        "This is MIME!",					DECODE_SIZE },
    {	"=?us-ascii?Q?White=20space_in_ASCII?=",		"iso-8859-1",
        "White space in ASCII",					DECODE_SIZE },
    {	"=?utf-8?b?Z29vZCBzaG93LCBsYWRz?=",			"UTF-8",
        "good show, lads",					DECODE_SIZE },
    {	"=?us-ascii?Q?Mixed=20?= =?utf-8?q?charsets?=",		"utf-8",
        "Mixed charsets",					DECODE_SIZE },

    /* things that should be changed, and truncated */
    {	"Subject: =?cool?q?this_one_is_tooooo_long_for_us?=",	"cool",
        "Subject: this one is",					21 },
    {	"=?yes?b?QW5kIG5vdyBmb3Igc29tZXRoaW5nIGNvbXBsZXRlbHk=?=", "yes",
        "And now for somethin",					21 },

    /* things that should not be changed */
    {	"Subject: leave this alone",				"charset",
        "Subject: leave this alone",				DECODE_SIZE },
    {	"=?US-ASCII?Q?charset_mismatch?=",			"charset",
        "=?US-ASCII?Q?charset_mismatch?=",			DECODE_SIZE },
    {	"=?charset?Q? not a word ?=",				"charset",
        "=?charset?Q? not a word ?=",				DECODE_SIZE },
    {	"=?charset?x?unknown_encoding?=",			"charset",
        "=?charset?x?unknown_encoding?=",			DECODE_SIZE },
    {	"=?charset?b?invalid-base64!?=",			"charset",
        "=?charset?b?invalid-base64!?=",			DECODE_SIZE },
    {	"LONG line with small buffer size",			"charset",
        "LONG line with small buffer size",			16 },

    /* end */
    { 0, 0, 0, 1}
  };
  struct tests *tcase;
  int worked;
  int allworked = 0;
  int rc;
  int s;

  for (i=0; cases[i].out; i++) {
    tcase = &cases[i];

    strncpy(header_buffer, tcase->head, DECODE_SIZE);

    worked = decode_header(header_buffer, tcase->charset, tcase->msize);
    rc = strncmp(header_buffer, tcase->out, DECODE_SIZE);
    s  = strnlen(header_buffer, DECODE_SIZE);

    printf("Want: <%s> using charset <%s>\nTo decode to: <%s>\n", tcase->head, tcase->charset, tcase->out);
    if (worked == -1) {
      printf("Decode ERROR: should be no change to source\n<%s>\n", header_buffer);
      if(rc) { printf("But MISMATCH!\n"); }
      printf("\n");
    } else if (worked == 0) {
      printf("Decode found no words: should be no change to source\n<%s>\n", header_buffer);
      if(rc) { printf("But MISMATCH!\n"); }
      printf("\n");
    } else {
      printf("Decoded %d word(s) (correctly? %s) to get:\n<%s>\n", worked, 
		(0 == rc ? "YES": "NO!"), header_buffer );
      printf("Buffer size needed: %d, max size %d, okay? %s\n\n", s,
    		tcase->msize, ((s<=tcase->msize)? "si" : "no"));
    }

    if(0 != rc) { allworked = 1; printf("UNEXPECTED RESULT\n"); break; }
  }
  return allworked;
} /* test_decode_header() */

int
main()
{
  return (test_decode_b64() ||
          test_decode_qp() ||
          test_decode_header());
}

#endif /* _RUN_TESTS */
