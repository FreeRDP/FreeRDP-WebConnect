#!/usr/bin/perl
#
use strict;
use warnings;

my %dres = (
    'html_both' => '^(.*)\<!-- DEBUG.*\<!-- \/DEBUG --\>(.*)$',
    'html_start' => '^(.*)\<!-- DEBUG',
    'html_end' => '^.*\<!-- \/DEBUG --\>(.*)',
    'css_both' => '^(.*)\/\*\ DEBUG.*\/\* \/DEBUG \*\/(.*)$',
    'css_start' => '^(.*)\/\* DEBUG',
    'css_end' => '^.*\/\* \/DEBUG \*\/(.*)',
    'js_both' => '^(.*)\/\*\ DEBUG.*\/\* \/DEBUG \*\/(.*)$',
    'js_start' => '^(.*)\/\* DEBUG',
    'js_end' => '^.*\/\* \/DEBUG \*\/(.*)',
);

sub do_strip($$) {
    my $fn = shift;
    my $type = shift;
    my $reboth = $dres{$type.'_both'};
    my $restart = $dres{$type.'_start'};
    my $reend = $dres{$type.'_end'};
    my $in_debug = 0;
    open(I,"<$fn") || die "Can't read $fn: $!\n";
    while (<I>) {
        chomp;
        if ($type eq 'html') {
            if (/\<script\s+language="javascript" type="text\/javascript"\>/) {
                $reboth = $dres{'js_both'};
                $restart = $dres{'js_start'};
                $reend = $dres{'js_end'};
            }
            if (/\<\/script\>/) {
                $reboth = $dres{$type.'_both'};
                $restart = $dres{$type.'_start'};
                $reend = $dres{$type.'_end'};
            }
        }
        while (/$reboth/) {
            $_ = $1 . $2;
        }
        if ($in_debug) {
            if (/$reend/) {
                $in_debug = 0;
                $_ = $1;
            }
        } else {
            if (/$restart/) {
                $in_debug = 1;
                $_ = $1;
                print $_, "\n" if ($_ ne '');
            }
        }
        print $_, "\n" if (!$in_debug);
    }
    close I;
}

sub do_cat($) {
    my $fn = shift;
    open(I,"<$fn") || die "Can't read $fn: $!\n";
    print while (<I>);
    close I;
}

foreach (@ARGV) {
    if (/\.(html|css|js)$/) {
        do_strip($_, $1);
        next;
    }
    do_cat($_);
}
