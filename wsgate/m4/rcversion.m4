AC_DEFUN([FE_RCVERSION],
[
_ver=$VERSION
[_dotcount=`echo $ECHO_N "$_ver$ECHO_C"|awk '{print gsub(/\./,x)}'`]
while test $_dotcount -lt 3 ; do
    _ver=${_ver}.0
    _dotcount=`expr $_dotcount + 1`
done
if test -n "$SVNREV" ; then
    _ver=`echo $ECHO_N "$_ver$ECHO_C"|sed -e "s/\.0$/.$SVNREV/"`
elif test -n "$GITREV" ; then
    _ver=`echo $ECHO_N "$_ver$ECHO_C"|sed -e "s/\.0$/.$GITREV/"`
fi
RC_VERSION=`echo $ECHO_N "$_ver$ECHO_C"|tr . ,`
RC_VERSION_STR="\"${_ver}\""
FULLVERSION=$_ver
AC_SUBST(RC_VERSION)
AC_SUBST(RC_VERSION_STR)
AC_SUBST(FULLVERSION)
])
