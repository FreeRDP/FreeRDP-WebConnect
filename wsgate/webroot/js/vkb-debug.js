/*!
 * This code is based on:
 * HTML Virtual Keyboard Interface Script - v1.49
 *   Copyright (c) 2011 - GreyWyvern
 *  - Licenced for free distribution under the BSDL
 *          http://www.opensource.org/licenses/bsd-license.php
 */
var VKI_attach, VKI_close;

(function() {
    var self = this;

    this.VKI_version = "1.49";
    this.VKI_showVersion = false;
    this.VKI_target = false;
    this.VKI_shift = this.VKI_shiftlock = false;
    this.VKI_altgr = this.VKI_altgrlock = false;
    this.VKI_ctrl = this.VKI_ctrllock = false;
    this.VKI_dead = false;
    this.VKI_deadBox = true; // Show the dead keys checkbox
    this.VKI_deadkeysOn = false;  // Turn dead keys on by default
    this.VKI_numberPad = true;  // Allow user to open and close the number pad
    this.VKI_numberPadOn = false;  // Show number pad by default
    this.VKI_kts = this.VKI_kt = "US International";  // Default keyboard layout
    this.VKI_langAdapt = false;  // Use lang attribute of input to select keyboard
    this.VKI_size = 5;  // Default keyboard size (1-5)
    this.VKI_sizeAdj = false;  // Allow user to adjust keyboard size
    this.VKI_clearPasswords = false;  // Clear password fields on focus
    this.VKI_imageURI = ''; //"keyboard.png";  // If empty string, use imageless mode
    this.VKI_clickless = 0;  // 0 = disabled, > 0 = delay in ms
    this.VKI_activeTab = 0;  // Tab moves to next: 1 = element, 2 = keyboard enabled element
    this.VKI_enterSubmit = false;  // Submit forms when Enter is pressed
    this.VKI_keyCenter = 3;

    this.VKI_isWebKit = RegExp("KHTML").test(navigator.userAgent);
    this.VKI_isMoz = (!this.VKI_isWebKit && navigator.product == "Gecko");

    /* ***** i18n text strings ************************************* */
    this.VKI_i18n = {
        '00': "Display Number Pad",
        '01': "Display virtual keyboard interface",
        '02': "Select keyboard layout",
        '03': "Dead keys",
        '04': "On",
        '05': "Off",
        '06': "Close the keyboard",
        '07': "Clear",
        '08': "Clear this input",
        '09': "Version",
        '10': "Decrease keyboard size",
        '11': "Increase keyboard size"
    };


    /* ***** Create keyboards ************************************** */
    this.VKI_layout = {};

    // - Lay out each keyboard in rows of sub-arrays.  Each sub-array
    //   represents one key.
    //
    // - Each sub-array consists of four slots described as follows:
    //     example: ["a", "A", "\u00e1", "\u00c1"]
    //
    //          a) Normal character
    //          A) Character + Shift/Caps
    //     \u00e1) Character + Alt/AltGr/AltLk
    //     \u00c1) Character + Shift/Caps + Alt/AltGr/AltLk
    //
    //   You may include sub-arrays which are fewer than four slots.
    //   In these cases, the missing slots will be blanked when the
    //   corresponding modifier key (Shift or AltGr) is pressed.
    //
    // - If the second slot of a sub-array matches one of the following
    //   strings:
    //     "Tab", "Caps", "Shift", "Enter", "Bksp",
    //     "Alt" OR "AltGr", "AltLk"
    //   then the function of the key will be the following,
    //   respectively:
    //     - Insert a tab
    //     - Toggle Caps Lock (technically a Shift Lock)
    //     - Next entered character will be the shifted character
    //     - Insert a newline (textarea), or close the keyboard
    //     - Delete the previous character
    //     - Next entered character will be the alternate character
    //     - Toggle Alt/AltGr Lock
    //
    //   The first slot of this sub-array will be the text to display
    //   on the corresponding key.  This allows for easy localisation
    //   of key names.
    //
    // - Layout dead keys (diacritic + letter) should be added as
    //   property/value pairs of objects with hash keys equal to the
    //   diacritic.  See the "this.VKI_deadkey" object below the layout
    //   definitions.  In each property/value pair, the value is what
    //   the diacritic would change the property name to.
    //
    // - Note that any characters beyond the normal ASCII set should be
    //   entered in escaped Unicode format.  (eg \u00a3 = Pound symbol)
    //   You can find Unicode values for characters here:
    //     http://unicode.org/charts/
    //
    // - To remove a keyboard, just delete it, or comment it out of the
    //   source code. If you decide to remove the US International
    //   keyboard layout, make sure you change the default layout
    //   (this.VKI_kt) above so it references an existing layout.

    this.VKI_layout['Deutsch'] = {
        'name': "German", 'keys': [
            [["^", "\u00b0"], ["1", "!"], ["2", '"', "\u00b2"], ["3", "\u00a7", "\u00b3"], ["4", "$"], ["5", "%"], ["6", "&"], ["7", "/", "{"], ["8", "(", "["], ["9", ")", "]"], ["0", "=", "}"], ["\u00df", "?", "\\"], ["\u00b4", "`"], ["Bksp", "Bksp"]],
        [["Tab", "Tab"], ["q", "Q", "@"], ["w", "W"], ["e", "E", "\u20ac"], ["r", "R"], ["t", "T"], ["z", "Z"], ["u", "U"], ["i", "I"], ["o", "O"], ["p", "P"], ["\u00fc", "\u00dc"], ["+", "*", "~"], ["#", "'"]],
        [["Caps", "Caps"], ["a", "A"], ["s", "S"], ["d", "D"], ["f", "F"], ["g", "G"], ["h", "H"], ["j", "J"], ["k", "K"], ["l", "L"], ["\u00f6", "\u00d6"], ["\u00e4", "\u00c4"], ["Enter", "Enter"]],
        [["Shift", "Shift"], ["<", ">", "\u00a6"], ["y", "Y"], ["x", "X"], ["c", "C"], ["v", "V"], ["b", "B"], ["n", "N"], ["m", "M", "\u00b5"], [",", ";"], [".", ":"], ["-", "_"], ["Shift", "Shift"]],
        [[" ", " ", " ", " "], ["AltGr", "AltGr"]]
            ], 'lang': ["de"] };
    this.VKI_layout['Espa\u00f1ol'] = {
        'name': "Spanish", 'keys': [
            [["\u00ba", "\u00aa", "\\"], ["1", "!", "|"], ["2", '"', "@"], ["3", "'", "#"], ["4", "$", "~"], ["5", "%", "\u20ac"], ["6", "&", "\u00ac"], ["7", "/"], ["8", "("], ["9", ")"], ["0", "="], ["'", "?"], ["\u00a1", "\u00bf"], ["Bksp", "Bksp"]],
        [["Tab", "Tab"], ["q", "Q"], ["w", "W"], ["e", "E"], ["r", "R"], ["t", "T"], ["y", "Y"], ["u", "U"], ["i", "I"], ["o", "O"], ["p", "P"], ["`", "^", "["], ["+", "*", "]"], ["\u00e7", "\u00c7", "}"]],
        [["Caps", "Caps"], ["a", "A"], ["s", "S"], ["d", "D"], ["f", "F"], ["g", "G"], ["h", "H"], ["j", "J"], ["k", "K"], ["l", "L"], ["\u00f1", "\u00d1"], ["\u00b4", "\u00a8", "{"], ["Enter", "Enter"]],
        [["Shift", "Shift"], ["<", ">"], ["z", "Z"], ["x", "X"], ["c", "C"], ["v", "V"], ["b", "B"], ["n", "N"], ["m", "M"], [",", ";"], [".", ":"], ["-", "_"], ["Shift", "Shift"]],
        [[" ", " ", " ", " "], ["AltGr", "AltGr"]]
            ], 'lang': ["es"] };
    this.VKI_layout['Fran\u00e7ais'] = {
        'name': "French", 'keys': [
            [["\u00b2", "\u00b3"], ["&", "1"], ["\u00e9", "2", "~"], ['"', "3", "#"], ["'", "4", "{"], ["(", "5", "["], ["-", "6", "|"], ["\u00e8", "7", "`"], ["_", "8", "\\"], ["\u00e7", "9", "^"], ["\u00e0", "0", "@"], [")", "\u00b0", "]"], ["=", "+", "}"], ["Bksp", "Bksp"]],
        [["Tab", "Tab"], ["a", "A"], ["z", "Z"], ["e", "E", "\u20ac"], ["r", "R"], ["t", "T"], ["y", "Y"], ["u", "U"], ["i", "I"], ["o", "O"], ["p", "P"], ["^", "\u00a8"], ["$", "\u00a3", "\u00a4"], ["*", "\u03bc"]],
        [["Caps", "Caps"], ["q", "Q"], ["s", "S"], ["d", "D"], ["f", "F"], ["g", "G"], ["h", "H"], ["j", "J"], ["k", "K"], ["l", "L"], ["m", "M"], ["\u00f9", "%"], ["Enter", "Enter"]],
        [["Shift", "Shift"], ["<", ">"], ["w", "W"], ["x", "X"], ["c", "C"], ["v", "V"], ["b", "B"], ["n", "N"], [",", "?"], [";", "."], [":", "/"], ["!", "\u00a7"], ["Shift", "Shift"]],
        [[" ", " ", " ", " "], ["AltGr", "AltGr"]]
            ], 'lang': ["fr"] };
    this.VKI_layout['Italiano'] = {
        'name': "Italian", 'keys': [
            [["\\", "|"], ["1", "!"], ["2", '"'], ["3", "\u00a3"], ["4", "$", "\u20ac"], ["5", "%"], ["6", "&"], ["7", "/"], ["8", "("], ["9", ")"], ["0", "="], ["'", "?"], ["\u00ec", "^"], ["Bksp", "Bksp"]],
        [["Tab", "Tab"], ["q", "Q"], ["w", "W"], ["e", "E", "\u20ac"], ["r", "R"], ["t", "T"], ["y", "Y"], ["u", "U"], ["i", "I"], ["o", "O"], ["p", "P"], ["\u00e8", "\u00e9", "[", "{"], ["+", "*", "]", "}"], ["\u00f9", "\u00a7"]],
        [["Caps", "Caps"], ["a", "A"], ["s", "S"], ["d", "D"], ["f", "F"], ["g", "G"], ["h", "H"], ["j", "J"], ["k", "K"], ["l", "L"], ["\u00f2", "\u00e7", "@"], ["\u00e0", "\u00b0", "#"], ["Enter", "Enter"]],
        [["Shift", "Shift"], ["<", ">"], ["z", "Z"], ["x", "X"], ["c", "C"], ["v", "V"], ["b", "B"], ["n", "N"], ["m", "M"], [",", ";"], [".", ":"], ["-", "_"], ["Shift", "Shift"]],
        [[" ", " ", " ", " "], ["AltGr", "AltGr"]]
            ], 'lang': ["it"] };
    this.VKI_layout['Nederlands'] = {
        'name': "Dutch", 'keys': [
            [["@", "\u00a7", "\u00ac"], ["1", "!", "\u00b9"], ["2", '"', "\u00b2"], ["3", "#", "\u00b3"], ["4", "$", "\u00bc"], ["5", "%", "\u00bd"], ["6", "&", "\u00be"], ["7", "_", "\u00a3"], ["8", "(", "{"], ["9", ")", "}"], ["0", "'"], ["/", "?", "\\"], ["\u00b0", "~", "\u00b8"], ["Bksp", "Bksp"]],
        [["Tab", "Tab"], ["q", "Q"], ["w", "W"], ["e", "E", "\u20ac"], ["r", "R", "\u00b6"], ["t", "T"], ["y", "Y"], ["u", "U"], ["i", "I"], ["o", "O"], ["p", "P"], ["\u00a8", "^"], ["*", "|"], ["<", ">"]],
        [["Caps", "Caps"], ["a", "A"], ["s", "S", "\u00df"], ["d", "D"], ["f", "F"], ["g", "G"], ["h", "H"], ["j", "J"], ["k", "K"], ["l", "L"], ["+", "\u00b1"], ["\u00b4", "`"], ["Enter", "Enter"]],
        [["Shift", "Shift"], ["]", "[", "\u00a6"], ["z", "Z", "\u00ab"], ["x", "X", "\u00bb"], ["c", "C", "\u00a2"], ["v", "V"], ["b", "B"], ["n", "N"], ["m", "M", "\u00b5"], [",", ";"], [".", ":", "\u00b7"], ["-", "="], ["Shift", "Shift"]],
        [[" ", " ", " ", " "], ["AltGr", "AltGr"]]
            ], 'lang': ["nl"] };

    this.VKI_layout['Norsk'] = {
        'name': "Norwegian", 'keys': [
            [["|", "\u00a7"], ["1", "!"], ["2", '"', "@"], ["3", "#", "\u00a3"], ["4", "\u00a4", "$"], ["5", "%"], ["6", "&"], ["7", "/", "{"], ["8", "(", "["], ["9", ")", "]"], ["0", "=", "}"], ["+", "?"], ["\\", "`", "\u00b4"], ["Bksp", "Bksp"]],
        [["Tab", "Tab"], ["q", "Q"], ["w", "W"], ["e", "E", "\u20ac"], ["r", "R"], ["t", "T"], ["y", "Y"], ["u", "U"], ["i", "I"], ["o", "O"], ["p", "P"], ["\u00e5", "\u00c5"], ["\u00a8", "^", "~"], ["'", "*"]],
        [["Caps", "Caps"], ["a", "A"], ["s", "S"], ["d", "D"], ["f", "F"], ["g", "G"], ["h", "H"], ["j", "J"], ["k", "K"], ["l", "L"], ["\u00f8", "\u00d8"], ["\u00e6", "\u00c6"], ["Enter", "Enter"]],
        [["Shift", "Shift"], ["<", ">"], ["z", "Z"], ["x", "X"], ["c", "C"], ["v", "V"], ["b", "B"], ["n", "N"], ["m", "M", "\u03bc", "\u039c"], [",", ";"], [".", ":"], ["-", "_"], ["Shift", "Shift"]],
        [[" ", " ", " ", " "], ["AltGr", "AltGr"]]
            ], 'lang': ["no", "nb", "nn"] };
    this.VKI_layout['Polski'] = {
        'name': "Polish (214)", 'keys': [
            [["\u02DB", "\u00B7"], ["1", "!", "~"], ["2", '"', "\u02C7"], ["3", "#", "^"], ["4", "\u00A4", "\u02D8"], ["5", "%", "\u00B0"], ["6", "&", "\u02DB"], ["7", "/", "`"], ["8", "(", "\u00B7"], ["9", ")", "\u00B4"], ["0", "=", "\u02DD"], ["+", "?", "\u00A8"], ["'", "*", "\u00B8"], ["Bksp", "Bksp"]],
        [["Tab", "Tab"], ["q", "Q", "\\"], ["w", "W", "\u00A6"], ["e", "E"], ["r", "R"], ["t", "T"], ["z", "Z"], ["u", "U", "\u20AC"], ["i", "I"], ["o", "O"], ["p", "P"], ["\u017C", "\u0144", "\u00F7"], ["\u015B", "\u0107", "\u00D7"], ["\u00F3", "\u017A"]],
        [["Caps", "Caps"], ["a", "A"], ["s", "S", "\u0111"], ["d", "D", "\u0110"], ["f", "F"], ["g", "G"], ["h", "H"], ["j", "J"], ["k", "K"], ["l", "L"], ["\u0142", "\u0141", "$"], ["\u0105", "\u0119", "\u00DF"], ["Enter", "Enter"]],
        [["Shift", "Shift"], ["<", ">"], ["y", "Y"], ["x", "X"], ["c", "C"], ["v", "V", "@"], ["b", "B", "{"], ["n", "N", "}"], ["m", "M", "\u00A7"], [",", ";", "<"], [".", ":", ">"], ["-", "_"], ["Shift", "Shift"]],
        [[" ", " ", " ", " "], ["AltGr", "AltGr"]]
            ]};
    this.VKI_layout['Portugu\u00eas Brasileiro'] = {
        'name': "Portuguese (Brazil)", 'keys': [
            [["'", '"'], ["1", "!", "\u00b9"], ["2", "@", "\u00b2"], ["3", "#", "\u00b3"], ["4", "$", "\u00a3"], ["5", "%", "\u00a2"], ["6", "\u00a8", "\u00ac"], ["7", "&"], ["8", "*"], ["9", "("], ["0", ")"], ["-", "_"], ["=", "+", "\u00a7"], ["Bksp", "Bksp"]],
        [["Tab", "Tab"], ["q", "Q", "/"], ["w", "W", "?"], ["e", "E", "\u20ac"], ["r", "R"], ["t", "T"], ["y", "Y"], ["u", "U"], ["i", "I"], ["o", "O"], ["p", "P"], ["\u00b4", "`"], ["[", "{", "\u00aa"], ["Enter", "Enter"]],
        [["Caps", "Caps"], ["a", "A"], ["s", "S"], ["d", "D"], ["f", "F"], ["g", "G"], ["h", "H"], ["j", "J"], ["k", "K"], ["l", "L"], ["\u00e7", "\u00c7"], ["~", "^"], ["]", "}", "\u00ba"], ["/", "?"]],
        [["Shift", "Shift"], ["\\", "|"], ["z", "Z"], ["x", "X"], ["c", "C", "\u20a2"], ["v", "V"], ["b", "B"], ["n", "N"], ["m", "M"], [",", "<"], [".", ">"], [":", ":"], ["Shift", "Shift"]],
        [[" ", " ", " ", " "], ["AltGr", "AltGr"]]
            ], 'lang': ["pt-BR"] };

    this.VKI_layout['Portugu\u00eas'] = {
        'name': "Portuguese", 'keys': [
            [["\\", "|"], ["1", "!"], ["2", '"', "@"], ["3", "#", "\u00a3"], ["4", "$", "\u00a7"], ["5", "%"], ["6", "&"], ["7", "/", "{"], ["8", "(", "["], ["9", ")", "]"], ["0", "=", "}"], ["'", "?"], ["\u00ab", "\u00bb"], ["Bksp", "Bksp"]],
        [["Tab", "Tab"], ["q", "Q"], ["w", "W"], ["e", "E", "\u20ac"], ["r", "R"], ["t", "T"], ["y", "Y"], ["u", "U"], ["i", "I"], ["o", "O"], ["p", "P"], ["+", "*", "\u00a8"], ["\u00b4", "`"], ["~", "^"]],
        [["Caps", "Caps"], ["a", "A"], ["s", "S"], ["d", "D"], ["f", "F"], ["g", "G"], ["h", "H"], ["j", "J"], ["k", "K"], ["l", "L"], ["\u00e7", "\u00c7"], ["\u00ba", "\u00aa"], ["Enter", "Enter"]],
        [["Shift", "Shift"], ["<", ">", "\\"], ["z", "Z"], ["x", "X"], ["c", "C"], ["v", "V"], ["b", "B"], ["n", "N"], ["m", "M"], [",", ";"], [".", ":"], ["-", "_"], ["Shift", "Shift"]],
        [[" ", " ", " ", " "], ["AltGr", "AltGr"]]
            ], 'lang': ["pt"] };

    this.VKI_layout['Rom\u00e2n\u0103'] = {
        'name': "Romanian", 'keys': [
            [["\u201E", "\u201D", "`", "~"], ["1", "!", "~"], ["2", "@", "\u02C7"], ["3", "#", "^"], ["4", "$", "\u02D8"], ["5", "%", "\u00B0"], ["6", "^", "\u02DB"], ["7", "&", "`"], ["8", "*", "\u02D9"], ["9", "(", "\u00B4"], ["0", ")", "\u02DD"], ["-", "_", "\u00A8"], ["=", "+", "\u00B8", "\u00B1"], ["Bksp", "Bksp"]],
        [["Tab", "Tab"], ["q", "Q"], ["w", "W"], ["e", "E", "\u20AC"], ["r", "R"], ["t", "T"], ["y", "Y"], ["u", "U"], ["i", "I"], ["o", "O"], ["p", "P", "\u00A7"], ["\u0103", "\u0102", "[", "{"], ["\u00EE", "\u00CE", "]", "}"], ["\u00E2", "\u00C2", "\\", "|"]],
        [["Caps", "Caps"], ["a", "A"], ["s", "S", "\u00df"], ["d", "D", "\u00f0", "\u00D0"], ["f", "F"], ["g", "G"], ["h", "H"], ["j", "J"], ["k", "K"], ["l", "L", "\u0142", "\u0141"], ["\u0219", "\u0218", ";", ":"], ["\u021B", "\u021A", "\'", "\""], ["Enter", "Enter"]],
        [["Shift", "Shift"], ["\\", "|"], ["z", "Z"], ["x", "X"], ["c", "C", "\u00A9"], ["v", "V"], ["b", "B"], ["n", "N"], ["m", "M"], [",", ";", "<", "\u00AB"], [".", ":", ">", "\u00BB"], ["/", "?"], ["Shift", "Shift"]],
        [[" ", " ", " ", " "], ["AltGr", "AltGr"]]
            ], 'lang': ["ro"] };

    this.VKI_layout['\u0420\u0443\u0441\u0441\u043a\u0438\u0439'] = {
        'name': "Russian", 'keys': [
            [["\u0451", "\u0401"], ["1", "!"], ["2", '"'], ["3", "\u2116"], ["4", ";"], ["5", "%"], ["6", ":"], ["7", "?"], ["8", "*"], ["9", "("], ["0", ")"], ["-", "_"], ["=", "+"], ["Bksp", "Bksp"]],
        [["Tab", "Tab"], ["\u0439", "\u0419"], ["\u0446", "\u0426"], ["\u0443", "\u0423"], ["\u043A", "\u041A"], ["\u0435", "\u0415"], ["\u043D", "\u041D"], ["\u0433", "\u0413"], ["\u0448", "\u0428"], ["\u0449", "\u0429"], ["\u0437", "\u0417"], ["\u0445", "\u0425"], ["\u044A", "\u042A"], ["\\", "/"]],
        [["Caps", "Caps"], ["\u0444", "\u0424"], ["\u044B", "\u042B"], ["\u0432", "\u0412"], ["\u0430", "\u0410"], ["\u043F", "\u041F"], ["\u0440", "\u0420"], ["\u043E", "\u041E"], ["\u043B", "\u041B"], ["\u0434", "\u0414"], ["\u0436", "\u0416"], ["\u044D", "\u042D"], ["Enter", "Enter"]],
        [["Shift", "Shift"], ["/", "|"], ["\u044F", "\u042F"], ["\u0447", "\u0427"], ["\u0441", "\u0421"], ["\u043C", "\u041C"], ["\u0438", "\u0418"], ["\u0442", "\u0422"], ["\u044C", "\u042C"], ["\u0431", "\u0411"], ["\u044E", "\u042E"], [".", ","], ["Shift", "Shift"]],
        [[" ", " "]]
            ], 'lang': ["ru"] };

    this.VKI_layout['Deutsch (Schweiz)'] = {
        'name': "Swiss German", 'keys': [
            [["\u00A7", "\u00B0"], ["1", "+", "\u00A6"], ["2", '"', "@"], ["3", "*", "#"], ["4", "\u00E7", "\u00B0"], ["5", "%", "\u00A7"], ["6", "&", "\u00AC"], ["7", "/", "|"], ["8", "(", "\u00A2"], ["9", ")"], ["0", "="], ["'", "?", "\u00B4"], ["^", "`", "~"], ["Bksp", "Bksp"]],
        [["Tab", "Tab"], ["q", "Q"], ["w", "W"], ["e", "E", "\u20AC"], ["r", "R"], ["t", "T"], ["z", "Z"], ["u", "U"], ["i", "I"], ["o", "O"], ["p", "P"], ["\u00FC", "\u00E8", "["], ["\u00A8", "!", "]"], ["$", "\u00A3", "}"]],
        [["Caps", "Caps"], ["a", "A"], ["s", "S"], ["d", "D"], ["f", "F"], ["g", "G"], ["h", "H"], ["j", "J"], ["k", "K"], ["l", "L"], ["\u00F6", "\u00E9"], ["\u00E4", "\u00E0", "{"], ["Enter", "Enter"]],
        [["Shift", "Shift"], ["<", ">", "\\"], ["y", "Y"], ["x", "X"], ["c", "C"], ["v", "V"], ["b", "B"], ["n", "N"], ["m", "M"], [",", ";"], [".", ":"], ["-", "_"], ["Shift", "Shift"]],
        [[" ", " ", " ", " "], ["AltGr", "AltGr"]]
            ], 'lang': ["de-CH"] };
    this.VKI_layout['United Kingdom'] = {
        'name': "United Kingdom", 'keys': [
            [["`", "\u00ac", "\u00a6"], ["1", "!"], ["2", '"'], ["3", "\u00a3"], ["4", "$", "\u20ac"], ["5", "%"], ["6", "^"], ["7", "&"], ["8", "*"], ["9", "("], ["0", ")"], ["-", "_"], ["=", "+"], ["Bksp", "Bksp"]],
        [["Tab", "Tab"], ["q", "Q"], ["w", "W"], ["e", "E", "\u00e9", "\u00c9"], ["r", "R"], ["t", "T"], ["y", "Y"], ["u", "U", "\u00fa", "\u00da"], ["i", "I", "\u00ed", "\u00cd"], ["o", "O", "\u00f3", "\u00d3"], ["p", "P"], ["[", "{"], ["]", "}"], ["#", "~"]],
        [["Caps", "Caps"], ["a", "A", "\u00e1", "\u00c1"], ["s", "S"], ["d", "D"], ["f", "F"], ["g", "G"], ["h", "H"], ["j", "J"], ["k", "K"], ["l", "L"], [";", ":"], ["'", "@"], ["Enter", "Enter"]],
        [["Shift", "Shift"], ["\\", "|"], ["z", "Z"], ["x", "X"], ["c", "C"], ["v", "V"], ["b", "B"], ["n", "N"], ["m", "M"], [",", "<"], [".", ">"], ["/", "?"], ["Shift", "Shift"]],
        [[" ", " ", " ", " "], ["AltGr", "AltGr"]]
            ], 'lang': ["en-gb"] };
    this.VKI_layout['US Standard'] = {
        'name': "US Standard", 'keys': [
            [["`", "~"], ["1", "!"], ["2", "@"], ["3", "#"], ["4", "$"], ["5", "%"], ["6", "^"], ["7", "&"], ["8", "*"], ["9", "("], ["0", ")"], ["-", "_"], ["=", "+"], ["Bksp", "Bksp"]],
        [["Tab", "Tab"], ["q", "Q"], ["w", "W"], ["e", "E"], ["r", "R"], ["t", "T"], ["y", "Y"], ["u", "U"], ["i", "I"], ["o", "O"], ["p", "P"], ["[", "{"], ["]", "}"], ["\\", "|"]],
        [["Caps", "Caps"], ["a", "A"], ["s", "S"], ["d", "D"], ["f", "F"], ["g", "G"], ["h", "H"], ["j", "J"], ["k", "K"], ["l", "L"], [";", ":"], ["'", '"'], ["Enter", "Enter"]],
        [["Shift", "Shift"], ["z", "Z"], ["x", "X"], ["c", "C"], ["v", "V"], ["b", "B"], ["n", "N"], ["m", "M"], [",", "<"], [".", ">"], ["/", "?"], ["Shift", "Shift"]],
        [[" ", " "]]
            ], 'lang': ["en-us"] };

    this.VKI_layout['US International'] = {
        'name': "US International", 'keys': [
            [["`", "~"], ["1", "!", "\u00a1", "\u00b9"], ["2", "@", "\u00b2"], ["3", "#", "\u00b3"], ["4", "$", "\u00a4", "\u00a3"], ["5", "%", "\u20ac"], ["6", "^", "\u00bc"], ["7", "&", "\u00bd"], ["8", "*", "\u00be"], ["9", "(", "\u2018"], ["0", ")", "\u2019"], ["-", "_", "\u00a5"], ["=", "+", "\u00d7", "\u00f7"], ["Bksp", "Bksp"]],
        [["Tab", "Tab"], ["q", "Q", "\u00e4", "\u00c4"], ["w", "W", "\u00e5", "\u00c5"], ["e", "E", "\u00e9", "\u00c9"], ["r", "R", "\u00ae"], ["t", "T", "\u00fe", "\u00de"], ["y", "Y", "\u00fc", "\u00dc"], ["u", "U", "\u00fa", "\u00da"], ["i", "I", "\u00ed", "\u00cd"], ["o", "O", "\u00f3", "\u00d3"], ["p", "P", "\u00f6", "\u00d6"], ["[", "{", "\u00ab"], ["]", "}", "\u00bb"], ["\\", "|", "\u00ac", "\u00a6"]],
        [["Caps", "Caps"], ["a", "A", "\u00e1", "\u00c1"], ["s", "S", "\u00df", "\u00a7"], ["d", "D", "\u00f0", "\u00d0"], ["f", "F"], ["g", "G"], ["h", "H"], ["j", "J"], ["k", "K"], ["l", "L", "\u00f8", "\u00d8"], [";", ":", "\u00b6", "\u00b0"], ["'", '"', "\u00b4", "\u00a8"], ["Enter", "Enter"]],
        [["Shift", "Shift"], ["z", "Z", "\u00e6", "\u00c6"], ["x", "X"], ["c", "C", "\u00a9", "\u00a2"], ["v", "V"], ["b", "B"], ["n", "N", "\u00f1", "\u00d1"], ["m", "M", "\u00b5"], [",", "<", "\u00e7", "\u00c7"], [".", ">"], ["/", "?", "\u00bf"], ["Shift", "Shift"]],
        [[" ", " ", " ", " "], ["Alt", "Alt"]]
            ], 'lang': ["en"] };

    /* ***** Define Dead Keys ************************************** */
    this.VKI_deadkey = {};

    // - Lay out each dead key set as an object of property/value
    //   pairs.  The rows below are wrapped so uppercase letters are
    //   below their lowercase equivalents.
    //
    // - The property name is the letter pressed after the diacritic.
    //   The property value is the letter this key-combo will generate.
    //
    // - Note that if you have created a new keyboard layout and want
    //   it included in the distributed script, PLEASE TELL ME if you
    //   have added additional dead keys to the ones below.

    this.VKI_deadkey['"'] = this.VKI_deadkey['\u00a8'] = this.VKI_deadkey['\u309B'] = { // Umlaut / Diaeresis / Greek Dialytika / Hiragana/Katakana Voiced Sound Mark
        'a': "\u00e4", 'e': "\u00eb", 'i': "\u00ef", 'o': "\u00f6", 'u': "\u00fc", 'y': "\u00ff", '\u03b9': "\u03ca", '\u03c5': "\u03cb", '\u016B': "\u01D6", '\u00FA': "\u01D8", '\u01D4': "\u01DA", '\u00F9': "\u01DC",
        'A': "\u00c4", 'E': "\u00cb", 'I': "\u00cf", 'O': "\u00d6", 'U': "\u00dc", 'Y': "\u0178", '\u0399': "\u03aa", '\u03a5': "\u03ab", '\u016A': "\u01D5", '\u00DA': "\u01D7", '\u01D3': "\u01D9", '\u00D9': "\u01DB",
        '\u304b': "\u304c", '\u304d': "\u304e", '\u304f': "\u3050", '\u3051': "\u3052", '\u3053': "\u3054", '\u305f': "\u3060", '\u3061': "\u3062", '\u3064': "\u3065", '\u3066': "\u3067", '\u3068': "\u3069",
        '\u3055': "\u3056", '\u3057': "\u3058", '\u3059': "\u305a", '\u305b': "\u305c", '\u305d': "\u305e", '\u306f': "\u3070", '\u3072': "\u3073", '\u3075': "\u3076", '\u3078': "\u3079", '\u307b': "\u307c",
        '\u30ab': "\u30ac", '\u30ad': "\u30ae", '\u30af': "\u30b0", '\u30b1': "\u30b2", '\u30b3': "\u30b4", '\u30bf': "\u30c0", '\u30c1': "\u30c2", '\u30c4': "\u30c5", '\u30c6': "\u30c7", '\u30c8': "\u30c9",
        '\u30b5': "\u30b6", '\u30b7': "\u30b8", '\u30b9': "\u30ba", '\u30bb': "\u30bc", '\u30bd': "\u30be", '\u30cf': "\u30d0", '\u30d2': "\u30d3", '\u30d5': "\u30d6", '\u30d8': "\u30d9", '\u30db': "\u30dc"
    };
    this.VKI_deadkey['~'] = { // Tilde / Stroke
        'a': "\u00e3", 'l': "\u0142", 'n': "\u00f1", 'o': "\u00f5",
        'A': "\u00c3", 'L': "\u0141", 'N': "\u00d1", 'O': "\u00d5"
    };
    this.VKI_deadkey['^'] = { // Circumflex
        'a': "\u00e2", 'e': "\u00ea", 'i': "\u00ee", 'o': "\u00f4", 'u': "\u00fb", 'w': "\u0175", 'y': "\u0177",
        'A': "\u00c2", 'E': "\u00ca", 'I': "\u00ce", 'O': "\u00d4", 'U': "\u00db", 'W': "\u0174", 'Y': "\u0176"
    };
    this.VKI_deadkey['\u02c7'] = { // Baltic caron
        'c': "\u010D", 'd': "\u010f", 'e': "\u011b", 's': "\u0161", 'l': "\u013e", 'n': "\u0148", 'r': "\u0159", 't': "\u0165", 'u': "\u01d4", 'z': "\u017E", '\u00fc': "\u01da",
        'C': "\u010C", 'D': "\u010e", 'E': "\u011a", 'S': "\u0160", 'L': "\u013d", 'N': "\u0147", 'R': "\u0158", 'T': "\u0164", 'U': "\u01d3", 'Z': "\u017D", '\u00dc': "\u01d9"
    };
    this.VKI_deadkey['\u02d8'] = { // Romanian and Turkish breve
        'a': "\u0103", 'g': "\u011f",
        'A': "\u0102", 'G': "\u011e"
    };
    this.VKI_deadkey['-'] = this.VKI_deadkey['\u00af'] = { // Macron
        'a': "\u0101", 'e': "\u0113", 'i': "\u012b", 'o': "\u014d", 'u': "\u016B", 'y': "\u0233", '\u00fc': "\u01d6",
        'A': "\u0100", 'E': "\u0112", 'I': "\u012a", 'O': "\u014c", 'U': "\u016A", 'Y': "\u0232", '\u00dc': "\u01d5"
    };
    this.VKI_deadkey['`'] = { // Grave
        'a': "\u00e0", 'e': "\u00e8", 'i': "\u00ec", 'o': "\u00f2", 'u': "\u00f9", '\u00fc': "\u01dc",
        'A': "\u00c0", 'E': "\u00c8", 'I': "\u00cc", 'O': "\u00d2", 'U': "\u00d9", '\u00dc': "\u01db"
    };
    this.VKI_deadkey["'"] = this.VKI_deadkey['\u00b4'] = this.VKI_deadkey['\u0384'] = { // Acute / Greek Tonos
        'a': "\u00e1", 'e': "\u00e9", 'i': "\u00ed", 'o': "\u00f3", 'u': "\u00fa", 'y': "\u00fd", '\u03b1': "\u03ac", '\u03b5': "\u03ad", '\u03b7': "\u03ae", '\u03b9': "\u03af", '\u03bf': "\u03cc", '\u03c5': "\u03cd", '\u03c9': "\u03ce", '\u00fc': "\u01d8",
        'A': "\u00c1", 'E': "\u00c9", 'I': "\u00cd", 'O': "\u00d3", 'U': "\u00da", 'Y': "\u00dd", '\u0391': "\u0386", '\u0395': "\u0388", '\u0397': "\u0389", '\u0399': "\u038a", '\u039f': "\u038c", '\u03a5': "\u038e", '\u03a9': "\u038f", '\u00dc': "\u01d7"
    };
    this.VKI_deadkey['\u02dd'] = { // Hungarian Double Acute Accent
        'o': "\u0151", 'u': "\u0171",
        'O': "\u0150", 'U': "\u0170"
    };
    this.VKI_deadkey['\u0385'] = { // Greek Dialytika + Tonos
        '\u03b9': "\u0390", '\u03c5': "\u03b0"
    };
    this.VKI_deadkey['\u00b0'] = this.VKI_deadkey['\u00ba'] = { // Ring
        'a': "\u00e5", 'u': "\u016f",
        'A': "\u00c5", 'U': "\u016e"
    };
    this.VKI_deadkey['\u02DB'] = { // Ogonek
        'a': "\u0106", 'e': "\u0119", 'i': "\u012f", 'o': "\u01eb", 'u': "\u0173", 'y': "\u0177",
        'A': "\u0105", 'E': "\u0118", 'I': "\u012e", 'O': "\u01ea", 'U': "\u0172", 'Y': "\u0176"
    };
    this.VKI_deadkey['\u02D9'] = { // Dot-above
        'c': "\u010B", 'e': "\u0117", 'g': "\u0121", 'z': "\u017C",
        'C': "\u010A", 'E': "\u0116", 'G': "\u0120", 'Z': "\u017B"
    };
    this.VKI_deadkey['\u00B8'] = this.VKI_deadkey['\u201a'] = { // Cedilla
        'c': "\u00e7", 's': "\u015F",
        'C': "\u00c7", 'S': "\u015E"
    };
    this.VKI_deadkey[','] = { // Comma
        's': "\u0219", 't': "\u021B",
        'S': "\u0218", 'T': "\u021A"
    };
    this.VKI_deadkey['\u3002'] = { // Hiragana/Katakana Point
        '\u306f': "\u3071", '\u3072': "\u3074", '\u3075': "\u3077", '\u3078': "\u307a", '\u307b': "\u307d",
        '\u30cf': "\u30d1", '\u30d2': "\u30d4", '\u30d5': "\u30d7", '\u30d8': "\u30da", '\u30db': "\u30dd"
    };


    /* ***** Define Symbols **************************************** */
    this.VKI_symbol = {
        '\u00a0': "NB\nSP", '\u200b': "ZW\nSP", '\u200c': "ZW\nNJ", '\u200d': "ZW\nJ"
    };


    /* ***** Layout Number Pad ************************************* */
    this.VKI_numpad = [
        [["$"], ["\u00a3"], ["\u20ac"], ["\u00a5"]],
        [["7"], ["8"], ["9"], ["/"]],
        [["4"], ["5"], ["6"], ["*"]],
        [["1"], ["2"], ["3"], ["-"]],
        [["0"], ["."], ["="], ["+"]]
            ];

    /* ****************************************************************
     * Common mouse event actions
     *
     */
    function VKI_mouseEvents(elem) {
        if ('td' == elem.get('tag')) {
            if (!elem.click) elem.click = function() {
                var evt = this.ownerDocument.createEvent('MouseEvents');
                evt.initMouseEvent('click', true, true, this.ownerDocument.defaultView, 1, 0, 0, 0, 0, false, false, false, false, 0, null);
                this.dispatchEvent(evt);
            };
            elem.VKI_clickless = 0;
            VKI_addListener(elem, 'dblclick', function() { return false; }, false);
        }
        VKI_addListener(elem, 'mouseover', function() {
            if (this.nodeName == "TD" && self.VKI_clickless) {
                var _self = this;
                clearTimeout(this.VKI_clickless);
                this.VKI_clickless = setTimeout(function() { _self.click(); }, self.VKI_clickless);
            }
        }, false);
        VKI_addListener(elem, 'mouseout', function() {
            if (this.nodeName == "TD") clearTimeout(this.VKI_clickless);
        }, false);
        VKI_addListener(elem, 'mousedown', function() {
            if (this.nodeName == "TD") clearTimeout(this.VKI_clickless);
        }, false);
        VKI_addListener(elem, 'mouseup', function() {
            if (this.nodeName == "TD") clearTimeout(this.VKI_clickless);
        }, false);
    }


    /* ***** Build the keyboard interface ************************** */
    if (!this.VKI_layout[this.VKI_kt]) {
        throw 'No layout named "' + this.VKI_kt + '"';
    }

    this.VKI_keyboard = new Element('table', {
        'id': 'keyboardInputMaster',
        'dir': 'ltr',
        'cellspacing': 0
    }).addEvent('click', function(e) { e.stopPropagation(); });
    // this.VKI_keyboard = document.createElement('table');
    // this.VKI_keyboard.id = "keyboardInputMaster";
    // this.VKI_keyboard.dir = "ltr";
    // this.VKI_keyboard.cellSpacing = "0";
    // VKI_addListener(this.VKI_keyboard, 'click', function(e) {
        // e = e || event;
        // if (e.stopPropagation) { e.stopPropagation(); } else e.cancelBubble = true;
        // return false;
    // }, false);

    this.VKI_langCode = {};
    var thead = new Element('thead');
    // var thead = document.createElement('thead');
    var tr = new Element('tr');
    // var tr = document.createElement('tr');
    var th = new Element('th', {'colspan':2});
    // var th = document.createElement('th');
    //th.colSpan = "2";

    var kbSelect = new Element('div', {'title': this.VKI_i18n['02']});
    // var kbSelect = document.createElement('div');
    // kbSelect.title = this.VKI_i18n['02'];
    VKI_addListener(kbSelect, 'click', function() {
        var ol = this.getElementsByTagName('ol')[0];
        if (!ol.style.display) {
            ol.style.display = "block";
            var li = ol.getElementsByTagName('li');
            for (var x = 0, scr = 0; x < li.length; x++) {
                if (VKI_kt == li[x].firstChild.nodeValue) {
                    li[x].className = "selected";
                    scr = li[x].offsetTop - li[x].offsetHeight * 2;
                } else li[x].className = "";
            } setTimeout(function() { ol.scrollTop = scr; }, 0);
        } else ol.style.display = "";
    }, false);
    kbSelect.appendChild(document.createTextNode(this.VKI_kt));
    kbSelect.appendChild(document.createTextNode(" \u25be"));
    kbSelect.langCount = 0;
    var ol = document.createElement('ol');
    for (ktype in this.VKI_layout) {
        if (typeof this.VKI_layout[ktype] == "object") {
            if (!this.VKI_layout[ktype].lang) { 
                this.VKI_layout[ktype].lang = [];
            }
            for (var x = 0; x < this.VKI_layout[ktype].lang.length; x++) {
                this.VKI_langCode[this.VKI_layout[ktype].lang[x].toLowerCase().replace(/-/g, "_")] = ktype;
            }
            var li = document.createElement('li');
            li.title = this.VKI_layout[ktype].name;
            VKI_addListener(li, 'click', function(e) {
                e = e || event;
                if (e.stopPropagation) {
                    e.stopPropagation();
                } else {
                    e.cancelBubble = true;
                }
                this.parentNode.style.display = "";
                self.VKI_kts = self.VKI_kt = kbSelect.firstChild.nodeValue = this.firstChild.nodeValue;
                self.VKI_buildKeys();
                // self.VKI_position(true);
            }, false);
            VKI_mouseEvents(li);
            li.appendChild(document.createTextNode(ktype));
            ol.appendChild(li);
            kbSelect.langCount++;
        }
    }
    kbSelect.appendChild(ol);
    if (kbSelect.langCount > 1) {
        th.appendChild(kbSelect);
    }
    this.VKI_langCode.index = [];
    for (prop in this.VKI_langCode) {
        if (prop != "index" && typeof this.VKI_langCode[prop] == "string") {
            this.VKI_langCode.index.push(prop);
        }
    }
    this.VKI_langCode.index.sort();
    this.VKI_langCode.index.reverse();
    if (this.VKI_numberPad) {
        var span = new Element('span', {
            'html': '#',
            'title': this.VKI_i18n['00']
        }).addEvent('click', function() {
            kbNumpad.style.display = (!kbNumpad.style.display) ? "none" : "";
            // self.VKI_position(true);
        }.bind(this)).inject(th);
        // var span = document.createElement('span');
        // span.appendChild(document.createTextNode("#"));
        // span.title = this.VKI_i18n['00'];
        //VKI_addListener(span, 'click', function() {
        //    kbNumpad.style.display = (!kbNumpad.style.display) ? "none" : "";
        //    self.VKI_position(true);
        //}, false);
        VKI_mouseEvents(span);
        // th.appendChild(span);
    }
    this.VKI_kbsize = function(e) {
        this.VKI_size = Math.min(5, Math.max(1, this.VKI_size));
        for (var i = 0; i < 6; i++) {
            this.VKI_keyboard.removeClass('keyboardInputSize' + i);
        }
        if (this.VKI_size != 2) {
            this.VKI_keyboard.addClass('keyboardInputSize' + this.VKI_size);
        }
        // this.VKI_position(true);
    };
    if (this.VKI_sizeAdj) {
        var small = document.createElement('small');
        small.title = this.VKI_i18n['10'];
        VKI_addListener(small, 'click', function() {
            --self.VKI_size;
            self.VKI_kbsize();
        }, false);
        VKI_mouseEvents(small);
        small.appendChild(document.createTextNode("\u21d3"));
        th.appendChild(small);
        var big = document.createElement('big');
        big.title = this.VKI_i18n['11'];
        VKI_addListener(big, 'click', function() {
            ++self.VKI_size;
            self.VKI_kbsize();
        }, false);
        VKI_mouseEvents(big);
        big.appendChild(document.createTextNode("\u21d1"));
        th.appendChild(big);
    }

    var span = document.createElement('span');
    span.appendChild(document.createTextNode(this.VKI_i18n['07']));
    span.title = this.VKI_i18n['08'];
    VKI_addListener(span, 'click', function() {
        self.VKI_target.value = "";
        self.VKI_target.focus();
        return false;
    }, false);
    VKI_mouseEvents(span);
    th.appendChild(span);

    var closeBox = new Element('strong', {
        'html': 'X',
        'title': this.VKI_i18n['06']
    });

    VKI_addListener(closeBox, 'click', function() { self.VKI_close(); }, false);
    VKI_mouseEvents(closeBox);
    th.appendChild(closeBox);

    tr.appendChild(th);
    thead.appendChild(tr);
    this.VKI_keyboard.appendChild(thead);

    var tbody = document.createElement('tbody');
    var tr = document.createElement('tr');
    var td = document.createElement('td');
    var div = document.createElement('div');

    if (this.VKI_deadBox) {
        var label = document.createElement('label');
        var checkbox = document.createElement('input');
        checkbox.type = "checkbox";
        checkbox.title = this.VKI_i18n['03'] + ": " + ((this.VKI_deadkeysOn) ? this.VKI_i18n['04'] : this.VKI_i18n['05']);
        checkbox.defaultChecked = this.VKI_deadkeysOn;
        VKI_addListener(checkbox, 'click', function() {
            this.title = self.VKI_i18n['03'] + ": " + ((this.checked) ? self.VKI_i18n['04'] : self.VKI_i18n['05']);
            self.VKI_modify("");
            return true;
        }, false);
        label.appendChild(checkbox);
        checkbox.checked = this.VKI_deadkeysOn;
        div.appendChild(label);
        this.VKI_deadkeysOn = checkbox;
    } else
        this.VKI_deadkeysOn.checked = this.VKI_deadkeysOn;
    if (this.VKI_showVersion) {
        var vr = document.createElement('var');
        vr.title = this.VKI_i18n['09'] + " " + this.VKI_version;
        vr.appendChild(document.createTextNode("v" + this.VKI_version));
        div.appendChild(vr);
    }
    td.appendChild(div);
    tr.appendChild(td);
    var kbNumpad = document.createElement('td');
    kbNumpad.id = "keyboardInputNumpad";
    if (!this.VKI_numberPadOn) kbNumpad.style.display = "none";
    var ntable = document.createElement('table');
    ntable.cellSpacing = "0";
    var ntbody = document.createElement('tbody');
    for (var x = 0; x < this.VKI_numpad.length; x++) {
        var ntr = document.createElement('tr');
        for (var y = 0; y < this.VKI_numpad[x].length; y++) {
            var ntd = document.createElement('td');
            VKI_addListener(ntd, 'click', VKI_keyClick, false);
            VKI_mouseEvents(ntd);
            ntd.appendChild(document.createTextNode(this.VKI_numpad[x][y]));
            ntr.appendChild(ntd);
        } ntbody.appendChild(ntr);
    } ntable.appendChild(ntbody);
    kbNumpad.appendChild(ntable);
    tr.appendChild(kbNumpad);
    tbody.appendChild(tr);
    this.VKI_keyboard.appendChild(tbody);

    /* ****************************************************************
     * Private table cell attachment function for generic characters
     *
     */
    function VKI_keyClick() {
        var done = false, character = "\xa0";
        if (this.firstChild.nodeName.toLowerCase() != "small") {
            if ((character = this.firstChild.nodeValue) == "\xa0") return false;
        } else character = this.firstChild.getAttribute('char');
        if (self.VKI_deadkeysOn.checked && self.VKI_dead) {
            if (self.VKI_dead != character) {
                if (character != " ") {
                    if (self.VKI_deadkey[self.VKI_dead][character]) {
                        self.VKI_insert(self.VKI_deadkey[self.VKI_dead][character]);
                        done = true;
                    }
                } else {
                    self.VKI_insert(self.VKI_dead);
                    done = true;
                }
            } else done = true;
        } self.VKI_dead = false;

        if (!done) {
            if (self.VKI_deadkeysOn.checked && self.VKI_deadkey[character]) {
                self.VKI_dead = character;
                this.className += " dead";
                if (self.VKI_shift) self.VKI_modify("Shift");
                if (self.VKI_altgr) self.VKI_modify("AltGr");
            } else self.VKI_insert(character);
        } self.VKI_modify("");
        return false;
    }


    /* ****************************************************************
     * Build or rebuild the keyboard keys
     *
     */
    this.VKI_buildKeys = function() {
        this.VKI_shift = this.VKI_shiftlock = this.VKI_altgr = this.VKI_altgrlock = this.VKI_dead = false;
        var container = this.VKI_keyboard.tBodies[0].getElementsByTagName('div')[0];
        var tables = container.getElementsByTagName('table');
        for (var x = tables.length - 1; x >= 0; x--) container.removeChild(tables[x]);

        for (var x = 0, hasDeadKey = false, lyt; lyt = this.VKI_layout[this.VKI_kt].keys[x++];) {
            var table = document.createElement('table');
            table.cellSpacing = "0";
            if (lyt.length <= this.VKI_keyCenter) table.className = "keyboardInputCenter";
            var tbody = document.createElement('tbody');
            var tr = document.createElement('tr');
            for (var y = 0, lkey; lkey = lyt[y++];) {
                var td = document.createElement('td');
                if (this.VKI_symbol[lkey[0]]) {
                    var text = this.VKI_symbol[lkey[0]].split("\n");
                    var small = document.createElement('small');
                    small.setAttribute('char', lkey[0]);
                    for (var z = 0; z < text.length; z++) {
                        if (z) small.appendChild(document.createElement("br"));
                        small.appendChild(document.createTextNode(text[z]));
                    } td.appendChild(small);
                } else td.appendChild(document.createTextNode(lkey[0] || "\xa0"));

                var className = [];
                if (this.VKI_deadkeysOn.checked)
                    for (key in this.VKI_deadkey)
                        if (key === lkey[0]) { className.push("deadkey"); break; }
                if (lyt.length > this.VKI_keyCenter && y == lyt.length) className.push("last");
                if (lkey[0] == " " || lkey[1] == " ") className.push("space");
                td.className = className.join(" ");

                switch (lkey[1]) {
                    case "Caps": case "Shift":
                    case "Alt": case "AltGr": case "AltLk":
                        VKI_addListener(td, 'click', (function(type) { return function() { self.VKI_modify(type); return false; }})(lkey[1]), false);
                        break;
                    case "Tab":
                        VKI_addListener(td, 'click', function() {
                            /*
                               if (self.VKI_activeTab) {
                               if (self.VKI_target.form) {
                               var target = self.VKI_target, elems = target.form.elements;
                               self.VKI_close();
                               for (var z = 0, me = false, j = -1; z < elems.length; z++) {
                               if (j == -1 && elems[z].getAttribute("VKI_attached")) j = z;
                               if (me) {
                               if (self.VKI_activeTab == 1 && elems[z]) break;
                               if (elems[z].getAttribute("VKI_attached")) break;
                               } else if (elems[z] == target) me = true;
                               } if (z == elems.length) z = Math.max(j, 0);
                               if (elems[z].getAttribute("VKI_attached")) {
                               self.VKI_show(elems[z]);
                               } else elems[z].focus();
                               } else self.VKI_target.focus();
                               } else */ self.VKI_insert("\t");
                            return false;
                        }, false);
                        break;
                    case "Bksp":
                        VKI_addListener(td, 'click', function() {
                            /*
                               self.VKI_target.focus();
                               if (self.VKI_target.setSelectionRange && !self.VKI_target.readOnly) {
                               var rng = [self.VKI_target.selectionStart, self.VKI_target.selectionEnd];
                               if (rng[0] < rng[1]) rng[0]++;
                               self.VKI_target.value = self.VKI_target.value.substr(0, rng[0] - 1) + self.VKI_target.value.substr(rng[1]);
                               self.VKI_target.setSelectionRange(rng[0] - 1, rng[0] - 1);
                               } else if (self.VKI_target.createTextRange && !self.VKI_target.readOnly) {
                               try {
                               self.VKI_target.range.select();
                               } catch(e) { self.VKI_target.range = document.selection.createRange(); }
                               if (!self.VKI_target.range.text.length) self.VKI_target.range.moveStart('character', -1);
                               self.VKI_target.range.text = "";
                               } else self.VKI_target.value = self.VKI_target.value.substr(0, self.VKI_target.value.length - 1);
                               if (self.VKI_shift) self.VKI_modify("Shift");
                               if (self.VKI_altgr) self.VKI_modify("AltGr");
                               self.VKI_target.focus();
                               return true;
                               */
                            self.VKI_insert("\b")
                        }, false);
                        break;
                    case "Enter":
                        VKI_addListener(td, 'click', function() {
                            /*
                               if (self.VKI_target.nodeName != "TEXTAREA") {
                               if (self.VKI_enterSubmit && self.VKI_target.form) {
                               for (var z = 0, subm = false; z < self.VKI_target.form.elements.length; z++)
                               if (self.VKI_target.form.elements[z].type == "submit") subm = true;
                               if (!subm) self.VKI_target.form.submit();
                               }
                               self.VKI_close();
                               } else */ self.VKI_insert("\n");
                            return true;
                        }, false);
                        break;
                    default:
                        VKI_addListener(td, 'click', VKI_keyClick, false);

                }
                VKI_mouseEvents(td);
                tr.appendChild(td);
                for (var z = 0; z < 4; z++)
                    if (this.VKI_deadkey[lkey[z] = lkey[z] || ""]) hasDeadKey = true;
            } tbody.appendChild(tr);
            table.appendChild(tbody);
            container.appendChild(table);
        }
        if (this.VKI_deadBox)
            this.VKI_deadkeysOn.style.display = (hasDeadKey) ? "inline" : "none";
    };

    this.VKI_buildKeys();
    VKI_addListener(this.VKI_keyboard, 'selectstart', function() { return false; }, false);
    this.VKI_keyboard.unselectable = "on";


    /* ****************************************************************
     * Controls modifier keys
     *
     */
    this.VKI_modify = function(type) {
        switch (type) {
            case "Alt":
            case "AltGr": this.VKI_altgr = !this.VKI_altgr; break;
            case "AltLk": this.VKI_altgr = 0; this.VKI_altgrlock = !this.VKI_altgrlock; break;
            case "Caps": this.VKI_shift = 0; this.VKI_shiftlock = !this.VKI_shiftlock; break;
            case "Shift": this.VKI_shift = !this.VKI_shift; break;
        }
        var vchar = 0;
        if (!this.VKI_shift != !this.VKI_shiftlock) vchar += 1;
        if (!this.VKI_altgr != !this.VKI_altgrlock) vchar += 2;

        var tables = this.VKI_keyboard.tBodies[0].getElementsByTagName('div')[0].getElementsByTagName('table');
        for (var x = 0; x < tables.length; x++) {
            var tds = tables[x].getElementsByTagName('td');
            for (var y = 0; y < tds.length; y++) {
                var className = [], lkey = this.VKI_layout[this.VKI_kt].keys[x][y];
                switch (lkey[1]) {
                    case "Alt":
                    case "AltGr":
                        if (this.VKI_altgr) className.push("pressed");
                        break;
                    case "AltLk":
                        if (this.VKI_altgrlock) className.push("pressed");
                        break;
                    case "Shift":
                        if (this.VKI_shift) className.push("pressed");
                        break;
                    case "Caps":
                        if (this.VKI_shiftlock) className.push("pressed");
                        break;
                    case "Tab": case "Enter": case "Bksp": break;
                    default:
                        if (type) {
                            tds[y].removeChild(tds[y].firstChild);
                            if (this.VKI_symbol[lkey[vchar]]) {
                                var text = this.VKI_symbol[lkey[vchar]].split("\n");
                                var small = document.createElement('small');
                                small.setAttribute('char', lkey[vchar]);
                                for (var z = 0; z < text.length; z++) {
                                    if (z) small.appendChild(document.createElement("br"));
                                    small.appendChild(document.createTextNode(text[z]));
                                } tds[y].appendChild(small);
                            } else tds[y].appendChild(document.createTextNode(lkey[vchar] || "\xa0"));
                        }
                        if (this.VKI_deadkeysOn.checked) {
                            var character = tds[y].firstChild.nodeValue || tds[y].firstChild.className;
                            if (this.VKI_dead) {
                                if (character == this.VKI_dead) className.push("pressed");
                                if (this.VKI_deadkey[this.VKI_dead][character]) className.push("target");
                            }
                            if (this.VKI_deadkey[character]) className.push("deadkey");
                        }
                }

                if (y == tds.length - 1 && tds.length > this.VKI_keyCenter) className.push("last");
                if (lkey[0] == " " || lkey[1] == " ") className.push("space");
                tds[y].className = className.join(" ");
            }
        }
    };


    /* ****************************************************************
     * Insert text at the cursor
     *
     */
    this.VKI_insert = function(text) {
        /*
           this.VKI_target.focus();
           if (this.VKI_target.maxLength) this.VKI_target.maxlength = this.VKI_target.maxLength;
           if (typeof this.VKI_target.maxlength == "undefined" ||
           this.VKI_target.maxlength < 0 ||
           this.VKI_target.value.length < this.VKI_target.maxlength) {
           if (this.VKI_target.setSelectionRange && !this.VKI_target.readOnly && !this.VKI_isIE) {
           var rng = [this.VKI_target.selectionStart, this.VKI_target.selectionEnd];
           this.VKI_target.value = this.VKI_target.value.substr(0, rng[0]) + text + this.VKI_target.value.substr(rng[1]);
           if (text == "\n" && this.VKI_isOpera) rng[0]++;
           this.VKI_target.setSelectionRange(rng[0] + text.length, rng[0] + text.length);
           } else if (this.VKI_target.createTextRange && !this.VKI_target.readOnly) {
           try {
           this.VKI_target.range.select();
           } catch(e) { this.VKI_target.range = document.selection.createRange(); }
           this.VKI_target.range.text = text;
           this.VKI_target.range.collapse(true);
           this.VKI_target.range.select();
           } else this.VKI_target.value += text;
           if (this.VKI_shift) this.VKI_modify("Shift");
           if (this.VKI_altgr) this.VKI_modify("AltGr");
           this.VKI_target.focus();
           } else if (this.VKI_target.createTextRange && this.VKI_target.range)
           this.VKI_target.range.select();
           */
        console.debug('VK:',text);
    };


    /* ****************************************************************
     * Show the keyboard interface
     *
     */
    this.VKI_show = function() {
        this.VKI_keyboard.setStyles({
            'position': 'absolute',
            'left': this.posX,
            'top': this.posY,
            'z-index': 999
        }).inject(document.body);
        // this.VKI_position(true);
        // if (self.VKI_isMoz || self.VKI_isWebKit) {
        //    this.VKI_position(true);
        //}
    };

/* ****************************************************************
 * Position the keyboard
 *
 */
/*
   this.VKI_position = function(force) {
   if (self.VKI_target) {
   var kPos = VKI_findPos(self.VKI_keyboard), wDim = VKI_innerDimensions(), sDis = VKI_scrollDist();
   var place = false, fudge = self.VKI_target.offsetHeight + 3;
   if (force !== true) {
   if (kPos[1] + self.VKI_keyboard.offsetHeight - sDis[1] - wDim[1] > 0) {
   place = true;
   fudge = -self.VKI_keyboard.offsetHeight - 3;
   } else if (kPos[1] - sDis[1] < 0) place = true;
   }
   if (place || force === true) {
   var iPos = VKI_findPos(self.VKI_target), scr = self.VKI_target;
   while (scr = scr.parentNode) {
   if (scr == document.body) break;
   if (scr.scrollHeight > scr.offsetHeight || scr.scrollWidth > scr.offsetWidth) {
   if (!scr.getAttribute("VKI_scrollListener")) {
   scr.setAttribute("VKI_scrollListener", true);
   VKI_addListener(scr, 'scroll', function() { self.VKI_position(true); }, false);
   } // Check if the input is in view
   var pPos = VKI_findPos(scr), oTop = iPos[1] - pPos[1], oLeft = iPos[0] - pPos[0];
   var top = oTop + self.VKI_target.offsetHeight;
   var left = oLeft + self.VKI_target.offsetWidth;
   var bottom = scr.offsetHeight - oTop - self.VKI_target.offsetHeight;
   var right = scr.offsetWidth - oLeft - self.VKI_target.offsetWidth;
   self.VKI_keyboard.style.display = (top < 0 || left < 0 || bottom < 0 || right < 0) ? "none" : "";
   }
   }
   self.VKI_keyboard.style.top = iPos[1] - ((self.VKI_target.keyboardPosition == "fixed" && !self.VKI_isMoz) ? sDis[1] : 0) + fudge + "px";
   self.VKI_keyboard.style.left = Math.max(10, Math.min(wDim[0] - self.VKI_keyboard.offsetWidth - 25, iPos[0])) + "px";
   }
   if (force === true) self.VKI_position();
   }
   };
   */


    /* ****************************************************************
     * Close the keyboard interface
     *
     */
    this.VKI_close = VKI_close = function() {
        this.VKI_keyboard.dispose();
        if (this.VKI_kt != this.VKI_kts) {
            kbSelect.firstChild.nodeValue = this.VKI_kt = this.VKI_kts;
            this.VKI_buildKeys();
        }
        kbSelect.getElementsByTagName('ol')[0].style.display = "";;
    };


    /* ***** Private functions *************************************** */
    function VKI_addListener(elem, type, func, cap) {
        if (elem.addEventListener) {
            elem.addEventListener(type, function(e) { func.call(elem, e); }, cap);
        } else if (elem.attachEvent)
            elem.attachEvent('on' + type, function() { func.call(elem); });
    }

    this.VKI_kbsize();
})();
