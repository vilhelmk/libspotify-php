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

zend_class_entry *spotifytrack_ce;

PHP_METHOD(SpotifyTrack, __construct)
{
	zval *object = getThis();
	zval *parent;
	sp_track *track;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oz", &parent, spotify_ce, &track) == FAILURE) {
		return;
	}

	spotify_object *p = (spotify_object*)zend_object_store_get_object((parent) TSRMLS_CC);
	spotifytrack_object *obj = (spotifytrack_object*)zend_object_store_get_object(object TSRMLS_CC);
	obj->session = p->session;
	obj->track = track;

	zend_update_property(spotifytrack_ce, getThis(), "spotify", strlen("spotify"), parent TSRMLS_CC);

	sp_track_add_ref(obj->track);
}

PHP_METHOD(SpotifyTrack, __destruct)
{
	spotifytrack_object *obj = (spotifytrack_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	sp_track_release(obj->track);
}

PHP_METHOD(SpotifyTrack, getName)
{
	zval *object = getThis();
	spotifytrack_object *p = (spotifytrack_object*)zend_object_store_get_object(object TSRMLS_CC);

	RETURN_STRING(sp_track_name(p->track), 1);
}

PHP_METHOD(SpotifyTrack, getURI)
{
	char uri[256];
	spotifytrack_object *p = (spotifytrack_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	// TODO add support for offset in the track link
	sp_link *link = sp_link_create_from_track(p->track, 0);
	sp_link_as_string(link, uri, 256);
	sp_link_release(link);

	RETURN_STRING(uri, 1);
}

PHP_METHOD(SpotifyTrack, getAlbum)
{
	zval *object = getThis(), temp, *spotifyobject;
	spotifytrack_object *p = (spotifytrack_object*)zend_object_store_get_object(object TSRMLS_CC);

	sp_album *album = sp_track_album(p->track);
	if (NULL == album) {
		RETURN_FALSE;
	}

	spotifyobject = GET_THIS_PROPERTY(spotifytrack_ce, "spotify");

	object_init_ex(return_value, spotifyalbum_ce);
	SPOTIFY_METHOD2(SpotifyAlbum, __construct, &temp, return_value, spotifyobject, album);
}

PHP_METHOD(SpotifyTrack, getArtist)
{
	zval *object = getThis(), temp, *spotifyobject;
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

	spotifyobject = GET_THIS_PROPERTY(spotifytrack_ce, "spotify");

	object_init_ex(return_value, spotifyartist_ce);
	SPOTIFY_METHOD2(SpotifyArtist, __construct, &temp, return_value, spotifyobject, artist);
}

PHP_METHOD(SpotifyTrack, getDuration)
{
	zval *object = getThis();
	spotifytrack_object *p = (spotifytrack_object*)zend_object_store_get_object(object TSRMLS_CC);

	int duration_ms = sp_track_duration(p->track);
	RETURN_DOUBLE((float)duration_ms / 1000.0f);
}

PHP_METHOD(SpotifyTrack, getPopularity)
{
	spotifytrack_object *p = (spotifytrack_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(sp_track_popularity(p->track));
}

PHP_METHOD(SpotifyTrack, getIndex)
{
	spotifytrack_object *p = (spotifytrack_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(sp_track_index(p->track));
}

PHP_METHOD(SpotifyTrack, isStarred)
{
	spotifytrack_object *p = (spotifytrack_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (sp_track_is_starred(p->session, p->track)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}

PHP_METHOD(SpotifyTrack, isAvailable)
{
	spotifytrack_object *p = (spotifytrack_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	sp_track_availability availability = sp_track_get_availability(p->session, p->track);

	if (availability == SP_TRACK_AVAILABILITY_AVAILABLE) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}

PHP_METHOD(SpotifyTrack, setStarred)
{
	bool starred;
	zval *object = getThis();
	spotifytrack_object *p = (spotifytrack_object*)zend_object_store_get_object(object TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &starred) == FAILURE) {
		return;
	}

	sp_track_set_starred(p->session, &p->track, 1, starred);
}

PHP_METHOD(SpotifyTrack, __toString)
{
	spotifytrack_object *p = (spotifytrack_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_STRING(sp_track_name(p->track), 1);
}

zend_function_entry spotifytrack_methods[] = {
	PHP_ME(SpotifyTrack, __construct,            NULL,   ZEND_ACC_PRIVATE|ZEND_ACC_CTOR)
	PHP_ME(SpotifyTrack, __destruct,	NULL,	ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(SpotifyTrack, getName,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrack, getURI,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrack, getAlbum,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrack, getArtist,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrack, getDuration,	NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrack, getPopularity,	NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrack, getIndex,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrack, isAvailable,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrack, isStarred,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrack, setStarred,	NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyTrack, __toString,	NULL,	ZEND_ACC_PUBLIC)
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

	zend_object_std_init(&obj->std, type TSRMLS_CC);
    #if PHP_VERSION_ID < 50399
    	zend_hash_copy(obj->std.properties, &type->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));
    #else
    	 object_properties_init(&(obj->std), type);
   	#endif

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

	zend_declare_property_null(spotifytrack_ce, "spotify", strlen("spotify"), ZEND_ACC_PROTECTED TSRMLS_CC);
}
