dnl We are trying 'svn info' and 'subwcrev', if none of them
dnl succeeds, set the revision to 0.
dnl
dnl Note: The reason for using m4_esyscmd to let the commands
dnl run at autoconf time is, that ordinary users usually do
dnl *not* have an SVN working-directory and therefore this
dnl can *not* be done at configure time.
dnl
AC_DEFUN([FE_SVNREV],dnl
# $Id: svnrev.m4 30 2008-11-18 02:12:59Z Fritz Elfert $
#
# FE_SVNREV([VAR])
# Assign the SVN revision of the current working directory
# at autoconf runtime to a variable and AC_SUBST it.
#
m4_pushdef([_tmp1],m4_esyscmd([svn info -R 2>/dev/null|awk '/^Revision:/ {print $2}'|sort -nr|head -1]))dnl
m4_pushdef([_tmp2],m4_esyscmd([subwcrev . 2>/dev/null|awk '/revision [0-9]/ {print $5}']))dnl
$1=m4_if(_tmp1,[],[m4_if(_tmp2,[],[0],_tmp2)],_tmp1)dnl
[AC_SUBST($1)]dnl
m4_popdef([_tmp2])dnl
m4_popdef([_tmp1]))dnl
