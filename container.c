#include "php_spotify.h"

static sp_playlistcontainer_callbacks plc_callbacks;
static int is_loaded;
static int num_playlists;
static sp_playlist **playlists;
static container_browse_data *browse_data;
static sp_playlistcontainer *pc;

static void playlistcontainer_loaded(sp_playlistcontainer *pc1, void *userdata) {
	zval ret, retval;
	int i;

	num_playlists = sp_playlistcontainer_num_playlists(pc);

	for (i=0; i<num_playlists; i++) {
		sp_playlist *playlist = sp_playlistcontainer_playlist(pc, i);
		if (!sp_playlist_is_loaded(playlist)) {
			return;
		}
	}

	playlists = (sp_playlist**)emalloc(sizeof(sp_playlist*) * num_playlists);

	for (i=0; i<num_playlists; i++) {
		playlists[i] = sp_playlistcontainer_playlist(pc, i);
	}

	sp_playlistcontainer_remove_callbacks(pc, &plc_callbacks, userdata);
	is_loaded = 1;
}

static sp_playlistcontainer_callbacks plc_callbacks = {
	NULL,
	NULL,
	NULL,
	playlistcontainer_loaded
};

void get_playlistcontainer_playlists(zval *return_value, container_browse_data *p, sp_playlistcontainer *pcont)
{
	int timeout = 0, i;
	zval tempretval;

	metadata_updated_fn = playlistcontainer_loaded;
	is_loaded = 0;
	browse_data = p;
	pc = pcont;

	sp_playlistcontainer_add_callbacks(pc, &plc_callbacks, NULL);

	while (!is_loaded || timeout == 0) {
		sp_session_process_events(p->session, &timeout);
	}

	metadata_updated_fn = NULL;

	array_init(return_value);

	for (i=0; i<num_playlists; i++) {
		zval *playlist;
		ALLOC_INIT_ZVAL(playlist);
		object_init_ex(playlist, spotifyplaylist_ce);
		SPOTIFY_METHOD2(SpotifyPlaylist, __construct, &tempretval, playlist, p->obj, playlists[i]);
		add_next_index_zval(return_value, playlist);
	}

//	playlists = NULL;
	num_playlists = 0;
}
