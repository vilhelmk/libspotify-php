<?php

$s = new Spotify("spotify_appkey.key", "user", "pass");
$f = $s->getStarredPlaylist();
var_dump($f->getTracks());
