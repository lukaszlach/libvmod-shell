<?php

while ($line = fgets(STDIN)) {
    $line = rtrim($line);
    eval("\$result = ($line);");
    // make sure $result is a single line
    $result = str_replace("\n", " ", $result);
    echo "$result\n";
    fflush(STDOUT);
}