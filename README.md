# Higly experimental libspotify PHP lib

For a lot of reasons the function names is not the same and does not work the same as the c-lib. Don't whine about it.

As this is higly experimental you should make note of a few things:

  - The api might (and will) change.
  - Don't whine if stuff crashes (because it will), fix it instead.

## Installation:

  - Install libspotify.
  - add appkey.c (fetch your key from developer.spotify.com; this will improve in the future)
  - Run these commands: phpize; ./configure --enable-spotify; make && make install

## Usage:

    $spotify = new Spotify("/path/to/key.file", "username", "password");
    $starredPlaylist = $spotify->getStarredPlaylist();
	var_dump($starredPlaylist->getTracks());
