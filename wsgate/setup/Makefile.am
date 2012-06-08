.setupdir:
	$(RM) -rf setupdir
	$(MAKE) -C $(top_builddir) DESTDIR=$(abs_builddir)/setupdir \
		transform= prefix=/ exec_prefix=/ sbindir=/bin \
		pkglibexecdir=/bin pkgdatadir=/webroot sysconfdir=/etc \
		install
	cp -av $(top_builddir)/doc/html setupdir/doc
	cp openssl.cnf setupdir/etc
	find "$(subst :," ",$(DLLPATH))" -name openssl.exe|xargs --no-run-if-empty cp -t setupdir/bin
	touch $@

.dlldeps: .setupdir findrequires.pl
	env DEPDEBUG=1 perl findrequires.pl "$(DLLPATH)" setupdir/bin/*.{exe,dll} 2>$@|xargs --no-run-if-empty cp -t setupdir/bin
	touch $@

.stripsetup: .setupdir
	$(STRIP) --strip-unneeded setupdir/bin/*.exe `ls setupdir/bin/*.dll`
	touch $@

nativesetup: .setupdir .dlldeps .stripsetup
	"$(ISCC)" "$(srcdir)/wsgate.iss"

crosssetup: .setupdir .dlldeps .stripsetup
	$(ISCC) "$(srcdir)/wsgate.iss"

setup: $(SETUP)

EXTRA_DIST = wsgate.iss version.iss openssl.cnf
MAINTAINERCLEANFILES = Makefile.in
CLEANFILES = -r .setupdir .dlldeps .stripsetup setupdir
