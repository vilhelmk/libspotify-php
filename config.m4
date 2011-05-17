PHP_ARG_ENABLE(spotify, whether to enable Spotify support, [--enable-spotify   Enable Spotify support])

if test "$PHP_SPOTIFY" = "yes"; then
  AC_DEFINE(HAVE_SPOTIFY, 1, [Whether you have Spotify])
  PHP_ADD_LIBRARY_WITH_PATH(spotify, "", SPOTIFY_SHARED_PATH)
  PHP_NEW_EXTENSION(spotify, spotify.c playlist.c track.c artist.c album.c, $ext_shared)
fi
