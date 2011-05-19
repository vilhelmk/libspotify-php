#include "php_spotify.h"

zend_class_entry *spotify_ce;
zend_object_handlers spotify_object_handlers;
static int is_logged_in;

static void logged_in(sp_session *session, sp_error error) ;

void (*metadata_updated_fn)(void);

static void metadata_updated(sp_session *sess)
{
	if (metadata_updated_fn) {
		metadata_updated_fn();
	}
}

static sp_session_callbacks callbacks = {
	&logged_in,
	NULL,
	&metadata_updated,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

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
	config.cache_location = "tmp";
	config.settings_location = "tmp";
	config.user_agent = "libspotify-php";

	key_filename = Z_STRVAL_P(z_key);

	fp = fopen(key_filename, "rb");
	if (!fp) {
		zend_throw_exception((zend_class_entry*)zend_exception_get_default(), "unable to open spotify key file", 0 TSRMLS_CC);
		return;
	}

	fseek(fp, 0L, SEEK_END);
	key_size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	if (key_size > 4096) {
		fclose(fp);
		zend_throw_exception((zend_class_entry*)zend_exception_get_default(), "key file is way too large to be a key file", 0 TSRMLS_CC);
		return;
	}

	obj->key_data = (char*)emalloc(sizeof(char) * key_size);
	if (fread(obj->key_data, 1, key_size, fp) != key_size) {
		fclose(fp);
		zend_throw_exception((zend_class_entry*)zend_exception_get_default(), "failed reading key file", 0 TSRMLS_CC);
		return;
	}
	fclose(fp);

	config.application_key = obj->key_data;
	config.application_key_size = key_size;

	config.callbacks = &callbacks;

	error = sp_session_create(&config, &session);
	if (SP_ERROR_OK != error) {
		zend_throw_exception((zend_class_entry*)zend_exception_get_default(), "unable to create session", 0 TSRMLS_CC);
		return;
	}

	sp_session_login(session, Z_STRVAL_P(z_user), Z_STRVAL_P(z_pass));

	obj->session = session;
	obj->timeout = 0;

	do {
		sp_session_process_events(obj->session, &obj->timeout);
	} while (obj->timeout == 0 || !is_logged_in);
}

PHP_METHOD(Spotify, __destruct)
{
	spotify_object *obj = (spotify_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	int timeout = 0;

    do {
        sp_session_process_events(obj->session, &timeout);
    } while (timeout == 0);

	sp_session_logout(obj->session);
	//sp_session_release(g_session);

	timeout = 0;
	do {
		sp_session_process_events(obj->session, &timeout);
	} while (timeout == 0);

	efree(obj->key_data);
}

PHP_METHOD(Spotify, getPlaylists)
{
	zval *object = getThis();

	spotify_object *obj = (spotify_object*)zend_object_store_get_object(object TSRMLS_CC);

	container_browse_data pcfg;
	pcfg.session = obj->session;
	pcfg.obj = object;

	sp_playlistcontainer *container = sp_session_playlistcontainer(obj->session);
	get_playlistcontainer_playlists(return_value, &pcfg, container);
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

static int has_entity_loaded;

static void entity_loaded()
{
	has_entity_loaded = 1;
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
		metadata_updated_fn = entity_loaded;
		do {
			sp_session_process_events(p->session, &timeout);
		} while (has_entity_loaded != 1 || timeout == 0);
		has_entity_loaded = 0;
		metadata_updated_fn = NULL;
	}

	object_init_ex(return_value, spotifyplaylist_ce);
	SPOTIFY_METHOD2(SpotifyPlaylist, __construct, &temp, return_value, object, playlist);
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
		metadata_updated_fn = entity_loaded;
		do {
			sp_session_process_events(p->session, &timeout);
		} while (has_entity_loaded != 1 || timeout == 0);
		has_entity_loaded = 0;
		metadata_updated_fn = NULL;
	}
	
	object_init_ex(return_value, spotifytrack_ce);
	SPOTIFY_METHOD2(SpotifyTrack, __construct, &temp, return_value, object, track);
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

	while (!sp_album_is_loaded(album)) {
		metadata_updated_fn = entity_loaded;
		do {
			sp_session_process_events(p->session, &timeout);
		} while (has_entity_loaded != 1 || timeout == 0);
		has_entity_loaded = 0;
		metadata_updated_fn = NULL;
	}

	object_init_ex(return_value, spotifyalbum_ce);
	SPOTIFY_METHOD2(SpotifyAlbum, __construct, &temp, return_value, object, album);
}

static void logged_in(sp_session *session, sp_error error)
{
	is_logged_in = 1;

	if (SP_ERROR_OK != error) {
		char *errMsg;
		spprintf(&errMsg, 0, "login failed: %s", sp_error_message(error));
		zend_throw_exception((zend_class_entry*)zend_exception_get_default(), errMsg, 0 TSRMLS_CC);
	}
}

function_entry spotify_methods[] = {
	PHP_ME(Spotify, __construct,            NULL,   ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Spotify, __destruct,				NULL,	ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(Spotify, getPlaylists,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(Spotify, getStarredPlaylist,     NULL,   ZEND_ACC_PUBLIC)
	PHP_ME(Spotify, getPlaylistByURI,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(Spotify, getTrackByURI,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(Spotify, getAlbumByURI,			NULL,	ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

void spotify_free_storage(void *object TSRMLS_DC)
{
	spotify_object *obj = (spotify_object*)object;
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
   // obj->std.ce = type;

	zend_object_std_init(&obj->std, type TSRMLS_CC);
    zend_hash_copy(obj->std.properties, &type->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));

    retval.handle = zend_objects_store_put(obj, NULL,
        spotify_free_storage, NULL TSRMLS_CC);
    retval.handlers = &spotify_object_handlers;

    return retval;
}

PHP_MINIT_FUNCTION(spotify)
{
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

	return SUCCESS;
}

zend_module_entry spotify_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_SPOTIFY_EXTNAME,
    NULL,        /* Functions */
    PHP_MINIT(spotify),        /* MINIT */
    NULL,        /* MSHUTDOWN */
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
