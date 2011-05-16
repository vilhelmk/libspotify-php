#include "php_spotify.h"

zend_class_entry *spotifyplaylist_ce;
void (*metadata_updated_fn)(void);

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
        add_assoc_string(track_array, "name", estrdup(sp_track_name(t)), 1);

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

PHP_METHOD(SpotifyPlaylist, __construct)
{
	zval *object = getThis();
	zval *parent;
	sp_playlist *playlist;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oz", &parent, spotify_ce, &playlist) == FAILURE) {
		return;
	}

	spotify_object *p = (spotify_object*)zend_object_store_get_object((parent) TSRMLS_CC);
	spotifyplaylist_object *obj = (spotifyplaylist_object*)zend_object_store_get_object(object TSRMLS_CC);
	obj->session = p->session;
	obj->playlist = playlist;
}

PHP_METHOD(SpotifyPlaylist, getName)
{
	zval *object = getThis();
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(object TSRMLS_CC);

	RETURN_STRING(sp_playlist_name(p->playlist), 1);
}

PHP_METHOD(SpotifyPlaylist, getTracks)
{
	zval *object = getThis();
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(object TSRMLS_CC);
	
	playlist_array = return_value;
	playlist_browse = p->playlist;
	sp_playlist_add_callbacks(playlist_browse, &pl_callbacks, NULL);
	playlist_browse_try();

	int timeout = 0;
	while (playlist_browsing) {
		sp_session_process_events(p->session, &timeout);
	} 
}

function_entry spotifyplaylist_methods[] = {
    PHP_ME(SpotifyPlaylist, __construct,            NULL,   ZEND_ACC_PRIVATE|ZEND_ACC_CTOR)
	PHP_ME(SpotifyPlaylist, getName,		NULL,	ZEND_ACC_PUBLIC)
    PHP_ME(SpotifyPlaylist, getTracks,     NULL,   ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

void spotifyplaylist_free_storage(void *object TSRMLS_DC)
{
	spotifyplaylist_object *obj = (spotifyplaylist_object*)object;
	zend_hash_destroy(obj->std.properties);
	FREE_HASHTABLE(obj->std.properties);
	efree(obj);
}

zend_object_value spotifyplaylist_create_handler(zend_class_entry *type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;

	spotifyplaylist_object *obj = (spotifyplaylist_object *)emalloc(sizeof(spotifyplaylist_object));
	memset(obj, 0, sizeof(spotifyplaylist_object));
   // obj->std.ce = type;

	zend_object_std_init(&obj->std, type TSRMLS_CC);
    zend_hash_copy(obj->std.properties, &type->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));

    retval.handle = zend_objects_store_put(obj, NULL,
        spotifyplaylist_free_storage, NULL TSRMLS_CC);
    retval.handlers = &spotify_object_handlers;

    return retval;
}

void spotify_init_playlist(TSRMLS_D)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "SpotifyPlaylist", spotifyplaylist_methods);
	spotifyplaylist_ce = zend_register_internal_class(&ce TSRMLS_CC);
	spotifyplaylist_ce->create_object = spotifyplaylist_create_handler;
	memcpy(&spotify_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	spotify_object_handlers.clone_obj = NULL;
}
