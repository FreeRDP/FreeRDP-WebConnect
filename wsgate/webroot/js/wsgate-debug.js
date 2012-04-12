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

var wsgate = wsgate || {}

wsgate.WSrunner = new Class( {
    Implements: Events,
    initialize: function(url) {
        this.url = url;
    },
    Run: function() {
        this.sock = new WebSocket(this.url);
        this.sock.binaryType = 'arraybuffer';
        this.sock.onopen = this.onWSopen.bind(this);
        this.sock.onclose = this.onWSclose.bind(this);
        this.sock.onmessage = this.onWSmsg.bind(this);
        this.sock.onerror = this.onWSerr.bind(this);
    }
});

wsgate.RDP = new Class( {
    Extends: wsgate.WSrunner,
    initialize: function(url, canvas) {
        this.parent(url);
        this.canvas = canvas;
        this.cctx = canvas.getContext('2d');
    },
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
                // wsgate.log.debug('BINRESP: ', evt.data);
                op = new Uint32Array(evt.data, 0, 1);
                switch (op[0]) {
                    case 0:
                        // BeginPaint
                        wsgate.log.debug('BeginPaint');
                        break;
                    case 1:
                        // EndPaint
                        wsgate.log.debug('EndPaint');
                        break;
                    case 2:
                        // Bitmap
                        var hdr = new Uint32Array(evt.data, 4, 13);
                        var bmdata = new Uint8Array(evt.data, 56);
                        var comp = 1; //bmdata[bmdata.length - 1];
                        wsgate.log.debug('Bitmap:',
                                (comp ? ' C ' : ' U '),
                                ' x: ', hdr[0], ' y: ', hdr[1],
                                ' w: ', hdr[4], ' h: ', hdr[5],
                                ' bpp: ', hdr[6]
                                );
                        if ((hdr[6] == 16) || (hdr[6] == 15)) {
                            if (comp) {
                                var outB = this.cctx.createImageData(hdr[4], hdr[5]);
                                if (!wsgate.dRLE16(bmdata, hdr[8], hdr[4], outB.data)) {
                                    this.cctx.putImageData(outB, hdr[0], hdr[1]);
                                } else {
                                    wsgate.log.warn('Decompression error');
                                }
                            } else {
                                wsgate.log.warn('Not yet implemented');
                            }
                        } else {
                            wsgate.log.warn('BPP <> 15/16 not yet implemented');
                        }
                        break;
                    default:
                        wsgate.log.warn('Unknown BINRESP: ', evt.data.byteLength);
                }
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

wsgate.EchoTest = new Class( {
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

wsgate.SpeedTest = new Class( {
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
}

wsgate.getRGB16 = function(inA, inI, outA) {
    var pix = inA[inI] + (inA[inI + 1] * 0x0100);
    outA[0] = (pix & 0xF800) >> 11; // red
    outA[1] = (pix & 0x7E0) >> 5;   // green
    outA[2] = (pix & 0x1F);         // blue
}
wsgate.dRLE16 = function(inA, inLength, width, outA) {
    var inI = 0;
    var outI = 0;
    var maxI = (width * 4);
    var alpha = 255;
    var rlen;
    var rgb = new Uint8Array(3);
    var rgb2 = new Uint8Array(3);
    var copyI;
    while (inI < inLength) {
        switch (inA[inI]) {
            case 254:
                inI++;
                outA[outI++] = 0;
                outA[outI++] = 0;
                outA[outI++] = 0;
                outA[outI++] = alpha;
                break;
            case 253:
                inI++;
                outA[outI++] = 255;
                outA[outI++] = 255;
                outA[outI++] = 255;
                outA[outI++] = alpha;
                break;
            case 0:
                inI++;
                rlen = inA[inI++] + 32;
                while ((outI < maxI) && (rlen > 0)) {
                    outA[outI++] = 0;
                    outA[outI++] = 0;
                    outA[outI++] = 0;
                    outA[outI++] = alpha;
                }
                if (rlen <= 0) {
                    break;
                }
                copyI = outI - maxI;
                while (rlen--) {
                    outA[outI++] = outA[copyI++];
                    outA[outI++] = outA[copyI++];
                    outA[outI++] = outA[copyI++];
                    outA[outI++] = outA[copyI++];
                }
                break;
            case 240:
                inI++;
                rlen = inA[inI] + (inA[inI + 1] * 0x0100);
                inI += 2;
                while ((outI < maxI) && (rlen > 0)) {
                    outA[outI++] = 0;
                    outA[outI++] = 0;
                    outA[outI++] = 0;
                    outA[outI++] = alpha;
                }
                if (rlen <= 0) {
                    break;
                }
                copyI = outI - maxI;
                while (rlen--) {
                    outA[outI++] = outA[copyI++];
                    outA[outI++] = outA[copyI++];
                    outA[outI++] = outA[copyI++];
                    outA[outI++] = outA[copyI++];
                }
                break;
            case 96:
                inI++;
                rlen = inA[inI++] + 32;
                wsgate.getRGB16(inA, inI, rgb);
                inI += 2;
                while (rlen--) {
                    outA[outI++] = rgb[0];
                    outA[outI++] = rgb[1];
                    outA[outI++] = rgb[2];
                    outA[outI++] = alpha;
                }
                break;
            case 243:
                inI++;
                rlen = inA[inI] + (inA[inI + 1] * 0x0100);
                inI += 2;
                wsgate.getRGB16(inA, inI, rgb);
                inI += 2;
                while (rlen--) {
                    outA[outI++] = rgb[0];
                    outA[outI++] = rgb[1];
                    outA[outI++] = rgb[2];
                    outA[outI++] = alpha;
                }
                break;
            case 128:
                inI++;
                rlen = inA[inI] + 32;
                inI++;
                while (rlen--) {
                    wsgate.getRGB16(inA, inI, rgb);
                    inI += 2;
                    outA[outI++] = rgb[0];
                    outA[outI++] = rgb[1];
                    outA[outI++] = rgb[2];
                    outA[outI++] = alpha;
                }
                break;
            case 244:
                inI++;
                rlen = inA[inI] + (inA[inI + 1] * 0x0100);
                inI += 2;
                while (rlen--) {
                    wsgate.getRGB16(inA, inI, rgb);
                    inI += 2;
                    outA[outI++] = rgb[0];
                    outA[outI++] = rgb[1];
                    outA[outI++] = rgb[2];
                    outA[outI++] = alpha;
                }
                break;
            case 224:
                inI++;
                rlen = inA[inI++] + 16;
                wsgate.getRGB16(inA, inI, rgb);
                inI += 2;
                wsgate.getRGB16(inA, inI, rgb2);
                inI = (inI + 2);
                while (rlen--) {
                    outA[outI++] = rgb[0];
                    outA[outI++] = rgb[1];
                    outA[outI++] = rgb[2];
                    outA[outI++] = alpha;
                    outA[outI++] = rgb2[0];
                    outA[outI++] = rgb2[1];
                    outA[outI++] = rgb2[2];
                    outA[outI++] = alpha;
                }
                break;
            case 248:
                inI++;
                rlen = inA[inI] + (inA[inI + 1] * 0x0100);
                inI += 2;
                wsgate.getRGB16(inA, inI, rgb);
                inI += 2;
                wsgate.getRGB16(inA, inI, rgb2);
                inI += 2;
                while (rlen--) {
                    outA[outI++] = rgb[0];
                    outA[outI++] = rgb[1];
                    outA[outI++] = rgb[2];
                    outA[outI++] = alpha;
                    outA[outI++] = rgb2[0];
                    outA[outI++] = rgb2[1];
                    outA[outI++] = rgb2[2];
                    outA[outI++] = alpha;
                }
                break;
            case 241:
            case 242:
            case 245:
            case 246:
            case 247:
            case 250:
            case 251:
            case 252:
            case 0xFF:
                return false;
            case 192:
            case 193:
                return false;
            case 32:
            case 64:
            case 160:
                return false;
            default:
                if (inA[inI] < 192) {
                    switch (inA[inI] & 224) {
                        case 0:
                            rlen = inA[inI++] & 31;
                            while ((outI < maxI) && (rlen > 0)) {
                                outA[outI++] = 0;
                                outA[outI++] = 0;
                                outA[outI++] = 0;
                                outA[outI++] = alpha;
                            }
                            if (rlen <= 0) {
                                break;
                            }
                            copyI = outI - maxI;
                            while (rlen--) {
                                outA[outI++] = outA[copyI++];
                                outA[outI++] = outA[copyI++];
                                outA[outI++] = outA[copyI++];
                                outA[outI++] = outA[copyI++];
                            }
                            break;
                        case 96:
                            rlen = inA[inI++] & 31;
                            wsgate.getRGB16(inA, inI, rgb);
                            inI += 2;
                            while (rlen--) {
                                outA[outI++] = rgb[0];
                                outA[outI++] = rgb[1];
                                outA[outI++] = rgb[2];
                                outA[outI++] = alpha;
                            }
                            break;
                        case 128:
                            rlen = inA[inI++] & 31;
                            while (rlen--) {
                                wsgate.getRGB16(inA, inI, rgb);
                                inI += 2;
                                outA[outI++] = rgb[0];
                                outA[outI++] = rgb[1];
                                outA[outI++] = rgb[2];
                                outA[outI++] = alpha;
                            }
                            break;
                        case 32:
                        case 64:
                        case 160:
                        default:
                            return false;
                    }
                } else {
                    switch (inA[inI] & 240) {
                        case 224:
                            rlen = inA[inI++] & 15;
                            wsgate.getRGB16(inA, inI, rgb);
                            inI += 2;
                            wsgate.getRGB16(inA, inI, rgb2);
                            inI += 2;
                            while (rlen--) {
                                outA[outI++] = rgb[0];
                                outA[outI++] = rgb[1];
                                outA[outI++] = rgb[2];
                                outA[outI++] = alpha;
                                outA[outI++] = rgb2[0];
                                outA[outI++] = rgb2[1];
                                outA[outI++] = rgb2[2];
                                outA[outI++] = alpha;
                            }
                            break;
                        case 192:
                        case 193:
                        default:
                            return false;
                    }
                }
        }
    }
    return true;
}

