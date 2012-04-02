# _FE_CHECK_PROG(VARIABLE, PROG-TO-CHECK-FOR,
#               [VALUE-IF-FOUND], [VALUE-IF-NOT-FOUND],
#               [PATH], [REJECT])
# -----------------------------------------------------
AC_DEFUN([_FE_CHECK_PROG],
[# Extract the first word of "$2", so it can be a program name with args.
set dummy $2; ac_word=$[2]
AC_MSG_CHECKING([for $ac_word])
AC_CACHE_VAL(ac_cv_prog_$1,
[if test -n "$$1"; then
  ac_cv_prog_$1="$$1" # Let the user override the test.
else
m4_ifvaln([$6],
[  ac_prog_rejected=no])dnl
_AS_PATH_WALK([$5],
[for ac_exec_ext in '' $ac_executable_extensions; do
  if AS_EXECUTABLE_P(["$as_dir/$ac_word$ac_exec_ext"]); then
m4_ifvaln([$6],
[    if test "$as_dir/$ac_word$ac_exec_ext" = "$6"; then
       ac_prog_rejected=yes
       continue
     fi])dnl
    ac_cv_prog_$1="$3"
    _AS_ECHO([found $as_dir/$ac_word$ac_exec_ext])
    break 2
  fi
done])
m4_ifvaln([$6],
[if test $ac_prog_rejected = yes; then
  # We found a bogon in the path, so make sure we never use it.
  set dummy $ac_cv_prog_$1
  shift
  if test $[@%:@] != 0; then
    # We chose a different compiler from the bogus one.
    # However, it has the same basename, so the bogon will be chosen
    # first if we set $1 to just the basename; use the full file name.
    shift
    ac_cv_prog_$1="$as_dir/$ac_word${1+' '}$[@]"
m4_if([$2], [$4],
[  else
    # Default is a loser.
    AC_MSG_ERROR([$1=$6 unacceptable, but no other $4 found in dnl
m4_default([$5], [\$PATH])])
])dnl
  fi
fi])dnl
dnl If no 4th arg is given, leave the cache variable unset,
dnl so FE_CHECK_PROGS will keep looking.
m4_ifvaln([$4],
[  test -z "$ac_cv_prog_$1" && ac_cv_prog_$1="$4"])dnl
fi])dnl
$1=$ac_cv_prog_$1
if test -n "$$1"; then
  AC_MSG_RESULT([$$1])
else
  AC_MSG_RESULT([no])
fi
])# _FE_CHECK_PROG

# AC_CHECK_TARGET_TOOL(VARIABLE, PROG-TO-CHECK-FOR, [VALUE-IF-NOT-FOUND], [PATH])
# -------------------------------------------------------------------------------
# (Use different variables $1 and ac_ct_$1 so that cache vars don't conflict.)
AC_DEFUN([AC_CHECK_TARGET_TOOL],
[AC_REQUIRE([AC_CANONICAL_TARGET])dnl
AC_CHECK_PROG([$1], [$target_alias-$2], [$target_alias-$2], , [$4])
if test -z "$ac_cv_prog_$1"; then
  if test "$build" = "$target"; then
    ac_ct_$1=$$1
    _FE_CHECK_PROG([ac_ct_$1], [$2], [$2], [$3], [$4])
    $1=$ac_ct_$1
  else
    $1="$3"
  fi
else
  $1="$ac_cv_prog_$1"
fi
])# AC_CHECK_TARGET_TOOL

m4_ifndef([_AS_ECHO_LOG],[
m4_define([_AS_ECHO_LOG],
[_AS_ECHO([$as_me:$LINENO: $1], [AS_MESSAGE_LOG_FD])])])
])
m4_ifndef([_AC_DO_STDERR],[
m4_define([_AC_DO_STDERR],[_AC_EVAL_STDERR($1)])])
