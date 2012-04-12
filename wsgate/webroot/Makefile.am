pkgdata_DATA = \
	index.html favicon.ico \
	css/style-debug.css css/style.css \
	js/modernizr.js  js/mootools.js  js/wsgate.js \
	js/modernizr-debug.js  js/mootools-debug.js  js/wsgate-debug.js

%.js: %-debug.js
	@$(MKDIR_P) js
	@if [ -n "$(JAVA)" ] ; then \
		echo "Compressing $< -> $@" ; \
		$(JAVA) -jar $(top_srcdir)/tools/yuicompressor-2.4.7.jar -o $@ $< ; \
	else \
		cp $< $@ ; \
	fi

%.css: %-debug.css
	@$(MKDIR_P) css
	@if [ -n "$(JAVA)" ] ; then \
		echo "Compressing $< -> $@" ; \
		$(JAVA) -jar $(top_srcdir)/tools/yuicompressor-2.4.7.jar -o $@ $< ; \
	else \
		cp $< $@ ; \
	fi

DISTCLEANFILES = -r js css

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = index.html favicon.ico \
	css/style-debug.css \
	js/modernizr-debug.js  js/mootools-debug.js  js/wsgate-debug.js
