dnl
dnl $1 = language
dnl $2 = variable to change
dnl $2 = flag to test
dnl
AC_DEFUN([fe_CHECK_FLAG],[
  AC_REQUIRE([AC_LANG_PUSH])dnl
  AC_REQUIRE([AC_LANG_POP])dnl

  compiler=m4_if($1,[C],[$CC],[$CXX])
  AC_MSG_CHECKING(whether $compiler accepts $3)
  AC_LANG_PUSH($1)
  ac_save_$2="$$2"
  dnl Some flags are dependant, so we set all previously checked
  dnl flags when testing. Except for -Werror which we have to
  dnl check on its own, because some of our compiler flags cause
  dnl warnings from the autoconf test program!
  if test "$3" = "-Werror" ; then
    $2="$$2 $3"
  else
    $2="$$2 $ac_checked_$2 $3"
  fi
  AC_LINK_IFELSE([AC_LANG_PROGRAM([],[int x;])],[optok=yes],[optok=no])
  $2="$ac_save_$2"
  AC_LANG_POP
  if test $optok = "no"; then
    AC_MSG_RESULT(no)
  else
    ac_checked_$2="$ac_checked_$2 $3"
    AC_MSG_RESULT(yes)
  fi
])
