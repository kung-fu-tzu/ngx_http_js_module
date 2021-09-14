Please, consider using an upcomming [nginScript](https://www.nginx.com/resources/wiki/nginScript/) official module with a specialized JS virtual machine and a well known nginx stability and memory footprint.

**THIS PROJECT IS OBSOLETE**, sorry bro.

About
=====

This module main goal is to make nginx as scriptable as it could to be.

Every scripting language known to me has its own standard library. This is a very good opportunity for a programmer that uses a language to write standard (i.e. multithreaded, synchronous, apache-based) web applications. But it is no good for the embedding into an async application like nginx. For example the [node.js][] developers have to re-implement almost all existing libraries from scratch (and they do so). If you don't fully understand the difference between the applications based on sync and async principles please read about [asynchronous I/O][asynchronous] first.

And SpiderMonkey doesn't have any library. And nobody expect to have it in this language. More of that, the browser interface is also async and event-based. So we can relax and just script nginx instead of making another Web 2.0 framework.

In other words the nxg_http_js_module tries to reflect the nginx functionality in JS.

You can touch the ground running if you have ever seen [ngx_http_perl_module][] or [mod_perl][] before.

[node.js]: http://nodejs.org/
[asynchronous]: http://en.wikipedia.org/wiki/Asynchronous_I/O
[ngx_http_perl_module]: http://wiki.nginx.org/HttpPerlModule
[mod_perl]: http://nginx.org/en/docs/http/ngx_http_perl_module.html



Features
=============

* full port of `ngx_http_perl_module`;
* support for native nginx sub-requests with JS callback for the the response and its body;
* cosy headers management (via `Nginx.HeadersIn` and `Nginx.HeaderOut` classes);
* fast native cookies support with the same code nginx uses itself (via `Nginx.HeadersIn.cookies`);
* environment variables handling with the code taken from Mozilla's JS interpreter;
* lots of useful properties of nginx object (`Nginx.time`, `Nginx.prefix`, `Nginx.version`, etc);
* plain `require()` function that finds JS files walking through `Nginx.prefix` + `JSLIB` environment variable (like `RUBYLIB` and `PERL5LIB`);
* initial support for files via `Nginx.File` (create/delete/rename, simple open/close/read/write, all in UTF-8);
* handy tests suit written in plain JavaScript (using asynchronous test framework from [liby.js][])

The code uses `ngx_assert()` and `ngx_log_debug()` almost everywhere, so the debugging must not be a pain.

[liby.js]: https://github.com/peter-leonov/liby



Installation
============

Installation is straightforward. Just add this module with a familiar `--add-module` configuration directive in [nginx wiki][compiling modules].

[compiling modules]: http://wiki.nginx.org/Nginx3rdPartyModules#Compiling_third_party_modules


With a fresh SpiderMonkey build from sources the ngx_http_js_module module was successfully tested on:

* Ubuntu 8.04.3 32-bit (2.6.18-virtuozzo; 2.6.24-23-openvz in VirtualBox)
* Ubuntu 10.04 32-bit (VirtualBox 3.2.4)
* FreeBSD 8.0 32-bit (Parallels Desktop 5.0 Mac)
* FreeBSD 7.3 32-bit (VirtualBox 3.2.4)
* OpenBSD 4.7 64-bit (VirtualBox 3.2.4)
* Debian 5.0 32-bit (VirtualBox 3.2.4)
* Debian 5.0 64-bit (VirtualBox 3.2.4)
* Debian 5.0 32-bit PowerPC (Mac mini G4)
* Mac OS 10.6.3 32-bit and 64-bit (Core Duo and Core 2 Duo iMacs)



Requirements
------------

* nginx versions: 0.8.11 (tested up to 0.8.54), 0.9.7, 1.0.1;
* SpiderMonkey 1.7.0;
* curl near 7.19 for automated testing.


### installing SpiderMonkey

This module requires the SpiderMonkey 1.7 (with the [JSAPI][] on 2010-03-26) being properly installed in your system.

Notes on SpiderMonkey support on different platforms (2010-05-27):

* Ubuntu has relatively bad support for SpiderMonkey at the moment: [1.5 and 1.8 versions][on ubuntu] only;
* Debian [has much greater][on debian] SpiderMonkey in it and may be useful;
* MacPorts is [no good][on mac];
* FreeBSD got [the same][on freebsd] as MacPorts.

[JSAPI]: https://developer.mozilla.org/en/SpiderMonkey/JSAPI_Reference
[on ubuntu]: http://packages.ubuntu.com/search?keywords=spidermonkey&searchon=names&suite=all&section=all
[on debian]: http://packages.debian.org/search?keywords=spidermonkey&searchon=names&suite=all&section=all
[on mac]: http://www.macports.org/ports.php?by=name&substr=spidermonkey
[on freebsd]: http://www.FreeBSD.org/cgi/ports.cgi?query=spidermonkey&stype=all


In short all those are suitable to run ngx_http_js_module, but those may lack a support for the JSON module or UTF-8 strings.

Any way we can always build SpiderMonkey ourselves from sources. [Firefox 3.6 sources][firefox sources] (or my [github mirror][spidermonkey mirror] of the `js/` sub-folder) ships with an independent SpiderMonkey source tree. This means we can build a SpiderMonkey library and install it with all the header files without even touching the Firefox source code. All we have to do is the following:

[firefox sources]: https://developer.mozilla.org/en/Download_Mozilla_Source_Code
[spidermonkey mirror]: http://github.com/kung-fu-tzu/spidermonkey

	cd firefox-sources/js/src
	./configure [--prefix=/usr/] [--disable-jit] [--disable-tests] [--enable-debug]
	make
	sudo make install

If you have a 64-bit Mac (even with only 32-bit kernel) use `--disable-jit` to be able to `make` SpiderMonkey. Use `--enable-debug` if you plan to develop a little ;)

On a clear FreeBSD 8.0 at least the following have to be installed from ports: gmake, perl, python, zip. Run `gmake` and `gmake install` after `./configure`.

This could create the following in `prefix`:

* bin/js-config
* include/js/*
* lib/libmozjs.(so|dylib)
* lib/libjs_static.a

ngx_http_js_module relies only on `libmozjs.(so|dylib)` library and on `include/js/*` headers.

### troubleshoot

If you get an error like the following:

	[error]: reserved slot index out of range at <no filename>:0
	[error]: Can`t JS_InitStandardClasses() at <no filename>:0
	[emerg]: global object initialization failed in /www/ngx_http_js_module/nginx.conf:24

it means you have a wrong `libjs` version installed. Try to uninstall it first (with kinda like `sudo apt-get remove libmozjs-dev`) and then install the latest version.


configure
---------

The JS module could be compiled as any other nginx module:

	./configure --add-module=/absolute/path/to/the/ngx_http_js_module/

If you have installed Spidermonkey at non-standard path, or nginx cannot automatically find the library, you should set some variables before running configure:

	export SPIDERMONKEY_INC=/path/to/spidermonkey/include       # allows config to find <jsapi.h>
	export SPIDERMONKEY_LIB=/path/to/spidermonkey/lib           # allows config to find libmozjs
	
	./configure ...

If you are on an ELF-based platform and do not want to bother with `LD_LIBRARY_PATH` please define `LD_RUN_PATH` to set [rpath][] like so:

	export LD_RUN_PATH=/path/to/spidermonkey/lib                # adds the lib path to the directories that nginx
	                                                            # will search to find libmozjs on Linux/Solaris
	
	./configure ...

[rpath]: http://www.eyrie.org/~eagle/notes/rpath.html

If you want to look into the guts and do something there, please configure like the following:

	HTTP_JS_COLOR=yes ./configure --with-debug --add-module=/absolute/path/to/the/ngx_http_js_module/

in which:

* `--with-debug` flag compiles nginx with all the debug features ([debugging log][] for example);
* `HTTP_JS_COLOR=yes` environment variable enables the colored logging for the JS module.

[debugging log]: http://nginx.org/en/docs/debugging_log.html

If you want to have both the vanilla and JS-flavored nginx use `--prefix=`

	./configure --prefix=/usr/local/nginx-js/ --add-module=/absolute/path/to/the/ngx_http_js_module/


make
----

Then run make as you usual do:

	make

After the make process has successfully completed you may run some simple tests for JS module (before the actual installation) with this:

	make test-js

This will run nginx on 19090 port and issue some requests to it with curl.

If you get the following error:

	.../objs/nginx: error while loading shared libraries: libmozjs.so: cannot open shared object file: No such file or directory

please, try to set `LD_LIBRARY_PATH` environment variable like so:

	export LD_LIBRARY_PATH="/usr/local/lib/"

Or try to define the `LD_RUN_PATH` as described above and re-compile.

And then you may run `make test-js` again.

`make test-js` (calling `run-tests` from the module sources) tries to set the `LD_LIBRARY_PATH` environment variable for you.


install
-------

Then you may peacefully run:
make install



Configuration
=============

This module tries to mimic the perl modules in most cases.


Simple handler example
----------------------

	location /demo/random {
		js 'function (r) { r.sendString(Math.random()); return Nginx.OK }';
	}

curl http://localhost/demo/random

	0.540526149221515


Full handler example
--------------------
	http {
		js_load "js/demos/handler.js";
		
		server {
			location = /demo/handler {
				js  Hello.handler;
			}
		}
	}

in handler.js:

	Hello = {
		handler: function (r) {
			r.sendHttpHeader('text/html')
			
			if (r.headerOnly) {
				return Nginx.OK
			}
			
			r.print('hello!\n<br/>')
			
			if (Nginx.File.exists(r.filename)) {
				r.print(' exists!\n')
			}
			
			r.sendSpecial(Nginx.HTTP_LAST)
			
			return Nginx.OK
		}
	}

curl -I http://localhost/demo/handler

	HTTP/1.1 200 OK
	Server: nginx/0.8.37
	Date: Sun, 23 May 2010 15:18:26 GMT
	Content-Type: text/html
	Connection: keep-alive
	

curl http://localhost/demo/handler

	hello!
	<br/>


Variable example
----------------

	js_set  $msie6  '
	
		function (r) {
			var ua = r.headersIn["User-Agent"]
			
			if (/Opera/.test(ua)) {
				return ""
			}
			
			if (/ MSIE [6-9]\.\d+/.test(ua)) {
				return "1"
			}
			
			return "";
		}
	
	';
	
	location = /demo/msie6 {
		if ($msie6) {
			return 404;
		}
		
		rewrite ^ /;
	}

curl http://localhost/demo/msie6

	<!DOCTYPE html>
	<html>
	<head>
	<title>Welcome to nginx!</title>
	</head>
	<body>
	<h1>Welcome to nginx!</h1>
	</body>
	</html>


Access example
--------------

	location = /demo/secret-data {
		js_access  '
			
			function (r) {
				if (r.headersIn.cookies.goodUser)
					return Nginx.OK
				
				return Nginx.HTTP_FORBIDDEN
			}
			
		';
	}

curl http://localhost/demo/secret-data

	<html>
	<head><title>403 Forbidden</title></head>
	<body bgcolor="white">
	<center><h1>403 Forbidden</h1></center>
	<hr><center>nginx/0.7.62</center>
	</body>
	</html>



Directives
==========


js
---

**syntax**: js object.property | 'function (r) { ... }' | any other JS expression that returns a Function  
**default**: none  
**context**: location

Defines a JS handler for current location. The request object is passed to a function as the first argument.


js_access
---

**syntax**: js_access object.property | 'function (r) { ... }' | any other JS expression that returns a Function  
**default**: none  
**context**: location

Defines a JS handler for the current location at an access phase. The request object is passed to a function as the first argument.
Return `Nginx.OK` to pass the request further or return `Nginx.HTTP_FORBIDDEN` to deny an access to the location.


js_load
-------

**syntax**: js_load path  
**default**: none  
**context**: http

Simply loads and executes the file specified by path. A script from the file will be compiled and run in the global scope.


js_set
------

**syntax**: js_set $variable object.property | 'function (r) { ... }' | any other JS expression that returns a Function  
**default**: none  
**context**: http

Defines a variable read handler (write handler is planned). The request object is passed to a function as the first argument.



Request object
==============

This object of class Nginx.Request is the most important one. It let us do almost everything we could expect in the HTTP server: read and write headers of both a request and a response, check and then get or reject a request body, send data back to the client with over keep-alive connection, set a timer on a request,  redirect a request and so on.


Note about GC
-------------

Every time a request reaches a JS handler a native request structure will be wrapped in an Nginx.Request instance and passed to the JS code. Then some work is doing in JS. After that nginx terminates the native request structure with all its data (headers, variables, request body, etc.), and the module will mark the wrapper object (and all satellite objects like Nginx.HeadersIn and Nginx.Cookies) as inactive. Every call to such a deactivated wrapper will cause an exception.


TODO
----

There are some thing that must be implemented to get the full and intuitive request wrapper:

* [safe properties enumeration][#27]: now we may somehow interact with a request enumerating its properties;
* [send “last chunk” on the request completeness][#28]: we have to send the HTTP_LAST_ manually for now;
* [binary data API][#29] is needed: today we can response with a plain text only;
* [sane return values and exceptions][#31]: for now the returned `Nginx.OK` is a most common sign of success.

[#27]: http://github.com/kung-fu-tzu/ngx_http_js_module/issues/issue/27
[#28]: http://github.com/kung-fu-tzu/ngx_http_js_module/issues/issue/28
[#29]: http://github.com/kung-fu-tzu/ngx_http_js_module/issues/issue/29
[#31]: http://github.com/kung-fu-tzu/ngx_http_js_module/issues/issue/31


Properties
----------


### uri

Tell us the uri of a request.


### method

Gives us a request method: `GET`, `POST`, etc.


### filename

Maps the uri to the filename respecting the current nginx configuration. Returned path may (and always will) point to an inexistent file.


### remoteAddr

Return a string representation (i.e. `"81.19.68.137"`) of the client IP.


### headersIn

Returns a wrapper object (of class Nginx.HeadersIn) for the input headers structure. This wrapper helps us to get (and some times set) the request headers.

	r.print(r.headersIn['User-Agent'])


### headersOut

Return a wrapper object (of class Nginx.HeadersOut) for the output headers structure. This wrapper gives us a chance to set (and get, for example in the header/body filter) the output headers.

	r.headersOut['X-Powered-By'] = 'MegaGigaAwesome CMS'


### args

Stores the arguments (nginx handles request URI and arguments separately).


### headerOnly

Indicates does the client expect a body in the response or not. It will be `true` for a `HEAD` request for example.


### bodyFilename

Tell us in which file nginx have the request body stored. We have to `getBody()` before use this property to be sure that nginx has the body received already. nginx could store the request body in a temporary file if it does not fit in memory or if nginx was configured to so by the [client_body_in_file_only][] directive.

[client_body_in_file_only]: http://wiki.nginx.org/NginxHttpCoreModule#client_body_in_file_only


### hasBody

Indicates the presence of the request body. It is a sister of the `headerOnly` property but in reflection around the wire.


### body

If the request body fits in memory (can be tweaked with [client_body_buffer_size][]) we can get it with the `body` property. Otherwise use the `bodyFilename` property.

[client_body_buffer_size]: http://wiki.nginx.org/NginxHttpCoreModule#client_body_buffer_size


### variables

Gives an access to the request variables. It return a wrapper object (of type Nginx.Variables) which can read and write all the variables the rewrite module can set. Our own variables defined with `js_set` are also available through the `variables` property.

	if (r.variables.ancient_browser)
		return Nginx.HTTP_FORBIDDEN


### allowRanges


Allows and disallows range request.

	if (r.headersIn.cookies.goodUser)
		r.allowRanges = true


### internal

Indicates if the request is “internal” in terms of nginx. We can switch it if we want.

	// do some security checks
	r.internal = true


Methods
-------


### log(message [, level])

Logs a `message` through the nginx built-in log mechanism. Does the same as `Nginx.logError()` but with the request log context preserved.

If `level` is omitted `Nginx.LOG_INFO` is used.


### sendHttpHeader(contentType)

Sends the header. In addition this method sets the response status to `200` if not it set, and sets the `headersOut["Content-Type"]` to the value of argument `contentType`.

On success returns `Nginx.OK`.

	r.sendHttpHeader('text/html; charset=utf-8')


### print(string)

Sends data of a UTF-8 encoded `string`. This method is able to send only text, not binary data. This means we can not send a string containing the binary zero symbol (i.e. `"\0"`). Also this method could not send an empty string, it just returns without doing any work.

On success returns `Nginx.OK`.

	r.print('Hello, World!')


### flush()

This method is pretty simple, it just flushes the request output chain. In terms of nginx it sends a special buffer with a `flush` field set on.

On success returns `Nginx.OK` (the same as `print()`).

	// sending the response body in two chunks
	r.print('Hello')
	r.flush()
	r.print(', World!')


### sendString(string [, contentType])

This is a combo-method. It does many thing at once: calculates the size in bytes of the `string`, sets the `Content-Length` header, sets an optional `Content-Type` header, then sends the response headers (like `sendHttpHeader()` does) and sends the `string` (like `print()` does). Overcomplicated? Yes, but all this must be done on the nginx side when the client doesn't support HTTP/1.1. The main thing in all this is that the JS can not calculate a real bytes count will be send on the wire. JS cal only tell us a count of UTF-16 characters in a string, while we need the count of bytes. In short, just do not use this method if you do not really need it ;)

On success returns `Nginx.OK`. Due to the overcomplicating, this method may return an error and throw an exception.

	var body = ''
	// ...
	body += 'Hello'
	// ...
	body += ', World!'
	
	r.sendString(body, 'text/plain')


### sendSpecial(number)

Send a “special” value through the request. The only tested special value is `Nginx.HTTP_LAST` (the `NGX_HTTP_LAST` in terms of nginx).

On success returns `Nginx.OK`.

	r.puts('The reques is done.')
	r.sendSpecial(Nginx.HTTP_LAST)

AFAIK, sending the `Nginx.HTTP_LAST` signals nginx to send a last chunk in a chunked response, otherwise a connection will hang.


### getBody(callback)

As far as nginx is asynchronous by nature, we can't get the response body at once. We have to wait for it to arrive on the wire, go through the OS kernel buffers and only then we can catch the body data. This method asks nginx to wait for all this things to happen and then call the `callback`. In the callback it is guarantied that the request body related things (`r.body` and `r.bodyFilename`) will be useful to get the data of the request body.

On success returns `Nginx.OK` if the body is ready ATM and `Nginx.AGAIN` if the network could be touched before the body is ready.

The following example shows how to get all the request body in memory.

In the nginx.conf:

	location /ajax {
		client_max_body_size 512K;
		client_body_buffer_size 512K;
		
		js handler;
	}

The handler:

	function handler (r) {
		r.sendHttpHeader('text/plain; charset=utf-8')
		
		r.print('waiting for body')
		r.flush()
		
		function onbody ()
		{
			r.print('got a body: ' + r.body)
			r.sendSpecial(Nginx.HTTP_LAST)
		}
		
		r.getBody(body)
		
		return Nginx.OK
	}

As of 0.2.16 we can return a HTTP status code just from the callback. If not `Nginx.OK` is used.


### discardBody()

This method ask nginx to discard body with all the tenderness it has. It is not a trivial thing ignoring request body, but we can relax relying on nginx wisdom ;)

On success returns `Nginx.OK`.

	function handler (r) {
		r.discardBody()
		
		r.sendHttpHeader('text/plain; charset=utf-8')
		r.puts('the request body is not good for me')
		r.sendSpecial(Nginx.HTTP_LAST)
		
		return Nginx.OK
	}


### sendfile(path, offset, bytes)

This method helps to add the content of a file to the request body. We can send more then one file and even the same file more then once. We can set the frame in file to be sent with the `offset` and `bytes` arguments. The two arguments are optional. If only `offset` is specified, nginx will send the file from the `offset` byte from the begin of the file and till the end of the file. If neither `offset` nor `byte` was specified, nginx will send the entire file to the client.

As far as `sendfile()` adds just a file buf into the output chain, we can send files mixed with strings, specials and flushes.

On success returns `Nginx.OK` (the same as `print()`).

In file.txt:

	send me please!

in handler:

	r.sendfile('file.txt')
	r.print('\ncan be split into: ')
	r.flush()
	
	r.sendfile('file.txt', 0, 4)
	r.print(', ')
	r.sendfile('file.txt', 5, 2)
	r.print(' and ')
	r.sendfile('file.txt', 8, 7)
	
	r.sendSpecial(Nginx.HTTP_LAST)

the result:

	send me please!
	can be split into: send, me, please!


### redirect(uri, args)

Yes, it's kinda like rewrite :)

Note that nginx stores uri (is used while finding a location) and arguments (the data after the `?` character) separately. And to avoid additional uri parsing we can specify the `uri` and `args` arguments for `redirect()`.

On success returns `Nginx.OK`.

	var cookies = r.headersIn.cookies
	
	if (Nginx.md5(cookies.username + ':a secret') != cookies.signature)
	{
		r.redirect('/login', 'from=' + r.uri)
		return Nginx.OK
	}


### setTimer(callback, timeout)

This method creates a nginx internal timer associated with the request. It means that the timer will be automatically cleared on the request destruction. And that the only one timer may be set at once. To be able to set more that one timer per request, see the the timers.js in `js/` folder of the module.

The `callback` must be a function (a closure) and `timeout` is specified in milliseconds.

On success returns `Nginx.OK`.

Example of a cascade timer setting:

	function handler (r)
	{
		r.sendHttpHeader('text/html')
		
		var count = 10
		
		function sayHello ()
		{
			r.print('Hello # ' + count + '!\n')
			r.flush()
			
			if (--count > 0)
				r.setTimer(sayHello, 250)
			else
				r.sendSpecial(Nginx.HTTP_LAST)
		}
		
		sayHello()
		
		return Nginx.OK
	}

produces (with 250ms delay for each line):

	Hello # 10!
	Hello # 9!
	Hello # 8!
	Hello # 7!
	Hello # 6!
	Hello # 5!
	Hello # 4!
	Hello # 3!
	Hello # 2!
	Hello # 1!

In this example nginx will wait till all the `sayHello()`'s will fire and only after that finalize the request.


### clearTimer()

Just clears the timer if set. There is no arguments as far as the only one timer can be set per request (without a third-party library as timers.js).


### subrequest(uri, callback)

Sub-requests in nginx are by no means an AJAX requests. Sub-requests are quite useless at the point as far as they share the same headers and variables set in the main request, in the same time requests are processed in parallel. In short use this if and only if you know what you are doing ;)

This methods creates a sub-request (a dependent request with shared almost everything) and directs it to the `uri`. After the sub-request is complete (always asynchronous) nginx will invoke the `callback`.

On the successful sub-request creation the method return a sub-request object (actually a `Nginx.Request` instance). It looks like a general request but it is not, be careful with it as far as we do not know all the cases in which it can crash ;)

The callback itself is very interesting part:

	function callback (sr, body, rc) { /* ... */ }

The callback takes three parameters: sub-request (`sr`) in which context it was invoked, the response body data (`body`) and the sub-request “result code” (`rc`). The last one is an internal nginx code and now is used for test purposes only.

This is a good idea to issue a sub-request through the `proxy_passs` even to the nginx itself. This trick helps to deal with other modules in cost of establishing a loopback connection from nginx to nginx itself.

In nginx.conf:

	location /nginx.org
	{
		proxy_pass http://nginx.org/;
		proxy_buffers 3 512K;
		proxy_buffer_size 512K;
	}

the handler:

	function handler (r)
	{
		var uri = '/nginx.org'
		
		r.sendHttpHeader('text/plain; charset=utf-8')
		r.print('accessing ' + uri + '\n')
		r.flush()
		
		function callback (sr, body, rc)
		{
			r.print('got body with length ' + body.length + '\n')
			r.sendSpecial(Nginx.HTTP_LAST)
		}
		
		r.subrequest(uri, callback)
	}



Nginx
=====

This is all-in-one variable. It collects those things doesn't fit anywhere else. `Nginx.md5()` is a good example of that kind of things.


Properties
----------


### time

Every call to `new Date()` issues a syscall and gives the most current date time available. nginx in its turn caches time for a performance reasons. It stores it in the every event process “cycle” so the time can be obtained without a syscall. `Nginx.time` takes that cached time for us.

Return a number with milliseconds from the epoch (like `+new Date()` does).


### prefix

Gives a string with a current nginx prefix. Prefix is the path to the “home directory” of nginx configuration. It may be set with different ways see the [configuration time prefix][] and [`-p` option][p option]. Note that some versions of nginx may take the prefix from the configuration file path.

[configuration time prefix]: http://wiki.nginx.org/NginxInstallOptions
[p option]: http://wiki.nginx.org/NginxCommandLine#Options


### pid

To pid or not to pid? It is just a process id. AFAIK is also cached as a time.


### version

nginx version as a number. `8038` for example.

	if (Nginx.version >= 8038)
		// do sime crazy stuff


### VERSION

nginx version as a string. `""0.8.38""` for example.

	r.print('we are using nginx ' + Nginx.VERSION + ', and you?')


### log levels

Reflects `NGX_LOG_*` constants:

* LOG_STDERR
* LOG_EMERG
* LOG_ALERT
* LOG_CRIT
* LOG_ERR
* LOG_WARN
* LOG_NOTICE
* LOG_INFO
* LOG_DEBUG

Log at debug level:

	Nginx.logError(this.LOG_WARN, 'do not forget to buy some milk!')

See more about [error_log][].

[error_log]: http://wiki.nginx.org/NginxHttpMainModule#error_log


### HTTP response codes

Reflects `NGX_HTTP_*` constants:

* HTTP_OK
* HTTP_CREATED
* HTTP_ACCEPTED
* HTTP_NO_CONTENT
* HTTP_PARTIAL_CONTENT
* HTTP_SPECIAL_RESPONSE
* HTTP_MOVED_PERMANENTLY
* HTTP_MOVED_TEMPORARILY
* HTTP_NOT_MODIFIED
* HTTP_BAD_REQUEST
* HTTP_UNAUTHORIZED
* HTTP_FORBIDDEN
* HTTP_NOT_FOUND
* HTTP_NOT_ALLOWED
* HTTP_REQUEST_TIME_OUT
* HTTP_CONFLICT
* HTTP_LENGTH_REQUIRED
* HTTP_PRECONDITION_FAILED
* HTTP_REQUEST_ENTITY_TOO_LARGE
* HTTP_REQUEST_URI_TOO_LARGE
* HTTP_UNSUPPORTED_MEDIA_TYPE
* HTTP_RANGE_NOT_SATISFIABLE
* HTTP_CLOSE
* HTTP_OWN_CODES
* HTTPS_CERT_ERROR
* HTTPS_NO_CERT
* HTTP_TO_HTTPS
* HTTP_CLIENT_CLOSED_REQUEST
* HTTP_INTERNAL_SERVER_ERROR
* HTTP_NOT_IMPLEMENTED
* HTTP_BAD_GATEWAY
* HTTP_SERVICE_UNAVAILABLE
* HTTP_GATEWAY_TIME_OUT
* HTTP_INSUFFICIENT_STORAGE

Indicate a bad request:

	function handler (r) {
		if (r.args.length > 100)
			return Nginx.HTTP_BAD_REQUEST
	}


### specials

Reflects some `NGX_HTTP_*` flags:

* HTTP_LAST
* HTTP_FLUSH

For example send the last chunk:

	function handler (r) {
		r.sendHttpHeader('text/plain')
		
		r.print('Hello, World!')
		r.sendSpecial(Nginx.HTTP_LAST)
		
		return Nginx.OK
	}


### nginx internal response codes

Reflects some `NGX_*` constants:

* OK
* ERROR
* AGAIN
* BUSY
* DONE
* DECLINED
* ABORT

Tell a caller that all is OK:

	function handler (r) {
		r.sendString('All is OK!')
		return Nginx.OK
	}


Methods
-------


### md5(str)

Calculates a MD5 sum of the given string `str`. As far as nobody expect it to be calculated on the raw UTF-16 bytes vector of the string, `md5()` first converts the string to a UTF-8 representation and then does the main work. The can be some issues with this method applied on a real unicode string. If you encounter some call me anytime ;)


### logError(level, message)

Writes a `message` to the global nginx error log at the `level`.

	Nginx.logError(this.LOG_EMERG, 'forgot to buy the milk!!!')

See more about [error_log][].


Nginx.HeadersIn
===============

`Nginx.HeadersIn` with `Nginx.HeadersOut` do all the job with headers. It is useful to learn [how does headers work][] under the hood.

[how does headers work]: http://wiki.nginx.org/HeadersManagement

We can't create `Nginx.HeadersIn` instance directly but only with `r.headersIn`. That's because of the headers instance have to know to which request object (and `headers_in` structure in terms of nginx) it belongs.


Properties
----------

All the properties we set or get with `Nginx.headersIn` instance will be map to the native request headers. So we can not set any property on the instance without nginx knowing about it. Think of it as a hash implementation with some smart logic behind it.

	r.headersIn['Content-Length']

There are a few special read-only properties described below.


### cookies

It is another wrapper to the request headers just like `headersIn` itself, but it focuses on the request cookies directly. We can read the cookie value by its key like so:

	r.headersIn.cookies['session']

or like so:

	var cookies = r.headersIn.cookies
	cookies.session

the result could be the same.

See the `Nginx.Cookies` class description for details (coming soon).


### $contentLength

This property is used for test purposes only. It reflects the multilevel cache architecture of headers in nginx.

We can set the `Content-Length` header with this:

	r.headersIn['Content-Length] = '555'

and then check:

	r.headersIn.$contentLength === '555'

This property is just a reader for `r->headers_in.content_length`.


### $contentLengthN

Like the `$contentLength` this property is used for test purposes only.

We can set the `Content-Length` header with this:

	r.headersIn['Content-Length] = '555'

and then check:

	r.headersIn.$contentLengthN === 555

Note that type of `$contentLengthN` is number here. nginx does the same (with `r->headers_in.content_length_n`) on the native side.


### $range

Like the `$contentLength` this property is used for test purposes only.

It does fully duplicate the logic of `$contentLength` but for the `Range` header.



Nginx.HeadersOut
================

Again, as for `Nginx.HeadersIn` we can not create `Nginx.HeadersOut` instance directly but only with `r.headersOut` request property. That's because of the output headers instance have to know to which request object (and `headers_out` structure in terms of nginx) it belongs.

	r.headersOut['WWW-Authenticate'] = 'Basic realm="Nginx Area"'

This class is almost the same as `Nginx.HeadersIn`, except in some special properties.


Properties
----------

All properties are proxies to or from the native request output headers.


### $dateTime

Number (in seconds) representation of `Date-Time` output header. Reflects `r->headers_out.date_time` of the native side.


### $contentLengthN

Number (in bytes) representation of `Content-Length` output header. Reflects `r->headers_out.content_length_n` of the native side.


### $lastModified

Number (in seconds) representation of `Last-Modified` output header. Reflects `r->headers_out.last_modified_time` of the native side.


### $contentTypeLen

Number (in bytes) representation of `Content-Type` output header. Reflects `r->headers_out.content_type_len` of the native side.


### $contentTypeLowcase

Lowercased string representation of `Content-Type` output header. Reflects `r->headers_out.content_type_lowcase` of the native side.



Nginx.Cookies
=============

This class is just a fast and lightweight wrapper for cookies in nginx. It does not parse `Cookie` header only search trough it if we read a property. This way of access to the cookies is not very fast if we have to read many values many times. Cookie names can not be enumerated. If you need a full-featured cookie management experience you may just parse the `Cookie` header and store its data in a simple object.

In short use this class if you want to do only few lookups and go.


Properties
----------

As in headers wrappers all the properties in `Nginx.Cookie` instance are mapped to the content of `Cookie` header with a functions built in nginx.
At the moment we can't delete or add or edit cookies value with this class (try to use `r.headersOut['Cookie'] = 'all cokies here'` instead).

This class instances have some additional functionality good for test purposes only.

### length

Return a count of cookies.


Methods
-------

### empty()

Marks the cookies headers array as empty. This method does not try to fully delete cookies headers, just marks the native array as empty.



Nginx.Variables
===============

Variables in nginx are even more complicated thing then the headers. AFAIK, variable may be cached or not, indexed or not, has getter/setter or not. Every module that adds a variable may handle its value with different ways: share the value between different variables, invalidate cache or change the setter and getter function. Huge amount of flexibility! And all this is a subject to change (the last one was in 0.8.36).

We can access all the nginx variables defined by variouse modules with a simple hash-like inteface (yeap, like headers and cookies):

	r.variables.limit_rate = "4096" // 4k

and it should work ;)

On an attempt to set inexistent variable this class could throw an exception (`can't find variable …`). On getting inexistent variable just returns `undefined`.

In short we can safely get a variable value as far as it is supported by nginx. Setting a value is much more complicated thing. This module tries to duplicate the logic from the [rewrite module][].

[rewrite module]: http://wiki.nginx.org/NginxHttpRewriteModule



Nginx.File
==========

This class is a tiny wrapper around `ngx_fd_t`. File descriptor (`ngx_fd_t`) has such a value that [fits in a pointer][uintptr_t]. This makes a work with `Nginx.File` object relatively fast and its memory footprint almost nothing.

[uintptr_t]: http://nginx.org/pipermail/nginx-devel/2010-April/000200.html


TODO
----

There are some simple things left for future:

* [full support for `File.open`][#32]: now we can open a file only one way;
* [add support for open file cache][#33]: for now the `Nginx.File` is just a lightweight wrapper for `fd`;
* [File#seek][#34].

[#32]: http://github.com/kung-fu-tzu/ngx_http_js_module/issues/issue/32
[#33]: http://github.com/kung-fu-tzu/ngx_http_js_module/issues/issue/33
[#34]: http://github.com/kung-fu-tzu/ngx_http_js_module/issues/issue/34


Static properties
-----------------


### openFiles

Gives a count of files open with `Nginx.File`. Very useful for debugging. See “close and openFiles” test in [tests/file.js][].

[tests/file.js]: http://github.com/kung-fu-tzu/ngx_http_js_module/blob/master/js/tests/file.js


### open modes

Reflects some `NGX_FILE_*` constants:

* RDONLY
* WRONLY
* RDWR
* CREATE_OR_OPEN
* OPEN
* TRUNCATE
* APPEND
* NONBLOCK
* DEFAULT_ACCESS
* OWNER_ACCESS


### return codes

Reflects some `NGX_FILE_*` constants:

* INVALID
* ERROR


### useful stuff

Reflects some `NGX_FILE_*` constants:

* HAVE_CASELESS_FILESYSTEM


Static methods
--------------


### open

Tries to open file and on success return an instance of `Nginx.File`.

	var file = Nginx.File.open(Nginx.prefix + 'nginx.conf')

For now file will be created or opened (`Nginx.File.CREATE_OR_OPEN`), with read/write access (`Nginx.File.RDWR`) and with a default file access (`Nginx.File.DEFAULT_ACCESS`).

On failure return `null`.


### rename(src, dst)

Just renames file `src` to file `dst`.


### remove(path)

Just removes file this `path`.


### exists(path)

Checks if `path` directs to a file.

Return `null` if path does not exist at all, `false` if there is something but not a file and return `true` on an existent plain file.


### getAccess(fname)

Returns an access code of `fname`. If `fname` points to inexistent entry this mthod returns `null`.

	var access = Nginx.File.getAccess('db.json')
	if (access != 0644)
		return Nginx.ERROR


### setAccess(fname, access)

Returns an access code of `fname`. As simple as it could to be.

	if (Nginx.File.setAccess('db.json', 0644) == Nginx.ERROR)
		return Nginx.HTTP_INTERNAL_SERVER_ERROR


Instance properties
-------------------


### size

Return a size of a file in bytes.

An equivalent to the Ruby's `File.read` (reads the entire file in memory):

	Nginx.File.read = function (name) {
		var file = this.open(name)
		if (!file)
			return null
		
		return file.read(file.size)
	}


Instance methods
----------------


### read(count)

Reads `count` of bytes from the current file position and tries to convert the bytes to a string.

On success returns a string with the content of the file. Otherwise return `null`.


### write(string)

Converts `string` to the UTF-8 bytes sequence and writes to the file from the current position.


### seek(offset)

Set a position in the file to `offset`. For now `SEEK_SET` is supported only.


### close()

Closes the corresponding `ngx_fd_t` stored within this `Nginx.File` instance and marks the instance as deactivated. Do not use a closed file at all, it would throw an exception.



Nginx.Dir
=========

Very simple interface to file system directories.


Static methods
--------------


### create(path, access)

Creates a directory with `path` if all the path except the last part is already exists.

	if (Dir.create('users/' + uname, 0700) == File.ERROR)
		throw new Error('could not create user home dir')


### remove(path)

Removes the last directory specified by `path`. Acts just like `rm` shell command.


### createPath(path, access)

Creates full `path` step by step. It acts like `mkdir -p` shell command.

On success return `Nginx.OK`. On error returns `errno`.

	Nginx.Dir.createPath('/a/b/c/ddd/', 0755)

Note that `createPath()` expects absolute path or relative path starting with `'./'`. The trailing slash is required also. Expecting absolute path it ignores one leading char (i.e. `'/'`) on Unices and tree leading chars (i.e. `C:/`) on windows.


### removeTree(path)

Walks trough the directories starting at `path` and deletes everything it meets: files, directories, sockets, fifos, etc.

On success returns `Nginx.OK`. Otherwise `errno`.

USE WITH CARE! `ngx_walk_tree()` (on which `removeTree()` is based) is very a very smart function, much smarter than me ;)


### walkTree(path, onfile, ondirenter, ondirleave, onspecial)

This method is a straight forward interface to the smart `ngx_walk_tree()` function. It takes four callbacks:

* `onfile` is called if file is met; takes four parameters: path, size, access, and mtime;
* `ondirenter` is called on entering a directory; takes three parameters: path, access, mtime;
* `ondirleave` is called on leaving a directory; takes three parameters: path, access, mtime (the same as in `ondirenter`);
* `onspecial` is called if some special entry is met (like socket or fifo); takes only one parameter: path.

Note that all the additional parameters like `mtime` and `size` are taken from the directory entry, so it costs almost nothing. For more detailed definition please see [the source of `ngx_walk_tree()`][ngx_walk_tree].

On success returns `Nginx.OK`. Otherwise `errno` or the value returned by a callback.

[ngx_walk_tree]: http://lxr.evanmiller.org/http/source/core/ngx_file.c#L822


Author
======

Peter Leonov (Пётр Леонов) <gojpeg@gmail.com> with a help from:

* [Yichun Zhang][]
* [Maxim Dounin][]
* [Igor Sysoev][]

[Yichun Zhang]: http://github.com/agentzh
[Maxim Dounin]: http://github.com/mdounin
[Igor Sysoev]: http://sysoev.ru/en/


Initially developed for fun and [Inshaker, the cocktail site][inshaker] (in Russian, very AJAXy, best viewed with Google Chrome [in-browser translation][]).

[in-browser translation]: http://www.google.com/support/chrome/bin/answer.py?answer=173424
[Google Chrome]: http://www.google.com/chrome
[inshaker]: http://www.inshaker.ru/

Copyright & License
===================

Copyright (c) 2008-2010, Peter Leonov <gojpeg@gmail.com>.

This module is licensed under the terms of the BSD license.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

*  Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

*  Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

*  Neither the name of the Inshaker nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


See Also
========

* [nginx Lua module][] — embed power of Lua into nginx;
* [nginx echo module][] — brings `echo`, `sleep`, `time`, `exec` and more shell-style goodies to nginx config file;
* [nginx V8 module][] ([github mirror][nginx V8 module mirror]) — enables you to run any JavaScript script Google's V8 JavaScript Engine supports;
* [node.js][] — evented I/O for V8 JavaScript.

[nginx Lua module]: http://github.com/chaoslawful/lua-nginx-module
[nginx echo module]: http://github.com/agentzh/echo-nginx-module
[nginx V8 module]: http://code.google.com/p/ngxv8/
[nginx V8 module mirror]: http://github.com/kung-fu-tzu/ngxv8
[node.js]: http://nodejs.org/
