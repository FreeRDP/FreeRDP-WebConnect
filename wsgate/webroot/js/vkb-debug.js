/*
 * This code is based on:
 * HTML Virtual Keyboard Interface Script - v1.49
 *   Copyright (c) 2011 - GreyWyvern
 *  - Licensed for free distribution under the BSDL
 *          http://www.opensource.org/licenses/bsd-license.php
 * Found at: http://www.greywyvern.com/code/javascript/keyboard
 *
 * Extensive modfications and mootoolization:
 * Copyright (c) 2012 Fritz Elfert
 */

var wsgate = wsgate || {}

wsgate.vkbd = new Class({
    Implements: [Options, Events],
    options: {
        target: null,
        version: true,              // Show the version
        deadcheck: true,            // Show dead keys checkbox
        deadkeys: false,            // Turn dead keys on by default
        numpadtoggle: true,         // Show numpad toggle
        numpad: true,               // Show number pad by default
        layouts: ['en_US','de_DE'], // Keyboard layouts to load
        deflayout: 'de_DE',         // Default keyboard layout
        size: 5,                    // Initial size
        sizeswitch: true,           // Show size-adjust controls
        hoverclick: 0,              // Click a key after n ms (0 = off)
        clicksound: '/vkbclick.ogg'
    },
    initialize: function(options) {
        this.setOptions(options);
        this.VERSION = '1.49';
        this.posX = 100;
        this.posY = 100;
        this.dragging = false;
        this.VKI_shift = this.VKI_shiftlock = false;
        this.VKI_altgr = this.VKI_altgrlock = false;
        this.VKI_alt = this.VKI_altlock = false;
        this.VKI_ctrl = this.VKI_ctrllock = false;
        this.VKI_dead = false; // Flag: dead key active
        this.VKI_kt = 'US International';
        this.VKI_size = this.options.size;
        this.VKI_keyCenter = 3;
        /* ***** i18n text strings ************************************* */
        this.VKI_i18n = {
            '00': 'Display Number Pad',
            '01': 'Display virtual keyboard interface',
            '02': 'Select keyboard layout',
            '03': 'Dead keys',
            '04': 'On',
            '05': 'Off',
            '06': 'Close the keyboard',
            '07': 'Clear',
            '08': 'Clear this input',
            '09': 'Version',
            '10': 'Decrease keyboard size',
            '11': 'Increase keyboard size'
        };

        // Load layouts
        this.VKI_layout = {};
        this.options.layouts.each(function(n) {
            new Request.JSON({
                'url': '/js/vkbl-' + n + '.json',
                'method': 'get',
                'async': false,
                'onSuccess': function(obj) {
                    this.VKI_layout[obj.displayname] = obj;
                    if (n == this.options.deflayout) {
                        this.VKI_kt = obj.displayname;
                    }
                }.bind(this)
            }).addEvent('error', function(txt, err) {
                console.debug('err:', err);
                console.debug('txt:', txt);
            }).send();
        }, this);
        /*
        if (this.options.clicksound) {
            this.audio = new Audio(this.options.clicksound);
        }
        */
        this.VKI_deadkey = {};
        this.VKI_deadkey['"'] = this.VKI_deadkey['\u00a8'] = this.VKI_deadkey['\u309B'] = { // Umlaut / Diaeresis / Greek Dialytika / Hiragana/Katakana Voiced Sound Mark
            'a': '\u00e4', 'e': '\u00eb', 'i': '\u00ef', 'o': '\u00f6', 'u': '\u00fc', 'y': '\u00ff', '\u03b9': '\u03ca', '\u03c5': '\u03cb',
            '\u016B': '\u01D6', '\u00FA': '\u01D8', '\u01D4': '\u01DA', '\u00F9': '\u01DC', 'A': '\u00c4', 'E': '\u00cb', 'I': '\u00cf',
            'O': '\u00d6', 'U': '\u00dc', 'Y': '\u0178', '\u0399': '\u03aa', '\u03a5': '\u03ab', '\u016A': '\u01D5', '\u00DA': '\u01D7',
            '\u01D3': '\u01D9', '\u00D9': '\u01DB', '\u304b': '\u304c', '\u304d': '\u304e', '\u304f': '\u3050', '\u3051': '\u3052',
            '\u3053': '\u3054', '\u305f': '\u3060', '\u3061': '\u3062', '\u3064': '\u3065', '\u3066': '\u3067', '\u3068': '\u3069',
            '\u3055': '\u3056', '\u3057': '\u3058', '\u3059': '\u305a', '\u305b': '\u305c', '\u305d': '\u305e', '\u306f': '\u3070',
            '\u3072': '\u3073', '\u3075': '\u3076', '\u3078': '\u3079', '\u307b': '\u307c', '\u30ab': '\u30ac', '\u30ad': '\u30ae',
            '\u30af': '\u30b0', '\u30b1': '\u30b2', '\u30b3': '\u30b4', '\u30bf': '\u30c0', '\u30c1': '\u30c2', '\u30c4': '\u30c5',
            '\u30c6': '\u30c7', '\u30c8': '\u30c9', '\u30b5': '\u30b6', '\u30b7': '\u30b8', '\u30b9': '\u30ba', '\u30bb': '\u30bc',
            '\u30bd': '\u30be', '\u30cf': '\u30d0', '\u30d2': '\u30d3', '\u30d5': '\u30d6', '\u30d8': '\u30d9', '\u30db': '\u30dc'
        };
        this.VKI_deadkey['~'] = { // Tilde / Stroke
            'a': '\u00e3', 'l': '\u0142', 'n': '\u00f1', 'o': '\u00f5',
            'A': '\u00c3', 'L': '\u0141', 'N': '\u00d1', 'O': '\u00d5'
        };
        this.VKI_deadkey['^'] = { // Circumflex
            'a': '\u00e2', 'e': '\u00ea', 'i': '\u00ee', 'o': '\u00f4', 'u': '\u00fb', 'w': '\u0175', 'y': '\u0177',
            'A': '\u00c2', 'E': '\u00ca', 'I': '\u00ce', 'O': '\u00d4', 'U': '\u00db', 'W': '\u0174', 'Y': '\u0176'
        };
        this.VKI_deadkey['\u02c7'] = { // Baltic caron
            'c': '\u010D', 'd': '\u010f', 'e': '\u011b', 's': '\u0161', 'l': '\u013e', 'n': '\u0148', 'r': '\u0159', 't': '\u0165',
            'u': '\u01d4', 'z': '\u017E', '\u00fc': '\u01da', 'C': '\u010C', 'D': '\u010e', 'E': '\u011a', 'S': '\u0160',
            'L': '\u013d', 'N': '\u0147', 'R': '\u0158', 'T': '\u0164', 'U': '\u01d3', 'Z': '\u017D', '\u00dc': '\u01d9'
        };
        this.VKI_deadkey['\u02d8'] = { // Romanian and Turkish breve
            'a': '\u0103', 'g': '\u011f',
            'A': '\u0102', 'G': '\u011e'
        };
        this.VKI_deadkey['-'] = this.VKI_deadkey['\u00af'] = { // Macron
            'a': '\u0101', 'e': '\u0113', 'i': '\u012b', 'o': '\u014d', 'u': '\u016B', 'y': '\u0233', '\u00fc': '\u01d6',
            'A': '\u0100', 'E': '\u0112', 'I': '\u012a', 'O': '\u014c', 'U': '\u016A', 'Y': '\u0232', '\u00dc': '\u01d5'
        };
        this.VKI_deadkey['`'] = { // Grave
            'a': '\u00e0', 'e': '\u00e8', 'i': '\u00ec', 'o': '\u00f2', 'u': '\u00f9', '\u00fc': '\u01dc',
            'A': '\u00c0', 'E': '\u00c8', 'I': '\u00cc', 'O': '\u00d2', 'U': '\u00d9', '\u00dc': '\u01db'
        };
        this.VKI_deadkey["'"] = this.VKI_deadkey['\u00b4'] = this.VKI_deadkey['\u0384'] = { // Acute / Greek Tonos
            'a': '\u00e1', 'e': '\u00e9', 'i': '\u00ed', 'o': '\u00f3', 'u': '\u00fa', 'y': '\u00fd', '\u03b1': '\u03ac',
            '\u03b5': '\u03ad', '\u03b7': '\u03ae', '\u03b9': '\u03af', '\u03bf': '\u03cc', '\u03c5': '\u03cd', '\u03c9': '\u03ce',
            '\u00fc': '\u01d8', 'A': '\u00c1', 'E': '\u00c9', 'I': '\u00cd', 'O': '\u00d3', 'U': '\u00da', 'Y': '\u00dd',
            '\u0391': '\u0386', '\u0395': '\u0388', '\u0397': '\u0389', '\u0399': '\u038a', '\u039f': '\u038c', '\u03a5': '\u038e',
            '\u03a9': '\u038f', '\u00dc': '\u01d7'
        };
        this.VKI_deadkey['\u02dd'] = { // Hungarian Double Acute Accent
            'o': '\u0151', 'u': '\u0171',
            'O': '\u0150', 'U': '\u0170'
        };
        this.VKI_deadkey['\u0385'] = { // Greek Dialytika + Tonos
            '\u03b9': '\u0390', '\u03c5': '\u03b0'
        };
        this.VKI_deadkey['\u00b0'] = this.VKI_deadkey['\u00ba'] = { // Ring
            'a': '\u00e5', 'u': '\u016f',
            'A': '\u00c5', 'U': '\u016e'
        };
        this.VKI_deadkey['\u02DB'] = { // Ogonek
            'a': '\u0106', 'e': '\u0119', 'i': '\u012f', 'o': '\u01eb', 'u': '\u0173', 'y': '\u0177',
            'A': '\u0105', 'E': '\u0118', 'I': '\u012e', 'O': '\u01ea', 'U': '\u0172', 'Y': '\u0176'
        };
        this.VKI_deadkey['\u02D9'] = { // Dot-above
            'c': '\u010B', 'e': '\u0117', 'g': '\u0121', 'z': '\u017C',
            'C': '\u010A', 'E': '\u0116', 'G': '\u0120', 'Z': '\u017B'
        };
        this.VKI_deadkey['\u00B8'] = this.VKI_deadkey['\u201a'] = { // Cedilla
            'c': '\u00e7', 's': '\u015F',
            'C': '\u00c7', 'S': '\u015E'
        };
        this.VKI_deadkey[','] = { // Comma
            's': '\u0219', 't': '\u021B',
            'S': '\u0218', 'T': '\u021A'
        };
        this.VKI_deadkey['\u3002'] = { // Hiragana/Katakana Point
            '\u306f': '\u3071', '\u3072': '\u3074', '\u3075': '\u3077', '\u3078': '\u307a', '\u307b': '\u307d',
            '\u30cf': '\u30d1', '\u30d2': '\u30d4', '\u30d5': '\u30d7', '\u30d8': '\u30da', '\u30db': '\u30dd'
        };
        this.VKI_symbol = {
            '\u00a0': 'NB\nSP', '\u200b': 'ZW\nSP', '\u200c': 'ZW\nNJ', '\u200d': 'ZW\nJ'
        };
        this.VKI_numpad = [
            [['$'], ['\u00a3'], ['\u20ac'], ['\u00a5']],
            [['7'], ['8'], ['9'], ['/']],
            [['4'], ['5'], ['6'], ['*']],
            [['1'], ['2'], ['3'], ['-']],
            [['0'], ['.'], ['='], ['+']]
                ];
        this.cblock = [
            [['Prt', 0x2c],['Slk', 0],['\u231b', 0x13]],
            [['Ins', 0x2d],['\u21f1', 0x24],['\u21d1', 0x21]],
            [['Del', 0x2e],['\u21f2', 0x23],['\u21d3', 0x23]],
            [[''],['\u2191', 0x26],['']],
            [['\u2190', 0x25],['\u2193', 0x28],['\u2192', 0x27]],
            ];
        this.fnblock = [
            ['Esc', 0x1b], ['F1', 0x70], ['F2', 0x71], ['F3', 0x72], ['F4', 0x73], ['F5', 0x74],
            ['F6', 0x75], ['F7', 0x76], ['F8', 0x77], ['F9', 0x78], ['F10', 0x79], ['F11', 0x7a],
            ['F12', 0x7b]
            ];
        if (!this.VKI_layout[this.VKI_kt]) {
            throw 'No layout named "' + this.VKI_kt + '"';
        }
        this.VKI_keyboard = new Element('table', {
            'id': 'keyboardInputMaster',
            'dir': 'ltr',
            'cellspacing': 0,
            'class': 'hidden',
            'styles': {
                'position': 'absolute',
                'left': this.posX,
                'top': this.posY,
                'z-index': 999
            },
            'events': {
                'click': function(e) { e.stopPropagation(); },
                'selectstart': function(e) { e.stopPropagation(); }
            }
        }).inject(document.body);
        this.VKI_langCode = {};
        var thead = new Element('thead', {
            'events': {
                'mousedown': this.dragStart.bind(this),
            }
        }).inject(this.VKI_keyboard);
        var tr = new Element('tr').inject(thead);
        var th = new Element('th', {'colspan':3}).inject(tr);

        var nlayouts = 0;
        Object.each(this.VKI_layout, function(item) { if ('object' == typeof(item)) { nlayouts++; } });
        // Build layout selector if more than one layouts
        if (nlayouts > 1) {
            this.kbSelect = new Element('div', {
                'html': this.VKI_kt,
                'title': this.VKI_i18n['02']
            }).addEvent('click', function(e) {
                var ol = e.target.getElement('ol');
                if (!ol.style.display) {
                    ol.setStyle('display','block');
                    var scr = 0;
                    ol.getElements('li').each(function(li) {
                        if (this.VKI_kt == li.firstChild.nodeValue) {
                            li.addClass('selected');
                            scr = li.offsetTop - li.offsetHeight * 2;
                        } else {
                            li.removeClass('selected');
                        }
                    });
                    setTimeout(function() { ol.scrollTop = scr; }, 0);
                } else {
                    ol.setStyle('display','');
                }
            }.bind(this)).appendText(' \u25be').inject(th);
            var ol = new Element('ol').inject(this.kbSelect);
            for (ktype in this.VKI_layout) {
                if (typeof this.VKI_layout[ktype] == 'object') {
                    if (!this.VKI_layout[ktype].lang) { 
                        this.VKI_layout[ktype].lang = [];
                    }
                    for (var x = 0; x < this.VKI_layout[ktype].lang.length; x++) {
                        this.VKI_langCode[this.VKI_layout[ktype].lang[x].toLowerCase().replace(/-/g, '_')] = ktype;
                    }
                    var li = new Element('li', {
                        'title': this.VKI_layout[ktype].name,
                        'html': ktype
                    }).inject(ol).addEvent('click', function(e) {
                        e.stopPropagation();
                        var el = e.target;
                        el.parentNode.setStyle('display','');
                        this.VKI_kt = this.kbSelect.firstChild.nodeValue = el.get('text');
                        this.VKI_buildKeys();
                    }.bind(this));
                }
            }
        }
        // Sort the layout selector alphabetically
        this.VKI_langCode.index = [];
        for (prop in this.VKI_langCode) {
            if (prop != 'index' && typeof this.VKI_langCode[prop] == 'string') {
                this.VKI_langCode.index.push(prop);
            }
        }
        this.VKI_langCode.index.sort();
        this.VKI_langCode.index.reverse();

        // Build Number-pad toggle-button
        if (this.options.numpadtoggle) {
            new Element('span', {
                'html': '#',
                'title': this.VKI_i18n['00']
            }).addEvent('click', function() {
                this.kbNumpad.toggleClass('hidden');
            }.bind(this)).inject(th);
        }

        // Build SizeUp and SizeDown buttons
        if (this.options.sizeswitch) {
            new Element('small', {
                'html': '\u21d3',
                'title': this.VKI_i18n['10']
            }).addEvent('click', function() {
                this.VKI_size--;
                this.VKI_kbsize();
            }.bind(this)).inject(th);
            new Element('big', {
                'html': '\u21d1',
                'title': this.VKI_i18n['11']
            }).addEvent('click', function() {
                this.VKI_size++;
                this.VKI_kbsize();
            }.bind(this)).inject(th);
        }

        // Build Clear button
        if (this.options.target) {
            new Element('span', {
                'html': this.VKI_i18n['07'],
                'title': this.VKI_i18n['08']
            }).addEvent('click', function() {
                this.options.target.value = '';
                this.options.target.focus();
            }.bind(this)).inject(th);
        }

        // Build Close box
        var closeBox = new Element('strong', {
            'html': 'X',
            'title': this.VKI_i18n['06']
        }).addEvent('click', function() {
            this.hide();
        }.bind(this)).inject(th);

        var tbody = new Element('tbody').inject(this.VKI_keyboard);
        var tr = new Element('tr').inject(tbody);
        var td = new Element('td').inject(tr);
        var div = new Element('div').inject(td);

        // Build deadKey checkbox
        if (this.options.deadcheck) {
            var label = new Element('label').inject(div);
            this.deadCheckbox = new Element('input', {
                'type': 'checkbox',
                'title': this.VKI_i18n['03'] + ': ' + (this.options.deadkeys ? this.VKI_i18n['04'] : this.VKI_i18n['05']),
                'defaultchecked': this.options.deadkeys,
                'checked': this.options.deadkeys
            }).addEvent('click', function(e) {
                el = e.target;
                el.set('title', this.VKI_i18n['03'] + ': ' + ((el.checked) ? this.VKI_i18n['04'] : this.VKI_i18n['05']));
                this.modify('');
            }.bind(this)).inject(label);
        } else {
            this.deadCheckbox = new Element('input', {
                'type': 'checkbox',
                'defaultchecked': this.options.deadkeys,
                'checked': this.options.deadkeys
            });
        }

        // Build Version display
        if (this.options.version) {
            new Element('var', {
                'html': 'v' + this.VERSION,
                'title': this.VKI_i18n['09'] + ' ' + this.VERSION
            }).inject(div);
        }

        // Build cursor block
        this.kbCursor = new Element('td', {
            'id': 'keyboardInputCursor'
        }).inject(tr);
        var ctable = new Element('table', {
            'cellspacing': '0'
        }).inject(this.kbCursor);
        var ctbody = new Element('tbody').inject(ctable);
        this.cblock.each(function(row) {
            ctr = new Element('tr').inject(ctbody);
            row.each(function(col) {
                this.VKI_stdEvents(new Element('td', {
                    'html': col[0],
                    'class': (col[0].length ? '' : 'none'),
                    'char': col[1],
                    'events': {
                        'click': (col[1] ? this.kclick2.bind(this) : this.dummy)
                    }
                }).inject(ctr));
            }.bind(this));
        }.bind(this));

        // Build NumPad
        this.kbNumpad = new Element('td', {
            'id': 'keyboardInputNumpad',
            'class': (this.options.numpad ? '' : 'hidden')
        }).inject(tr);
        var ntable = new Element('table', {
            'cellspacing': '0'
        }).inject(this.kbNumpad);
        var ntbody = new Element('tbody').inject(ntable);
        for (var x = 0; x < this.VKI_numpad.length; x++) {
            var ntr = new Element('tr').inject(ntbody);
            for (var y = 0; y < this.VKI_numpad[x].length; y++) {
                this.VKI_stdEvents(new Element('td', {
                    'html': this.VKI_numpad[x][y],
                    'events': {
                        'click': this.kclick.bind(this)
                    }
                }).inject(ntr));
            }
        }

        // Build normal keys
        this.VKI_buildKeys();
        this.VKI_keyboard.unselectable = 'on';
        this.VKI_kbsize();
    },
    dummy: function() {
    },
    VKI_stdEvents: function(elem) {
        if ('td' == elem.get('tag')) {
            // elem.addEvent('dblclick', function(e) { e.PreventDefault(); });
            if (this.options.hoverclick) {
                elem.clid = 0;
                elem.addEvents({
                    'mouseover': function(e) {
                        var el = e.target;
                        clearTimeout(el.clid);
                        el.clid = function() {
                            this.fireEvent('click', this, 0);
                        }.delay(this.options.hoverclick);
                    }.bind(this),
                    'mouseout': function() { clearTimeout(this.clid); }.bind(elem),
                    'mousedown': function() { clearTimeout(this.clid); }.bind(elem),
                    'mouseup': function() { clearTimeout(this.clid); }.bind(elem)
                });
            }
        }
    },
    VKI_kbsize: function(e) {
        this.VKI_size = Math.min(5, Math.max(1, this.VKI_size));
        for (var i = 0; i < 6; i++) {
            this.VKI_keyboard.removeClass('keyboardInputSize' + i);
        }
        if (this.VKI_size != 2) {
            this.VKI_keyboard.addClass('keyboardInputSize' + this.VKI_size);
        }
    },
    kdown: function(evt) {
    },
    kup: function(evt) {
    },
    kclick: function(evt) {
        var done = false, character = '\xa0', el = evt.target;
        if (el.firstChild.nodeName.toLowerCase() != 'small') {
            if ((character = el.firstChild.nodeValue) == '\xa0') {
                return;
            }
        } else {
            character = el.firstChild.get('char');
        }
        if (this.deadCheckbox.checked && this.VKI_dead) {
            if (this.VKI_dead != character) {
                if (character != ' ') {
                    if (this.VKI_deadkey[this.VKI_dead][character]) {
                        this.kpress(this.VKI_deadkey[this.VKI_dead][character]);
                        done = true;
                    }
                } else {
                    this.kpress(this.VKI_dead);
                    done = true;
                }
            } else {
                done = true;
            }
        }
        this.VKI_dead = false;

        if (!done) {
            if (this.deadCheckbox.checked && this.VKI_deadkey[character]) {
                this.VKI_dead = character;
                el.addClass('dead');
                if (this.VKI_shift) {
                    this.modify('Shift');
                }
                if (this.VKI_alt) {
                    this.modify('Alt');
                }
                if (this.VKI_altgr) {
                    this.modify('AltGr');
                }
                if (this.VKI_ctrl) {
                    this.modify('Ctrl');
                }
            } else {
                this.kpress(character);
            }
        }
        this.modify('');
    },
    kclick2: function(evt) {
        var c = evt.target.get('char');
        if (c) {
            this.kpress(c, true);
            this.VKI_dead = false;
            this.modify('');
        }
    },
    VKI_buildKeys: function() {
        this.VKI_shift = this.VKI_shiftlock = this.VKI_altgr = this.VKI_altgrlock = this.VKI_dead = false;
        var container = this.VKI_keyboard.tBodies[0].getElement('div');
        container.getElements('table').each(function(el) { el.destroy(); });
        for (var x = 0, hasDeadKeys = false, lyt; lyt = this.VKI_layout[this.VKI_kt].keys[x++];) {
            var table = new Element('table',{
                'cellspacing': '0'
            }).inject(container);
            if (lyt.length <= this.VKI_keyCenter) {
                table.addClass('keyboardInputCenter');
            }
            var tbody = new Element('tbody').inject(table);
            var tr = new Element('tr').inject(tbody);
            for (var y = 0, lkey; lkey = lyt[y++];) {
                var td = new Element('td').inject(tr);
                if (this.VKI_symbol[lkey[0]]) {
                    var text = this.VKI_symbol[lkey[0]].split('\n');
                    var small = new Element('small', {
                        'char': lkey[0]
                    }).inject(td);
                    for (var z = 0; z < text.length; z++) {
                        if (z) {
                            new Element('br').inject(small);
                        }
                        small.appendText(text[z]);
                    }
                } else {
                    td.appendText(lkey[0] || '\xa0');
                }

                if (this.deadCheckbox.checked) {
                    for (key in this.VKI_deadkey) {
                        if (key === lkey[0]) {
                            td.addClass('deadkey');
                            break;
                        }
                    }
                }
                if (lyt.length > this.VKI_keyCenter && y == lyt.length) {
                    td.addClass('last');
                }
                if (lkey[0] == ' ' || lkey[1] == ' ') {
                    td.addClass('space');
                }

                switch (lkey[1]) {
                    case 'Caps':
                    case 'Ctrl':
                    case 'Shift':
                    case 'Alt':
                    case 'AltGr':
                    case 'AltLk':
                        td.addEvent('click', function(type) {
                            // XXX
                            this.modify(type);
                        }.bind(this, lkey[1]));
                        break;
                    case 'Tab':
                        td.addEvent('click', function(e) {
                            e.preventDefault();
                            this.kpress('\t');
                        }.bind(this));
                        break;
                    case 'Bksp':
                        td.addEvent('click', function(e) {
                            e.preventDefault();
                            this.kpress('\b');
                        }.bind(this));
                        break;
                    case 'Enter':
                        td.addEvent('click', function(e) {
                            e.preventDefault();
                            this.kpress('\r');
                        }.bind(this));
                        break;
                    default:
                        td.addEvent('click', this.kclick.bind(this));
                        break;
                }
                this.VKI_stdEvents(td);
                for (var z = 0; z < 4; z++) {
                    if (this.VKI_deadkey[lkey[z] = lkey[z] || '']) {
                        hasDeadKeys = true;
                    }
                }
            }
        }
        // Hide deadkey checkbox if layout has no deadkeys
        if (this.options.deadcheck) {
            this.deadCheckbox.setStyle('display', hasDeadKeys ? 'inline' : 'none');
        }
    },
    modify: function(type) {
        switch (type) {
            case 'Alt':
                this.VKI_alt = !this.VKI_alt;
                break;
            case 'AltGr':
                this.VKI_altgr = !this.VKI_altgr;
                break;
            case 'AltLk':
                this.VKI_altgr = 0;
                this.VKI_altgrlock = !this.VKI_altgrlock;
                break;
            case 'Caps':
                this.VKI_shift = 0;
                this.VKI_shiftlock = !this.VKI_shiftlock;
                break;
            case 'Ctrl':
                this.VKI_ctrl = !this.VKI_ctrl;
                break;
            case 'Shift':
                this.VKI_shift = !this.VKI_shift;
                break;
        }
        var vchar = 0;
        if (!this.VKI_shift != !this.VKI_shiftlock) {
            vchar += 1;
        }
        if (!this.VKI_altgr != !this.VKI_altgrlock) {
            vchar += 2;
        }
        var tables = this.VKI_keyboard.tBodies[0].getElement('div').getElements('table');
        for (var x = 0; x < tables.length; x++) {
            var tds = tables[x].getElements('td');
            for (var y = 0; y < tds.length; y++) {
                var classes = {}, lkey = this.VKI_layout[this.VKI_kt].keys[x][y];
                switch (lkey[1]) {
                    case 'Alt':
                        if (this.VKI_alt) {
                            classes.pressed = 1;
                        }
                        break;
                    case 'AltGr':
                        if (this.VKI_altgr) {
                            classes.pressed = 1;
                        }
                        break;
                    case 'AltLk':
                        if (this.VKI_altgrlock) {
                            classes.pressed = 1;
                        }
                        break;
                    case 'Shift':
                        if (this.VKI_shift) {
                            classes.pressed = 1;
                        }
                        break;
                    case 'Caps':
                        if (this.VKI_shiftlock) {
                            classes.pressed = 1;
                        }
                        break;
                    case 'Ctrl':
                        if (this.VKI_ctrl) {
                            classes.pressed = 1;
                        }
                        break;
                    case 'Tab':
                    case 'Enter':
                    case 'Bksp':
                        break;
                    default:
                        if (type) {
                            tds[y].removeChild(tds[y].firstChild);
                            if (this.VKI_symbol[lkey[vchar]]) {
                                var text = this.VKI_symbol[lkey[vchar]].split('\n');
                                var small = new Element('small', {
                                    'char': lkey[vchar]
                                }).inject(tds[y]);
                                for (var z = 0; z < text.length; z++) {
                                    if (z) {
                                        new Element('br').inject(small);
                                    }
                                    small.appendText(text[z]);
                                }
                            } else {
                                tds[y].appendText(lkey[vchar] || '\xa0');
                            }
                        }
                        if (this.deadCheckbox.checked) {
                            var character =
                                tds[y].firstChild.nodeValue || tds[y].firstChild.className;
                            if (this.VKI_dead) {
                                if (character == this.VKI_dead) {
                                    classes.pressed = 1;
                                }
                                if (this.VKI_deadkey[this.VKI_dead][character]) {
                                    classes.target = 1;
                                }
                            }
                            if (this.VKI_deadkey[character]) {
                                classes.deadkey = 1;
                            }
                        }
                        break;
                }
                if (y == tds.length - 1 && tds.length > this.VKI_keyCenter) {
                    classes.last = 1;
                }
                if (lkey[0] == ' ' || lkey[1] == ' ') {
                    classes.space = 1;
                }
                tds[y].removeClass('pressed').removeClass('target').removeClass('deadkey').removeClass('last').removeClass('space');
                Object.each(classes, function(i,k) { tds[y].addClass(k); });
            }
        }
    },
    kpress: function(text, special) {
        var e = {
            code: text.charCodeAt(0),
            shift: this.VKI_shift,
            control: this.VKI_ctrl,
            alt: this.VKI_alt,
            meta: this.VKI_altgr,
            special: special
        };
        this.fireEvent('vkpress', e);
        if (this.VKI_alt) {
            this.modify("Alt");
        }
        if (this.VKI_altgr) {
            this.modify("AltGr");
        }
        if (this.VKI_ctrl) {
            this.modify("Ctrl");
        }
        if (this.VKI_shift) {
            this.modify("Shift");
        }
    },
    show: function() {
        this.VKI_keyboard.removeClass('hidden');
    },
    hide: function() {
        this.VKI_keyboard.addClass('hidden');
    },
    toggle: function() {
        this.VKI_keyboard.toggleClass('hidden');
    },
    dragEnd: function(evt) {
        this.dragging = false;
        window.removeEvent('mouseup', this.dragEnd.bind(this));
        window.removeEvent('mousemove', this.dragMove.bind(this));
        window.removeEvent('touchmove', this.dragMove.bind(this));
    },
    dragStart: function(evt) {
        this.dragX = evt.page.x;
        this.dragY = evt.page.y;
        this.dragging = true;
        window.addEvent('mouseup', this.dragEnd.bind(this));
        window.addEvent('mousemove', this.dragMove.bind(this));
        window.addEvent('touchmove', this.dragMove.bind(this));
    },
    dragMove: function(evt) {
        if (this.dragging) {
            this.posX += evt.page.x - this.dragX;
            this.posY += evt.page.y - this.dragY;
            this.dragX = evt.page.x;
            this.dragY = evt.page.y;
            this.VKI_keyboard.setStyles({'top':this.posY,'left':this.posX});
        }
    }
});
