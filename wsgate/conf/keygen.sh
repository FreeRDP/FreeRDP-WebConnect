#!/bin/sh

WSGATE_CONFIG=/etc/wsgate.ini

test -f /etc/sysconfig/wsgate && . /etc/sysconfig/wsgate

test -f $WSGATE_CONFIG || exit 0

CERTFILE=`grep ^certfile $WSGATE_CONFIG|cut -d= -f2`
if [ -n "$CERTFILE" -a -f "$CERTFILE" ] ; then
    exit 0
fi
if [ -z "$CERTFILE" ] ; then
    CERTFILE=/etc/pki/tls/certs/wsgate.pem
    /etc/pki/tls/certs/make-dummy-cert /etc/pki/tls/certs/wsgate.pem
    echo "certfile = $CERTFILE" >> $WSGATE_CONFIG
else
    /etc/pki/tls/certs/make-dummy-cert "$CERTFILE"
fi
