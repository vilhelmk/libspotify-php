<?php

spotify_init("user", "pass");

$data = spotify_get_starred_playlist();
var_dump($data);

if (spotify_destroy()) {
	printf("logout out\n");
} else {
	printf("no session\n");
}
