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

zend_class_entry *spotifyartist_ce;

PHP_METHOD(SpotifyArtist, __construct)
{
	zval *object = getThis();
	zval *parent;
	sp_artist *artist;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oz", &parent, spotify_ce, &artist) == FAILURE) {
		return;
	}

	spotify_object *p = (spotify_object*)zend_object_store_get_object((parent) TSRMLS_CC);
	spotifyartist_object *obj = (spotifyartist_object*)zend_object_store_get_object(object TSRMLS_CC);
	obj->session = p->session;
	obj->artist = artist;

	zend_update_property(spotifyartist_ce, getThis(), "spotify", strlen("spotify"), parent TSRMLS_CC);

	sp_artist_add_ref(obj->artist);
}

PHP_METHOD(SpotifyArtist, __destruct)
{
	spotifyartist_object *p = (spotifyartist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	sp_artist_release(p->artist);
}

PHP_METHOD(SpotifyArtist, getName)
{
	zval *object = getThis();
	spotifyartist_object *p = (spotifyartist_object*)zend_object_store_get_object(object TSRMLS_CC);

	RETURN_STRING(sp_artist_name(p->artist), 1);
}

PHP_METHOD(SpotifyArtist, getURI)
{
	char uri[256];
	zval *object = getThis();
	spotifyartist_object *p = (spotifyartist_object*)zend_object_store_get_object(object TSRMLS_CC);

	sp_link *link = sp_link_create_from_artist(p->artist);
	sp_link_as_string(link, uri, 256);
	sp_link_release(link);

	RETURN_STRING(uri, 1);
}

PHP_METHOD(SpotifyArtist, getAlbums)
{
	zval tempretval;

	object_init_ex(return_value, spotifyalbumiterator_ce);
	SPOTIFY_METHOD1(SpotifyAlbumIterator, __construct, &tempretval, return_value, getThis());
}

PHP_METHOD(SpotifyArtist, __toString)
{
	spotifyartist_object *p = (spotifyartist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_STRING(sp_artist_name(p->artist), 1);
}

function_entry spotifyartist_methods[] = {
	PHP_ME(SpotifyArtist, __construct,		NULL,	ZEND_ACC_PRIVATE|ZEND_ACC_CTOR)
	PHP_ME(SpotifyArtist, __destruct,		NULL,	ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(SpotifyArtist, getName,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyArtist, getURI,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyArtist, getAlbums,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyArtist, __toString,		NULL,	ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

void spotifyartist_free_storage(void *object TSRMLS_DC)
{
	spotifyartist_object *obj = (spotifyartist_object*)object;
	zend_hash_destroy(obj->std.properties);
	FREE_HASHTABLE(obj->std.properties);
	efree(obj);
}

zend_object_value spotifyartist_create_handler(zend_class_entry *type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;

	spotifyartist_object *obj = (spotifyartist_object *)emalloc(sizeof(spotifyartist_object));
	memset(obj, 0, sizeof(spotifyartist_object));

	zend_object_std_init(&obj->std, type TSRMLS_CC);
    zend_hash_copy(obj->std.properties, &type->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));

    retval.handle = zend_objects_store_put(obj, NULL,
        spotifyartist_free_storage, NULL TSRMLS_CC);
    retval.handlers = &spotify_object_handlers;

    return retval;
}

void spotify_init_artist(TSRMLS_D)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "SpotifyArtist", spotifyartist_methods);
	spotifyartist_ce = zend_register_internal_class(&ce TSRMLS_CC);
	spotifyartist_ce->create_object = spotifyartist_create_handler;

	zend_declare_property_null(spotifyartist_ce, "spotify", strlen("spotify"), ZEND_ACC_PROTECTED TSRMLS_CC);
}
