FreeRDP-WebConnect will be a gateway for seamless access to your RDP-Sessions
in any HTML5-compliant browser. In particular it relies on the
Canvas (http://www.w3.org/TR/2dcontext/) and
WebSockets (http://www.w3.org/TR/websockets/) feature.

The server side WebSockets implementation handles current RFC6455
(http://tools.ietf.org/html/rfc6455) only, so browsers that implement
the older drafts do *not* work. With RFC6455 being raised to the
"Proposed Standard" level, this should change now really soon.

On the server side, a standalone daemon - written in C++ - provides a
Web page via HTTPS (or HTTP, if configured) and uses FreeRDP libs to
connect as a client to any RDP session.

Although the project is still in a very early development phase (project was started
on April 3, 2012), it already can display an RDP session right now. 

Added automated build/install script setup-all.sh
For unattended setup, installing all prereqs and deleting conflicting packages, run the script as root, specifying the following command:
./setup-all.sh -f -i -d

For addition details on the setup script and webconnect prereqs consult wsgate/README.