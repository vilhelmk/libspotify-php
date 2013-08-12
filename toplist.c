/*
Copyright (c) 2012 Liam McLoughlin, hexxeh@hexxeh.net

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

zend_class_entry *spotifytoplist_ce;

PHP_METHOD(SpotifyToplist, browse);

PHP_METHOD(SpotifyToplist, __construct)
{
	zval *object = getThis();
	zval *parent;
	sp_toplistbrowse *toplistbrowse;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &parent) == FAILURE) {
                return;
        }

	spotify_object *p = (spotify_object*)zend_object_store_get_object((parent) TSRMLS_CC);
	spotifytoplist_object *obj = (spotifytoplist_object*)zend_object_store_get_object(object TSRMLS_CC);
	obj->session = p->session;
	obj->toplistbrowse = NULL;

	zend_update_property(spotifytoplist_ce, getThis(), "spotify", strlen("spotify"), parent TSRMLS_CC);
}

PHP_METHOD(SpotifyToplist, __destruct)
{
	spotifytoplist_object *p = (spotifytoplist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

        if (p->toplistbrowse != NULL) {
                sp_toplistbrowse_release(p->toplistbrowse);
        }
}

static void toplistbrowse_complete(sp_toplistbrowse *result, void *userdata)
{
        spotifytoplist_object *p = (spotifytoplist_object*)userdata;
        p->toplistbrowse = result;
	sp_toplistbrowse_add_ref(p->toplistbrowse);
}

PHP_METHOD(SpotifyToplist, getRegionCode)
{
	spotifytoplist_object *p = (spotifytoplist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	char *region;
	int region_length;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &region, &region_length) == FAILURE) {
                return;
        }

	if(region_length > 2)
	{
		RETURN_FALSE;
	}

	int regioncode = SP_TOPLIST_REGION(region[0], region[1]);

	RETURN_LONG(regioncode);
}

PHP_METHOD(SpotifyToplist, getTracks)
{
        int i, num_tracks, timeout = 0;
        zval tempretval, *spotifyobject;
        spotifytoplist_object *p = (spotifytoplist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	long region = 0;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &region) == FAILURE) {
                return;
        }

	spotifyobject = GET_THIS_PROPERTY(spotifytoplist_ce, "spotify");

        sp_toplistbrowse *tmpbrowse = sp_toplistbrowse_create(p->session, SP_TOPLIST_TYPE_TRACKS, region, NULL, toplistbrowse_complete, p);
        while (!sp_toplistbrowse_is_loaded(tmpbrowse)) {
                sp_session_process_events(p->session, &timeout);
        }

        array_init(return_value);

        num_tracks = sp_toplistbrowse_num_tracks(p->toplistbrowse);
        for (i=0; i<num_tracks; i++) {
                sp_track *track = sp_toplistbrowse_track(p->toplistbrowse, i);
                while (!sp_track_is_loaded(track)) {
                        sp_session_process_events(p->session, &timeout);
                }

                sp_track_availability avail = sp_track_get_availability(p->session, track);

                if(avail != SP_TRACK_AVAILABILITY_AVAILABLE)
                {
                        sp_track_release(track);
                        continue;
                }

                zval *z_track;
                ALLOC_INIT_ZVAL(z_track);
                object_init_ex(z_track, spotifytrack_ce);
                SPOTIFY_METHOD2(SpotifyTrack, __construct, &tempretval, z_track, spotifyobject, track);

                add_next_index_zval(return_value, z_track);
        }
}

PHP_METHOD(SpotifyToplist, getAlbums)
{
        int i, num_albums, timeout = 0;
        zval tempretval, *spotifyobject;
        spotifytoplist_object *p = (spotifytoplist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	long region = 0;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &region) == FAILURE) {
                return;
        }

	spotifyobject = GET_THIS_PROPERTY(spotifytoplist_ce, "spotify");

        sp_toplistbrowse *tmpbrowse = sp_toplistbrowse_create(p->session, SP_TOPLIST_TYPE_ALBUMS, region, NULL, toplistbrowse_complete, p);
        while (!sp_toplistbrowse_is_loaded(tmpbrowse)) {
                sp_session_process_events(p->session, &timeout);
        }

        array_init(return_value);

        num_albums = sp_toplistbrowse_num_albums(p->toplistbrowse);
        for (i=0; i<num_albums; i++) {
                sp_album *album = sp_toplistbrowse_album(p->toplistbrowse, i);
                while (!sp_album_is_loaded(album)) {
                        sp_session_process_events(p->session, &timeout);
                }

                bool avail = sp_album_is_available(album);

                if(!avail)
                {
                        sp_album_release(album);
                        continue;
                }

                zval *z_album;
                ALLOC_INIT_ZVAL(z_album);
                object_init_ex(z_album, spotifyalbum_ce);
                SPOTIFY_METHOD2(SpotifyAlbum, __construct, &tempretval, z_album, spotifyobject, album);

                add_next_index_zval(return_value, z_album);
        }
}

PHP_METHOD(SpotifyToplist, getArtists)
{
        int i, num_artists, timeout = 0;
        zval tempretval, *spotifyobject;
        spotifytoplist_object *p = (spotifytoplist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	long region = 0;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &region) == FAILURE) {
                return;
        }

	spotifyobject = GET_THIS_PROPERTY(spotifytoplist_ce, "spotify");

        sp_toplistbrowse *tmpbrowse = sp_toplistbrowse_create(p->session, SP_TOPLIST_TYPE_ARTISTS, region, NULL, toplistbrowse_complete, p);
        while (!sp_toplistbrowse_is_loaded(tmpbrowse)) {
                sp_session_process_events(p->session, &timeout);
        }

        array_init(return_value);

        num_artists = sp_toplistbrowse_num_artists(p->toplistbrowse);
        for (i=0; i<num_artists; i++) {
                sp_artist *artist = sp_toplistbrowse_artist(p->toplistbrowse, i);
                while (!sp_artist_is_loaded(artist)) {
                        sp_session_process_events(p->session, &timeout);
                }

                zval *z_artist;
                ALLOC_INIT_ZVAL(z_artist);
                object_init_ex(z_artist, spotifyartist_ce);
                SPOTIFY_METHOD2(SpotifyArtist, __construct, &tempretval, z_artist, spotifyobject, artist);

                add_next_index_zval(return_value, z_artist);
        }
}

PHP_METHOD(SpotifyToplist, __toString)
{
	spotifytoplist_object *p = (spotifytoplist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_STRING("SpotifyToplist", 1);
}

zend_function_entry spotifytoplist_methods[] = {
	PHP_ME(SpotifyToplist, __construct,		NULL,	ZEND_ACC_PRIVATE|ZEND_ACC_CTOR)
	PHP_ME(SpotifyToplist, __destruct,		NULL,	ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(SpotifyToplist, getRegionCode,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyToplist, getTracks,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyToplist, getAlbums,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyToplist, getArtists,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyToplist, __toString,		NULL,	ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

void spotifytoplist_free_storage(void *object TSRMLS_DC)
{
	spotifytoplist_object *obj = (spotifytoplist_object*)object;
	zend_hash_destroy(obj->std.properties);
	FREE_HASHTABLE(obj->std.properties);
	efree(obj);
}

zend_object_value spotifytoplist_create_handler(zend_class_entry *type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;

	spotifytoplist_object *obj = (spotifytoplist_object *)emalloc(sizeof(spotifytoplist_object));
	memset(obj, 0, sizeof(spotifytoplist_object));
   // obj->std.ce = type;

	zend_object_std_init(&obj->std, type TSRMLS_CC);
    #if PHP_VERSION_ID < 50399
    	zend_hash_copy(obj->std.properties, &type->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));
    #else
    	 object_properties_init(&(obj->std), type);
   	#endif

    retval.handle = zend_objects_store_put(obj, NULL,
        spotifytoplist_free_storage, NULL TSRMLS_CC);
    retval.handlers = &spotify_object_handlers;

    return retval;
}

void spotify_init_toplist(TSRMLS_D)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "SpotifyToplist", spotifytoplist_methods);
	spotifytoplist_ce = zend_register_internal_class(&ce TSRMLS_CC);
	spotifytoplist_ce->create_object = spotifytoplist_create_handler;

	zend_declare_property_null(spotifytoplist_ce, "spotify", strlen("spotify"), ZEND_ACC_PROTECTED TSRMLS_CC);
}
