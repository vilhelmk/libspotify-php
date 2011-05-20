#include "php_spotify.h"

zend_class_entry *spotifyalbum_ce;

PHP_METHOD(SpotifyAlbum, browse);

PHP_METHOD(SpotifyAlbum, __construct)
{
	zval *object = getThis();
	zval *parent;
	sp_album *album;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &parent, &album) == FAILURE) {
		return;
	}

	spotify_object *p = (spotify_object*)zend_object_store_get_object((parent) TSRMLS_CC);
	spotifyalbum_object *obj = (spotifyalbum_object*)zend_object_store_get_object(object TSRMLS_CC);
	obj->session = p->session;
	obj->album = album;
	obj->albumbrowse = NULL;

	sp_album_add_ref(obj->album);
}

PHP_METHOD(SpotifyAlbum, __destruct)
{
	spotifyalbum_object *obj = (spotifyalbum_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (obj->albumbrowse != NULL) {
		sp_albumbrowse_release(obj->albumbrowse);
	}

	sp_album_release(obj->album);
}

PHP_METHOD(SpotifyAlbum, getName)
{
	zval *object = getThis();
	spotifyalbum_object *p = (spotifyalbum_object*)zend_object_store_get_object(object TSRMLS_CC);

	RETURN_STRING(sp_album_name(p->album), 1);
}

PHP_METHOD(SpotifyAlbum, getYear)
{
	spotifyalbum_object *p = (spotifyalbum_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(sp_album_year(p->album));
}

PHP_METHOD(SpotifyAlbum, getArtist)
{
	sp_artist *artist;
	zval temp;
	spotifyalbum_object *p = (spotifyalbum_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	artist = sp_album_artist(p->album);

	object_init_ex(return_value, spotifyartist_ce);
	SPOTIFY_METHOD2(SpotifyArtist, __construct, &temp, return_value, getThis(), artist);
}

PHP_METHOD(SpotifyAlbum, getNumTracks)
{
	zval tempretval, *thisptr = getThis();
	spotifyalbum_object *p = (spotifyalbum_object*)zend_object_store_get_object(thisptr TSRMLS_CC);
	SPOTIFY_METHOD(SpotifyAlbum, browse, &tempretval, thisptr);
	RETURN_LONG(sp_albumbrowse_num_tracks(p->albumbrowse));
}

PHP_METHOD(SpotifyAlbum, getTracks)
{
	int num_tracks, i;
	zval tempretval, *thisptr = getThis();

	spotifyalbum_object *p = (spotifyalbum_object*)zend_object_store_get_object(thisptr TSRMLS_CC);

	SPOTIFY_METHOD(SpotifyAlbum, browse, &tempretval, thisptr);

	array_init(return_value);

	num_tracks = sp_albumbrowse_num_tracks(p->albumbrowse);

	for (i=0; i<num_tracks; i++) {
		sp_track *track = sp_albumbrowse_track(p->albumbrowse, i);

		zval *z_track;
		ALLOC_INIT_ZVAL(z_track);
		object_init_ex(z_track, spotifytrack_ce);
		SPOTIFY_METHOD2(SpotifyTrack, __construct, &tempretval, z_track, thisptr, track);
		add_next_index_zval(return_value, z_track);
	}
}

PHP_METHOD(SpotifyAlbum, getType)
{
	spotifyalbum_object *p = (spotifyalbum_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(sp_album_type(p->album));
}

PHP_METHOD(SpotifyAlbum, __toString)
{
	spotifyalbum_object *p = (spotifyalbum_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_STRING(sp_album_name(p->album), 1);
}

static void albumbrowse_complete(sp_albumbrowse *result, void *userdata)
{
	spotifyalbum_object *p = (spotifyalbum_object*)userdata;
	p->albumbrowse = result;
}

PHP_METHOD(SpotifyAlbum, browse)
{
	int timeout = 0;

	spotifyalbum_object *p = (spotifyalbum_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	if (p->albumbrowse != NULL) {
		RETURN_TRUE;
	}

	sp_albumbrowse *tmpbrowse = sp_albumbrowse_create(p->session, p->album, albumbrowse_complete, p);
	while (!sp_albumbrowse_is_loaded(tmpbrowse)) {
		sp_session_process_events(p->session, &timeout);
	}

	RETURN_TRUE;
}

function_entry spotifyalbum_methods[] = {
	PHP_ME(SpotifyAlbum, __construct,		NULL,	ZEND_ACC_PRIVATE|ZEND_ACC_CTOR)
	PHP_ME(SpotifyAlbum, __destruct,		NULL,	ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(SpotifyAlbum, getName,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbum, getYear,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbum, getArtist,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbum, getNumTracks,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbum, getTracks,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbum, getType,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbum, browse,			NULL,	ZEND_ACC_PRIVATE)
	PHP_ME(SpotifyAlbum, __toString,		NULL,	ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

void spotifyalbum_free_storage(void *object TSRMLS_DC)
{
	spotifyalbum_object *obj = (spotifyalbum_object*)object;
	zend_hash_destroy(obj->std.properties);
	FREE_HASHTABLE(obj->std.properties);
	efree(obj);
}

zend_object_value spotifyalbum_create_handler(zend_class_entry *type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;

	spotifyalbum_object *obj = (spotifyalbum_object *)emalloc(sizeof(spotifyalbum_object));
	memset(obj, 0, sizeof(spotifyalbum_object));
   // obj->std.ce = type;

	zend_object_std_init(&obj->std, type TSRMLS_CC);
    zend_hash_copy(obj->std.properties, &type->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));

    retval.handle = zend_objects_store_put(obj, NULL,
        spotifyalbum_free_storage, NULL TSRMLS_CC);
    retval.handlers = &spotify_object_handlers;

    return retval;
}

void spotify_init_album(TSRMLS_D)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "SpotifyAlbum", spotifyalbum_methods);
	spotifyalbum_ce = zend_register_internal_class(&ce TSRMLS_CC);
	spotifyalbum_ce->create_object = spotifyalbum_create_handler;
	//memcpy(&spotify_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	//spotify_object_handlers.clone_obj = NULL;

	zend_declare_class_constant_long(spotifyalbum_ce, "TYPE_ALBUM", strlen("TYPE_ALBUM"), 0 TSRMLS_CC);
	zend_declare_class_constant_long(spotifyalbum_ce, "TYPE_SINGLE", strlen("TYPE_SINGLE"), 1 TSRMLS_CC);
	zend_declare_class_constant_long(spotifyalbum_ce, "TYPE_COMPILATION", strlen("TYPE_COMPILATION"), 2 TSRMLS_CC);
	zend_declare_class_constant_long(spotifyalbum_ce, "TYPE_UNKNOWN", strlen("TYPE_UNKNOWN"), 3 TSRMLS_CC);
}
