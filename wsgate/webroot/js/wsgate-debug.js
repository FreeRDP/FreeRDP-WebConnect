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
        try {
            this.sock = new WebSocket(this.url);
        } catch (err) { }
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
    Disconnect: function() {
        this.sock.close();
        this.cctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
    },
    onMouseMove: function(evt) {
        var buf, a;
        // wsgate.log.debug('M x: ', evt.client.x, ' y: ', evt.client.y);
        if (this.sock.readyState == this.sock.OPEN) {
            buf = new ArrayBuffer(16);
            a = new Uint32Array(buf);
            a[0] = 0;
            a[1] = 0x0800; // PTR_FLAGS_MOVE
            a[2] = evt.client.x;
            a[3] = evt.client.y;
            this.sock.send(buf);
        }
    },
    onMouseDown: function(evt) {
        var buf, a;
        wsgate.log.debug(evt);
        evt.stop();
        /*
           wsgate.log.debug('MC x: ', evt.client.x, ' y: ', evt.client.y);
           if (this.sock.readyState == this.sock.OPEN) {
           buf = new ArrayBuffer(16);
           a = new Uint32Array(buf);
           a[0] = 0;
           a[1] = 0x0800; // PTR_FLAGS_DOWN
           a[2] = evt.client.x;
           a[3] = evt.client.y;
           this.sock.send(buf);
           }
           */
    },
    onMouseUp: function(evt) {
        wsgate.log.debug(evt);
        var buf, a;
        /*
           wsgate.log.debug('MC x: ', evt.client.x, ' y: ', evt.client.y);
           if (this.sock.readyState == this.sock.OPEN) {
           buf = new ArrayBuffer(16);
           a = new Uint32Array(buf);
           a[0] = 0;
           a[1] = 0x0800; // PTR_FLAGS_DOWN
           a[2] = evt.client.x;
           a[3] = evt.client.y;
           this.sock.send(buf);
           }
           */
    },
    onWSmsg: function(evt) {
        var op, hdr, data;
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
                op = new Uint32Array(evt.data, 0, 1);
                switch (op[0]) {
                    case 0:
                        // BeginPaint
                        // wsgate.log.debug('BeginPaint');
                        break;
                    case 1:
                        // EndPaint
                        // wsgate.log.debug('EndPaint');
                        break;
                    case 2:
                        /// Single bitmap
                        //
                        //  0 uint32 Destination X
                        //  1 uint32 Destination Y
                        //  2 uint32 Width
                        //  3 uint32 Height
                        //  4 uint32 Bits per Pixel
                        //  5 uint32 Flag: Compressed
                        //  6 uint32 DataSize
                        //
                        hdr = new Uint32Array(evt.data, 4, 7);
                        bmdata = new Uint8Array(evt.data, 32);
                        compressed =  (hdr[5] != 0);
                        /*
                           wsgate.log.debug('Bitmap:',
                           (compressed ? ' C ' : ' U '),
                           ' x: ', hdr[0], ' y: ', hdr[1],
                           ' w: ', hdr[2], ' h: ', hdr[3],
                           ' bpp: ', hdr[4]
                           );
                           */
                        if ((hdr[4] == 16) || (hdr[4] == 15)) {
                            var outB = this.cctx.createImageData(hdr[2], hdr[3]);
                            if (compressed) {
                                if (!wsgate.dRLE16(bmdata, hdr[6], hdr[2], outB.data)) {
                                    this.cctx.putImageData(outB, hdr[0], hdr[1]);
                                } else {
                                    wsgate.log.warn('Decompression error');
                                }
                            } else {
                                wsgate.dRGB162RGPA(bmdata, hdr[6], outB.data);
                                this.cctx.putImageData(outB, hdr[0], hdr[1]);
                                wsgate.log.warn('Uncompressed not yet implemented');
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
        this.canvas.addEvent('mousemove', this.onMouseMove.bind(this));
        this.canvas.addEvent('mousedown', this.onMouseDown.bind(this), true);
        this.canvas.addEvent('mouseup', this.onMouseUp.bind(this), true);
        this.fireEvent('connected');
    },
    onWSclose: function(evt) {
        this.canvas.removeEvents();
        this.fireEvent('disconnected');
    },
    onWSerr: function (evt) {
        this.canvas.removeEvents();
        wsgate.log.warn(evt);
        this.fireEvent('disconnected');
        this.fireEvent('error', evt);
    }
});
/* DEBUG */
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
/* /DEBUG */

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

wsgate.copyRGBA = function(inA, inI, outA, outI) {
    outA[outI++] = inA[inI++];
    outA[outI++] = inA[inI++];
    outA[outI++] = inA[inI++];
    outA[outI] = inA[inI];
}
wsgate.xorbufRGBAPel16 = function(inA, inI, outA, outI, pel) {
    var pelR = (pel & 0xF800) >> 11;
    var pelG = (pel & 0x7E0) >> 5;
    var pelB = pel & 0x1F;
    // 656 -> 888
    pelR = (pelR << 3 & ~0x7) | (pelR >> 2);
    pelG = (pelG << 2 & ~0x3) | (pelG >> 4);
    pelB = (pelB << 3 & ~0x7) | (pelB >> 2);

    outA[outI++] = inA[inI] ^ pelR;
    outA[outI++] = inA[inI] ^ pelG;
    outA[outI++] = inA[inI] ^ pelB;
    outA[outI] = 255;                                 // alpha
}
wsgate.buf2RGBA = function(inA, inI, outA, outI) {
    var pel = inA[inI] | (inA[inI + 1] << 8);
    var pelR = (pel & 0xF800) >> 11;
    var pelG = (pel & 0x7E0) >> 5;
    var pelB = pel & 0x1F;
    // 656 -> 888
    pelR = (pelR << 3 & ~0x7) | (pelR >> 2);
    pelG = (pelG << 2 & ~0x3) | (pelG >> 4);
    pelB = (pelB << 3 & ~0x7) | (pelB >> 2);

    outA[outI++] = pelR;
    outA[outI++] = pelG;
    outA[outI++] = pelB;
    outA[outI] = 255;                    // alpha
}
wsgate.dRGB162RGPA = function(inA, inLength, outA) {
    var inI = 0;
    var outI = 0;
    while (inI < inLength) {
        wsgate.buf2RGBA(inA, inI, outA, outI);
        inI += 2;
        outI += 4;
    }
}
wsgate.pel2RGBA = function (pel, outA, outI) {
    var pelR = (pel & 0xF800) >> 11;
    var pelG = (pel & 0x7E0) >> 5;
    var pelB = pel & 0x1F;
    // 656 -> 888
    pelR = (pelR << 3 & ~0x7) | (pelR >> 2);
    pelG = (pelG << 2 & ~0x3) | (pelG >> 4);
    pelB = (pelB << 3 & ~0x7) | (pelB >> 2);

    outA[outI++] = pelR;
    outA[outI++] = pelG;
    outA[outI++] = pelB;
    outA[outI] = 255;                    // alpha
}

wsgate.ExtractCodeId = function(bOrderHdr) {
    var code;
    switch (bOrderHdr) {
        case 0xF0:
        case 0xF1:
        case 0xF6:
        case 0xF8:
        case 0xF3:
        case 0xF2:
        case 0xF7:
        case 0xF4:
        case 0xF9:
        case 0xFA:
        case 0xFD:
        case 0xFE:
            return bOrderHdr;
    }
    code = bOrderHdr >> 5;
    switch (code) {
        case 0x00:
        case 0x01:
        case 0x03:
        case 0x02:
        case 0x04:
            return code;
    }
    return bOrderHdr >> 4;
}
wsgate.ExtractRunLength = function(code, inA, inI, advance) {
    var runLength = 0;
    var ladvance = 1;
    switch (code) {
        case 0x02:
            runLength = inA[inI] & 0x1F;
            if (0 == runLength) {
                runLength = inA[inI + 1] + 1;
                ladvance += 1;
            } else {
                runLength *= 8;
            }
            break;
        case 0x0D:
            runLength = inA[inI] & 0x0F;
            if (0 == runLength) {
                runLength = inA[inI + 1] + 1;
                ladvance += 1;
            } else {
                runLength *= 8;
            }
            break;
        case 0x00:
        case 0x01:
        case 0x03:
        case 0x04:
            runLength = inA[inI] & 0x1F;
            if (0 == runLength) {
                runLength = inA[inI + 1] + 32;
                ladvance += 1;
            }
            break;
        case 0x0C:
        case 0x0E:
            runLength = inA[inI] & 0x0F;
            if (0 == runLength) {
                runLength = inA[inI + 1] + 16;
                ladvance += 1;
            }
            break;
        case 0xF0:
        case 0xF1:
        case 0xF6:
        case 0xF8:
        case 0xF3:
        case 0xF2:
        case 0xF7:
        case 0xF4:
            runLength = inA[inI + 1] | (inA[inI + 2] << 8);
            ladvance += 2;
            break;
    }
    advance.val = ladvance;
    return runLength;
}

wsgate.WriteFgBgImage16to16 = function(outA, outI, rowDelta, bitmask, fgPel, cBits) {
    var cmpMask = 0x01;

    while (cBits-- > 0) {
        if (bitmask & cmpMask) {
            wsgate.xorbufRGBAPel16(outA, outI - rowDelta, outA, outI, fgPel);
        } else {
            wsgate.copyRGBA(outA, outI - rowDelta, outA, outI);
        }
        outI += 4;
        cmpMask <<= 1;
    }
    return outI;
}

wsgate.WriteFirstLineFgBgImage16to16 = function(outA, outI, bitmask, fgPel, cBits) {
    var cmpMask = 0x01;

    while (cBits-- > 0) {
        if (bitmask & cmpMask) {
            wsgate.pel2RGBA(fgPel, outA, outI);
        } else {
            wsgate.pel2RGBA(0, outA, outI);
        }
        outI += 4;
        cmpMask <<= 1;
    }
    return outI;
}

wsgate.dRLE16 = function(inA, inLength, width, outA) {
    var runLength;
    var code, pixelA, pixelB, bitmask;
    var inI = 0;
    var outI = 0;
    var fInsertFgPel = false;
    var fFirstLine = true;
    var fgPel = 0xFFFFFF;
    var rowDelta = width * 4;
    var advance = {val: 0};

    while (inI < inLength) {
        if (fFirstLine) {
            if (outI >= rowDelta) {
                fFirstLine = false;
                fInsertFgPel = false;
            }
        }
        code = wsgate.ExtractCodeId(inA[inI]);
        if (code == 0x00 || code == 0xF0) {
            runLength = wsgate.ExtractRunLength(code, inA, inI, advance);
            inI += advance.val;
            if (fFirstLine) {
                if (fInsertFgPel) {
                    wsgate.pel2RGBA(fgPel, outA, outI);
                    outI += 4;
                    runLength -= 1;
                }
                while (runLength-- > 0) {
                    wsgate.pel2RGBA(0, outA, outI);
                    outI += 4;
                }
            } else {
                if (fInsertFgPel) {
                    wsgate.xorbufRGBAPel16(outA, outI - rowDelta, outA, outI, fgPel);
                    outI += 4;
                    runLength -= 1;
                }
                while (runLength-- > 0) {
                    wsgate.copyRGBA(outA, outI - rowDelta, outA, outI);
                    outI += 4;
                }
            }
            fInsertFgPel = true;
            continue;
        }
        fInsertFgPel = false;
        switch (code) {
            case 0x01:
            case 0xF1:
            case 0x0C:
            case 0xF6:
                runLength = wsgate.ExtractRunLength(code, inA, inI, advance);
                inI += advance.val;
                if (code == 0x0C || code == 0xF6) {
                    fgPel = inA[inI] | (inA[inI + 1] << 8);
                    inI += 2;
                }
                if (fFirstLine) {
                    while (runLength-- > 0) {
                        wsgate.pel2RGBA(fgPel, outA, outI);
                        outI += 4;
                    }
                } else {
                    while (runLength-- > 0) {
                        wsgate.xorbufRGBAPel16(outA, outI - rowDelta, outA, outI, fgPel);
                        outI += 4;
                    }
                }
                break;
            case 0x0E:
            case 0xF8:
                runLength = wsgate.ExtractRunLength(code, inA, inI, advance);
                inI += advance.val;
                pixelA = inA[inI] | (inA[inI + 1] << 8);
                inI += 2;
                pixelB = inA[inI] | (inA[inI + 1] << 8);
                inI += 2;
                while (runLength-- > 0) {
                    wsgate.pel2RGBA(pixelA, outA, outI);
                    outI += 4;
                    wsgate.pel2RGBA(pixelB, outA, outI);
                    outI += 4;
                }
                break;
            case 0x03:
            case 0xF3:
                runLength = wsgate.ExtractRunLength(code, inA, inI, advance);
                inI += advance.val;
                pixelA = inA[inI] | (inA[inI + 1] << 8);
                inI += 2;
                while (runLength-- > 0) {
                    wsgate.pel2RGBA(pixelA, outA, outI);
                    outI += 4;
                }
                break;
            case 0x02:
            case 0xF2:
            case 0x0D:
            case 0xF7:
                runLength = wsgate.ExtractRunLength(code, inA, inI, advance);
                inI += advance.val;
                if (code == 0x0D || code == 0xF7) {
                    fgPel = inA[inI] | (inA[inI + 1] << 8);
                    inI += 2;
                }
                if (fFirstLine) {
                    while (runLength >= 8) {
                        bitmask = inA[inI++];
                        outI = wsgate.WriteFirstLineFgBgImage16to16(outA, outI, bitmask, fgPel, 8);
                        runLength -= 8;
                    }
                } else {
                    while (runLength >= 8) {
                        bitmask = inA[inI++];
                        outI = wsgate.WriteFgBgImage16to16(outA, outI, rowDelta, bitmask, fgPel, 8);
                        runLength -= 8;
                    }
                }
                if (runLength-- > 0) {
                    bitmask = inA[inI++];
                    if (fFirstLine) {
                        outI = wsgate.WriteFirstLineFgBgImage16to16(outA, outI, bitmask, fgPel, runLength);
                    } else {
                        outI = wsgate.WriteFgBgImage16to16(outA, outI, rowDelta, bitmask, fgPel, runLength);
                    }
                }
                break;
            case 0x04:
            case 0xF4:
                runLength = wsgate.ExtractRunLength(code, inA, inI, advance);
                inI += advance.val;
                while (runLength-- > 0) {
                    wsgate.pel2RGBA(inA[inI] | (inA[inI + 1] << 8), outA, outI);
                    inI += 2;
                    outI += 4;
                }
                break;
            case 0xF9:
                inI += 1;
                if (fFirstLine) {
                    outI = wsgate.WriteFirstLineFgBgImage16to16(outA, outI, 0x03, fgPel, 8);
                } else {
                    outI = wsgate.WriteFgBgImage16to16(outA, outI, rowDelta, 0x03, fgPel, 8);
                }
                break;
            case 0xFA:
                inI += 1;
                if (fFirstLine) {
                    outI = wsgate.WriteFirstLineFgBgImage16to16(outA, outI, 0x05, fgPel, 8);
                } else {
                    outI = wsgate.WriteFgBgImage16to16(outA, outI, rowDelta, 0x05, fgPel, 8);
                }
                break;
            case 0xFD:
                inI += 1;
                wsgate.pel2RGBA(0xFFFF, outA, outI);
                outI += 4;
                break;
            case 0xFE:
                inI += 1;
                wsgate.pel2RGBA(0, outA, outI);
                outI += 4;
                break;
        }
    }
}

