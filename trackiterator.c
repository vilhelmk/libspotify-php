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

zend_class_entry *spotifytrackiterator_ce;

#define TYPE_ALBUM 1
#define TYPE_PLAYLIST 0

void spotify_albumbrowse_complete(sp_albumbrowse *result, void *userdata) {
	
}

PHP_METHOD(SpotifyTrackIterator, __construct)
{
	zval *thisptr = getThis(), *parent, *spotifyobject, *type;
	spotifytrackiterator_object *obj;
	spotifyalbum_object *albumobj;
	spotifyplaylist_object *playlistobj;
	int timeout = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zzO", &parent, &type, &spotifyobject, spotify_ce) == FAILURE) {
		return;
	}

	obj = (spotifytrackiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	obj->position = 0;
	obj->type = Z_LVAL_P(type);

	if (obj->type < 0 || obj->type > 1) {
		zend_throw_exception(zend_exception_get_default(), "SpotifyTrackIterator::__construct() type is invalid.");
		return;
	}

	switch (obj->type) {
	case TYPE_ALBUM:
		albumobj = (spotifyalbum_object*)zend_object_store_get_object(parent TSRMLS_CC);
		obj->session = albumobj->session;
		obj->albumbrowse = sp_albumbrowse_create(obj->session, albumobj->album, spotify_albumbrowse_complete, obj);

		while (!sp_albumbrowse_is_loaded(obj->albumbrowse)) {
			sp_session_process_events(obj->session, &timeout);
		}

		obj->length = sp_albumbrowse_num_tracks(obj->albumbrowse);
		sp_albumbrowse_add_ref(obj->albumbrowse);
		break;
	
	case TYPE_PLAYLIST:
		playlistobj = (spotifyplaylist_object*)zend_object_store_get_object(parent TSRMLS_CC);
		obj->session = playlistobj->session;
		obj->playlist = playlistobj->playlist;
		obj->length = sp_playlist_num_tracks(obj->playlist);

		sp_playlist_add_ref(obj->playlist);
		break;
	}

	zend_update_property(spotifytrackiterator_ce, getThis(), "spotify", strlen("spotify"), spotifyobject TSRMLS_CC);
}

PHP_METHOD(SpotifyTrackIterator, __destruct)
{
	spotifytrackiterator_object *p = (spotifytrackiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	switch (p->type) {
	case TYPE_ALBUM:
		sp_albumbrowse_release(p->albumbrowse);
		break;
	case TYPE_PLAYLIST:
		sp_playlist_release(p->playlist);
		break;
	}
}

static void get_track_by_index(spotifytrackiterator_object *p, zval *thisptr, int index, zval **return_value) {
	zval temp, *spotifyobject;
	sp_track *track;
	int timeout;

	switch (p->type) {
	case TYPE_ALBUM:
		track = sp_albumbrowse_track(p->albumbrowse, index);
		break;
	case TYPE_PLAYLIST:
		track = sp_playlist_track(p->playlist, index);
		break;
	}

	while (!sp_track_is_loaded(track)) {
		sp_session_process_events(p->session, &timeout);
	}

	spotifyobject = GET_PROPERTY(spotifytrackiterator_ce, thisptr, "spotify");

	object_init_ex(*return_value, spotifytrack_ce);
	SPOTIFY_METHOD2(SpotifyTrack, __construct, &temp, *return_value, spotifyobject, track);
}

PHP_METHOD(SpotifyTrackIterator, current)
{
	spotifytrackiterator_object *p;
	zval temp, *spotifyobject;
	
	p = (spotifytrackiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	get_track_by_index(p, getThis(), p->position, &return_value);
}

PHP_METHOD(SpotifyTrackIterator, key)
{
	spotifytrackiterator_object *p = (spotifytrackiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(p->position);
}

PHP_METHOD(SpotifyTrackIterator, next)
{
	spotifytrackiterator_object *p = (spotifytrackiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	p->position++;
}

PHP_METHOD(SpotifyTrackIterator, rewind)
{
	spotifytrackiterator_object *p = (spotifytrackiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	p->position = 0;
}

PHP_METHOD(SpotifyTrackIterator, valid)
{
	spotifytrackiterator_object *p = (spotifytrackiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	if (p->position >= p->length) {
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}
}

PHP_METHOD(SpotifyTrackIterator, offsetExists)
{
	zval *index;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &index) == FAILURE) {
		return;
	}

	spotifytrackiterator_object *p = (spotifytrackiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	if (Z_LVAL_P(index) >= p->length || Z_LVAL_P(index) < 0) {
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}
}

PHP_METHOD(SpotifyTrackIterator, offsetGet)
{
	zval *index;
	spotifytrackiterator_object *p;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &index) == FAILURE) {
		return;
	}
	
	p = (spotifytrackiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	get_track_by_index(p, getThis(), Z_LVAL_P(index), &return_value);
}

PHP_METHOD(SpotifyTrackIterator, offsetSet)
{

}

PHP_METHOD(SpotifyTrackIterator, offsetUnset)
{

}

PHP_METHOD(SpotifyTrackIterator, count)
{
	spotifytrackiterator_object *p = (spotifytrackiterator_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
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

function_entry spotifytrackiterator_methods[] = {
	PHP_ME(SpotifyTrackIterator, __construct,		NULL,	ZEND_ACC_PRIVATE|ZEND_ACC_CTOR)
	PHP_ME(SpotifyTrackIterator, __destruct,		NULL,	ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)

	// ArrayIterator
	PHP_ME(SpotifyTrackIterator, current,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrackIterator, key,				NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrackIterator, next,				NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrackIterator, rewind,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrackIterator, valid,				NULL,	ZEND_ACC_PUBLIC)

	// ArrayAccess
	PHP_ME(SpotifyTrackIterator, offsetExists,		arginfo_offsetExists,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrackIterator, offsetGet,			arginfo_offsetGet,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrackIterator, offsetSet,			arginfo_offsetSet,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrackIterator, offsetUnset,		arginfo_offsetUnset,	ZEND_ACC_PUBLIC)

	// Other functions
	PHP_ME(SpotifyTrackIterator, count,				NULL,	ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

void spotifytrackiterator_free_storage(void *object TSRMLS_DC)
{
	spotifytrackiterator_object *obj = (spotifytrackiterator_object*)object;
	zend_hash_destroy(obj->std.properties);
	FREE_HASHTABLE(obj->std.properties);
	efree(obj);
}

zend_object_value spotifytrackiterator_create_handler(zend_class_entry *type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;

	spotifytrackiterator_object *obj = (spotifytrackiterator_object *)emalloc(sizeof(spotifytrackiterator_object));
	memset(obj, 0, sizeof(spotifytrackiterator_object));

	zend_object_std_init(&obj->std, type TSRMLS_CC);
    zend_hash_copy(obj->std.properties, &type->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));

    retval.handle = zend_objects_store_put(obj, NULL,
        spotifytrackiterator_free_storage, NULL TSRMLS_CC);
    retval.handlers = &spotify_object_handlers;

    return retval;
}

void spotify_init_trackiterator(TSRMLS_D)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "SpotifyTrackIterator", spotifytrackiterator_methods);
	spotifytrackiterator_ce = zend_register_internal_class(&ce TSRMLS_CC);
	spotifytrackiterator_ce->create_object = spotifytrackiterator_create_handler;

	zend_class_implements(spotifytrackiterator_ce TSRMLS_CC, 1, zend_ce_iterator);
	zend_class_implements(spotifytrackiterator_ce TSRMLS_CC, 1, zend_ce_arrayaccess);

	zend_declare_class_constant_long(spotifytrackiterator_ce, "TYPE_PLAYLIST", strlen("TYPE_PLAYLIST"), 0 TSRMLS_CC);
	zend_declare_class_constant_long(spotifytrackiterator_ce, "TYPE_ALBUM", strlen("TYPE_ALBUM"), 1 TSRMLS_CC); 

	zend_declare_property_null(spotifytrackiterator_ce, "spotify", strlen("spotify"), ZEND_ACC_PROTECTED TSRMLS_CC);
}
