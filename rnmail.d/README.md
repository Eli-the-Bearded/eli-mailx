Example Rnmail
==============

Since Rnmail is the preferred way to send email in eli-mailx it would be
remiss not to include an example of how the scripts that power that.

Config
------

Some environment variables of note:

A default from header will be created using three variables, like this:

    From: "$RNMAILNAME" <$USER@$HOST>

Sent email will be saved in $MAILRECORD; aborted mail will save to
$LOGDIR/dead.letter or $HOME/dead.letter

The $RNMAILER program should look like "sendmail" and provide a
`sendmail` like interface, including the `-f from@address` envelope
address specifier or else you end up in weird long-untested fall
back code.

You can use $DOTDIR to override $HOME in the next.

You can use $MAILSIGNATURE to provide a signature (defaults to
$HOME/.mail_sig with a fallback to $HOME/.signature)

Your $HOME/.mailrc or /etc/mailrc should set the RNMAIL setting or
as a fall back, it can be in your environment. Use the `-r` option
for the setting.

The default editor is $VISUAL, $EDITOR is the fall back.

The `reply-headers` script is where I customize from addresses for
personal domains. It has To: address to From: header rules for that.

Rnmail
------

This is the main beast, providing the interactive elements of the mailer.
I use it with:

    set RNMAIL="Rnmail -r"

in $HOME/.mailrc

reply-headers
-------------

The `-r` option to `Rnmail` takes a message file and uses `reply-headers`
to parse that message to create the reply headers. It has code to
conditionally rewrite the sender address based on the recipent. This is
useful if you have mail send to multiple addresses end up on the same host
and you want the outgoing address to match the incoming. It is expected
that individual users will have their own copy of `reply-headers` with
their own rules.

See the `%fromtrans` and `%fromname` name transformation hashes and the
`$name_r` and `$addr_r` capturing regular expressions.

reply-body
----------

This is not used automatically, but a suggested `vi` command to run this
is included in the reply headers. It creates an attribution line and
quotes the message body from the mail file passed to `Rnmail`. The format
of the attribution line changes to a more corporate modern top-post style
if `--top` is specified.

bmime
-----

This creates MIME message skeleton for N attachments with `-N`. The
`Rnmail` script uses it for creating a reply template with attachments.
It was originally written for standalone use.

mbox.saver
----------

Pure unmodified `trn` helper file for making mbox friendly formatting.

