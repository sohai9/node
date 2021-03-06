// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "uv.h"
#include "node.h"
#include "node_internals.h"
#include "env-inl.h"

namespace node {
namespace {

using v8::Array;
using v8::Context;
using v8::FunctionCallbackInfo;
using v8::Integer;
using v8::Isolate;
using v8::Local;
using v8::Map;
using v8::Object;
using v8::String;
using v8::Value;


void ErrName(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args);
  if (env->options()->pending_deprecation && env->EmitErrNameWarning()) {
    if (ProcessEmitDeprecationWarning(
        env,
        "Directly calling process.binding('uv').errname(<val>) is being"
        " deprecated. "
        "Please make sure to use util.getSystemErrorName() instead.",
        "DEP0119").IsNothing())
    return;
  }
  int err;
  if (!args[0]->Int32Value(env->context()).To(&err)) return;
  CHECK_LT(err, 0);
  const char* name = uv_err_name(err);
  args.GetReturnValue().Set(OneByteString(env->isolate(), name));
}


void Initialize(Local<Object> target,
                Local<Value> unused,
                Local<Context> context,
                void* priv) {
  Environment* env = Environment::GetCurrent(context);
  Isolate* isolate = env->isolate();
  target->Set(env->context(),
              FIXED_ONE_BYTE_STRING(isolate, "errname"),
              env->NewFunctionTemplate(ErrName)
                  ->GetFunction(env->context())
                  .ToLocalChecked()).FromJust();

#define V(name, _) NODE_DEFINE_CONSTANT(target, UV_##name);
  UV_ERRNO_MAP(V)
#undef V

  Local<Map> err_map = Map::New(isolate);

#define V(name, msg) do {                                                     \
  Local<Value> arr[] = {                                                      \
    OneByteString(isolate, #name),                                            \
    OneByteString(isolate, msg)                                               \
  };                                                                          \
  if (err_map->Set(context,                                                   \
                   Integer::New(isolate, UV_##name),                          \
                   Array::New(isolate, arr, arraysize(arr))).IsEmpty()) {     \
    return;                                                                   \
  }                                                                           \
} while (0);
  UV_ERRNO_MAP(V)
#undef V

  target->Set(context, FIXED_ONE_BYTE_STRING(isolate, "errmap"),
              err_map).FromJust();
}

}  // anonymous namespace
}  // namespace node

NODE_MODULE_CONTEXT_AWARE_INTERNAL(uv, node::Initialize)
