#include "php_spotify.h"

zend_class_entry *spotifytrack_ce;

PHP_METHOD(SpotifyTrack, __construct)
{
	zval *object = getThis();
	zval *parent;
	sp_track *track;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oz", &parent, spotifyplaylist_ce, &track) == FAILURE) {
		return;
	}

	spotify_object *p = (spotify_object*)zend_object_store_get_object((parent) TSRMLS_CC);
	spotifytrack_object *obj = (spotifytrack_object*)zend_object_store_get_object(object TSRMLS_CC);
	obj->session = p->session;
	obj->track = track;;
}

PHP_METHOD(SpotifyTrack, getName)
{
	zval *object = getThis();
	spotifytrack_object *p = (spotifytrack_object*)zend_object_store_get_object(object TSRMLS_CC);

	RETURN_STRING(sp_track_name(p->track), 1);
}

PHP_METHOD(SpotifyTrack, getAlbum)
{
	zval *object = getThis(), *album;
	spotifytrack_object *p = (spotifytrack_object*)zend_object_store_get_object(object TSRMLS_CC);
}

PHP_METHOD(SpotifyTrack, getArtist)
{
	zval *object = getThis(), temp;
	spotifytrack_object *p = (spotifytrack_object*)zend_object_store_get_object(object TSRMLS_CC);

	int num_artists = sp_track_num_artists(p->track);
	if (0 == num_artists) {
		RETURN_FALSE;
	}

	// FIXME add support for multiple artists

	sp_artist *artist = sp_track_artist(p->track, 0);
	if (NULL == artist) {
		RETURN_FALSE;
	}

	object_init_ex(return_value, spotifyartist_ce);
	SPOTIFY_METHOD2(SpotifyArtist, __construct, &temp, return_value, object, artist);
}

function_entry spotifytrack_methods[] = {
    PHP_ME(SpotifyTrack, __construct,            NULL,   ZEND_ACC_PRIVATE|ZEND_ACC_CTOR)
	PHP_ME(SpotifyTrack, getName,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrack, getAlbum,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrack, getArtist,		NULL,	ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

void spotifytrack_free_storage(void *object TSRMLS_DC)
{
	spotifytrack_object *obj = (spotifytrack_object*)object;
	zend_hash_destroy(obj->std.properties);
	FREE_HASHTABLE(obj->std.properties);
	efree(obj);
}

zend_object_value spotifytrack_create_handler(zend_class_entry *type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;

	spotifytrack_object *obj = (spotifytrack_object *)emalloc(sizeof(spotifytrack_object));
	memset(obj, 0, sizeof(spotifytrack_object));
   // obj->std.ce = type;

	zend_object_std_init(&obj->std, type TSRMLS_CC);
    zend_hash_copy(obj->std.properties, &type->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));

    retval.handle = zend_objects_store_put(obj, NULL,
        spotifytrack_free_storage, NULL TSRMLS_CC);
    retval.handlers = &spotify_object_handlers;

    return retval;
}

void spotify_init_track(TSRMLS_D)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "SpotifyTrack", spotifytrack_methods);
	spotifytrack_ce = zend_register_internal_class(&ce TSRMLS_CC);
	spotifytrack_ce->create_object = spotifytrack_create_handler;
//	memcpy(&spotify_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
//	spotify_object_handlers.clone_obj = NULL;
}
