#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_spotify.h"

#include <stdlib.h>
#include <stdio.h>
#include <libspotify/api.h>

sp_session *g_session;
void (*metadata_updated_fn)(void);

static function_entry spotify_functions[] = {
	PHP_FE(spotify_init, NULL)
	PHP_FE(spotify_destroy, NULL)
	PHP_FE(spotify_get_starred_playlist, NULL)
    {NULL, NULL, NULL}
};

zend_module_entry spotify_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_SPOTIFY_EXTNAME,
    spotify_functions,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#if ZEND_MODULE_API_NO >= 20010901
    PHP_SPOTIFY_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SPOTIFY
ZEND_GET_MODULE(spotify)
#endif

static void metadata_updated(sp_session *sess)
{
    if(metadata_updated_fn)
        metadata_updated_fn();
}

static int is_logged_in;
static int is_logged_out;

static void logged_in(sp_session *session, sp_error error)
{
	is_logged_out = 0;
	is_logged_in = 1;
}

static void logged_out(sp_session *session)
{
	is_logged_in = 0;
	is_logged_out = 1;
}

static sp_session_callbacks callbacks = {
    &logged_in,
    &logged_out,
    &metadata_updated,
    NULL, //&connection_error,
    NULL,
    NULL, // &notify_main_thread,
    NULL,
    NULL,
    NULL //&log_message
};

int timeout = 0;

PHP_FUNCTION(spotify_init)
{
	zval *z_username;
	zval *z_password;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &z_username, &z_password) == FAILURE) {
		return;
	}


	sp_session_config config;
	sp_error error;
	sp_session *session;

	memset(&config, 0, sizeof(config));

	config.api_version = SPOTIFY_API_VERSION;
	config.cache_location = "tmp";
	config.settings_location = "tmp";
	config.user_agent = "libspotify-php";

	extern const char g_appkey[];
	extern const size_t g_appkey_size;
	config.application_key = g_appkey;
	config.application_key_size = g_appkey_size;

	config.callbacks = &callbacks;

	error = sp_session_create(&config, &session);
    if (SP_ERROR_OK != error) {
        fprintf(stderr, "failed to create session: %s\n",
                        sp_error_message(error));
		RETURN_BOOL(0);
		return;
    }

    sp_session_login(session, Z_STRVAL_P(z_username), Z_STRVAL_P(z_password));
    g_session = session;

	do {
		sp_session_process_events(g_session, &timeout);
	} while (timeout == 0 || !is_logged_in);

	RETURN_BOOL(1);
}

PHP_FUNCTION(spotify_destroy)
{
	do {
		sp_session_process_events(g_session, &timeout);
	} while (timeout == 0);

	if (g_session) {
		sp_session_logout(g_session);
		//sp_session_release(g_session);
		RETURN_BOOL(1);
	} else {
		RETURN_BOOL(0);
	}
}

static sp_playlist *playlist_browse;
static sp_playlist_callbacks pl_callbacks;
static zval *playlist_array;
int playlist_browsing;

static void playlist_browse_try(void) {
	int i, tracks;

	playlist_browsing = 1;
	metadata_updated_fn = playlist_browse_try;

	if (!sp_playlist_is_loaded(playlist_browse)) {
		return;
	}

	tracks = sp_playlist_num_tracks(playlist_browse);
	for (i=0; i<tracks; i++) { 
		sp_track *t = sp_playlist_track(playlist_browse, i);
		if (!sp_track_is_loaded(t))
			return;
	}

	array_init(playlist_array);

	for (i=0; i<tracks; i++) {
		sp_track *t = sp_playlist_track(playlist_browse, i);
		zval *track_array;
		ALLOC_INIT_ZVAL(track_array);
		array_init(track_array);
		add_assoc_string(track_array, "name", strdup(sp_track_name(t)), 1);
		
		char *ts;
		int duration = sp_track_duration(t);
		spprintf(&ts, 0, "%02d:%02d", duration/60000, (duration/1000)/60);
		add_assoc_string(track_array, "duration", ts, 1); 

		sp_link *l = sp_link_create_from_track(t, 0);
		char url[256];
		sp_link_as_string(l, url, sizeof(url));
		add_assoc_string(track_array, "link", url, 1);
		sp_link_release(l);

		sp_artist *a = sp_track_artist(t, 0); // FIXME add support for showing multiple artists
		add_assoc_string(track_array, "artist", (char*)sp_artist_name(a), 1);
		sp_artist_release(a);

		add_assoc_long(track_array, "ts", sp_playlist_track_create_time(playlist_browse, i));

		add_next_index_zval(playlist_array, track_array);
	}

	sp_playlist_remove_callbacks(playlist_browse, &pl_callbacks, NULL);
	sp_playlist_release(playlist_browse);

	playlist_browse = NULL;
	metadata_updated_fn = NULL; 
	playlist_browsing = 0;
}

static void pl_state_change(sp_playlist *pl, void *userdata)
{
    playlist_browse_try();
}

static void pl_tracks_added(sp_playlist *pl, sp_track * const * tracks,
                int num_tracks, int position, void *userdata)
{
}

static void pl_tracks_removed(sp_playlist *pl, const int *tracks,
                  int num_tracks, void *userdata)
{
}

static void pl_tracks_moved(sp_playlist *pl, const int *tracks,
                int num_tracks, int new_position, void *userdata)
{
}

static void pl_renamed(sp_playlist *pl, void *userdata)
{
}

static sp_playlist_callbacks pl_callbacks = {
    pl_tracks_added,                                                                                 
    pl_tracks_removed,                                                                               
    pl_tracks_moved,                                                                                 
    pl_renamed,                                                                                      
    pl_state_change,
};

PHP_FUNCTION(spotify_get_starred_playlist)
{
    do {
        sp_session_process_events(g_session, &timeout);
    } while (timeout == 0);

	if (g_session) {
		sp_playlist *playlist = sp_session_starred_create(g_session);

		playlist_array = return_value;
	
		playlist_browse = playlist;
		sp_playlist_add_callbacks(playlist_browse, &pl_callbacks, NULL);
		playlist_browse_try();

		while (playlist_browsing) {
			sp_session_process_events(g_session, &timeout);
		}
	} else {
		RETURN_FALSE;
	}
}
