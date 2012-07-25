JS_SOURCES = \
	js/modernizr-debug.js  js/mootools-debug.js  js/wsgate-debug.js \
	js/simpletabs-debug.js js/vkb-debug.js
JS_GENERATED = $(JS_SOURCES:%-debug.js=%.js)
VKB_LAYOUTS = $(wildcard js/vkbl-*.json)

nobase_pkgdata_DATA = \
	index-debug.html index.html favicon.ico FreeRDP_Logo.png \
	c_default.png c_none.png css/style-debug.css css/style.css \
	css/vkb-debug.css css/vkb.css \
	$(JS_SOURCES) $(JS_GENERATED) $(VKB_LAYOUTS)

%.html: %-debug.html
	$(AM_V_GEN)$(STRIP_DEBUG) $< > $@

%.js: %-debug.js
	$(AM_V_GEN)$(MKDIR_P) js && $(STRIP_DEBUG) $< | $(JS_COMPRESS) > $@

%.css: %-debug.css
	$(AM_V_GEN)$(MKDIR_P) css && $(STRIP_DEBUG) $< | $(CSS_COMPRESS) > $@

CLEANFILES = index.html css/style.css css/vkb.css $(JS_GENERATED)

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = index-debug.html favicon.ico FreeRDP_Logo.png c_default.png \
	c_none.png css/style-debug.css css/vkb-debug.css $(JS_SOURCES) $(VKB_LAYOUTS)
