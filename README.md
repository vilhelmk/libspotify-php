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
    
	$coolTrack = $spotify->getTrackByURI('spotify:track:6JEK0CvvjDjjMUBFoXShNZ');
     
	// List all playlists
	$playlists = $spotify->getPlaylists(); // returns array of SpotifyPlaylist
	foreach ($playlists as $playlist) {
		printf("%s (%d tracks, by %s)\n", $playlist, $playlist->getNumTracks(), %playlist->getOwner());

		foreach ($playlist->getTracks() as $track) {
			$duration = $track->getDuration();
			printf("  -> %s - %s [%02d:%02d]\n", $track->getArtist(), $track,
					$duration/60, $duration%60);
		}

		// and add a important piece of music
		$playlist->addTrack($coolTrack, 0 /*position*/);
	}

## Credits

Vilhelm K. Vardøy <vilhelmkv@gmail.com>
