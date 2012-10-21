var wsgate = wsgate || {}

wsgate.hasconsole = (typeof console !== 'undefined' && 'debug' in console && 'info' in console && 'warn' in console && 'error' in console);

wsgate.o2s = function(obj, depth) {
    depth = depth || [];
    if (depth.contains(obj)) {
        return '{SELF}';
    }
    switch (typeof(obj)) {
        case 'undefined':
            return 'undefined';
        case 'string':
            return '"' + obj.replace(/[\x00-\x1f\\"]/g, escape) + '"';
        case 'array':
            var string = [];
            depth.push(obj);
            for (var i = 0; i < obj.length; ++i) {
                string.push(wsgate.o2s(obj[i], depth));
            }
            depth.pop();
            return '[' + string + ']';
        case 'object':
        case 'hash':
            var string = [];
            depth.push(obj);
            var isE = (obj instanceof UIEvent);
            Object.each(obj, function(v, k) {
                if (v instanceof HTMLElement) {
                    string.push(k + '={HTMLElement}');
                } else if (isE && (('layerX' == k) || ('layerY' == k) ('view' == k))) {
                    string.push(k + '=!0');
                } else {
                    try {
                        var vstr = wsgate.o2s(v, depth);
                        if (vstr) {
                            string.push(k + '=' + vstr);
                        }
                    } catch (error) {
                        string.push(k + '=??E??');
                    }
                }
            });
            depth.pop();
            return '{' + string + '}';
        case 'number':
        case 'boolean':
            return '' + obj;
        case 'null':
            return 'null';
    }
    return null;
};

wsgate.Log = new Class({
    initialize: function() {
        this.ws = null;
    },
    _p: function(pfx, a) {
        var line = '';
        var i;
        for (i = 0; i < a.length; ++i) {
            switch (typeof(a[i])) {
                case 'string':
                case 'number':
                case 'boolean':
                case 'null':
                    line += a[i] + ' ';
                    break;
                default:
                    line += wsgate.o2s(a[i]) + ' ';
                    break;
            }
        }
        if (0 < line.length) {
            this.ws.send(pfx + line);
        }
    },
    drop: function() {
    },
    debug: function() {/* DEBUG */
        if (this.ws) {
            this._p('D:', arguments);
        }
        if (wsgate.hasconsole) {
            try { console.debug.apply(this, arguments); } catch (error) { }
        }
        /* /DEBUG */},
    info: function() {
        if (this.ws) {
            var a = Array.prototype.slice.call(arguments);
            a.unshift('I:');
            this._p.apply(this, a);
        }
        /* DEBUG */if (wsgate.hasconsole) {
            try { console.info.apply(this, arguments); } catch (error) { }
        }/* /DEBUG */
    },
    warn: function() {
        if (this.ws) {
            var a = Array.prototype.slice.call(arguments);
            a.unshift('W:');
            this._p.apply(this, a);
        }
        /* DEBUG */if (wsgate.hasconsole) {
            try { console.warn.apply(this, arguments); } catch (error) { }
        }/* /DEBUG */
    },
    err: function() {
        if (this.ws) {
            var a = Array.prototype.slice.call(arguments);
            a.unshift('E:');
            this._p.apply(this, a);
        }
        /* DEBUG */if (wsgate.hasconsole) {
            try { console.error.apply(this, arguments); } catch (error) { }
        }/* /DEBUG */
    },
    setWS: function(_ws) {
        this.ws = _ws;
    }
});

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
    initialize: function(url, canvas, cssCursor, useTouch, vkbd) {
        this.log = new wsgate.Log();
        this.canvas = canvas;
        this.cctx = canvas.getContext('2d');
        this.cctx.strokeStyle = 'rgba(255,255,255,0)';
        this.cctx.FillStyle = 'rgba(255,255,255,0)';
        this.bstore = new Element('canvas', {
            'width':this.canvas.width,
            'height':this.canvas.height,
        });
        this.bctx = this.bstore.getContext('2d');
        this.aMF = 0;
        this.Tcool = true;
        this.pTe = null;
        this.ccnt = 0;
        this.clx = 0;
        this.cly = 0;
        this.clw = 0;
        this.clh = 0;
        this.mX = 0;
        this.mY = 0;
        this.chx = 10;
        this.chy = 10;
        this.modkeys = [8, 16, 17, 18, 20, 144, 145];
        this.cursors = new Array();
        this.sid = null;
        this.open = false;
        this.cssC = cssCursor;
        this.uT = useTouch;
        if (!cssCursor) {
            this.cI = new Element('img', {
                'src': '/c_default.png',
                'styles': {
                    'position': 'absolute',
                'z-index': 998,
                'left': this.mX - this.chx,
                'top': this.mY - this.chy
                }
            }).inject(document.body);
        }
        if (vkbd) {
            vkbd.addEvent('vkpress', this.onKv.bind(this));
        }
        this.parent(url);
    },
    Disconnect: function() {
        this._reset();
    },
    /**
     * Position cursor image
     */
    cP: function() {
        this.cI.setStyles({'left': this.mX - this.chx, 'top': this.mY - this.chy});
    },
    /**
     * Check, if a given point is inside the clipping region.
     */
    _ckclp: function(x, y) {
        if (this.clw || this.clh) {
            return (
                    (x >= this.clx) &&
                    (x <= (this.clx + this.clw)) &&
                    (y >= this.cly) &&
                    (y <= (this.cly + this.clh))
                   );
        }
        // No clipping region
        return true;
    },
    /**
     * Main message loop.
     */
    _pmsg: function(data) { // process a binary RDP message from our queue
        var op, hdr, count, rects, bmdata, rgba, compressed, i, offs, x, y, sx, sy, w, h, dw, dh, bpp, color, len;
        op = new Uint32Array(data, 0, 1);
        switch (op[0]) {
            case 0:
                // BeginPaint
                // this.log.debug('BeginPaint');
                this._ctxS();
                break;
            case 1:
                // EndPaint
                // this.log.debug('EndPaint');
                this._ctxR();
                break;
            case 2:
                // Single bitmap
                //
                //  0 uint32 Destination X
                //  1 uint32 Destination Y
                //  2 uint32 Width
                //  3 uint32 Height
                //  4 uint32 Destination Width
                //  5 uint32 Destination Height
                //  6 uint32 Bits per Pixel
                //  7 uint32 Flag: Compressed
                //  8 uint32 DataSize
                //
                hdr = new Uint32Array(data, 4, 9);
                bmdata = new Uint8Array(data, 40);
                x = hdr[0];
                y = hdr[1];
                w = hdr[2];
                h = hdr[3];
                dw = hdr[4];
                dh = hdr[5];
                bpp = hdr[6];
                compressed =  (hdr[7] != 0);
                len = hdr[8];
                if ((bpp == 16) || (bpp == 15)) {
                    if (this._ckclp(x, y) && this._ckclp(x + dw, y + dh)) {
                        // this.log.debug('BMi:',(compressed ? ' C ' : ' U '),' x=',x,'y=',y,' w=',w,' h=',h,' l=',len);
                        var outB = this.cctx.createImageData(w, h);
                        if (compressed) {
                            wsgate.dRLE16_RGBA(bmdata, len, w, outB.data);
                            wsgate.flipV(outB.data, w, h);
                        } else {
                            wsgate.dRGB162RGBA(bmdata, len, outB.data);
                        }
                        this.cctx.putImageData(outB, x, y, 0, 0, dw, dh);
                    } else {
                        // this.log.debug('BMc:',(compressed ? ' C ' : ' U '),' x=',x,'y=',y,' w=',w,' h=',h,' bpp=',bpp);
                        // putImageData ignores the clipping region, so we must
                        // clip ourselves: We first paint into a second canvas,
                        // then use drawImage (which honors clipping).

                        var outB = this.bctx.createImageData(w, h);
                        if (compressed) {
                            wsgate.dRLE16_RGBA(bmdata, len, w, outB.data);
                            wsgate.flipV(outB.data, w, h);
                        } else {
                            wsgate.dRGB162RGBA(bmdata, len, outB.data);
                        }
                        this.bctx.putImageData(outB, 0, 0, 0, 0, dw, dh);
                        this.cctx.drawImage(this.bstore, 0, 0, dw, dh, x, y, dw, dh);
                    }
                } else {
                    this.log.warn('BPP <> 15/16 not yet implemented');
                }
                break;
            case 3:
                // Primary: OPAQUE_RECT_ORDER
                // x, y , w, h, color
                hdr = new Int32Array(data, 4, 4);
                rgba = new Uint8Array(data, 20, 4);
                // this.log.debug('Fill:',hdr[0], hdr[1], hdr[2], hdr[3], this._c2s(rgba));
                this.cctx.fillStyle = this._c2s(rgba);
                this.cctx.fillRect(hdr[0], hdr[1], hdr[2], hdr[3]);
                break;
            case 4:
                // SetBounds
                // left, top, right, bottom
                hdr = new Int32Array(data, 4, 4);
                this._cR(hdr[0], hdr[1], hdr[2] - hdr[0], hdr[3] - hdr[1], true);
                break; 
            case 5:
                // PatBlt
                if (28 == data.byteLength) {
                    // Solid brush style
                    // x, y, width, height, fgcolor, rop3
                    hdr = new Int32Array(data, 4, 4);
                    x = hdr[0];
                    y = hdr[1];
                    w = hdr[2];
                    h = hdr[3];
                    rgba = new Uint8Array(data, 20, 4);
                    this._ctxS();
                    this._cR(x, y, w, h, false);
                    if (this._sROP(new Uint32Array(data, 24, 1)[0])) {
                        this.cctx.fillStyle = this._c2s(rgba);
                        this.cctx.fillRect(x, y, w, h);
                    }
                    this._ctxR();
                } else {
                    this.log.warn('PatBlt: Patterned brush not yet implemented');
                }
                break; 
            case 6:
                // Multi Opaque rect
                // color, nrects
                // rect1.x,rect1.y,rect1.w,rect1.h ... rectn.x,rectn.y,rectn.w,rectn.h
                rgba = new Uint8Array(data, 4, 4);
                count = new Uint32Array(data, 8, 1);
                rects = new Uint32Array(data, 12, count[0] * 4);
                // this.log.debug('MultiFill: ', count[0], " ", this._c2s(rgba));
                this.cctx.fillStyle = this._c2s(rgba);
                offs = 0;
                // var c = this._c2s(rgba);
                for (i = 0; i < count[0]; ++i) {
                    this.cctx.fillRect(rects[offs], rects[offs+1], rects[offs+2], rects[offs+3]);
                    // this._fR(rects[offs], rects[offs+1], rects[offs+2], rects[offs+3], c);
                    offs += 4;
                }
                break; 
            case 7:
                // ScrBlt
                // rop3, x, y, w, h, sx, sy
                hdr = new Int32Array(data, 8, 6);
                x = hdr[0];
                y = hdr[1];
                w = hdr[2];
                h = hdr[3];
                sx = hdr[4];
                sy = hdr[5];
                if ((w > 0) && (h > 0)) {
                    if (this._sROP(new Uint32Array(data, 4, 1)[0])) {
                        if (this._ckclp(x, y) && this._ckclp(x + w, y + h)) {
                            // No clipping necessary
                            this.cctx.putImageData(this.cctx.getImageData(sx, sy, w, h), x, y);
                        } else {
                            // Clipping necessary
                            this.bctx.putImageData(this.cctx.getImageData(sx, sy, w, h), 0, 0);
                            this.cctx.drawImage(this.bstore, 0, 0, w, h, x, y, w, h);
                        }
                    }
                } else {
                    this.log.warn('ScrBlt: width and/or height is zero');
                }
                break; 
            case 8:
                // PTR_NEW
                // id, xhot, yhot
                hdr = new Uint32Array(data, 4, 3);
                if (this.cssC) {
                    this.cursors[hdr[0]] = 'url(/cur/'+this.sid+'/'+hdr[0]+') '+hdr[1]+' '+hdr[2]+',none';
                } else {
                    this.cursors[hdr[0]] = {u: '/cur/'+this.sid+'/'+hdr[0], x: hdr[1], y: hdr[2]};
                }
                break;
            case 9:
                // PTR_FREE
                // id
                this.cursors[new Uint32Array(data, 4, 1)[0]] = undefined;
                break;
            case 10:
                // PTR_SET
                // id
                // this.log.debug('PS:', this.cursors[new Uint32Array(data, 4, 1)[0]]);
                if (this.cssC) {
                    this.canvas.setStyle('cursor', this.cursors[new Uint32Array(data, 4, 1)[0]]);
                } else {
                    var cobj = this.cursors[new Uint32Array(data, 4, 1)[0]];
                    this.chx = cobj.x;
                    this.chy = cobj.y;
                    this.cI.src = cobj.u;
                }
                break;
            case 11:
                // PTR_SETNULL
                if (this.cssC) {
                    this.canvas.setStyle('cursor', 'none');
                } else {
                    this.cI.src = '/c_none.png';
                }
                break;
            case 12:
                // PTR_SETDEFAULT
                if (this.cssC) {
                    this.canvas.setStyle('cursor', 'default');
                } else {
                    this.chx = 10;
                    this.chy = 10;
                    this.cI.src = '/c_default.png';
                }
                break;
            default:
                this.log.warn('Unknown BINRESP: ', data.byteLength);
        }
    },
    _cR: function(x, y, w, h, save) {
        if (save) {
            this.clx = x;
            this.cly = y;
            this.clw = w;
            this.clh = h;
        }
        // Replace clipping region, NO intersection.
        this.cctx.beginPath();
        this.cctx.rect(0, 0, this.canvas.width, this.canvas.height);
        this.cctx.clip();
        if (x == y == 0) {
            // All zero means: reset to full canvas size
            if ((w == h == 0) || ((w == this.canvas.width) && (h == this.canvas.height))) {
                return;
            }
        }
        // New clipping region
        this.cctx.beginPath();
        this.cctx.rect(x, y, w, h);
        this.cctx.clip();
    },
    _fR: function(x, y, w, h, color) {
        return;
        if ((w < 2) || (h < 2)) {
            this.cctx.strokeStyle = color;
            this.cctx.beginPath();
            this.cctx.moveTo(x, y);
            if (w > h) {
                this.cctx.lineWidth = h;
                this.cctx.lineTo(x + w, y);
            } else {
                this.cctx.lineWidth = w;
                this.cctx.lineTo(x, y + h);
            }
            this.cctx.stroke();
        } else {
            this.cctx.fillStyle = color;
            this.cctx.fillRect(x, y, w, h);
        }
    },
    _sROP: function(rop) {
        switch (rop) {
            case 0x005A0049:
                // GDI_PATINVERT: D = P ^ D
                this.cctx.globalCompositeOperation = 'xor';
                return true;
                break;
            case 0x00F00021:
                // GDI_PATCOPY: D = P
                this.cctx.globalCompositeOperation = 'copy';
                return true;
                break;
            case 0x00CC0020:
                // GDI_SRCCOPY: D = S
                this.cctx.globalCompositeOperation = 'source-over';
                return true;
                break;
            default:
                this.log.warn('Unsupported raster op: ', rop.toString(16));
                break;
        }
        return false;
        /*
           case 0x00EE0086:
        // GDI_SRCPAINT: D = S | D
        break;
        case 0x008800C6:
        // GDI_SRCAND: D = S & D
        break;
        case 0x00660046:
        // GDI_SRCINVERT: D = S ^ D
        break;
        case 0x00440328:
        // GDI_SRCERASE: D = S & ~D
        break;
        case 0x00330008:
        // GDI_NOTSRCCOPY: D = ~S
        break;
        case 0x001100A6:
        // GDI_NOTSRCERASE: D = ~S & ~D
        break;
        case 0x00C000CA:
        // GDI_MERGECOPY: D = S & P
        break;
        case 0x00BB0226:
        // GDI_MERGEPAINT: D = ~S | D
        break;
        case 0x00FB0A09:
        // GDI_PATPAINT: D = D | (P | ~S)
        break;
        case 0x00550009:
        // GDI_DSTINVERT: D = ~D
        break;
        case 0x00000042:
        // GDI_BLACKNESS: D = 0
        break;
        case 0x00FF0062:
        // GDI_WHITENESS: D = 1
        break;
        case 0x00E20746:
        // GDI_DSPDxax: D = (S & P) | (~S & D)
        break;
        case 0x00B8074A:
        // GDI_PSDPxax: D = (S & D) | (~S & P)
        break;
        case 0x000C0324:
        // GDI_SPna: D = S & ~P
        break;
        case 0x00220326:
        // GDI_DSna D = D & ~S
        break;
        case 0x00220326:
        // GDI_DSna: D = D & ~S
        break;
        case 0x00A000C9:
        // GDI_DPa: D = D & P
        break;
        case 0x00A50065:
        // GDI_PDxn: D = D ^ ~P
        break;
        */
    },
    /**
     * Reset our state to disconnected
     */
    _reset: function() {
        this.log.setWS(null);
        if (this.sock.readyState == this.sock.OPEN) {
            this.fireEvent('disconnected');
            this.sock.close();
        }
        this.clx = 0;
        this.cly = 0;
        this.clw = 0;
        this.clh = 0;
        this.canvas.removeEvents();
        document.removeEvents();
        while (this.ccnt > 0) {
            this.cctx.restore();
            this.ccnt -= 1;
        }
        this.cctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
        document.title = document.title.replace(/:.*/, ': offline');
        if (this.cssC) {
            this.canvas.setStyle('cursor','default');
        } else {
            this.cI.src = '/c_default.png';
        }
        if (!this.cssC) {
            this.cI.removeEvents();
            this.cI.destroy();
        }
    },
    fT: function() {
        delete this.fTid;
        if (this.pT) {
            this.fireEvent('touch' + this.pT);
            this.pT = 0;
            return;
        }
        if (this.pTe) {
            this.onMd(this.pTe);
            this.pTe = null;
        }
    },
    cT: function() {
        this.log.debug('cT');
        this.Tcool = true;
    },
    /**
     * Event handler for touch start
     */
    onTs: function(evt) {
        var tn = evt.targetTouches.length;
        this.log.debug('Ts:', tn);
        switch (tn) {
            default:
                break;
            case 1:
                this.pTe = evt;
                evt.preventDefault();
                if ('number' == typeof(this.fTid)) {
                    clearTimeout(this.fTid);
                }
                this.fTid = this.fT.delay(50, this);
                break;
            case 2:
                this.pT = 2;
                this.Tcool = false;
                evt.preventDefault();
                if ('number' == typeof(this.fTid)) {
                    clearTimeout(this.fTid);
                }
                this.cT.delay(500, this)
                this.fTid = this.fT.delay(50, this);
                break;
            case 3:
                this.pT = 3;
                this.Tcool = false;
                evt.preventDefault();
                if ('number' == typeof(this.fTid)) {
                    clearTimeout(this.fTid);
                }
                this.cT.delay(500, this)
                this.fTid = this.fT.delay(50, this);
                break;
            case 4:
                this.pT = 4;
                this.Tcool = false;
                evt.preventDefault();
                if ('number' == typeof(this.fTid)) {
                    clearTimeout(this.fTid);
                }
                this.cT.delay(500, this)
                this.fTid = this.fT.delay(50, this);
                break;
        }
        return true;
    },
    /**
     * Event handler for touch start
     */
    onTe: function(evt) {
        if ((0 == evt.targetTouches.length) && this.Tcool) {
            evt.preventDefault();
            this.onMu(evt, evt.changedTouches[0].pageX, evt.changedTouches[0].pageY);
        }
    },
    /**
     * Event handler for touch move
     */
    onTm: function(evt) {
        // this.log.debug('Tm:', evt);
        if (1 == evt.targetTouches.length) {
            this.onMm(evt);
        }
    },
    /**
     * Event handler for mouse move events
     */
    onMm: function(evt) {
        var buf, a, x, y;
        evt.preventDefault();
        x = evt.event.layerX;
        y = evt.event.layerY;
        if (!this.cssC) {
            this.mX = x;
            this.mY = y;
            this.cP();
        }
        // this.log.debug('mM x: ', x, ' y: ', y);
        if (this.sock.readyState == this.sock.OPEN) {
            buf = new ArrayBuffer(16);
            a = new Uint32Array(buf);
            a[0] = 0; // WSOP_CS_MOUSE
            a[1] = 0x0800; // PTR_FLAGS_MOVE
            a[2] = x;
            a[3] = y;
            this.sock.send(buf);
        }
    },
    /**
     * Event handler for mouse down events
     */
    onMd: function(evt) {
        var buf, a, x, y, which;
        if (this.Tcool) {
            evt.preventDefault();
            if (evt.rightClick && evt.control && evt.alt) {
                this.fireEvent('touch3');
                return;
            }
            x = evt.event.layerX;
            y = evt.event.layerY;
            which = this._mB(evt);
            this.log.debug('mD b: ', which, ' x: ', x, ' y: ', y);
            if (this.sock.readyState == this.sock.OPEN) {
                buf = new ArrayBuffer(16);
                a = new Uint32Array(buf);
                a[0] = 0; // WSOP_CS_MOUSE
                a[1] = 0x8000 | which;
                a[2] = x;
                a[3] = y;
                this.sock.send(buf);
            }
        }
    },
    /**
     * Event handler for mouse up events
     */
    onMu: function(evt, x, y) {
        var buf, a, x, y, which;
        if (this.Tcool) {
            evt.preventDefault();
            x = x || evt.event.layerX;
            y = y || evt.event.layerY;
            which = this._mB(evt);
            this.log.debug('mU b: ', which, ' x: ', x, ' y: ', y);
            if (this.aMF) {
                this.fireEvent('mouserelease');
            }
            if (this.sock.readyState == this.sock.OPEN) {
                buf = new ArrayBuffer(16);
                a = new Uint32Array(buf);
                a[0] = 0; // WSOP_CS_MOUSE
                a[1] = which;
                a[2] = x;
                a[3] = y;
                this.sock.send(buf);
            }
        }
    },
    /**
     * Event handler for mouse wheel events
     */
    onMw: function(evt) {
        var buf, a, x, y;
        evt.preventDefault();
        x = evt.event.layerX;
        y = evt.event.layerY;
        // this.log.debug('mW d: ', evt.wheel, ' x: ', x, ' y: ', y);
        if (this.sock.readyState == this.sock.OPEN) {
            buf = new ArrayBuffer(16);
            a = new Uint32Array(buf);
            a[0] = 0; // WSOP_CS_MOUSE
            a[1] = 0x200 | ((evt.wheel > 0) ? 0x087 : 0x188);
            a[2] = 0;
            a[3] = 0;
            this.sock.send(buf);
        }
    },
    /**
     * Event handler for key down events
     */
    onKd: function(evt) {
        var a, buf;
        if (this.modkeys.contains(evt.code)) {
            evt.preventDefault();
            // this.log.debug('kD code: ', evt.code, ' ', evt);
            if (this.sock.readyState == this.sock.OPEN) {
                buf = new ArrayBuffer(12);
                a = new Uint32Array(buf);
                a[0] = 1; // WSOP_CS_KUPDOWN
                a[1] = 1; // down
                a[2] = evt.code;
                this.sock.send(buf);
            }
        }
    },
    /**
     * Event handler for key up events
     */
    onKu: function(evt) {
        var a, buf;
        if (this.modkeys.contains(evt.code)) {
            evt.preventDefault();
            // this.log.debug('kU code: ', evt.code);
            if (this.sock.readyState == this.sock.OPEN) {
                buf = new ArrayBuffer(12);
                a = new Uint32Array(buf);
                a[0] = 1; // WSOP_CS_KUPDOWN
                a[1] = 0; // up
                a[2] = evt.code;
                this.sock.send(buf);
            }
        }
    },
    /**
     * Event handler for virtual keyboard
     */
    onKv: function(evt) {
        var a, buf;
        if (this.sock.readyState == this.sock.OPEN) {
            // this.log.debug('kP code: ', evt.code);
            buf = new ArrayBuffer(12);
            a = new Uint32Array(buf);
            if (evt.special) {
                a[0] = 1; // WSOP_CS_KUPDOWN
                a[1] = 1; // down
                a[2] = evt.code;
                this.sock.send(buf);
                a[0] = 1; // WSOP_CS_KUPDOWN
                a[1] = 0; // up
                a[2] = evt.code;
            } else {
                a[0] = 2; // WSOP_CS_KPRESS
                a[1] = (evt.shift ? 1 : 0)|(evt.control ? 2 : 0)|(evt.alt ? 4 : 0)|(evt.meta ? 8 : 0);
                a[2] = evt.code;
            }
            this.sock.send(buf);
        }
    },
    /**
     * Event handler for key pressed events
     */
    onKp: function(evt) {
        var a, buf;
        evt.preventDefault();
        if (this.modkeys.contains(evt.code)) {
            return;
        }
        if (this.sock.readyState == this.sock.OPEN) {
            // this.log.debug('kP code: ', evt.code);
            buf = new ArrayBuffer(12);
            a = new Uint32Array(buf);
            a[0] = 2; // WSOP_CS_KPRESS
            a[1] = (evt.shift ? 1 : 0)|(evt.control ? 2 : 0)|(evt.alt ? 4 : 0)|(evt.meta ? 8 : 0);
            a[2] = evt.code;
            this.sock.send(buf);
        }
    },
    /**
     * Event handler for WebSocket RX events
     */
    onWSmsg: function(evt) {
        switch (typeof(evt.data)) {
            // We use text messages for alerts and debugging ...
            case 'string':
                // this.log.debug(evt.data);
                switch (evt.data.substr(0,2)) {
                    case "T:":
                            this._reset();
                            break;
                    case "E:":
                            this.log.err(evt.data.substring(2));
                            this.fireEvent('alert', evt.data.substring(2));
                            this._reset();
                            break;
                    case 'I:':
                            this.log.info(evt.data.substring(2));
                            break;
                    case 'W:':
                            this.log.warn(evt.data.substring(2));
                            break;
                    case 'D:':
                            this.log.debug(evt.data.substring(2));
                            break;
                    case 'S:':
                            this.sid = evt.data.substring(2);
                            break;
                }
                break;
                // ... and binary messages for the actual RDP stuff.
            case 'object':
                this._pmsg(evt.data);
                break;
        }

    },
    /**
     * Event handler for WebSocket connect events
     */
    onWSopen: function(evt) {
        this.open = true;
        this.log.setWS(this.sock);
        // Add listeners for the various input events
        this.canvas.addEvent('mousemove', this.onMm.bind(this));
        this.canvas.addEvent('mousedown', this.onMd.bind(this));
        this.canvas.addEvent('mouseup', this.onMu.bind(this));
        this.canvas.addEvent('mousewheel', this.onMw.bind(this));
        // Disable the browser's context menu
        this.canvas.addEvent('contextmenu', function(e) {e.stop();});
        // For touch devices
        if (this.uT) {
            this.canvas.addEvent('touchstart', this.onTs.bind(this));
            this.canvas.addEvent('touchend', this.onTe.bind(this));
            this.canvas.addEvent('touchmove', this.onTm.bind(this));
        }
        if (!this.cssC) {
            // Same events on pointer image
            this.cI.addEvent('mousemove', this.onMm.bind(this));
            this.cI.addEvent('mousedown', this.onMd.bind(this));
            this.cI.addEvent('mouseup', this.onMu.bind(this));
            this.cI.addEvent('mousewheel', this.onMw.bind(this));
            this.cI.addEvent('contextmenu', function(e) {e.stop();});
            if (this.uT) {
                this.cI.addEvent('touchstart', this.onTs.bind(this));
                this.cI.addEvent('touchend', this.onTe.bind(this));
                this.cI.addEvent('touchmove', this.onTm.bind(this));
            }
        }
        // The keyboard events need to be attached to the
        // document, because otherwise we seem to loose them.
        document.addEvent('keydown', this.onKd.bind(this));
        document.addEvent('keyup', this.onKu.bind(this));
        document.addEvent('keypress', this.onKp.bind(this));
        this.fireEvent('connected');
    },
    /**
     * Event handler for WebSocket disconnect events
     */
    onWSclose: function(evt) {
        if (Browser.name == 'chrome') {
            // Current chrome is buggy in that it does not
            // fire WebSockets error events, so we use the
            // wasClean flag in the close event.
            if ((!evt.wasClean) && (!this.open)) {
                this.fireEvent('alert', 'Could not connect to WebSockets gateway');
            }
        }
        this.open = false;
        this._reset();
        this.fireEvent('disconnected');
    },
    /**
     * Event handler for WebSocket error events
     */
    onWSerr: function (evt) {
        this.open = false;
        switch (this.sock.readyState) {
            case this.sock.CONNECTING:
                this.fireEvent('alert', 'Could not connect to WebSockets gateway');
                break;
        }
        this._reset();
    },
    /**
     * Convert a color value contained in an uint8 array into an rgba expression
     * that can be used to parameterize the canvas.
     */
    _c2s: function(c) {
        return 'rgba' + '(' + c[0] + ',' + c[1] + ',' + c[2] + ',' + ((0.0 + c[3]) / 255) + ')';
    },
    /**
     * Save the canvas state and remember this in our object.
     */
    _ctxS: function() {
        this.cctx.save();
        this.ccnt += 1;
    },
    /**
     * Restore the canvas state and remember this in our object.
     */
    _ctxR: function() {
        this.cctx.restore();
        this.ccnt -= 1;
    },
    /**
     * Convert the button information of a mouse event into
     * RDP-like flags.
     */
    _mB: function(evt) {
        if (this.aMF) {
            return this.aMF;
        }
        var bidx;
        if ('event' in evt && 'button' in evt.event) {
            bidx = evt.event.button;
        } else {
            bidx = evt.rightClick ? 2 : 0;
        }
        switch (bidx) {
            case 0:
                return 0x1000; // left button
            case 1:
                return 0x4000; // middle button
            case 2:
                return 0x2000; // right button
        }
        return 0x1000;
    },
    SetArtificialMouseFlags: function(mf) {
        if (null == mf) {
            this.aMF = 0;
            return;
        }
        this.aMF = 0x1000; // left button
        if (mf.r) {
            this.aMF = 0x2000; // right
        }
        if (mf.m) {
            this.aMF = 0x4000; // middle
        }
    }
});
wsgate.copyRGBA = function(inA, inI, outA, outI) {
    if ('subarray' in inA) {
        outA.set(inA.subarray(inI, inI + 4), outI);
    } else {
        outA[outI++] = inA[inI++];
        outA[outI++] = inA[inI++];
        outA[outI++] = inA[inI++];
        outA[outI] = inA[inI];
    }
}
wsgate.xorbufRGBAPel16 = function(inA, inI, outA, outI, pel) {
    var pelR = (pel & 0xF800) >> 11;
    var pelG = (pel & 0x7E0) >> 5;
    var pelB = pel & 0x1F;
    // 656 -> 888
    pelR = (pelR << 3 & ~0x7) | (pelR >> 2);
    pelG = (pelG << 2 & ~0x3) | (pelG >> 4);
    pelB = (pelB << 3 & ~0x7) | (pelB >> 2);

    outA[outI++] = inA[inI++] ^ pelR;
    outA[outI++] = inA[inI++] ^ pelG;
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

wsgate.flipV = function(inA, width, height) {
    var sll = width * 4;
    var half = height / 2;
    var lbot = sll * (height - 1);
    var ltop = 0;
    var tmp = new Uint8Array(sll);
    var i, j;
    if ('subarray' in inA) {
        for (i = 0; i < half ; ++i) {
            tmp.set(inA.subarray(ltop, ltop + sll));
            inA.set(inA.subarray(lbot, lbot + sll), ltop);
            inA.set(tmp, lbot);
            ltop += sll;
            lbot -= sll;
        }
    } else {
        for (i = 0; i < half ; ++i) {
            for (j = 0; j < sll; ++j) {
                tmp[j] = inA[ltop + j];
                inA[ltop + j] = inA[lbot + j];
                inA[lbot + j] = tmp[j];
            }
            ltop += sll;
            lbot -= sll;
        }
    }
}

wsgate.dRGB162RGBA = function(inA, inLength, outA) {
    var inI = 0;
    var outI = 0;
    while (inI < inLength) {
        wsgate.buf2RGBA(inA, inI, outA, outI);
        inI += 2;
        outI += 4;
    }
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

wsgate.WriteFgBgImage16toRGBA = function(outA, outI, rowDelta, bitmask, fgPel, cBits) {
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

wsgate.WriteFirstLineFgBgImage16toRGBA = function(outA, outI, bitmask, fgPel, cBits) {
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

wsgate.dRLE16_RGBA = function(inA, inLength, width, outA) {
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
                while (runLength > 0) {
                    wsgate.pel2RGBA(0, outA, outI);
                    runLength -= 1;
                    outI += 4;
                }
            } else {
                if (fInsertFgPel) {
                    wsgate.xorbufRGBAPel16(outA, outI - rowDelta, outA, outI, fgPel);
                    outI += 4;
                    runLength -= 1;
                }
                while (runLength > 0) {
                    wsgate.copyRGBA(outA, outI - rowDelta, outA, outI);
                    runLength -= 1;
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
                    while (runLength > 0) {
                        wsgate.pel2RGBA(fgPel, outA, outI);
                        runLength -= 1;
                        outI += 4;
                    }
                } else {
                    while (runLength > 0) {
                        wsgate.xorbufRGBAPel16(outA, outI - rowDelta, outA, outI, fgPel);
                        runLength -= 1;
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
                while (runLength > 0) {
                    wsgate.pel2RGBA(pixelA, outA, outI);
                    outI += 4;
                    wsgate.pel2RGBA(pixelB, outA, outI);
                    outI += 4;
                    runLength -= 1;
                }
                break;
            case 0x03:
            case 0xF3:
                runLength = wsgate.ExtractRunLength(code, inA, inI, advance);
                inI += advance.val;
                pixelA = inA[inI] | (inA[inI + 1] << 8);
                inI += 2;
                while (runLength > 0) {
                    wsgate.pel2RGBA(pixelA, outA, outI);
                    outI += 4;
                    runLength -= 1;
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
                        outI = wsgate.WriteFirstLineFgBgImage16toRGBA(outA, outI, bitmask, fgPel, 8);
                        runLength -= 8;
                    }
                } else {
                    while (runLength >= 8) {
                        bitmask = inA[inI++];
                        outI = wsgate.WriteFgBgImage16toRGBA(outA, outI, rowDelta, bitmask, fgPel, 8);
                        runLength -= 8;
                    }
                }
                if (runLength > 0) {
                    bitmask = inA[inI++];
                    if (fFirstLine) {
                        outI = wsgate.WriteFirstLineFgBgImage16toRGBA(outA, outI, bitmask, fgPel, runLength);
                    } else {
                        outI = wsgate.WriteFgBgImage16toRGBA(outA, outI, rowDelta, bitmask, fgPel, runLength);
                    }
                }
                break;
            case 0x04:
            case 0xF4:
                runLength = wsgate.ExtractRunLength(code, inA, inI, advance);
                inI += advance.val;
                while (runLength > 0) {
                    wsgate.pel2RGBA(inA[inI] | (inA[inI + 1] << 8), outA, outI);
                    inI += 2;
                    outI += 4;
                    runLength -= 1;
                }
                break;
            case 0xF9:
                inI += 1;
                if (fFirstLine) {
                    outI = wsgate.WriteFirstLineFgBgImage16toRGBA(outA, outI, 0x03, fgPel, 8);
                } else {
                    outI = wsgate.WriteFgBgImage16toRGBA(outA, outI, rowDelta, 0x03, fgPel, 8);
                }
                break;
            case 0xFA:
                inI += 1;
                if (fFirstLine) {
                    outI = wsgate.WriteFirstLineFgBgImage16toRGBA(outA, outI, 0x05, fgPel, 8);
                } else {
                    outI = wsgate.WriteFgBgImage16toRGBA(outA, outI, rowDelta, 0x05, fgPel, 8);
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

