# Experimental libspotify PHP lib

## Important notes

  - Read the [libspotify license][1], there are some important things to note; for example you CAN NOT "attempt to embed or integrate the API into any website or otherwise allow access to the Service via the web rather than via the Application.".
  - The behaviour and API might change; this library is not production ready.

[1]: http://developer.spotify.com/en/libspotify/terms-of-use/ "libspotify terms of use"

## Installation

  - Install the libspotify dev files and binary that fits your system, get them from the [Spotify developer website][2].
  - Run these commands: phpize; ./configure --enable-spotify; make && make install

[2]: http://developer.spotify.com/

## Usage

    $spotify = new Spotify("/path/to/key.file", "username", "password");
    
    // All tracks in the starred playlist
    $starredPlaylist = $spotify->getStarredPlaylist(); // returns SpotifyPlaylist
    foreach ($starredPlaylist as $track) {
        printf("%s - %s\n", $track->getArtist(), $track);
    }
     
	// List all playlists
	$playlists = $spotify->getPlaylists(); // returns array of SpotifyPlaylist
	foreach ($playlists as $playlist) {
		printf("%s\n", $playlist->getName());
		// print_r($playlist->getTracks());
	}
