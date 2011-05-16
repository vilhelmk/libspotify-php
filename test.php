<?php

$s = new Spotify("key.file", "user", "pass");
$f = $s->getStarredPlaylist();
var_dump($f->getTracks());
