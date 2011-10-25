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

zend_class_entry *spotifyuser_ce;

PHP_METHOD(SpotifyUser, __construct)
{
	zval *object = getThis();
	zval *parent;
	sp_user *user;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oz", &parent, spotify_ce, &user) == FAILURE) {
		return;
	}

	spotify_object *p = (spotify_object*)zend_object_store_get_object((parent) TSRMLS_CC);
	spotifyuser_object *obj = (spotifyuser_object*)zend_object_store_get_object(object TSRMLS_CC);
	obj->session = p->session;
	obj->user = user;

	zend_update_property(spotifyuser_ce, getThis(), "spotify", strlen("spotify"), parent TSRMLS_CC);

	sp_user_add_ref(obj->user);
}

PHP_METHOD(SpotifyUser, __destruct)
{
	spotifyuser_object *p = (spotifyuser_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	sp_user_release(p->user);
}

PHP_METHOD(SpotifyUser, getName)
{
	zval *object = getThis();
	spotifyuser_object *p = (spotifyuser_object*)zend_object_store_get_object(object TSRMLS_CC);

	RETURN_STRING(sp_user_display_name(p->user), 1);
}

PHP_METHOD(SpotifyUser, getURI)
{
	char uri[256];
	spotifyuser_object *p = (spotifyuser_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	sp_link *link = sp_link_create_from_user(p->user);
	sp_link_as_string(link, uri, 256);
	sp_link_release(link);

	RETURN_STRING(uri, 1);
}

PHP_METHOD(SpotifyUser, getCanonicalName)
{
	spotifyuser_object *p = (spotifyuser_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	const char *name = sp_user_canonical_name(p->user);
	if (!name) {
		RETURN_FALSE;
	}
	RETURN_STRING(name, 1);
}

PHP_METHOD(SpotifyUser, getFullName)
{
	spotifyuser_object *p = (spotifyuser_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	const char *name = sp_user_full_name(p->user);
	if (!name) {
		RETURN_FALSE;
	}
	RETURN_STRING(name, 1);
}

PHP_METHOD(SpotifyUser, __toString)
{
	spotifyuser_object *p = (spotifyuser_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_STRING(sp_user_display_name(p->user), 1);
}

function_entry spotifyuser_methods[] = {
	PHP_ME(SpotifyUser, __construct,		NULL,	ZEND_ACC_PRIVATE|ZEND_ACC_CTOR)
	PHP_ME(SpotifyUser, __destruct,			NULL,	ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(SpotifyUser, getName,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyUser, getURI,				NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyUser, getCanonicalName,	NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyUser, getFullName,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(SpotifyUser, __toString,		NULL,	ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

void spotifyuser_free_storage(void *object TSRMLS_DC)
{
	spotifyuser_object *obj = (spotifyuser_object*)object;
	zend_hash_destroy(obj->std.properties);
	FREE_HASHTABLE(obj->std.properties);
	efree(obj);
}

zend_object_value spotifyuser_create_handler(zend_class_entry *type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;

	spotifyuser_object *obj = (spotifyuser_object *)emalloc(sizeof(spotifyuser_object));
	memset(obj, 0, sizeof(spotifyuser_object));

	zend_object_std_init(&obj->std, type TSRMLS_CC);
    zend_hash_copy(obj->std.properties, &type->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));

    retval.handle = zend_objects_store_put(obj, NULL,
        spotifyuser_free_storage, NULL TSRMLS_CC);
    retval.handlers = &spotify_object_handlers;

    return retval;
}

void spotify_init_user(TSRMLS_D)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "SpotifyUser", spotifyuser_methods);
	spotifyuser_ce = zend_register_internal_class(&ce TSRMLS_CC);
	spotifyuser_ce->create_object = spotifyuser_create_handler;

	zend_declare_property_null(spotifyuser_ce, "spotify", strlen("spotify"), ZEND_ACC_PROTECTED TSRMLS_CC);
}
