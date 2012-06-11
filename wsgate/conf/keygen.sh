#!/bin/sh

WSGATE_CONFIG=/etc/wsgate.ini

test -f /etc/sysconfig/wsgate && . /etc/sysconfig/wsgate

test -f $WSGATE_CONFIG || exit 0

CERTFILE=`grep ^certfile $WSGATE_CONFIG|head -1|cut -d= -f2|tr -d " "`
MAKECERT=/etc/pki/tls/certs/make-dummy-cert
if test -x /usr/sbin/make-ssl-cert -a -d /etc/ssl/certs ; then
    MAKECERT="/usr/sbin/make-ssl-cert /usr/share/ssl-cert/ssleay.cnf"
fi
if test -n "$CERTFILE" -a -f "$CERTFILE" ; then
    exit 0
fi
if test -z "$CERTFILE" ; then
    if test -x /usr/sbin/make-ssl-cert -a -d /etc/ssl/certs ; then
        export CERTFILE=/etc/ssl/certs/wsgate.pem
    else
        export CERTFILE=/etc/pki/tls/certs/wsgate.pem
    fi
    perl -pi -e 'if(/^\[([a-z]+)\]/){if($1 eq "ssl"){$ss=1;}else{print "certfile = ".$ENV{"CERTFILE"}."\n" if($ss);}}' $WSGATE_CONFIG
fi
$MAKECERT "$CERTFILE"
chmod 0640 "$CERTFILE"
chgrp wsgate "$CERTFILE"
