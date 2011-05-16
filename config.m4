PHP_ARG_ENABLE(spotify, whether to enable Hello World support, [--enable-spotify   Enable Hello World support])

if test "$PHP_SPOTIFY" = "yes"; then
  AC_DEFINE(HAVE_SPOTIFY, 1, [Whether you have Hello World])
  PHP_NEW_EXTENSION(spotify, spotify.c appkey.c, $ext_shared)
  LDLIBS="-lspotify"
fi
