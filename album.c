/*
Copyright (c) 2011 Vilhelm K. Vardøy, vilhelmkv@gmail.com

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "php_spotify.h"

zend_class_entry *spotifyalbum_ce;

PHP_METHOD(SpotifyAlbum, __construct)
{
	zval *object = getThis();
	zval *parent;
	sp_album *album;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oz", &parent, spotify_ce, &album) == FAILURE) {
		return;
	}

	spotify_object *p = (spotify_object*)zend_object_store_get_object((parent) TSRMLS_CC);
	spotifyalbum_object *obj = (spotifyalbum_object*)zend_object_store_get_object(object TSRMLS_CC);
	obj->session = p->session;
	obj->album = album;

	zend_update_property(spotifyalbum_ce, getThis(), "spotify", strlen("spotify"), parent TSRMLS_CC);

	sp_album_add_ref(obj->album);
}

PHP_METHOD(SpotifyAlbum, __destruct)
{
	spotifyalbum_object *obj = (spotifyalbum_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	sp_album_release(obj->album);
}

PHP_METHOD(SpotifyAlbum, getName)
{
	zval *object = getThis();
	spotifyalbum_object *p = (spotifyalbum_object*)zend_object_store_get_object(object TSRMLS_CC);

	RETURN_STRING(sp_album_name(p->album), 1);
}

PHP_METHOD(SpotifyAlbum, getURI)
{
	char uri[256];
	spotifyalbum_object *p = (spotifyalbum_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	sp_link *link = sp_link_create_from_album(p->album);
	sp_link_as_string(link, uri, 256);
	sp_link_release(link);

	RETURN_STRING(uri, 1);
}

PHP_METHOD(SpotifyAlbum, getYear)
{
	spotifyalbum_object *p = (spotifyalbum_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(sp_album_year(p->album));
}

PHP_METHOD(SpotifyAlbum, getArtist)
{
	sp_artist *artist;
	zval temp, *spotifyobject;
	spotifyalbum_object *p = (spotifyalbum_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	spotifyobject = GET_THIS_PROPERTY(spotifyalbum_ce, "spotify");

	artist = sp_album_artist(p->album);

	object_init_ex(return_value, spotifyartist_ce);
	SPOTIFY_METHOD2(SpotifyArtist, __construct, &temp, return_value, spotifyobject, artist);
}

PHP_METHOD(SpotifyAlbum, getNumTracks)
{
	zend_throw_exception(zend_exception_get_default(), "SpotifyAlbum::getNumTracks() is deprecated. See SpotifyAlbum::getTracks().");
}

PHP_METHOD(SpotifyAlbum, getTracks)
{
	zval tempretval, *type, *spotifyobject;

	ALLOC_INIT_ZVAL(type);
	Z_TYPE_P(type) = IS_LONG;
	Z_LVAL_P(type) = 1;

	spotifyobject = GET_THIS_PROPERTY(spotifyalbum_ce, "spotify");

	object_init_ex(return_value, spotifytrackiterator_ce);
	SPOTIFY_METHOD3(SpotifyTrackIterator, __construct, &tempretval, return_value, getThis(), type, spotifyobject);
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

function_entry spotifyalbum_methods[] = {
	PHP_ME(SpotifyAlbum, __construct,		NULL,	ZEND_ACC_PRIVATE|ZEND_ACC_CTOR)
	PHP_ME(SpotifyAlbum, __destruct,		NULL,	ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(SpotifyAlbum, getName,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbum, getURI,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbum, getYear,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbum, getArtist,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbum, getNumTracks,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbum, getTracks,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbum, getType,			NULL,	ZEND_ACC_PUBLIC)
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

	zend_declare_class_constant_long(spotifyalbum_ce, "TYPE_ALBUM", strlen("TYPE_ALBUM"), 0 TSRMLS_CC);
	zend_declare_class_constant_long(spotifyalbum_ce, "TYPE_SINGLE", strlen("TYPE_SINGLE"), 1 TSRMLS_CC);
	zend_declare_class_constant_long(spotifyalbum_ce, "TYPE_COMPILATION", strlen("TYPE_COMPILATION"), 2 TSRMLS_CC);
	zend_declare_class_constant_long(spotifyalbum_ce, "TYPE_UNKNOWN", strlen("TYPE_UNKNOWN"), 3 TSRMLS_CC);

	zend_declare_property_null(spotifyalbum_ce, "spotify", strlen("spotify"), ZEND_ACC_PROTECTED TSRMLS_CC);
}
