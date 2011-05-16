# Higly experimental libspotify PHP lib

For a lot of reasons the function names is not the same and does not work the same as the c-lib. Don't whine about it.

As this is higly experimental you should make note of a few things:

  - The api might (and will) change.
  - Don't whine if stuff crashes (because it will), fix it instead.

## Installation:

  - Install the libspotify dev files and binary that fits your system.
  - Run these commands: phpize; ./configure --enable-spotify; make && make install

## Usage:

    $spotify = new Spotify("/path/to/key.file", "username", "password");
    
    // All tracks in the starred playlist
    $starredPlaylist = $spotify->getStarredPlaylist(); // returns SpotifyPlaylist
	var_dump($starredPlaylist->getTracks());
     
	// List all playlists
	$playlists = $spotify->getPlaylists(); // returns array of SpotifyPlaylist
	foreach ($playlists as $playlist) {
		printf("%s\n", $playlist->getName());
		// print_r($playlist->getTracks());
	}
