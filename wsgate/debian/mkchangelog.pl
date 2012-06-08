#!/usr/bin/perl

use Date::Manip;

my $version = shift(@ARGV);
my $inmsg = 0;
my $infiles = 0;
my $stamp = '';
my $name = '';
my $msg = '';
my $rev = 1;

open(P, 'git log --date-order|') || die "Can't run git: $!\n";
my @lines = <P>;
close P;
foreach (@lines) {
    chomp;
    if (/^commit\s+(.*)$/) {
        if ($inmsg) {
            $inmsg = 0;
            next if ($msg eq '');
            print 'wsgate (' . $version . '-' . $rev . ') unstable; urgency=low' . "\n";
            print $msg;
            print ' -- ' . $name . '  ' . $stamp . "\n\n";
            if ($msg =~ /Bumped up version/) {
                my @vt = split(/\./, $version);
                $vt[$#vt]--;
                $version = join('.', @vt);
            }
            $msg = '';
        }
        open(P, 'git describe --match initial '.$1.'|') ||  die "Can't run git: $!\n";
        my $tmp = <P>;
        close P;
        chomp($tmp);
        $tmp =~ s/^initial-(\d+)-.*$/\1/;
        $rev = $tmp;
        next;
    }
    if (/^Author:\s+(.*)$/) {
        $name = $1;
        next;
    }
    if (/^Date:\s+(.*)$/) {
        $stamp = UnixDate($1, "%a, %d %b %Y %H:%M:%S %z");
        $inmsg = 1;
        next;
    }
    if ($inmsg) {
        my $line = $_;
        $line =~ s/^\s+//;
        $line =~ s/\s+$//;
        $line =~ s/^[\-\*]/  */;
        next if ($line eq '');
        $line = "    ".$line unless ($line =~ /^\s\s\*/);
        $msg .= $line . "\n";
    }
}
