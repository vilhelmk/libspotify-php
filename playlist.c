#include "php_spotify.h"

zend_class_entry *spotifyplaylist_ce;

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

PHP_METHOD(SpotifyPlaylist, __destruct)
{
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	sp_playlist_release(p->playlist);
}

PHP_METHOD(SpotifyPlaylist, getName)
{
	zval *object = getThis();
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(object TSRMLS_CC);

	RETURN_STRING(sp_playlist_name(p->playlist), 1);
}

PHP_METHOD(SpotifyPlaylist, getTracks)
{
	int i, num_tracks, timeout = 0;
	zval *thisptr = getThis(), tempretval;
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(thisptr TSRMLS_CC);

	SPOTIFY_METHOD(SpotifyPlaylist, browse, &tempretval, thisptr);

	array_init(return_value);

	num_tracks = sp_playlist_num_tracks(p->playlist);
	for (i=0; i<num_tracks; i++) {
		sp_track *track = sp_playlist_track(p->playlist, i);
		while (!sp_track_is_loaded(track)) {
			sp_session_process_events(p->session, &timeout);
		}

		zval *z_track;
		ALLOC_INIT_ZVAL(z_track);
		object_init_ex(z_track, spotifytrack_ce);
		SPOTIFY_METHOD2(SpotifyTrack, __construct, &tempretval, z_track, thisptr, track);

		add_next_index_zval(return_value, z_track);
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

PHP_METHOD(SpotifyPlaylist, getDescription)
{
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_STRING(sp_playlist_get_description(p->playlist), 1);
}

PHP_METHOD(SpotifyPlaylist, getNumSubscribers)
{
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(sp_playlist_num_subscribers(p->playlist));
}

PHP_METHOD(SpotifyPlaylist, getTrackCreateTime)
{
	int index;
	zval *z_index;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &z_index) == FAILURE) {
		return;
	}


	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(sp_playlist_track_create_time(p->playlist, Z_LVAL_P(z_index)));
}

PHP_METHOD(SpotifyPlaylist, getTrackCreator)
{
	int index;
	zval *z_user, *thisptr = getThis(), tempretval;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) == FAILURE) {
		return;
	}

	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(thisptr TSRMLS_CC);
	z_user = sp_playlist_track_creator(p->playlist, index);
	if (!z_user) {
		RETURN_FALSE;
	}

	object_init_ex(return_value, spotifyuser_ce);
	SPOTIFY_METHOD2(SpotifyUser, __construct, &tempretval, return_value, thisptr, z_user);
}

PHP_METHOD(SpotifyPlaylist, isCollaborative)
{
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (sp_playlist_is_collaborative(p->playlist)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}

PHP_METHOD(SpotifyPlaylist, setCollaborative)
{
	bool collab;
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &collab) == FAILURE) {
		return;
	}

	sp_playlist_set_collaborative(p->playlist, collab);
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

PHP_METHOD(SpotifyPlaylist, addTrack)
{
	zval *track, retval, *z_position, *object = getThis();
	sp_error error;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|z", &track, spotifytrack_ce, &z_position) == FAILURE) {
		return;
	}

	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(object TSRMLS_CC);

	sp_track **tracks = (sp_track**)emalloc(sizeof(sp_track*) * 1);
	spotifytrack_object *track_obj = (spotifytrack_object*)zend_object_store_get_object(track TSRMLS_CC);
	tracks[0] = track_obj->track;

	int position;

	if (Z_TYPE_P(z_position) == IS_NULL || Z_LVAL_P(z_position) < 0) {
		position = sp_playlist_num_tracks(p->playlist);
	} else {
		position = Z_LVAL_P(z_position);
	}

	error = sp_playlist_add_tracks(p->playlist, tracks, 1, position, p->session);

	efree(tracks);

	if (SP_ERROR_OK == error) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}

PHP_METHOD(SpotifyPlaylist, browse)
{
	int timeout = 0;

	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	while (!sp_playlist_is_loaded(p->playlist)) {
		sp_session_process_events(p->session, &timeout);
	}
}

PHP_METHOD(SpotifyPlaylist, __toString)
{
	spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_STRING(sp_playlist_name(p->playlist), 1);
}

function_entry spotifyplaylist_methods[] = {
	PHP_ME(SpotifyPlaylist, __construct,		NULL,	ZEND_ACC_PRIVATE|ZEND_ACC_CTOR)
	PHP_ME(SpotifyPlaylist, __destruct,			NULL,	ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(SpotifyPlaylist, getName,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, getTracks,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, getOwner,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, getDescription,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, getNumSubscribers,	NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, getTrackCreateTime,	NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, getTrackCreator,	NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, isCollaborative,	NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, setCollaborative,	NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, rename,				NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, addTrack,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyPlaylist, browse,				NULL,	ZEND_ACC_PRIVATE)
	PHP_ME(SpotifyPlaylist, __toString,			NULL,	ZEND_ACC_PUBLIC)
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
