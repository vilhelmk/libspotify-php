#include "php_spotify.h"

static int is_loaded;
static sp_albumbrowse *browse;

static void browse_album_complete(sp_albumbrowse *result, void *userdata)
{
	is_loaded = 1;
	browse = result;
}

void browse_album_get_tracks(zval *return_value, container_browse_data *p, sp_album *album)
{
	int i, timeout = 0, num_tracks;
	zval tempretval;
	
	sp_albumbrowse *tmpbrowse = sp_albumbrowse_create(p->session, album, browse_album_complete, NULL);

	while (!is_loaded || !sp_albumbrowse_is_loaded(tmpbrowse)) {
		sp_session_process_events(p->session, &timeout);
	}

	metadata_updated_fn = NULL;
	num_tracks = sp_albumbrowse_num_tracks(browse);

	array_init(return_value);

	for (i=0; i<num_tracks; i++) {
		zval *z_track;
		ALLOC_INIT_ZVAL(z_track);
		object_init_ex(z_track, spotifytrack_ce);
		SPOTIFY_METHOD2(SpotifyTrack, __construct, &tempretval, z_track, p->obj, sp_albumbrowse_track(browse, i));
		add_next_index_zval(return_value, z_track);
	}

	is_loaded = 0;
}
