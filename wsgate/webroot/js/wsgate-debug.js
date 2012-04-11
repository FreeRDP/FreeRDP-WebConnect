// Provide a generic window.BlobBuilder
// See: https://developer.mozilla.org/en/DOM/BlobBuilder
function gBB() {
    if ('BlobBuilder' in window) {
        return;
    }
    if ('MozBlobBuilder' in window) {
        window.BlobBuilder = window.MozBlobBuilder;
    }
    if ('WebKitBlobBuilder' in window) {
        window.BlobBuilder = window.WebKitBlobBuilder;
    }
    if ('MSBlobBuilder' in window) {
        window.BlobBuilder = window.MSBlobBuilder;
    }
}
gBB();

var wsgate = wsgate || {};

wsgate.WSrunner = new Class({
    Implements: Events,
    initialize: function(url) {
        this.url = url;
    },
    Run: function() {
        this.sock = new WebSocket(this.url);
        this.sock.onopen = this.onWSopen.bind(this);
        this.sock.onclose = this.onWSclose.bind(this);
        this.sock.onmessage = this.onWSmsg.bind(this);
        this.sock.onerror = this.onWSerr.bind(this);
    }
});

wsgate.RDP = new Class({
    Extends: wsgate.WSrunner,
    onWSmsg: function(evt) {
        switch (typeof(evt.data)) {
            case 'string':
                wsgate.log.debug(evt.data);
                switch (evt.data.substr(0,2)) {
                    case "E:":
                            wsgate.log.err(evt.data.substring(2));
                            this.sock.close();
                            this.fireEvent('alert', evt.data.substring(2));
                            break;
                    case 'I:':
                            wsgate.log.info(evt.data.substring(2));
                            break;
                    case 'W:':
                            wsgate.log.warn(evt.data.substring(2));
                            break;
                    case 'D:':
                            wsgate.log.debug(evt.data.substring(2));
                            break;
                }
                break;
            case 'object':
                wsgate.log.debug('BINRESP: ', evt.data);
                break;
        }

    },
    onWSopen: function(evt) {
        wsgate.log.debug("CONNECTED");
    },
    onWSclose: function(evt) {
        wsgate.log.debug("DISCONNECTED");
    },
    onWSerr: function (evt) {
        wsgate.log.warn(evt);
    }
});

wsgate.EchoTest = new Class({
    Extends: wsgate.WSrunner,
    onWSmsg: function(evt) {
        wsgate.log.info('RESPONSE: ', evt.data);
        this.sock.close();
    },
    onWSopen: function(evt) {
        wsgate.log.debug("CONNECTED");
        this.send("WebSocket rocks");
    },
    onWSclose: function(evt) {
        wsgate.log.debug("DISCONNECTED");
    },
    onWSerr: function (evt) {
        wsgate.log.warn(evt);
    },
    send: function(msg) {
        wsgate.log.info("SENT: ", msg); 
        this.sock.send(msg);
    }
});

wsgate.SpeedTest = new Class({
    Extends: wsgate.WSrunner,
    initialize: function(url, blobsize, iterations) {
        this.parent(url);
        this.blobsize = blobsize;
        this.maxiter = iterations;
        this.count = this.maxiter;
        this.start = undefined;
        this.total = 0;
        this.bb = new BlobBuilder();
        this.ab = new ArrayBuffer(blobsize);
    },
    onWSmsg: function(evt) {
        var time = Date.now() - this.start;
        wsgate.log.debug('Data size: ', evt.data.size, ', roundtrip time: ', time, 'ms');
        this.total += time;
        if (--this.count) {
            this.doChunk();
        } else {
            var ave = this.total / this.maxiter;
            wsgate.log.info('Transfer ratio: ',
                (this.blobsize / ave * 2000).toFixed(1), ' [Bytes per Second] = ',
                (this.blobsize / ave * 0.0152587890625).toFixed(1), ' [Mbps]');
            this.sock.close();
        }

    },
    onWSopen: function(evt) {
        wsgate.log.debug("CONNECTED");
        this.start = undefined;
        this.total = 0;
        this.count = this.maxiter;
        this.doChunk();
    },
    onWSclose: function(evt) {
        wsgate.log.debug("DISCONNECTED");
    },
    onWSerr: function (evt) {
        wsgate.log.warn(evt);
    },
    doChunk: function() {
        this.start = Date.now();
        // FF appears to empty the Blob, while RIM does not.
        if (0 == this.bb.getBlob().size) {
            this.bb.append(this.ab);
        }
        this.sock.send(this.bb.getBlob());
    }
});

wsgate.log = {
    debug: function() {
        console.debug.apply(this, arguments);
    },
    info: function() {
        console.info.apply(this, arguments);
    },
    warn: function() {
        console.warn.apply(this, arguments);
    },
    err: function() {
        console.error.apply(this, arguments);
    }
};
