dnl
dnl $2 = variable to change
dnl $2 = flag to test
dnl
AC_DEFUN([fe_EXCEPTION_PROPAGATION],[
  AC_REQUIRE([AC_LANG_PUSH])dnl
  AC_REQUIRE([AC_LANG_POP])dnl
  AC_REQUIRE([fe_CHECK_FLAG])dnl

  tmp=
  fe_CHECK_FLAG([C++],[tmp],[-std=c++0x])
  if test -n "$ac_checked_tmp" ; then
    tmp="$CXXFLAGS"
    CXXFLAGS="-std=c++0x $CXXFLAGS"
    AC_MSG_CHECKING(whether $CXX supports exception propagation)
    AC_LANG_PUSH([C++])
    AC_LINK_IFELSE([AC_LANG_PROGRAM([
      #include <exception>
      using namespace std;
    ],[
      exception_ptr p;
      try {
        int i = 1;
      } catch (...) {
        p = current_exception();
      }
      rethrow_exception(p);
    ])],[
      AC_DEFINE_UNQUOTED([HAVE_EXCEPTION_PROPAGATION],[1],
        [Defined, when compiler supports exception propagation])
      AC_MSG_RESULT(yes)
    ],[
      CXXFLAGS="$tmp"
      AC_MSG_RESULT(no)
    ])
    AC_LANG_POP
  fi
])
