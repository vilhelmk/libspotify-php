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

zend_class_entry *spotify_ce;
zend_object_handlers spotify_object_handlers;

static void playlistcontainer_complete(sp_playlistcontainer *pc, void *userdata);
static void logged_in(sp_session *session, sp_error error);
static void logged_out(sp_session *session);
static void log_message(sp_session *session, const char *data);

static sp_session_callbacks callbacks = {
	&logged_in,
	&logged_out,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&log_message
};

PHP_METHOD(Spotify, initPlaylistContainer);

PHP_METHOD(Spotify, __construct)
{
	zval *object = getThis();
	zval *z_key, *z_user, *z_pass;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zzz", &z_key, &z_user, &z_pass) == FAILURE) {
		return;
	}

	sp_session_config config;
	sp_error error;
	sp_session *session;
	FILE *fp;
	long key_size;
	char *key_filename;

	spotify_object *obj = (spotify_object*)zend_object_store_get_object(object TSRMLS_CC);

	memset(&config, 0, sizeof(config));
	config.api_version = SPOTIFY_API_VERSION;
	config.cache_location = INI_STR("spotify.cache_location");
	config.settings_location = INI_STR("spotify.settings_location");
	config.user_agent = "libspotify-php";

	if (VCWD_ACCESS(config.cache_location, W_OK|X_OK|R_OK) != 0) {
		zend_throw_exception(zend_exception_get_default(), "spotify.cache_location is not writable or readable", 0 TSRMLS_CC);
		return;
	}

	if (VCWD_ACCESS(config.settings_location, W_OK|X_OK|R_OK) != 0) {
		zend_throw_exception(zend_exception_get_default(), "spotify.settings_location is not writable or readable", 0 TSRMLS_CC);
		return;
	}

	key_filename = Z_STRVAL_P(z_key);

	fp = fopen(key_filename, "rb");
	if (!fp) {
		zend_throw_exception(zend_exception_get_default(), "Unable to open spotify key file", 0 TSRMLS_CC);
		return;
	}

	fseek(fp, 0L, SEEK_END);
	key_size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	if (key_size > 4096) {
		fclose(fp);
		zend_throw_exception(zend_exception_get_default(), "Key file is way too large to be a key file", 0 TSRMLS_CC);
		return;
	}

	obj->key_data = (char*)emalloc(sizeof(char) * key_size);
	if (fread(obj->key_data, 1, key_size, fp) != key_size) {
		fclose(fp);
		zend_throw_exception(zend_exception_get_default(), "Failed reading key file", 0 TSRMLS_CC);
		return;
	}
	fclose(fp);

	config.application_key = obj->key_data;
	config.application_key_size = key_size;
	
	config.callbacks = &callbacks;
	config.userdata = obj;

	error = sp_session_create(&config, &session);
	if (SP_ERROR_OK != error) {
		char *errMsg;
		spprintf(&errMsg, 0, "Unable to create session: %s", sp_error_message(error));
		zend_throw_exception(zend_exception_get_default(), errMsg, 0 TSRMLS_CC);
		return;
	}

	sp_session_login(session, Z_STRVAL_P(z_user), Z_STRVAL_P(z_pass), 0, 0);

	obj->session = session;
	obj->timeout = 0;
	obj->playlistcontainer = NULL;

	do {
		sp_session_process_events(obj->session, &obj->timeout);
	} while (obj->timeout == 0 || !obj->is_logged_in);
}

PHP_METHOD(Spotify, __destruct)
{
	spotify_object *obj = (spotify_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	int timeout = 0;

	if (obj->playlistcontainer != NULL) {
		sp_playlistcontainer_release(obj->playlistcontainer);
	}

    do {
        sp_session_process_events(obj->session, &timeout);
    } while (timeout == 0);

	sp_session_logout(obj->session);

	timeout = 0;
	do {
		sp_session_process_events(obj->session, &timeout);
	} while (!obj->is_logged_out || timeout == 0);

	efree(obj->key_data);
}

PHP_METHOD(Spotify, getPlaylists)
{
	zval *thisptr = getThis(), tempretval;
	int i, timeout = 0, num_playlists;

	spotify_object *p = (spotify_object*)zend_object_store_get_object(thisptr TSRMLS_CC);

	SPOTIFY_METHOD(Spotify, initPlaylistContainer, &tempretval, thisptr);

	array_init(return_value);

	num_playlists = sp_playlistcontainer_num_playlists(p->playlistcontainer);
	for (i=0; i<num_playlists; i++) {
		sp_playlist *playlist = sp_playlistcontainer_playlist(p->playlistcontainer, i);

		zval *z_playlist;
		ALLOC_INIT_ZVAL(z_playlist);
		object_init_ex(z_playlist, spotifyplaylist_ce);
		SPOTIFY_METHOD2(SpotifyPlaylist, __construct, &tempretval, z_playlist, thisptr, playlist);
		add_next_index_zval(return_value, z_playlist);
	}
}

PHP_METHOD(Spotify, getStarredPlaylist)
{
	zval *object = getThis();
	zval temp;

	spotify_object *obj = (spotify_object*)zend_object_store_get_object(object TSRMLS_CC);

	do {
		sp_session_process_events(obj->session, &obj->timeout);
	} while (obj->timeout == 0);

	sp_playlist *playlist = sp_session_starred_create(obj->session);

	object_init_ex(return_value, spotifyplaylist_ce);
	SPOTIFY_METHOD2(SpotifyPlaylist, __construct, &temp, return_value, object, playlist);
}

PHP_METHOD(Spotify, getInboxPlaylist)
{
        zval *object = getThis();
        zval temp;

        spotify_object *obj = (spotify_object*)zend_object_store_get_object(object TSRMLS_CC);

        do {
                sp_session_process_events(obj->session, &obj->timeout);
        } while (obj->timeout == 0);

        sp_playlist *playlist = sp_session_inbox_create(obj->session);

        object_init_ex(return_value, spotifyplaylist_ce);
        SPOTIFY_METHOD2(SpotifyPlaylist, __construct, &temp, return_value, object, playlist);
}

PHP_METHOD(Spotify, getPlaylistByURI)
{
	zval *uri, temp, *object = getThis();
	int timeout = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &uri) == FAILURE) {
		return;
	}

	spotify_object *p = (spotify_object*)zend_object_store_get_object(object TSRMLS_CC);

	sp_link *link = sp_link_create_from_string(Z_STRVAL_P(uri));
	if (NULL == link) {
		RETURN_FALSE;
	}

	if (SP_LINKTYPE_PLAYLIST != sp_link_type(link)) {
		RETURN_FALSE;
	}

	sp_playlist *playlist = sp_playlist_create(p->session, link);
	if (NULL == playlist) {
		RETURN_FALSE;
	}

	while (!sp_playlist_is_loaded(playlist)) {	
		sp_session_process_events(p->session, &timeout);
	}

	object_init_ex(return_value, spotifyplaylist_ce);
	SPOTIFY_METHOD2(SpotifyPlaylist, __construct, &temp, return_value, object, playlist);

	sp_link_release(link);
}

PHP_METHOD(Spotify, getTrackByURI)
{
	zval *uri, temp, *object = getThis();
	int timeout = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &uri) == FAILURE) {
		return;
	}

	spotify_object *p = (spotify_object*)zend_object_store_get_object(object TSRMLS_CC);

	sp_link *link = sp_link_create_from_string(Z_STRVAL_P(uri));
	if (NULL == link) {
		RETURN_FALSE;		
	}

	if (SP_LINKTYPE_TRACK != sp_link_type(link)) {
		RETURN_FALSE;
	}

	sp_track *track = sp_link_as_track(link);

	while (!sp_track_is_loaded(track)) {
		sp_session_process_events(p->session, &timeout);
	}
	
	object_init_ex(return_value, spotifytrack_ce);
	SPOTIFY_METHOD2(SpotifyTrack, __construct, &temp, return_value, object, track);

	sp_link_release(link);
}

PHP_METHOD(Spotify, getAlbumByURI)
{
	zval *uri, temp, *object = getThis();
	int timeout = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &uri) == FAILURE) {
		return;
	}

	spotify_object *p = (spotify_object*)zend_object_store_get_object(object TSRMLS_CC);

	sp_link *link = sp_link_create_from_string(Z_STRVAL_P(uri));
	if (NULL == link) {
		RETURN_FALSE;
	}

	if (SP_LINKTYPE_ALBUM != sp_link_type(link)) {
		RETURN_FALSE;
	}

	sp_album *album = sp_link_as_album(link);

	object_init_ex(return_value, spotifyalbum_ce);
	SPOTIFY_METHOD2(SpotifyAlbum, __construct, &temp, return_value, object, album);

	sp_link_release(link);
}

PHP_METHOD(Spotify, getArtistByURI)
{
        zval *uri, temp, *object = getThis();
        int timeout = 0;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &uri) == FAILURE) {
                return;
        }

        spotify_object *p = (spotify_object*)zend_object_store_get_object(object TSRMLS_CC);

        sp_link *link = sp_link_create_from_string(Z_STRVAL_P(uri));
        if (NULL == link) {
                RETURN_FALSE;
        }

        if (SP_LINKTYPE_ARTIST != sp_link_type(link)) {
                RETURN_FALSE;
        }

        sp_artist *artist = sp_link_as_artist(link);

        object_init_ex(return_value, spotifyartist_ce);
        SPOTIFY_METHOD2(SpotifyArtist, __construct, &temp, return_value, object, artist);

        sp_link_release(link);
}

PHP_METHOD(Spotify, getToplist)
{
        zval temp, *object = getThis();

        spotify_object *p = (spotify_object*)zend_object_store_get_object(object TSRMLS_CC);

        object_init_ex(return_value, spotifytoplist_ce);

        SPOTIFY_METHOD1(SpotifyToplist, __construct, &temp, return_value, object);
}

PHP_METHOD(Spotify, getUser)
{
        sp_user *user;
        zval temp;
        spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

        user = sp_session_user(p->session);

        object_init_ex(return_value, spotifyuser_ce);
        SPOTIFY_METHOD2(SpotifyUser, __construct, &temp, return_value, getThis(), user);
}

PHP_METHOD(Spotify, getUserCountry)
{
        spotifyplaylist_object *p = (spotifyplaylist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

        int user_country = sp_session_user_country(p->session);

        RETURN_LONG(user_country);
}

static sp_playlistcontainer_callbacks playlistcontainer_callbacks = {
	NULL,
	NULL,
	NULL,
	playlistcontainer_complete
};

static void playlistcontainer_complete(sp_playlistcontainer *pc, void *userdata)
{
	sp_playlistcontainer_remove_callbacks(pc, &playlistcontainer_callbacks, userdata);
}

PHP_METHOD(Spotify, initPlaylistContainer)
{
	int timeout = 0;

	spotify_object *p = (spotify_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	if (p->playlistcontainer != NULL) {
		RETURN_TRUE;
	}

	p->playlistcontainer = sp_session_playlistcontainer(p->session);
	sp_playlistcontainer_add_callbacks(p->playlistcontainer, &playlistcontainer_callbacks, p);

	while (!sp_playlistcontainer_is_loaded(p->playlistcontainer)) {
		sp_session_process_events(p->session, &timeout);
	}

	sp_playlistcontainer_add_ref(p->playlistcontainer);

	RETURN_TRUE;
}
static void logged_in(sp_session *session, sp_error error)
{
	spotify_object *p = sp_session_userdata(session);
	p->is_logged_in = 1;

	if (SP_ERROR_OK != error) {
		p->is_logged_out = 1;
		sp_session_release(session);

		char *errMsg;
		spprintf(&errMsg, 0, "Login failed: %s", sp_error_message(error));
		zend_throw_exception(zend_exception_get_default(), errMsg, 0 TSRMLS_CC);
	}
}

static void logged_out(sp_session *session)
{
	spotify_object *p = sp_session_userdata(session);
	p->is_logged_out = 1;
}

static void log_message(sp_session *session, const char *data)
{
	//php_printf("SPOTIFY_DEBUG: %s", data);
}

function_entry spotify_methods[] = {
	PHP_ME(Spotify, __construct,            NULL,   ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Spotify, __destruct,				NULL,	ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(Spotify, getPlaylists,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(Spotify, getStarredPlaylist,     NULL,   ZEND_ACC_PUBLIC)
	PHP_ME(Spotify, getInboxPlaylist,     NULL,   ZEND_ACC_PUBLIC)
	PHP_ME(Spotify, getPlaylistByURI,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(Spotify, getTrackByURI,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(Spotify, getAlbumByURI,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(Spotify, getArtistByURI,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(Spotify, getToplist,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(Spotify, getUser,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(Spotify, getUserCountry,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(Spotify, initPlaylistContainer,	NULL,	ZEND_ACC_PRIVATE)
	{NULL, NULL, NULL}
};

void spotify_free_storage(void *object TSRMLS_DC)
{
	spotify_object *obj = (spotify_object*)object;

	sp_session_release(obj->session);

	zend_hash_destroy(obj->std.properties);
	FREE_HASHTABLE(obj->std.properties);
	efree(obj);
}

zend_object_value spotify_create_handler(zend_class_entry *type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;

	spotify_object *obj = (spotify_object *)emalloc(sizeof(spotify_object));
	memset(obj, 0, sizeof(spotify_object));

	zend_object_std_init(&obj->std, type TSRMLS_CC);
    zend_hash_copy(obj->std.properties, &type->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));

    retval.handle = zend_objects_store_put(obj, NULL,
        spotify_free_storage, NULL TSRMLS_CC);
    retval.handlers = &spotify_object_handlers;

    return retval;
}

PHP_INI_BEGIN()
PHP_INI_ENTRY("spotify.cache_location", "spotify_cache/", PHP_INI_ALL, NULL)
PHP_INI_ENTRY("spotify.settings_location", "spotify_settings/", PHP_INI_ALL, NULL)
PHP_INI_END()

PHP_MINIT_FUNCTION(spotify)
{
	REGISTER_INI_ENTRIES();

	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "Spotify", spotify_methods);
	spotify_ce = zend_register_internal_class(&ce TSRMLS_CC);
	spotify_ce->create_object = spotify_create_handler;
	memcpy(&spotify_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	spotify_object_handlers.clone_obj = NULL;

	spotify_init_playlist(TSRMLS_C);
	spotify_init_track(TSRMLS_C);
	spotify_init_artist(TSRMLS_C);
	spotify_init_album(TSRMLS_C);
	spotify_init_user(TSRMLS_C);
	spotify_init_toplist(TSRMLS_C);

	spotify_init_albumiterator(TSRMLS_C);
	spotify_init_trackiterator(TSRMLS_C);

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(spotify)
{
	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}

zend_module_entry spotify_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_SPOTIFY_EXTNAME,
    NULL,        /* Functions */
    PHP_MINIT(spotify),        /* MINIT */
    PHP_MSHUTDOWN(spotify),        /* MSHUTDOWN */
    NULL,        /* RINIT */
    NULL,        /* RSHUTDOWN */
    NULL,        /* MINFO */
#if ZEND_MODULE_API_NO >= 20010901
    PHP_SPOTIFY_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SPOTIFY
ZEND_GET_MODULE(spotify)
#endif
