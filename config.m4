PHP_ARG_ENABLE(spotify, whether to enable Spotify support, [--enable-spotify   Enable Spotify support])

if test "$PHP_SPOTIFY" = "yes"; then
  AC_DEFINE(HAVE_SPOTIFY, 1, [Whether you have Spotify])
  PHP_NEW_EXTENSION(spotify, spotify.c playlist.c appkey.c, $ext_shared)
  LDLIBS="-lspotify"
fi
