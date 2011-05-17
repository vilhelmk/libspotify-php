<?php

$s = new Spotify("spotify_appkey.key", "user", "pass");

//$f = $s->getStarredPlaylist();
//var_dump($f->getTracks());

$playlists = $s->getPlaylists();
echo $playlists[7]->getName() . "\n";

//if (!$playlists[7]->rename("hove 2010")) {
//	printf("rename failed\n");
//}

$tracks = $playlists[7]->getTracks();
foreach ($tracks as $track) {
	printf("%s - %s\n", $track->getArtist()->getName(), $track->getName());
}

