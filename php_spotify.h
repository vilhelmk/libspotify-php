#ifndef PHP_SPOTIFY_H
#define PHP_SPOTIFY_H 1

#define PHP_SPOTIFY_VERSION "0.0.7"
#define PHP_SPOTIFY_EXTNAME "spotify"

#include <stdlib.h>
#include <stdio.h>
#include <libspotify/api.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif 

#include "php.h"

typedef struct {
    zend_object std;
    sp_session *session;
    int is_logged_in;
    int timeout;
	char *key_data;
	sp_playlistcontainer *playlistcontainer;
} spotify_object;

typedef struct {
	zend_object std;
	sp_session *session;
	sp_playlist *playlist;
} spotifyplaylist_object;

typedef struct {
	zend_object std;
	sp_session *session;
	sp_track *track;
} spotifytrack_object;

typedef struct {
	zend_object std;
	sp_session *session;
	sp_artist *artist;
} spotifyartist_object;

typedef struct {
	zend_object std;
	sp_session *session;
	sp_album *album;
	sp_albumbrowse *albumbrowse;
} spotifyalbum_object;

typedef struct {
	zend_object std;
	sp_session *session;
	sp_user *user;
} spotifyuser_object;

typedef struct {
	sp_session *session;
	zval *obj;
} container_browse_data;

extern zend_module_entry spotify_module_entry;
#define phpext_spotify_ptr &spotify_module_entry

extern zend_object_handlers spotify_object_handlers;

extern void spotify_init_playlist(TSRMLS_D);

extern zend_class_entry *spotify_ce;
extern zend_class_entry *spotifyplaylist_ce;
extern zend_class_entry *spotifytrack_ce;
extern zend_class_entry *spotifyartist_ce;
extern zend_class_entry *spotifyalbum_ce;
extern zend_class_entry *spotifyuser_ce;

extern void get_playlistcontainer_playlists(zval *return_value, container_browse_data *p, sp_playlistcontainer *pc); 

#if ZEND_MODULE_API_NO >= 20090115
# define PUSH_PARAM(arg) zend_vm_stack_push(arg TSRMLS_CC)
# define POP_PARAM() (void)zend_vm_stack_pop(TSRMLS_C)
# define PUSH_EO_PARAM()
# define POP_EO_PARAM()
#else
# define PUSH_PARAM(arg) zend_ptr_stack_push(&EG(argument_stack), arg)
# define POP_PARAM() (void)zend_ptr_stack_pop(&EG(argument_stack))
# define PUSH_EO_PARAM() zend_ptr_stack_push(&EG(argument_stack), NULL)
# define POP_EO_PARAM() (void)zend_ptr_stack_pop(&EG(argument_stack))
#endif

#if ZEND_MODULE_API_NO >= 20060613
// normal, nice method
#define SPOTIFY_METHOD_BASE(classname, name) zim_##classname##_##name
#else
// gah!  wtf, php 5.1?
#define SPOTIFY_METHOD_BASE(classname, name) zif_##classname##_##name
#endif /* ZEND_MODULE_API_NO >= 20060613 */

#define SPOTIFY_METHOD_HELPER(classname, name, retval, thisptr, num, param) \
  PUSH_PARAM(param); PUSH_PARAM((void*)num);                \
  PUSH_EO_PARAM();                          \
  SPOTIFY_METHOD_BASE(classname, name)(num, retval, NULL, thisptr, 0 TSRMLS_CC); \
  POP_EO_PARAM();           \
  POP_PARAM(); POP_PARAM();

/* push parameters, call function, pop parameters */
#define SPOTIFY_METHOD(classname, name, retval, thisptr)          \
  SPOTIFY_METHOD_BASE(classname, name)(0, retval, NULL, thisptr, 0 TSRMLS_CC);

#define SPOTIFY_METHOD1(classname, name, retval, thisptr, param1)     \
  SPOTIFY_METHOD_HELPER(classname, name, retval, thisptr, 1, param1);

#define SPOTIFY_METHOD2(classname, name, retval, thisptr, param1, param2) \
  PUSH_PARAM(param1);                           \
  SPOTIFY_METHOD_HELPER(classname, name, retval, thisptr, 2, param2); \
  POP_PARAM();

#define SPOTIFY_METHOD3(classname, name, retval, thisptr, param1, param2, param3) \
  PUSH_PARAM(param1); PUSH_PARAM(param2);               \
  SPOTIFY_METHOD_HELPER(classname, name, retval, thisptr, 3, param3); \
  POP_PARAM(); POP_PARAM();

#endif
