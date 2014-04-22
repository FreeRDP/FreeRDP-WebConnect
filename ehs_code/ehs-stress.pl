#!/usr/bin/perl

#
# ehs-stress.pl was written by Michal Pleban
#

use LWP::UserAgent;
use HTTP::Request;

if ( @ARGV < 1 ) {
	PrintHelp ( );
	exit;
}

my %UserAgentOptions;

$SERVER = shift @ARGV;
$PROCESSES = 10;
$HITS = 10;
$PROTO = "http";
while  ( my $arg = shift @ARGV ) {
	if ( $arg eq "-p" ) {
		PrintHelp ( ) and exit if @ARGV == 0;
		$PROCESSES = shift @ARGV;
	} elsif ( $arg eq "-c" ) {
		PrintHelp ( ) and exit if @ARGV == 0;
		$HITS = shift @ARGV;
	} elsif ( $arg eq "-k" ) {
		$UserAgentOptions { keep_alive } = 1;
	} elsif ( $arg eq "-s" ) {
        $PROTO = "https";
	}
}

my $totalrequests = 0;

sub printstats {
    for ($i = 0; $i < $PROCESSES; $i++) {
        $Pid = wait();
        if ( $? == 0 ) {
            printf "PID $Pid exited successfully\n";
        } else {
            printf "PID $Pid failed\n";
        }
        # load number of requests from /tmp
        $totalrequests += `cat /tmp/ehsstress_$Pid`;
        unlink "/tmp/ehsstress_$Pid";
    }

    print "performed $totalrequests total requests\n";
    $end_time = time;
    $total_time = $end_time - $start_time;
    $avg_time = $totalrequests != 0 ? $total_time / $totalrequests : 0;
    print "It took $total_time seconds to run $totalrequests queries for an avg o $avg_time second / request\n";
}

$SIG{INT} = sub {
    print "\nQUIT\n";
    $HITS = -1;
    foreach (keys %kids) {
        kill($_);
    }
};

print "Starting $0 against $SERVER with $PROCESSES processes each performing $HITS queries\n";
$start_time = time;

for ($i = 0; $i < $PROCESSES; $i++) {
    print "Starting process " . ($i + 1) . "\n";
    if (!($kidpid = fork())) {
        $ua = LWP::UserAgent->new ( %UserAgentOptions );
        $ua->agent("ehs-stress_" . $kidpid);
        $ua->ssl_opts(verify_hostname => 0);

        $failed = 0;
        $requestsperformed = 0;
        for ($j = 0; $j < $HITS || $HITS == -1; $j++) {
            $l = int rand 16384;
            print "Sending request " . ($j + 1) . " from process $i\n" if ($j + 1) % 100 == 0;
            $r = HTTP::Request->new("GET", $PROTO . "://" . $SERVER . "/" );
            $response = $ua->request($r);
	    if (!$response->header('Date')) {
		print $response->as_string, "\n";
            	$failed = 1;
		last;
	    }
            $requestsperformed++;
        }
        `echo $requestsperformed >/tmp/ehsstress_$$`;
        exit ( $failed );
    } else {
        $kids{$kidpid}++;
    }
}

printstats();

sub PrintHelp {
    print "Usage: $0 <hostname>:<port> [-p <processes>] [-c <requests>] [-k]\n\t-p: number of processes to spawn (default 10)\n\t-c: request count per process (default 10)\n\t-k: if specified, persistent connections are used.\n\t-s: if specified, SSL is enabled.\n";
}
