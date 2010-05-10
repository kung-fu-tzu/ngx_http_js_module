
// Nginx.Request class

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <js/jsapi.h>

#include <ngx_http_js_module.h>
#include <nginx_js_glue.h>
#include <strings_util.h>
#include <classes/Request.h>
#include <classes/Request/HeadersIn.h>
#include <classes/Request/HeadersOut.h>
#include <classes/Chain.h>

#include <nginx_js_macroses.h>

JSObject *ngx_http_js__nginx_request__prototype;
JSClass ngx_http_js__nginx_request__class;
static JSClass *private_class = &ngx_http_js__nginx_request__class;

static void
cleanup_handler(void *data);

static void
method_setTimer_handler(ngx_event_t *ev);


JSObject *
ngx_http_js__nginx_request__wrap(JSContext *cx, ngx_http_request_t *r)
{
	ngx_http_js_ctx_t         *ctx;
	JSObject                  *request;
	
	TRACE_REQUEST("request_root");
	
	// get a js module context
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	if (ctx == NULL)
	{
		// or create a js module context;
		// ngx_pcalloc fills allocated memory with zeroes
		ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_js_ctx_t));
		if (ctx == NULL)
		{
			// or return an error
			return NULL;
		}
		
		ngx_http_set_ctx(r, ctx, ngx_http_js_module);
	}
	
	// check if the request is already wrapped
	if (ctx->js_request != NULL)
	{
		return ctx->js_request;
	}
	
	TRACE_REQUEST("request_wrap");
	
	request = JS_NewObject(cx, &ngx_http_js__nginx_request__class, ngx_http_js__nginx_request__prototype, NULL);
	if (!request)
	{
		ngx_log_error(NGX_LOG_CRIT, r->connection->log, 0, "could not create a wrapper object");
		return NULL;
	}
	
	JS_SetPrivate(cx, request, r);
	
	
	// We can't just store the wrapper in the request context without rooting it,
	// because the wrapper may be garbage collected out and we got a pointer to nothing
	// in our ctx->js_request which leads to a crash or worst. So leaving the ctx->js_request
	// empty.
	
	
	return request;
}

ngx_int_t
ngx_http_js__nginx_request__root_in(JSContext *cx, ngx_http_request_t *r, JSObject *request)
{
	ngx_http_js_ctx_t         *ctx;
	ngx_http_cleanup_t        *cln;
	
	TRACE_REQUEST("request_root");
	
	// get a js module context
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	if (ctx == NULL)
	{
		// or create a js module context;
		// ngx_pcalloc fills allocated memory with zeroes
		ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_js_ctx_t));
		if (ctx == NULL)
		{
			// or return an error
			return NGX_ERROR;
		}
		
		ngx_http_set_ctx(r, ctx, ngx_http_js_module);
	}
	
	if (ctx->js_request != NULL)
	{
		if (ctx->js_request != request)
		{
			ngx_log_error(NGX_LOG_CRIT, r->connection->log, 0, "trying to root JS request %p in ctx %p in place of JS request %p", request, ctx, ctx->js_request);
			return NGX_ERROR;
		}
		
		ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "trying to root the same JS request %p in the same ctx %p more than once", request, ctx);
		return NGX_OK;
	}
	
	
	ctx->js_request = request;
	
	if (!JS_AddNamedRoot(cx, &ctx->js_request, JS_REQUEST_ROOT_NAME))
	{
		ngx_log_error(NGX_LOG_CRIT, r->connection->log, 0, "could not add new root %s", JS_REQUEST_ROOT_NAME);
		ctx->js_request = NULL;
		return NGX_ERROR;
	}
	
	cln = ngx_http_cleanup_add(r, 0);
	if (cln == NULL)
	{
		ctx->js_request = NULL;
		return NGX_ERROR;
	}
	cln->data = r;
	cln->handler = cleanup_handler;
	
	return NGX_OK;
}

static void
cleanup_handler(void *data)
{
	ngx_http_request_t        *r;
	ngx_http_js_ctx_t         *ctx;
	
	r = data;
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "js request cleanup_handler(r=%p)", r);
	
	if (!(ctx = ngx_http_get_module_ctx(r, ngx_http_js_module)))
	{
		ngx_log_error(NGX_LOG_CRIT, r->connection->log, 0, "empty module context");
		return;
	}
	
	ngx_http_js__nginx_request__cleanup(ctx, r, js_cx);
}

void
ngx_http_js__nginx_request__cleanup(ngx_http_js_ctx_t *ctx, ngx_http_request_t *r, JSContext *cx)
{
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "js request cleanup");
	
	// let the Headers modules to deside what to clean up
	ngx_http_js__nginx_headers_in__cleanup(ctx, r, cx);
	ngx_http_js__nginx_headers_out__cleanup(ctx, r, cx);
	
	// second param has to be &ctx->js_request
	// because JS_AddRoot was used with it's address
	if (!JS_RemoveRoot(cx, &ctx->js_request))
		JS_ReportError(cx, "Can`t remove cleaned up root %s", JS_REQUEST_ROOT_NAME);
	
	
	if (ctx->js_timer.timer_set)
	{
		// implies timer_set = 0
		ngx_del_timer(&ctx->js_timer);
	}
	
	if (ctx->js_request)
	{
		// finaly mark the object as inactive
		// after that the GET_PRIVATE macros will raise an exception when called
		JS_SetPrivate(cx, ctx->js_request, NULL);
		// and set the native request as unwrapped
		ctx->js_request = NULL;
	}
	else
	{
		ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, COLOR_RED "trying to cleanup the request with an empty wrapper" COLOR_CLEAR);
	}
}


static JSBool
method_cleanup(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}


static JSBool
method_rootMe(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t *r;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	// force the “self” to be rooted in the “r”
	if (ngx_http_js__nginx_request__root_in(cx, r, self) != NGX_OK)
	{
		return JS_FALSE;
	}
	
	return JS_TRUE;
}

static JSBool
method_sendHttpHeader(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t *r;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	if (r->headers_out.status == 0)
	{
		r->headers_out.status = NGX_HTTP_OK;
	}
	
	if (argc == 1)
	{
		E(JSVAL_IS_STRING(argv[0]), "sendHttpHeader() takes one optional argument: contentType:String");
		
		E(js_str2ngx_str(cx, JSVAL_TO_STRING(argv[0]), r->pool, &r->headers_out.content_type),
			"Can`t js_str2ngx_str(cx, contentType)")
		
		r->headers_out.content_type_len = r->headers_out.content_type.len;
    }
	
	E(ngx_http_set_content_type(r) == NGX_OK, "Can`t ngx_http_set_content_type(r)")
	E(ngx_http_send_header(r) == NGX_OK, "Can`t ngx_http_send_header(r)");
	
	*rval = JSVAL_TRUE;
	return JS_TRUE;
}


static JSBool
method_print(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	ngx_buf_t           *b;
	size_t               len;
	JSString            *str;
	ngx_chain_t          out;
	ngx_int_t            rc;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	E(argc == 1, "Nginx.Request#print takes 1 argument: str");
	
	str = JS_ValueToString(cx, argv[0]);
	b = js_str2ngx_buf(cx, str, r->pool);
	if (b == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	len = b->last - b->pos;
	if (len == 0)
	{
		return JS_TRUE;
	}
	
	// ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "js prints string \"%*s\"", len > 25 ? 25 : len , b->last - len);
	
	out.buf = b;
	out.next = NULL;
	rc = ngx_http_output_filter(r, &out);
	
	*rval = INT_TO_JSVAL(rc);
	return JS_TRUE;
}


static JSBool
method_flush(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	ngx_buf_t           *b;
	ngx_chain_t          out;
	ngx_int_t            rc;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	b = ngx_calloc_buf(r->pool);
	if (b == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	b->flush = 1;
	
	out.buf = b;
	out.next = NULL;
	rc = ngx_http_output_filter(r, &out);
	
	*rval = INT_TO_JSVAL(rc);
	return JS_TRUE;
}


static JSBool
method_nextBodyFilter(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	ngx_buf_t           *b;
	size_t               len;
	JSString            *str;
	ngx_chain_t          out, *ch;
	ngx_int_t            rc;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	E(ngx_http_js_next_body_filter != NULL, "ngx_http_js_next_body_filter is NULL at this moment");
	
	if (argc == 1 && JSVAL_IS_STRING(argv[0]))
	{
		str = JSVAL_TO_STRING(argv[0]);
		b = js_str2ngx_buf(cx, str, r->pool);
		if (b == NULL)
		{
			JS_ReportOutOfMemory(cx);
			return JS_FALSE;
		}
		len = b->last - b->pos;
		if (len == 0)
		{
			return JS_TRUE;
		}
		
		b->last_buf = 1;
	
		out.buf = b;
		out.next = NULL;
		rc = ngx_http_js_next_body_filter(r, &out);
	}
	else if (argc == 1 && JSVAL_IS_OBJECT(argv[0]))
	{
		if ( (ch = JS_GetInstancePrivate(cx, JSVAL_TO_OBJECT(argv[0]), &ngx_http_js__nginx_chain__class, NULL)) == NULL )
		{
			JS_ReportError(cx, "second parameter is object but not a chain or chain has NULL private pointer");
			return JS_FALSE;
		}
		
		rc = ngx_http_js_next_body_filter(r, ch);
	}
	else if (argc == 0 || (argc == 1 && JSVAL_IS_VOID(argv[0])))
	{
		rc = ngx_http_js_next_body_filter(r, NULL);
	}
	else
	{
		E(0, "Nginx.Request#nextBodyFilter takes 1 optional argument: str:(String|undefined)");
	}
	
	*rval = INT_TO_JSVAL(rc);
	
	return JS_TRUE;
}


static JSBool
method_sendString(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	ngx_buf_t           *b;
	size_t               len;
	JSString            *str;
	ngx_chain_t          out;
	ngx_int_t            rc;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	E(( (argc == 1 && JSVAL_IS_STRING(argv[0])) || (argc == 2 && JSVAL_IS_STRING(argv[0]) && JSVAL_IS_STRING(argv[1])) ),
		"Nginx.Request#sendString takes 1 mandatory argument: str:String, and 1 optional: contentType:String");
	
	str = JS_ValueToString(cx, argv[0]);
	b = js_str2ngx_buf(cx, str, r->pool);
	if (b == NULL)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	len = b->last - b->pos;
	if (len == 0)
	{
		return JS_TRUE;
	}
	
	ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "js sending string \"%*s\"", len > 25 ? 25 : len , b->last - len);
	
	ngx_http_clear_content_length(r);
	r->headers_out.content_length_n = len;
	
	if (r->headers_out.status == 0)
	{
		r->headers_out.status = NGX_HTTP_OK;
	}
	
	if (argc == 2)
	{
		E(js_str2ngx_str(cx, JS_ValueToString(cx, argv[1]), r->pool, &r->headers_out.content_type),
			"Can`t js_str2ngx_str(cx, contentType)")
		
		r->headers_out.content_type_len = r->headers_out.content_type.len;
    }
	
	E(ngx_http_set_content_type(r) == NGX_OK, "Can`t ngx_http_set_content_type(r)")
	E(ngx_http_send_header(r) == NGX_OK, "Can`t ngx_http_send_header(r)");
	
	out.buf = b;
	out.next = NULL;
	rc = ngx_http_output_filter(r, &out);
	
	ngx_http_send_special(r, NGX_HTTP_FLUSH);
	
	*rval = INT_TO_JSVAL(rc);
	return JS_TRUE;
}


static JSBool
method_sendSpecial(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	E(argc == 1 && JSVAL_IS_INT(argv[0]), "Nginx.Request#sendSpecial takes 1 argument: flags:Number");
	
	*rval = INT_TO_JSVAL(ngx_http_send_special(r, (ngx_uint_t)JSVAL_TO_INT(argv[0])));
	return JS_TRUE;
}

void
method_getBody_handler(ngx_http_request_t *r);

static JSBool
method_getBody(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	E(argc == 1 && JSVAL_IS_OBJECT(argv[0]) && JS_ValueToFunction(cx, argv[0]), "Request#hasBody takes 1 argument: callback:Function");
	
	
	if (r->headers_in.content_length_n <= 0)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}
	
	E(JS_SetReservedSlot(cx, self, NGX_JS_REQUEST_SLOT__HAS_BODY_CALLBACK, argv[0]),
		"can't set slot NGX_JS_REQUEST_SLOT__HAS_BODY_CALLBACK(%d)", NGX_JS_REQUEST_SLOT__HAS_BODY_CALLBACK);
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "has body callback set");
	
	r->request_body_in_single_buf = 1;
	r->request_body_in_persistent_file = 1;
	r->request_body_in_clean_file = 1;
	
	if (r->request_body_in_file_only)
	{
		r->request_body_file_log_level = 0;
	}
	
	// ngx_http_read_client_request_body implies count++
	*rval = INT_TO_JSVAL(ngx_http_read_client_request_body(r, method_getBody_handler));
	return JS_TRUE;
}

void
method_getBody_handler(ngx_http_request_t *r)
{
	ngx_http_js_ctx_t                *ctx;
	JSObject                         *request;
	jsval                             rval, callback;
	ngx_int_t                         rc;
	
	TRACE_REQUEST_METHOD();
	
	// if (r->connection->error)
	// 	return;
	
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	ngx_assert(ctx);
	
	request = ctx->js_request;
	ngx_assert(request);
	
	if (JS_GetReservedSlot(js_cx, request, NGX_JS_REQUEST_SLOT__HAS_BODY_CALLBACK, &callback))
	{
		if (JS_CallFunctionValue(js_cx, request, callback, 0, NULL, &rval))
		{
			rc = NGX_DONE;
		}
		else
		{
			rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
	}
	else
	{
		JS_ReportError(js_cx, "can't get slot NGX_JS_REQUEST_SLOT__HAS_BODY_CALLBACK(%d)", NGX_JS_REQUEST_SLOT__HAS_BODY_CALLBACK);
		rc = NGX_ERROR;
	}
	
	// implies count--
	ngx_http_finalize_request(r, rc);
}


static JSBool
method_discardBody(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	*rval = INT_TO_JSVAL(ngx_http_discard_request_body(r));
	return JS_TRUE;
}


static JSBool
method_sendfile(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	char                      *filename;
	int                        offset;
	size_t                     bytes;
	ngx_str_t                  path;
	ngx_buf_t                 *b;
	ngx_open_file_info_t       of;
	ngx_http_core_loc_conf_t  *clcf;
	ngx_chain_t                out;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	
	E(argc >= 1 && JSVAL_IS_STRING(argv[0]),
		"Nginx.Request#sendfile takes 1 mandatory argument: filename:String, and 2 optional offset:Number and bytes:Number");
	
	
	E(filename = js_str2c_str(cx, JSVAL_TO_STRING(argv[0]), r->pool, NULL), "Can`t js_str2c_str()");
	ngx_assert(filename);
	
	offset = argc < 2 ? -1 : JSVAL_TO_INT(argv[1]);
	bytes = argc < 3 ? 0 : JSVAL_TO_INT(argv[2]);
	
	ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "sending file \"%s\" with offset=%d and bytes=%d", filename, offset, bytes);
	
	b = ngx_calloc_buf(r->pool);
	E(b != NULL, "Can`t ngx_calloc_buf()");
	
	b->file = ngx_pcalloc(r->pool, sizeof(ngx_file_t));
	E(b->file != NULL, "Can`t ngx_pcalloc()");
	
	clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
	ngx_assert(clcf);
	
	
	of.test_dir = 0;
	of.valid = clcf->open_file_cache_valid;
	of.min_uses = clcf->open_file_cache_min_uses;
	of.errors = clcf->open_file_cache_errors;
	of.events = clcf->open_file_cache_events;
	
	path.len = ngx_strlen(filename);
	
	path.data = ngx_pcalloc(r->pool, path.len + 1);
	E(path.data != NULL, "Can`t ngx_pcalloc()");
	
	(void) ngx_cpystrn(path.data, (u_char*)filename, path.len + 1);
	
	if (ngx_open_cached_file(clcf->open_file_cache, &path, &of, r->pool) != NGX_OK)
	{
		if (of.err != 0)
			ngx_log_error(NGX_LOG_CRIT, r->connection->log, ngx_errno, ngx_open_file_n " \"%s\" failed", filename);
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}
	
	if (offset == -1)
	{
		offset = 0;
	}
	
	if (bytes == 0)
	{
		bytes = of.size - offset;
	}
	
	b->in_file = 1;
	
	b->file_pos = offset;
	b->file_last = offset + bytes;
	
	b->file->fd = of.fd;
	b->file->log = r->connection->log;
	
	
	out.buf = b;
	out.next = NULL;
	
	*rval = INT_TO_JSVAL(ngx_http_output_filter(r, &out));
	return JS_TRUE;
}


static JSBool
method_setTimer(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	ngx_http_js_ctx_t   *ctx;
	ngx_event_t         *timer;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	E(argc == 2 && JSVAL_IS_OBJECT(argv[0]) && JS_ValueToFunction(cx, argv[0]) && JSVAL_IS_INT(argv[1]),
			"Nginx.Request#setTimer() takes two mandatory argument callback:Function and milliseconds:Number");
	
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	ngx_assert(ctx);
	timer = &ctx->js_timer;
	
	E(JS_SetReservedSlot(cx, self, NGX_JS_REQUEST_SLOT__SET_TIMER, argv[0]),
		"can't set slot NGX_JS_REQUEST_SLOT__SET_TIMER(%d)", NGX_JS_REQUEST_SLOT__SET_TIMER);
	
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "timer.timer_set = %i", timer->timer_set);
	
	if (!timer->timer_set)
	{
		// from ngx_cycle.c:740
		timer->handler = method_setTimer_handler;
		timer->log = r->connection->log;
		timer->data = r;
		
		r->main->count++;
	}
	
	// implies timer_set = 1;
	// AFAIK, the socond addition of the timer does not duplicate it
	ngx_add_timer(timer, (ngx_uint_t) argv[1]);
	
	return JS_TRUE;
}

static JSBool
method_clearTimer(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	ngx_http_request_t  *r;
	ngx_http_js_ctx_t   *ctx;
	ngx_event_t         *timer;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	ngx_assert(ctx);
	timer = &ctx->js_timer;
	
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "timer.timer_set = %i", timer->timer_set);
	
	if (timer->timer_set)
	{
		ngx_del_timer(timer);
		r->main->count--;
	}
	
	return JS_TRUE;
}

static void
method_setTimer_handler(ngx_event_t *timer)
{
	ngx_http_request_t  *r;
	ngx_int_t            rc;
	ngx_http_js_ctx_t   *ctx;
	jsval                rval, callback;
	JSObject            *request;
	
	r = timer->data;
	TRACE_REQUEST_METHOD();
	
	
	ctx = ngx_http_get_module_ctx(r, ngx_http_js_module);
	ngx_assert(ctx);
	
	request = ctx->js_request;
	
	if (!JS_GetReservedSlot(js_cx, request, NGX_JS_REQUEST_SLOT__SET_TIMER, &callback))
	{
		JS_ReportError(js_cx, "can't get slot NGX_JS_REQUEST_SLOT__SET_TIMER(%d)", NGX_JS_REQUEST_SLOT__SET_TIMER);
		rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	else
	{
		// here a new timeout handler may be set
		if (JS_CallFunctionValue(js_cx, request, callback, 0, NULL, &rval))
		{
			rc = NGX_DONE;
		}
		else
		{
			rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
	}
	
	// ngx_event_expire_timers() implies ngx_rbtree_delete() and timer_set = 0;
	
	// implies count--
	ngx_http_finalize_request(r, rc);
}


static ngx_int_t
method_subrequest_handler(ngx_http_request_t *sr, void *data, ngx_int_t rc);

static JSBool
method_subrequest(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	ngx_int_t                    rc;
	ngx_http_request_t          *r, *sr;
	ngx_http_post_subrequest_t  *psr;
	ngx_str_t                   *uri, args;
	ngx_uint_t                   flags;
	JSString                    *str;
	JSObject                    *subrequest;
	
	
	GET_PRIVATE(r);
	TRACE_REQUEST_METHOD();
	
	E(argc == 2 && JSVAL_IS_STRING(argv[0]) && JSVAL_IS_OBJECT(argv[1]) && JS_ValueToFunction(cx, argv[1]),
		"Request#subrequest takes 2 mandatory arguments: uri:String and callback:Function");
	
	str = JSVAL_TO_STRING(argv[0]);
	
	E(uri = ngx_palloc(r->pool, sizeof(ngx_str_t)), "Can`t ngx_palloc(ngx_str_t)");
	E(js_str2ngx_str(cx, str, r->pool, uri), "Can`t js_str2ngx_str()")
	E(uri->len, "empty uri passed");
	
	flags = 0;
	args.len = 0;
	args.data = NULL;
	
	E(ngx_http_parse_unsafe_uri(r, uri, &args, &flags) == NGX_OK, "Error in ngx_http_parse_unsafe_uri(%s)", uri->data)
	
	psr = NULL;
	E(psr = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t)), "Can`t ngx_palloc()");
	psr->handler = method_subrequest_handler;
	// psr->data = r;
	
	flags |= NGX_HTTP_SUBREQUEST_IN_MEMORY;
	
	
	sr = NULL;
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "before ngx_http_subrequest()");
	rc = ngx_http_subrequest(r, uri, &args, &sr, psr, flags);
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "after ngx_http_subrequest()");
	if (sr == NULL || rc != NGX_OK)
	{
		JS_ReportError(cx, "Can`t ngx_http_subrequest(...)");
		return JS_FALSE;
	}
	// sr->filter_need_in_memory = 1;
	
	subrequest = ngx_http_js__nginx_request__wrap(cx, sr);
	
	if (subrequest)
	{
		E(JS_SetReservedSlot(cx, subrequest, NGX_JS_REQUEST_SLOT__SUBREQUEST_CALLBACK, argv[1]),
			"can't set slot NGX_JS_REQUEST_SLOT__SUBREQUEST_CALLBACK(%d)", NGX_JS_REQUEST_SLOT__SUBREQUEST_CALLBACK);
		if (ngx_http_js__nginx_request__root_in(js_cx, sr, subrequest) != NGX_OK)
		{
			// forward the exception
			return JS_FALSE;
		}
	}
	else
	{
		JS_ReportError(cx, "couldn't wrap subrequest");
		return JS_FALSE;
	}
	
	*rval = INT_TO_JSVAL(rc);
	return JS_TRUE;
}

// we here are called form ngx_http_finalize_request() for the subrequest
// via sr->post_subrequest->handler(sr, data, rc)
// “rc” is the code was passed to the ngx_http_finalize_request()
static ngx_int_t
method_subrequest_handler(ngx_http_request_t *sr, void *data, ngx_int_t rc)
{
	ngx_http_js_ctx_t                *ctx;
	ngx_http_request_t               *r;
	JSObject                         *subrequest;
	jsval                             callback, rval, args[2];
	
	r = sr->main;
	TRACE_REQUEST_METHOD();
	
	ctx = ngx_http_get_module_ctx(sr, ngx_http_js_module);
	if (!ctx)
	{
		ngx_log_error(NGX_LOG_CRIT, sr->connection->log, 0, "subrequest handler with empty module context");
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	
	subrequest = ctx->js_request;
	// ctx->js_request may not be present if the subrequest construction has failed
	if (!subrequest)
	{
		return NGX_ERROR;
	}
	
	if (!JS_GetReservedSlot(js_cx, subrequest, NGX_JS_REQUEST_SLOT__SUBREQUEST_CALLBACK, &callback))
	{
		JS_ReportError(js_cx, "can't get slot NGX_JS_REQUEST_SLOT__SUBREQUEST_CALLBACK(%d)", NGX_JS_REQUEST_SLOT__SUBREQUEST_CALLBACK);
		return NGX_ERROR;
	}
	
	// LOG("sr->upstream = %p", sr->upstream);
	// LOG("sr->upstream = %s", sr->upstream->buffer.pos);
	
	if (sr->upstream)
	{
		JSString  *js_buf;
		js_buf = JS_NewStringCopyN(js_cx, (char*) sr->upstream->buffer.pos, sr->upstream->buffer.last-sr->upstream->buffer.pos);
		if (js_buf == NULL)
		{
			rc = NGX_ERROR;
		}
		else
		{
			args[0] = STRING_TO_JSVAL(js_buf);
		}
	}
	else if (ctx->chain_first != NULL)
	{
		args[0] = OBJECT_TO_JSVAL(ngx_http_js__nginx_chain__wrap(js_cx, ctx->chain_first, subrequest));
	}
	else
	{
		args[0] = JSVAL_VOID;
	}
	
	args[1] = INT_TO_JSVAL(rc);
	ngx_log_debug0(NGX_LOG_DEBUG_HTTP, sr->connection->log, 0, "calling subrequest js callback");
	
	if (!JS_CallFunctionValue(js_cx, subrequest, callback, 2, args, &rval))
		rc = NGX_ERROR;
	
	return rc;
}


static JSBool
request_constructor(JSContext *cx, JSObject *self, uintN argc, jsval *argv, jsval *rval)
{
	return JS_TRUE;
}


enum request_propid
{
	REQUEST_URI, REQUEST_METHOD, REQUEST_FILENAME, REQUEST_REMOTE_ADDR, REQUEST_ARGS,
	REQUEST_HEADERS_IN, REQUEST_HEADERS_OUT,
	REQUEST_HEADER_ONLY, REQUEST_BODY_FILENAME, REQUEST_HAS_BODY, REQUEST_BODY
};

static JSBool
request_getProperty(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t   *r;
	GET_PRIVATE(r);
	
	if (JSVAL_IS_INT(id))
	{
		switch (JSVAL_TO_INT(id))
		{
			case REQUEST_URI:
			NGX_STRING_to_JS_STRING_to_JSVAL(cx, r->uri, *vp);
			break;
			
			case REQUEST_METHOD:
			NGX_STRING_to_JS_STRING_to_JSVAL(cx, r->method_name, *vp);
			break;
			
			case REQUEST_FILENAME:
			{
				size_t root;
				ngx_str_t           filename;
				
				if (ngx_http_map_uri_to_path(r, &filename, &root, 0) == NULL)
					break;
				filename.len--;
				
				NGX_STRING_to_JS_STRING_to_JSVAL(cx, filename, *vp);
			}
			break;
			
			case REQUEST_REMOTE_ADDR:
			NGX_STRING_to_JS_STRING_to_JSVAL(cx, r->connection->addr_text, *vp);
			break;
			
			case REQUEST_ARGS:
			NGX_STRING_to_JS_STRING_to_JSVAL(cx, r->args, *vp);
			break;
			
			case REQUEST_HEADER_ONLY:
			*vp = r->header_only ? JSVAL_TRUE : JSVAL_FALSE;
			break;
			
			case REQUEST_HEADERS_IN:
			{
				JSObject             *headers;
				E(headers = ngx_http_js__nginx_headers_in__wrap(cx, self, r), "couldn't wrap headers_in");
				*vp = OBJECT_TO_JSVAL(headers);
			}
			break;
			
			case REQUEST_HEADERS_OUT:
			{
				JSObject             *headers;
				E(headers = ngx_http_js__nginx_headers_out__wrap(cx, self, r), "couldn't wrap headers_out");
				*vp = OBJECT_TO_JSVAL(headers);
			}
			break;
			
			case REQUEST_BODY_FILENAME:
			if (r->request_body != NULL && r->request_body->temp_file != NULL)
			NGX_STRING_to_JS_STRING_to_JSVAL(cx, r->request_body->temp_file->file.name, *vp);
			break;
			
			case REQUEST_HAS_BODY:
			*vp = r->headers_in.content_length_n <= 0 ? JSVAL_FALSE : JSVAL_TRUE;
			break;
			
			case REQUEST_BODY:
			if (r->request_body == NULL || r->request_body->temp_file || r->request_body->bufs == NULL)
			{
				*vp = JSVAL_VOID;
			}
			else
			{
				DATA_LEN_to_JS_STRING_to_JSVAL(cx, r->request_body->bufs->buf->pos, r->request_body->bufs->buf->last - r->request_body->bufs->buf->pos, *vp);
			}
			break;
		}
	}
	return JS_TRUE;
}

static JSBool
getter_allowRanges(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t   *r;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_GETTER();
	
	*vp = r->allow_ranges ? JSVAL_TRUE : JSVAL_FALSE;
	
	return JS_TRUE;
}

static JSBool
setter_allowRanges(JSContext *cx, JSObject *self, jsval id, jsval *vp)
{
	ngx_http_request_t   *r;
	JSBool                b;
	
	GET_PRIVATE(r);
	TRACE_REQUEST_SETTER();
	
	if (!JS_ValueToBoolean(cx, *vp, &b))
	{
		return JS_FALSE;
	}
	
	r->allow_ranges = b == JS_TRUE ? 1 : 0;
	
	return JS_TRUE;
}

JSPropertySpec ngx_http_js__nginx_request__props[] =
{
	{"uri",             REQUEST_URI,              JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"method",          REQUEST_METHOD,           JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"filename",        REQUEST_FILENAME,         JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"remoteAddr",      REQUEST_REMOTE_ADDR,      JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"headersIn",       REQUEST_HEADERS_IN,       JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"headersOut",      REQUEST_HEADERS_OUT,      JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"args",            REQUEST_ARGS,             JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"headerOnly",      REQUEST_HEADER_ONLY,      JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"bodyFilename",    REQUEST_BODY_FILENAME,    JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"hasBody",         REQUEST_HAS_BODY,         JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"body",            REQUEST_BODY,             JSPROP_READONLY|JSPROP_ENUMERATE,   NULL, NULL},
	{"allowRanges",     0,                        JSPROP_ENUMERATE,                   getter_allowRanges, setter_allowRanges},
	
	// TODO:
	// {"status",       MY_WIDTH,       JSPROP_ENUMERATE,  NULL, NULL},
	// {"allowRanges",       MY_ARRAY,       JSPROP_ENUMERATE,  NULL, NULL},
	{0, 0, 0, NULL, NULL}
};


JSFunctionSpec ngx_http_js__nginx_request__funcs[] =
{
	{"sendHttpHeader",    method_sendHttpHeader,       2, 0, 0},
	{"print",             method_print,                1, 0, 0},
	{"flush",             method_flush,                0, 0, 0},
	{"sendString",        method_sendString,           1, 0, 0},
	{"subrequest",        method_subrequest,           2, 0, 0},
	{"cleanup",           method_cleanup,              0, 0, 0},
	{"sendSpecial",       method_sendSpecial,          1, 0, 0},
	{"discardBody",       method_discardBody,          0, 0, 0},
	{"getBody",           method_getBody,              1, 0, 0},
	{"sendfile",          method_sendfile,             1, 0, 0},
	{"setTimer",          method_setTimer,             2, 0, 0},
	{"clearTimer",        method_clearTimer,           0, 0, 0},
	{"nextBodyFilter",    method_nextBodyFilter,       1, 0, 0},
	{"rootMe",            method_rootMe,               0, 0, 0},
	{0, NULL, 0, 0, 0}
};

JSClass ngx_http_js__nginx_request__class =
{
	"Request",
	JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(NGX_JS_REQUEST_SLOTS_COUNT),
	JS_PropertyStub, JS_PropertyStub, request_getProperty, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSBool
ngx_http_js__nginx_request__init(JSContext *cx, JSObject *global)
{
	JSObject    *nginxobj;
	jsval        vp;
	
	E(JS_GetProperty(cx, global, "Nginx", &vp), "global.Nginx is undefined or is not a function");
	nginxobj = JSVAL_TO_OBJECT(vp);
	
	ngx_http_js__nginx_request__prototype = JS_InitClass(cx, nginxobj, NULL, &ngx_http_js__nginx_request__class,  request_constructor, 0,
		ngx_http_js__nginx_request__props, ngx_http_js__nginx_request__funcs,  NULL, NULL);
	E(ngx_http_js__nginx_request__prototype, "Can`t JS_InitClass(Nginx.Request)");
	
	return JS_TRUE;
}