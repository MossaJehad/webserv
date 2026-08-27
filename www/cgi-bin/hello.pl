#!/usr/bin/perl
# Third CGI language (alongside Python and Bash) to demonstrate that CGI
# dispatch is driven purely by the configured file extension.
use strict;
use warnings;

my $method = $ENV{'REQUEST_METHOD'} || 'GET';
my $query  = $ENV{'QUERY_STRING'}   || '';
my $script = $ENV{'SCRIPT_NAME'}    || '';

my $body = '';
if ($method eq 'POST') {
    local $/ = undef;
    my $in = <STDIN>;          # read to EOF, as the CGI specification expects
    $body = defined($in) ? $in : '';
}

my $interpreter = $^X;
my $version     = $];
my $body_len    = length($body);

print "Content-Type: text/html\r\n";
print "\r\n";
print <<"HTML";
<!DOCTYPE html>
<html>
<head><title>Perl CGI</title></head>
<body style="font-family: sans-serif; background:#0f172a; color:#f8fafc; padding:40px;">
  <h2>Perl CGI is alive</h2>
  <p>Interpreter: $interpreter (Perl $version)</p>
  <p>SCRIPT_NAME: $script</p>
  <p>REQUEST_METHOD: $method</p>
  <p>QUERY_STRING: $query</p>
  <p>Body bytes read to EOF: $body_len</p>
  <a href="/" style="color:#38bdf8;">Return Home</a>
</body>
</html>
HTML
