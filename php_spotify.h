#ifndef PHP_SPOTIFY_H
#define PHP_SPOTIFY_H 1

#define PHP_SPOTIFY_VERSION "0.0.7"
#define PHP_SPOTIFY_EXTNAME "spotify"

PHP_FUNCTION(spotify_init);
PHP_FUNCTION(spotify_destroy);
PHP_FUNCTION(spotify_get_starred_playlist);

extern zend_module_entry spotify_module_entry;
#define phpext_spotify_ptr &spotify_module_entry

#endif
