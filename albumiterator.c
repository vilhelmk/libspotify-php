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
#include <zend_interfaces.h>

zend_class_entry *spotifyalbumiterator_ce;

void spotify_artistbrowse_complete(sp_artistbrowse *result, void *userdata) {
	
}

PHP_METHOD(SpotifyAlbumIterator, __construct)
{
	zval *thisptr = getThis(), *parent;
	spotifyartist_object *artistobj;
	int timeout = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &parent, spotifyartist_ce) == FAILURE) {
		return;
	}

	zval *spotifyobject = GET_PROPERTY(spotifyartist_ce, parent, "spotify");
	spotify_object *p = (spotify_object*)zend_object_store_get_object(spotifyobject TSRMLS_CC);

	artistobj = (spotifyartist_object*)zend_object_store_get_object(parent TSRMLS_CC);

	spotifyalbumiterator_object *obj = (spotifyalbumiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	obj->session = p->session;
	obj->artist = artistobj->artist;
	obj->artistbrowse = sp_artistbrowse_create(p->session, artistobj->artist, SP_ARTISTBROWSE_FULL, spotify_artistbrowse_complete, obj);

	while (!sp_artistbrowse_is_loaded(obj->artistbrowse)) {
		sp_session_process_events(p->session, &timeout);
	}

	obj->position = 0;
	obj->length = sp_artistbrowse_num_albums(obj->artistbrowse);

	zend_update_property(spotifyalbumiterator_ce, getThis(), "spotify", strlen("spotify"), spotifyobject TSRMLS_CC);

	sp_artistbrowse_add_ref(obj->artistbrowse);
}

PHP_METHOD(SpotifyAlbumIterator, __destruct)
{
	spotifyalbumiterator_object *p = (spotifyalbumiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	sp_artistbrowse_release(p->artistbrowse);
}

PHP_METHOD(SpotifyAlbumIterator, current)
{
	spotifyalbumiterator_object *p;
	sp_album *album;
	zval temp, *spotifyobject;
	int timeout;
	
	p = (spotifyalbumiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	album = sp_artistbrowse_album(p->artistbrowse, p->position);

	while (!sp_album_is_loaded(album)) {
		sp_session_process_events(p->session, &timeout);
	}

	spotifyobject = GET_THIS_PROPERTY(spotifyalbumiterator_ce, "spotify");

	object_init_ex(return_value, spotifyalbum_ce);
	SPOTIFY_METHOD2(SpotifyAlbum, __construct, &temp, return_value, spotifyobject, album);
}

PHP_METHOD(SpotifyAlbumIterator, key)
{
	spotifyalbumiterator_object *p = (spotifyalbumiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(p->position);
}

PHP_METHOD(SpotifyAlbumIterator, next)
{
	spotifyalbumiterator_object *p = (spotifyalbumiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	p->position++;
}

PHP_METHOD(SpotifyAlbumIterator, rewind)
{
	spotifyalbumiterator_object *p = (spotifyalbumiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	p->position = 0;
}

PHP_METHOD(SpotifyAlbumIterator, valid)
{
	spotifyalbumiterator_object *p = (spotifyalbumiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	if (p->position >= p->length) {
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}
}

PHP_METHOD(SpotifyAlbumIterator, offsetExists)
{
	zval *index;
	spotifyalbumiterator_object *p;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &index) == FAILURE) {
		return;
	}

	p = (spotifyalbumiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	if (Z_LVAL_P(index) >= p->length || Z_LVAL_P(index) < 0) {
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}
}

PHP_METHOD(SpotifyAlbumIterator, offsetGet)
{
	zval *index, temp, *spotifyobject;
	sp_album *album;
	spotifyalbumiterator_object *p;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &index) == FAILURE) {
		return;
	}

	p = (spotifyalbumiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	album = sp_artistbrowse_album(p->artistbrowse, Z_LVAL_P(index));

	spotifyobject = GET_THIS_PROPERTY(spotifyalbumiterator_ce, "spotify");

	object_init_ex(return_value, spotifyalbum_ce);
	SPOTIFY_METHOD2(SpotifyAlbum, __construct, &temp, return_value, spotifyobject, album);
}

PHP_METHOD(SpotifyAlbumIterator, offsetSet)
{

}

PHP_METHOD(SpotifyAlbumIterator, offsetUnset)
{

}

PHP_METHOD(SpotifyAlbumIterator, count)
{
	spotifyalbumiterator_object *p = (spotifyalbumiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(p->length);
}


ZEND_BEGIN_ARG_INFO_EX(arginfo_offsetExists, 0, 0, 1)
    ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_offsetGet, 0, 0, 1)
    ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_offsetSet, 0, 0, 1)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_offsetUnset, 0, 0, 1)
	ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()

zend_function_entry spotifyalbumiterator_methods[] = {
	PHP_ME(SpotifyAlbumIterator, __construct,		NULL,	ZEND_ACC_PRIVATE|ZEND_ACC_CTOR)
	PHP_ME(SpotifyAlbumIterator, __destruct,		NULL,	ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)

	// ArrayIterator
	PHP_ME(SpotifyAlbumIterator, current,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbumIterator, key,				NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbumIterator, next,				NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbumIterator, rewind,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbumIterator, valid,				NULL,	ZEND_ACC_PUBLIC)

	// ArrayAccess
	PHP_ME(SpotifyAlbumIterator, offsetExists,		arginfo_offsetExists,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbumIterator, offsetGet,			arginfo_offsetGet,		ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbumIterator, offsetSet,			arginfo_offsetSet,		ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyAlbumIterator, offsetUnset,		arginfo_offsetUnset,	ZEND_ACC_PUBLIC)

	// Other functions
	PHP_ME(SpotifyAlbumIterator, count,				NULL,	ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

void spotifyalbumiterator_free_storage(void *object TSRMLS_DC)
{
	spotifyalbumiterator_object *obj = (spotifyalbumiterator_object*)object;
	zend_hash_destroy(obj->std.properties);
	FREE_HASHTABLE(obj->std.properties);
	efree(obj);
}

zend_object_value spotifyalbumiterator_create_handler(zend_class_entry *type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;

	spotifyalbumiterator_object *obj = (spotifyalbumiterator_object *)emalloc(sizeof(spotifyalbumiterator_object));
	memset(obj, 0, sizeof(spotifyalbumiterator_object));

	zend_object_std_init(&obj->std, type TSRMLS_CC);
    #if PHP_VERSION_ID < 50399
    	zend_hash_copy(obj->std.properties, &type->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));
    #else
    	 object_properties_init(&(obj->std), type);
   	#endif

    retval.handle = zend_objects_store_put(obj, NULL,
        spotifyalbumiterator_free_storage, NULL TSRMLS_CC);
    retval.handlers = &spotify_object_handlers;

    return retval;
}

void spotify_init_albumiterator(TSRMLS_D)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "SpotifyAlbumIterator", spotifyalbumiterator_methods);
	spotifyalbumiterator_ce = zend_register_internal_class(&ce TSRMLS_CC);
	spotifyalbumiterator_ce->create_object = spotifyalbumiterator_create_handler;

	zend_class_implements(spotifyalbumiterator_ce TSRMLS_CC, 1, zend_ce_iterator);

	zend_declare_property_null(spotifyalbumiterator_ce, "spotify", strlen("spotify"), ZEND_ACC_PROTECTED TSRMLS_CC);
}
