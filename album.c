#include "php_spotify.h"

zend_class_entry *spotifyalbum_ce;

PHP_METHOD(SpotifyAlbum, __construct)
{
	zval *object = getThis();
	zval *parent;
	sp_album *album;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oz", &parent, spotifytrack_ce, &album) == FAILURE) {
		return;
	}

	spotify_object *p = (spotify_object*)zend_object_store_get_object((parent) TSRMLS_CC);
	spotifyalbum_object *obj = (spotifyalbum_object*)zend_object_store_get_object(object TSRMLS_CC);
	obj->session = p->session;
	obj->album = album;
}

PHP_METHOD(SpotifyAlbum, getName)
{
	zval *object = getThis();
	spotifyalbum_object *p = (spotifyalbum_object*)zend_object_store_get_object(object TSRMLS_CC);

	RETURN_STRING(sp_album_name(p->album), 1);
}

PHP_METHOD(SpotifyAlbum, __toString)
{
	spotifyalbum_object *p = (spotifyalbum_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_STRING(sp_album_name(p->album), 1);
}

function_entry spotifyalbum_methods[] = {
	PHP_ME(SpotifyAlbum, __construct,		NULL,	ZEND_ACC_PRIVATE|ZEND_ACC_CTOR)
	PHP_ME(SpotifyAlbum, getName,			NULL,	ZEND_ACC_PUBLIC)
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
}
