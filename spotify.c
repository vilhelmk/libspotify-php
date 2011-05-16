#include "php_spotify.h"

zend_object_handlers spotify_object_handlers;


static void logged_in(sp_session *session, sp_error error) ;

static sp_session_callbacks callbacks = {
	&logged_in,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

zend_class_entry *spotify_ce;

int is_logged_in;

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
	char *key_data, *key_filename;

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

	key_data = (char*)malloc(sizeof(char) * key_size);
	if (fread(key_data, 1, key_size, fp) != key_size) {
		fclose(fp);
		zend_throw_exception((zend_class_entry*)zend_exception_get_default(), "failed reading key file", 0 TSRMLS_CC);
		return;
	}
	fclose(fp);

	config.application_key = key_data;
	config.application_key_size = key_size;

	config.callbacks = &callbacks;

	error = sp_session_create(&config, &session);
	if (SP_ERROR_OK != error) {
		zend_throw_exception((zend_class_entry*)zend_exception_get_default(), "unable to create session", 0 TSRMLS_CC);
		return;
	}

	sp_session_login(session, Z_STRVAL_P(z_user), Z_STRVAL_P(z_pass));

	spotify_object *obj = (spotify_object*)zend_object_store_get_object(object TSRMLS_CC);
	obj->session = session;
	obj->timeout = 0;

	do {
		sp_session_process_events(obj->session, &obj->timeout);
	} while (obj->timeout == 0 || !is_logged_in);
}

PHP_METHOD(Spotify, getPlaylists)
{
	zval *object = getThis(), temp;
	int timeout = 0, i, num_playlists;
	spotify_object *obj = (spotify_object*)zend_object_store_get_object(object TSRMLS_CC);

	do {
		sp_session_process_events(obj->session, &timeout);		
	} while (timeout == 0);

	sp_playlistcontainer *container = sp_session_playlistcontainer(obj->session);
	num_playlists = sp_playlistcontainer_num_playlists(container);

	array_init(return_value);

	for (i=0; i<num_playlists; i++) {
		zval *spotifyplaylist;

		sp_playlist *playlist = sp_playlistcontainer_playlist(container, i);
		ALLOC_INIT_ZVAL(spotifyplaylist);
		object_init_ex(spotifyplaylist, spotifyplaylist_ce);
		SPOTIFY_METHOD2(SpotifyPlaylist, __construct, &temp, spotifyplaylist, object, playlist);

		add_next_index_zval(return_value, spotifyplaylist);
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
	PHP_ME(Spotify, getPlaylists,			NULL,	ZEND_ACC_PUBLIC)
    PHP_ME(Spotify, getStarredPlaylist,     NULL,   ZEND_ACC_PUBLIC)
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
