# lukaszlach / libvmod-shell

![Version](https://img.shields.io/badge/version-1.0-lightgrey.svg?style=flat)
![Version](https://img.shields.io/badge/Varnish-5-blue.svg?style=flat)

Varnish Module (VMOD) for executing long-running shell commands with support of two-way communication and command interface based on one-line request and one-line response, similar to [redirectors and rewrite scripts](https://wiki.squid-cache.org/Features/Redirectors) available in Squid Cache. Although this is not production-ready, it is useful for internal services and provisioning of features you are going to move to VMOD later, as allows you to use **any programming language or sh scripting** and expose it's features to VCL.

## Synopsis

```vcl
import shell;

new OBJECT = shell.exec(STRING cmd)
STRING <obj>.cmd(STRING value)
BOOL   <obj>.write(STRING value)
STRING <obj>.read()
INT    <obj>.pid()
```

## Building

```bash
git clone https://github.com/lukaszlach/libvmod-shell.git libvmod-shell/
cd libvmod-shell/
./autogen.sh
./configure
make
make install
```

## Examples

```vcl
vcl 4.0;
import shell;

sub vcl_init {
    new php = shell.exec("php /srv/eval.php");
    new bc  = shell.exec("bc -q");
}

sub vcl_deliver {
    set resp.http.php-result-1 = php.cmd("'PHP '.phpversion()"); // PHP 5.6.33-0+deb8u1
    set resp.http.php-result-2 = php.cmd("gethostbyaddr('" + client.ip + "')"); // localhost
    set resp.http.bc-result    = bc.cmd("(2*3+4*5-6*7)/8"); // -2
}
```

PHP script under `/srv/eval.php` reads single line with command from VCL and executes `eval` on it, returning the result, making sure `eval` output is a single line as well:

```php
<?php

while ($line = fgets(STDIN)) {
    $line = rtrim($line);
    eval("\$result = ($line);");
    // make sure $result is a single line
    $result = str_replace("\n", " ", $result);
    echo "$result\n";
    fflush(STDOUT);
}
```

> **Notice**: Mind that `read()` and `cmd()` calls are blocking, if your script takes long time to respond it may lower down request handling overall performance. 

As `shell.exec()` runs all commands through `sh -c`, you can use all it's power as well:

```vcl
vcl 4.0;
import shell;

sub vcl_init {
    new md5 = shell.exec("while read -r line; do echo -n $line | md5sum | cut -d' ' -f1; done");
}

sub vcl_recv {
    set req.http.X-Local-Secret = md5.cmd("secr3t" + client.ip + req.http.Host);
    if (req.http.X-Local-Secret != req.http.X-Secret) {
        return synth(403, "Forbidden");
    }
}
```

Above VCL adds authorization based on `X-Secret` request headers, which is compared to in-VCL generated MD5 hash combined from a secret value, client IP and `Host` header.

vmod-shell can also be used as a simple logging mechanism:

```vcl
vcl 4.0;
import shell;

sub vcl_init {
    new cat = shell.exec("cat > /tmp/vcl.log");
}

sub vcl_deliver {
    if (resp.status != 200) {
        cat.write("[" + now + "][HTTP " + resp.status + "] " + req.http.Host + req.url);
    }
}
```

Above VCL logs all non-200 responses to `/tmp/vcl.log`:

```
[Sun, 14 Jan 2018 11:52:28 GMT][HTTP 404] vmod.shell.com/image.jpg
[Sun, 14 Jan 2018 11:52:28 GMT][HTTP 403] vmod.shell.com/get.php
```

## Licence

MIT License

Copyright (c) 2018 Łukasz Lach <llach@llach.pl>

Portions Copyright (c) 2001 Hamid Alipour, Codingrecipes

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.