dnl Note: The reason for using m4_esyscmd to let the commands
dnl run at autoconf time is, that ordinary users usually do
dnl *not* have an GIT working-directory and therefore this
dnl can *not* be done at configure time.
dnl
# $Id$
#
# FE_GITREV([VAR])
# Assign the GIT revision of the current working directory
# at autoconf runtime to a variable and AC_SUBST it.
# In oder to work correctly,
#
AC_DEFUN([FE_GITREV],
m4_pushdef([_tmp],m4_esyscmd([git describe --match initial --long 2>/dev/null|cut -d- -f2]))dnl
$1=_tmp
[AC_SUBST($1)]
m4_popdef([_tmp])
)
