#!/bin/sh

WSGATE_CONFIG=/etc/wsgate.ini

test -f /etc/sysconfig/wsgate && . /etc/sysconfig/wsgate

test -f $WSGATE_CONFIG || exit 0

CERTFILE=`grep ^certfile $WSGATE_CONFIG|cut -d= -f2|tr -d " "`
if [ -n "$CERTFILE" -a -f "$CERTFILE" ] ; then
    exit 0
fi
if [ -z "$CERTFILE" ] ; then
    export CERTFILE=/etc/pki/tls/certs/wsgate.pem
    perl -pi -e 'if(/^\[([a-z]+)\]/){if($1 eq "ssl"){$ss=1;}else{print "certfile = ".$ENV{"CERTFILE"}."\n" if($ss);}}' $WSGATE_CONFIG
fi
/etc/pki/tls/certs/make-dummy-cert "$CERTFILE"
chmod 0640 "$CERTFILE"
chgrp wsgate "$CERTFILE"
