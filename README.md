eli-mailx
=========

Personal fork of mailx.

Several features that are important to me:

* mailx has good header search features that I find easy to use, this
  fork keeps those and adds some others.
  * Message sizes are highlighted in lines and bytes in index summary
    and can be searched using the same notation:
    * `f > 500M` find mail over 500 megabytes
    * `f = 10c`  find mail with 1000 (ten hundreds) of lines (using the
      same rounding rules the display uses)
  * Instead of needing to `set searchheaders`, a different search
    prefix can be used (have searchheaders enabled makes regular
    searches slower):
    * `f '%Header:term of interest` to find a "Header: with 
      "term of interest" in it.
  * A history of exactly one search for the use-case of searching with
    the `from` command non-destructively and then applying a command,
    e.g., `delete` to those same matched messages.

* Rnmail command, uses a shell script (derived from the one included with
  `rn`/`trn`) to compose repiles to mail
  * Because the 1980s native mail composer in `mailx` is bad.
  * Rnmail is a friendly front-end to `sendmail -oi`
    * Arbitrary headers can be set or edited.
    * So the MIME savvy user can compose MIME email.
    * Mail "From" any special address is easy to send.

* Header highlighting, selected by the same interface as header hiding.
  * Highlighting pre- and post-fixes an arbitrary string, which can be
    ordinary or an escape sequence.
    * Backticks now supported in settings for using `tput` to set
      escape sequences.

* Variables are used in more places, such as to hold the destination of
  mailboxes to `Save` files to, or read mail `-F`rom. The `echo` command
  has been made useful to examine variables.

* The `touch` command fixed to mark messages as read without reading them.
  * The `unread` command has always existed to mark as unread.

* The one-line index of messages in a mailbox has been tweaked to my
  tastes:
  * Terse date, for a longer subject line excerpt.
  * "Human" readable style (`ls -H`) message size descriptions.
  * Some terminal width sensitivity, for reading on my narrow phone screen
    or extra-wide xterms.

* The mailx I started with still had 1980s ideas of message sizes. No
  longer does this use 16 bit short for things like number of lines.

Some things to know about my mail set-up:

* I like the big mbox full of many messages model, not the directory
  full of single message files model. But I sort and use multiple
  mboxes.

* I prefer to deal with MIME mail with `procmail` rules during delivery,
  so the stuff that makes it to my inbox has path names to extracted
  files added as a MIME prefix.

* My version of `Rnmail` (not in this distribution for privacy reasons)
  has rules for determining the "From" address I will use and parses
  out the address from the headers to use as envelope from address
  when invoking `sendmail`.
  * I use separate email addresses for basically all non-personal email.
  * This facilitates sorting and makes address shaing obvious.
  * My Github address, my Wikipedia address, my Amazon address, etc, are
    all different and I want to use them sometimes when sending mail.
    * This is particularly important for mailing lists.
