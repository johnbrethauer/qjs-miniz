#include "quickjs.h"
#include "miniz.h"

static JSClassID js_reader_class_id;
static JSClassID js_writer_class_id;

static void js_reader_finalizer(JSRuntime *rt, JSValue val) {
  mz_zip_archive *zip = JS_GetOpaque(val, js_reader_class_id);
  mz_zip_reader_end(zip);
  js_free_rt(rt,zip);
}

static void js_writer_finalizer(JSRuntime *rt, JSValue val) {
  mz_zip_archive *zip = JS_GetOpaque(val, js_writer_class_id);
  mz_zip_writer_finalize_archive(zip);
  mz_zip_writer_end(zip);
  js_free_rt(rt,zip);
}

static JSClassDef js_reader_class = {
    "zip reader",
    .finalizer = js_reader_finalizer,
};

static JSClassDef js_writer_class = {
    "zip writer",
    .finalizer = js_writer_finalizer,
};

static mz_zip_archive *js2reader(JSContext *js, JSValue v)
{
  return JS_GetOpaque2(js, v, js_reader_class_id);
}

static mz_zip_archive *js2writer(JSContext *js, JSValue v)
{
  return JS_GetOpaque2(js, v, js_writer_class_id);
}

static JSValue js_miniz_read(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  size_t len;
  void *data = JS_GetArrayBuffer(js, &len, argv[0]);
  mz_zip_archive *zip = malloc(sizeof(*zip));
  mz_zip_reader_init_mem(zip, data, len, 0);
  JSValue jszip = JS_NewObjectClass(js, js_reader_class_id);
  JS_SetOpaque(jszip, zip);
  return jszip;
}

static JSValue js_miniz_write(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  const char *file = JS_ToCString(js, argv[0]);
  mz_zip_archive *zip = malloc(sizeof(*zip));
  mz_zip_writer_init_file(zip, file, 0);
  JS_FreeCString(js,file);
  JSValue jszip = JS_NewObjectClass(js, js_writer_class_id);
  JS_SetOpaque(jszip, zip);
  return jszip;
}

static const JSCFunctionListEntry js_miniz_funcs[] = {
  JS_CFUNC_DEF("read", 1, js_miniz_write),
  JS_CFUNC_DEF("write", 1, js_miniz_read),
};

JSValue js_writer_add_file(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  mz_zip_archive *zip = js2writer(js,self);
  const char *file = JS_ToCString(js, argv[0]);
  mz_zip_writer_add_file(zip, file, file, NULL, 0, MZ_DEFAULT_COMPRESSION);
  JS_FreeCString(js,file);
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_writer_funcs[] = {
  JS_CFUNC_DEF("add_file", 1, js_writer_add_file),
};

JSValue js_reader_mod(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  const char *file = JS_ToCString(js,argv[0]);
  mz_zip_archive *zip = js2reader(js, self);
  mz_zip_archive_file_stat pstat;
  mz_uint index = mz_zip_reader_locate_file(zip, file, NULL, 0);
  JS_FreeCString(js,file);
  if (index == -1) return JS_UNDEFINED;
  
  mz_zip_reader_file_stat(zip, index, &pstat);
  return JS_NewFloat64(js, pstat.m_time);
}

JSValue js_reader_exists(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  const char *file = JS_ToCString(js,argv[0]);
  mz_zip_archive *zip = js2reader(js, self);
  mz_uint index = mz_zip_reader_locate_file(zip, file, NULL, 0);
  JS_FreeCString(js,file);
  if (index == -1) return JS_NewBool(js, 0);
  return JS_NewBool(js, 1);
}

JSValue js_reader_slurp(JSContext *js, JSValue self, int argc, JSValue *argv)
{
  const char *file = JS_ToCString(js,argv[0]);
  mz_zip_archive *zip = js2reader(js, self);
  mz_uint index = mz_zip_reader_locate_file(zip, file, NULL, 0);
  if (index == -1) {
    JS_FreeCString(js,file);
    return JS_UNDEFINED;
  }
  size_t len;
  void *data = mz_zip_reader_extract_file_to_heap(zip, file, &len, 0);
  JS_FreeCString(js,file);
  JSValue ret = JS_NewArrayBufferCopy(js, data, len);
  free(data);
  return ret;
}

static const JSCFunctionListEntry js_reader_funcs[] = {
    JS_CFUNC_DEF("mod", 1, js_reader_mod),
    JS_CFUNC_DEF("exists", 1, js_reader_exists),
    JS_CFUNC_DEF("slurp", 1, js_reader_slurp),
};


JSValue js_miniz(JSContext *js)
{
  JS_NewClassID(&js_reader_class_id);
  JS_NewClass(JS_GetRuntime(js), js_reader_class_id, &js_reader_class);
  JSValue reader_proto = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, reader_proto, js_reader_funcs, sizeof(js_reader_funcs) / sizeof(JSCFunctionListEntry));
  JS_SetClassProto(js, js_reader_class_id, reader_proto);

  JS_NewClassID(&js_writer_class_id);
  JS_NewClass(JS_GetRuntime(js), js_writer_class_id, &js_writer_class);
  JSValue writer_proto = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, writer_proto, js_writer_funcs, sizeof(js_writer_funcs) / sizeof(JSCFunctionListEntry));
  JS_SetClassProto(js, js_writer_class_id, writer_proto);
  
  JSValue export = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, export, js_miniz_funcs, sizeof(js_miniz_funcs)/sizeof(JSCFunctionListEntry));
  return export;
}

static int js_miniz_init(JSContext *ctx, JSModuleDef *m) {
  JS_SetModuleExport(ctx, m, "default",js_miniz(ctx));
  return 0;
}

#ifdef JS_SHARED_LIBRARY
#define JS_INIT_MODULE js_init_module
#else
#define JS_INIT_MODULE js_init_module_miniz
#endif

JSModuleDef *JS_INIT_MODULE(JSContext *ctx, const char *module_name) {
  JSModuleDef *m = JS_NewCModule(ctx, module_name, js_miniz_init);
  if (!m) return NULL;
  JS_AddModuleExport(ctx, m, "default");
  return m;
}
