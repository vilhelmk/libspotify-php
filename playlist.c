#include "php_spotify.h"

zend_class_entry *spotifyplaylist_ce;
void (*metadata_updated_fn)(void);

static sp_playlist *playlist_browse;
static sp_playlist_callbacks pl_callbacks;
static zval *playlist_array, *parent_object;
static int playlist_browsing;

static void playlist_browse_try(void) {
    int i, tracks;
	zval temp;

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
		zval *track;
        sp_track *t = sp_playlist_track(playlist_browse, i);
		if (NULL == t) {
			continue;
		}

		ALLOC_INIT_ZVAL(track);
		object_init_ex(track, spotifytrack_ce);
		SPOTIFY_METHOD2(SpotifyTrack, __construct, &temp, track, parent_object, t);

        add_next_index_zval(playlist_array, track);
	}

    sp_playlist_remove_callbacks(playlist_browse, &pl_callbacks, NULL);
    sp_playlist_release(playlist_browse);

    playlist_browse = NULL;
    metadata_updated_fn = NULL;
    playlist_browsing = 0;
	parent_object = NULL;
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
	parent_object = object;
	sp_playlist_add_callbacks(playlist_browse, &pl_callbacks, NULL);
	playlist_browse_try();

	int timeout = 0;
	while (playlist_browsing) {
		sp_session_process_events(p->session, &timeout);
	} 
}

PHP_METHOD(SpotifyPlaylist, getOwner)
{
	sp_user *user;
	zval temp;
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	user = sp_playlist_owner(p->playlist);

	object_init_ex(return_value, spotifyuser_ce);
	SPOTIFY_METHOD2(SpotifyUser, __construct, &temp, return_value, getThis(), user);
}

PHP_METHOD(SpotifyPlaylist, rename)
{
	zval *object = getThis(), *z_name;
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(object TSRMLS_CC);
	sp_error error;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &z_name) == FAILURE) {
		return;
	}

	error = sp_playlist_rename(p->playlist, Z_STRVAL_P(z_name));
	if (SP_ERROR_OK != error) {
		RETURN_FALSE;
	}

	RETURN_TRUE;
}

PHP_METHOD(SpotifyPlaylist, __toString)
{
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_STRING(sp_playlist_name(p->playlist), 1);
}

function_entry spotifyplaylist_methods[] = {
    PHP_ME(SpotifyPlaylist, __construct,            NULL,   ZEND_ACC_PRIVATE|ZEND_ACC_CTOR)
	PHP_ME(SpotifyPlaylist, getName,		NULL,	ZEND_ACC_PUBLIC)
    PHP_ME(SpotifyPlaylist, getTracks,     NULL,   ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, getOwner,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, rename,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, __toString,		NULL,	ZEND_ACC_PUBLIC)
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
	//memcpy(&spotify_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	//spotify_object_handlers.clone_obj = NULL;
}
