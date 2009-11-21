
// Nginx.HeadersOut class

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <jsapi.h>
#include <assert.h>

#include "../ngx_http_js_module.h"
#include "../strings_util.h"
#include "Chain.h"

#include "../macroses.h"



JSObject *ngx_http_js__nginx_chain__prototype;
JSClass ngx_http_js__nginx_chain__class;
static JSClass *private_class = &ngx_http_js__nginx_chain__class;


JSObject *
ngx_http_js__nginx_chain__wrap(JSContext *cx, ngx_chain_t *ch, JSObject *request)
{
	TRACE();
	JSObject                  *chain;
	
	chain = JS_NewObject(cx, &ngx_http_js__nginx_chain__class, ngx_http_js__nginx_chain__prototype, NULL);
	if (!chain)
	{
		JS_ReportOutOfMemory(cx);
		return NULL;
	}
	
	JS_SetPrivate(cx, chain, ch);
	JS_SetReservedSlot(cx, chain, NGX_HTTP_JS_CHAIN_REQUEST_SLOT, OBJECT_TO_JSVAL(request));
	
	return chain;
}


static JSBool
method_toString(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	TRACE();
	ngx_chain_t  *ch, *next;
	int           len;
	char         *buff;
	
	GET_PRIVATE(ch);
	
	if (ch->buf && !ch->next)
	{
		*rval = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char*) ch->buf->pos, ch->buf->last - ch->buf->pos));
	}
	else
	{
		len = 0;
		for (next=ch; next; next = next->next)
		{
			if (next->buf)
				len += next->buf->last - next->buf->pos;
		}
		
		buff = JS_malloc(cx, len);
		E(buff, "Can`t JS_malloc");
		
		len = 0;
		for (next=ch; next; next = next->next)
		{
			if (next->buf)
			{
				ngx_memcpy(&buff[len], next->buf->pos, next->buf->last - next->buf->pos);
				len += next->buf->last - next->buf->pos;
			}
		}
		
		*rval = STRING_TO_JSVAL(JS_NewStringCopyN(cx, (char*) buff, len));
		
		JS_free(cx, buff);
	}
	
	return JS_TRUE;
}



static JSBool
constructor(JSContext *cx, JSObject *this, uintN argc, jsval *argv, jsval *rval)
{
	TRACE();
	return JS_TRUE;
}


// enum propid { HEADER_LENGTH };


static JSBool
getProperty(JSContext *cx, JSObject *this, jsval id, jsval *vp)
{
	ngx_chain_t  *ch;
	
	TRACE();
	GET_PRIVATE(ch);
	
	
	return JS_TRUE;
}


static JSBool
setProperty(JSContext *cx, JSObject *this, jsval id, jsval *vp)
{
	ngx_chain_t  *ch;
	
	TRACE();
	GET_PRIVATE(ch);
	
	
	return JS_TRUE;
}


JSPropertySpec ngx_http_js__nginx_chain__props[] =
{
	// {"uri",      REQUEST_URI,          JSPROP_READONLY,   NULL, NULL},
	{0, 0, 0, NULL, NULL}
};


JSFunctionSpec ngx_http_js__nginx_chain__funcs[] = {
    {"toString",       method_toString,          0, 0, 0},
    {0, NULL, 0, 0, 0}
};

JSClass ngx_http_js__nginx_chain__class =
{
	"Chain",
	JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1),
	JS_PropertyStub, JS_PropertyStub, getProperty, setProperty,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSBool
ngx_http_js__nginx_chain__init(JSContext *cx)
{
	JSObject    *nginxobj;
	JSObject    *global;
	jsval        vp;
	
	TRACE();
	global = JS_GetGlobalObject(cx);
	
	E(JS_GetProperty(cx, global, "Nginx", &vp), "global.Nginx is undefined or is not a function");
	nginxobj = JSVAL_TO_OBJECT(vp);
	
	ngx_http_js__nginx_chain__prototype = JS_InitClass(cx, nginxobj, NULL, &ngx_http_js__nginx_chain__class,  constructor, 0,
		ngx_http_js__nginx_chain__props, ngx_http_js__nginx_chain__funcs,  NULL, NULL);
	E(ngx_http_js__nginx_chain__prototype, "Can`t JS_InitClass(Nginx.HeadersOut)");
	
	return JS_TRUE;
}