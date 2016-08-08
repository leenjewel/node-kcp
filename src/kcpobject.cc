/**
 * Copyright 2016 leenjewel
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "kcpobject.h"
#include "node_buffer.h"
#include <string.h>

#define RECV_BUFFER_SIZE 4096

#define string2char(string, len, out) \
    do {\
    int __i__ = 0;\
    for (; __i__ < len; __i__++) {\
        out[__i__] = (char)((*string)[__i__]);\
    }\
    } while(0)

namespace node_kcp
{
    using v8::Context;
    using v8::Function;
    using v8::FunctionCallbackInfo;
    using v8::FunctionTemplate;
    using v8::Isolate;
    using v8::Local;
    using v8::MaybeLocal;
    using v8::Number;
    using v8::Integer;
    using v8::Object;
    using v8::Persistent;
    using v8::String;
    using v8::Value;
    using v8::Exception;
    using v8::Null;

    Persistent<Function> KCPObject::constructor;

    int KCPObject::kcp_output(const char *buf, int len, ikcpcb *kcp, void *user)
    {
        Isolate* isolate = Isolate::GetCurrent();
        KCPObject* thiz = (KCPObject*)user;
        if (thiz->output.IsEmpty()) {
            return len;
        }
        if (thiz->context.IsEmpty()) {
            const unsigned argc = 2;
            Local<Value> argv[argc] = {
                node::Buffer::Copy(isolate, buf, len).ToLocalChecked(),
                Number::New(isolate, len)
            };
            Local<Function> callback = Local<Function>::New(isolate, thiz->output);
            callback->Call(Null(isolate), argc, argv);
        } else {
            const unsigned argc = 3;
            Local<Value> argv[argc] = {
                node::Buffer::Copy(isolate, buf, len).ToLocalChecked(),
                Number::New(isolate, len),
                Local<Object>::New(isolate, thiz->context)
            };
            Local<Function> callback = Local<Function>::New(isolate, thiz->output);
            callback->Call(Null(isolate), argc, argv);
        }
        return len;
    }

    KCPObject::KCPObject(IUINT32 conv, void* user)
    {
        kcp = ikcp_create(conv, this);
        kcp->output = KCPObject::kcp_output;
    }

    KCPObject::~KCPObject()
    {
        if (kcp) {
            ikcp_release(kcp);
            kcp = NULL;
        }
        if (!output.IsEmpty()) {
            output.Reset();
        }
        if (!context.IsEmpty()) {
            context.Reset();
        }
    }

    void KCPObject::Init(Local<Object> exports)
    {
        Isolate* isolate = exports->GetIsolate();

        Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
        tpl->SetClassName(String::NewFromUtf8(isolate, "KCPObject"));
        tpl->InstanceTemplate()->SetInternalFieldCount(1);

        NODE_SET_PROTOTYPE_METHOD(tpl, "release", Release);
        NODE_SET_PROTOTYPE_METHOD(tpl, "context", GetContext);
        NODE_SET_PROTOTYPE_METHOD(tpl, "recv", Recv);
        NODE_SET_PROTOTYPE_METHOD(tpl, "send", Send);
        NODE_SET_PROTOTYPE_METHOD(tpl, "input", Input);
        NODE_SET_PROTOTYPE_METHOD(tpl, "output", Output);
        NODE_SET_PROTOTYPE_METHOD(tpl, "update", Update);
        NODE_SET_PROTOTYPE_METHOD(tpl, "check", Check);
        NODE_SET_PROTOTYPE_METHOD(tpl, "flush", Flush);
        NODE_SET_PROTOTYPE_METHOD(tpl, "peeksize", Peeksize);
        NODE_SET_PROTOTYPE_METHOD(tpl, "setmtu", Setmtu);
        NODE_SET_PROTOTYPE_METHOD(tpl, "wndsize", Wndsize);
        NODE_SET_PROTOTYPE_METHOD(tpl, "waitsnd", Waitsnd);
        NODE_SET_PROTOTYPE_METHOD(tpl, "nodelay", Nodelay);

        constructor.Reset(isolate, tpl->GetFunction());
        exports->Set(String::NewFromUtf8(isolate, "KCP"),
                tpl->GetFunction());
    }

    void KCPObject::New(const FunctionCallbackInfo<Value>& args)
    {
        Isolate* isolate = args.GetIsolate();

        if (!args[0]->IsNumber()) {
            isolate->ThrowException(Exception::TypeError(
                String::NewFromUtf8(isolate, "Wrong arguments")
            ));
            return;
        }
        if (args.IsConstructCall()) {
            uint32_t conv = args[0]->Uint32Value();
            KCPObject* kcpobj = new KCPObject(conv, 0);
            if (args[1]->IsObject()) {
                kcpobj->context.Reset(isolate, Local<Object>::Cast(args[1]));
            }
            kcpobj->Wrap(args.This());
            args.GetReturnValue().Set(args.This());
        } else {
            Local<Value> argv[2] = {
                args[0],
                args[1]
            };
            Local<Context> context = isolate->GetCurrentContext();
            Local<Function> cons = Local<Function>::New(isolate, constructor);
            Local<Object> ret = cons->NewInstance(context, 2, argv).ToLocalChecked();
            args.GetReturnValue().Set(ret);
        }
    }

    void KCPObject::GetContext(const v8::FunctionCallbackInfo<v8::Value>& args)
    {
        Isolate* isolate = args.GetIsolate();
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        if (!thiz->context.IsEmpty()) {
            args.GetReturnValue().Set(Local<Object>::New(isolate, thiz->context));
        }
    }

    void KCPObject::Release(const v8::FunctionCallbackInfo<v8::Value>& args)
    {
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        delete thiz;
    }

    void KCPObject::Recv(const v8::FunctionCallbackInfo<v8::Value>& args)
    {
        Isolate* isolate = args.GetIsolate();
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        int bufsize = 0;
        char* buf = NULL;
        int buflen = 0;
        char* data = NULL;
        int len = 0;
        char* tmp = NULL;
        int tmplen = 0;
        while(1) {
            tmplen = buflen;
            bufsize = ikcp_peeksize(thiz->kcp);
            if (bufsize <= 0) {
                bufsize = RECV_BUFFER_SIZE;
            }
            buf = (char*)malloc(bufsize);
            buflen = ikcp_recv(thiz->kcp, buf, bufsize);
            if (buflen < 0) {
                free(buf);
                break;
            }
            len += buflen;
            tmp = data;
            data = (char*)malloc(len);
            if (NULL != tmp) {
                memcpy(data, tmp, tmplen);
                free(tmp);
            }
            memcpy(data, buf, buflen);
            free(buf);
        }
        if (len > 0) {
            args.GetReturnValue().Set(
                node::Buffer::Copy(isolate, (const char*)data, len).ToLocalChecked()
            );
        }
        free(data);
    }

    void KCPObject::Input(const v8::FunctionCallbackInfo<v8::Value>& args)
    {
        Isolate* isolate = args.GetIsolate();
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        char* buf = NULL;
        int len = 0;
        Local<Value> arg0 = args[0];
        if (arg0->IsString()) {
            String::Value data(arg0);
            len = data.length();
            if (0 == len) {
                return;
            }
            if (!(buf = (char*)malloc(len))) {
                isolate->ThrowException(Exception::Error(
                    String::NewFromUtf8(isolate, "malloc error")
                ));
                return;
            }
            string2char(data, len, buf);
            int t = ikcp_input(thiz->kcp, (const char*)buf, len);
            Local<Number> ret = Number::New(isolate, t);
            args.GetReturnValue().Set(ret);
            free(buf);
        } else if (node::Buffer::HasInstance(arg0)) {
            len = node::Buffer::Length(arg0->ToObject());
            if (0 == len) {
                return;
            }
            buf = node::Buffer::Data(arg0->ToObject());
            int t = ikcp_input(thiz->kcp, (const char*)buf, len);
            Local<Number> ret = Number::New(isolate, t);
            args.GetReturnValue().Set(ret);
        }
    }

    void KCPObject::Send(const v8::FunctionCallbackInfo<v8::Value>& args)
    {
        Isolate* isolate = args.GetIsolate();
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        char* buf = NULL;
        int len = 0;
        Local<Value> arg0 = args[0];
        if (arg0->IsString()) {
            String::Value data(arg0);
            len = data.length();
            if (0 == len) {
                return;
            }
            if (!(buf = (char*)malloc(len))) {
                isolate->ThrowException(Exception::Error(
                    String::NewFromUtf8(isolate, "malloc error")
                ));
                return;
            }
            string2char(data, len, buf);
            int t = ikcp_send(thiz->kcp, (const char*)buf, len);
            Local<Number> ret = Number::New(isolate, t);
            args.GetReturnValue().Set(ret);
            free(buf);
        } else if (node::Buffer::HasInstance(arg0)) {
            len = node::Buffer::Length(arg0->ToObject());
            if (0 == len) {
                return;
            }
            buf = node::Buffer::Data(arg0->ToObject());
            int t = ikcp_send(thiz->kcp, (const char*)buf, len);
            Local<Number> ret = Number::New(isolate, t);
            args.GetReturnValue().Set(ret);
        }
    }

    void KCPObject::Output(const FunctionCallbackInfo<Value>& args)
    {
        if (!args[0]->IsFunction()) {
            return;
        }
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        thiz->output.Reset(args.GetIsolate(), Local<Function>::Cast(args[0]));
    }

    void KCPObject::Update(const FunctionCallbackInfo<Value>& args)
    {
        Isolate* isolate = args.GetIsolate();

        if (!args[0]->IsNumber()) {
            isolate->ThrowException(Exception::TypeError(
                String::NewFromUtf8(isolate, "Wrong arguments")
            ));
            return;
        }
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        auto arg0 = args[0]->IntegerValue();
        IUINT32 current = (IUINT32)(arg0 & 0xfffffffful);
        ikcp_update(thiz->kcp, current);
    }

    void KCPObject::Check(const FunctionCallbackInfo<Value>& args)
    {
        Isolate* isolate = args.GetIsolate();

        if (!args[0]->IsNumber()) {
            isolate->ThrowException(Exception::TypeError(
                String::NewFromUtf8(isolate, "Wrong arguments")
            ));
            return;
        }

        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        auto arg0 = args[0]->IntegerValue();
        IUINT32 current = (IUINT32)(arg0 & 0xfffffffful);
        IUINT32 ret = ikcp_check(thiz->kcp, current) - current;
        Local<Integer> num = Integer::NewFromUnsigned(isolate, (uint32_t)(ret>0?ret:0));
        args.GetReturnValue().Set(num);
    }

    void KCPObject::Flush(const FunctionCallbackInfo<Value>& args)
    {
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        ikcp_flush(thiz->kcp);
    }

    void KCPObject::Peeksize(const FunctionCallbackInfo<Value>& args)
    {
        Isolate* isolate = args.GetIsolate();
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        Local<Number> ret = Number::New(isolate, ikcp_peeksize(thiz->kcp));
        args.GetReturnValue().Set(ret);
    }

    void KCPObject::Setmtu(const v8::FunctionCallbackInfo<v8::Value>& args)
    {
        Isolate* isolate = args.GetIsolate();

        int mtu = 1400;
        if (args[0]->IsNumber()) {
            mtu = args[0]->Int32Value();
        }
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        Local<Number> ret = Number::New(isolate, ikcp_setmtu(thiz->kcp, mtu));
        args.GetReturnValue().Set(ret);
    }

    void KCPObject::Wndsize(const v8::FunctionCallbackInfo<v8::Value>& args)
    {
        Isolate* isolate = args.GetIsolate();
        int sndwnd = 32;
        int rcvwnd = 32;
        if (args[0]->IsNumber()) {
            sndwnd = args[0]->Int32Value();
        }
        if (args[1]->IsNumber()) {
            rcvwnd = args[1]->Int32Value();
        }
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        Local<Number> ret = Number::New(isolate, ikcp_wndsize(thiz->kcp, sndwnd, rcvwnd));
        args.GetReturnValue().Set(ret);
    }

    void KCPObject::Waitsnd(const v8::FunctionCallbackInfo<v8::Value>& args)
    {
        Isolate* isolate = args.GetIsolate();
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        Local<Number> ret = Number::New(isolate, ikcp_waitsnd(thiz->kcp));
        args.GetReturnValue().Set(ret);
    }

    void KCPObject::Nodelay(const v8::FunctionCallbackInfo<v8::Value>& args)
    {
        Isolate* isolate = args.GetIsolate();
        int nodelay = 0;
        int interval = 100;
        int resend = 0;
        int nc = 0;
        if (args[0]->IsNumber()) {
            nodelay = args[0]->Int32Value();
        }
        if (args[1]->IsNumber()) {
            interval = args[1]->Int32Value();
        }
        if (args[2]->IsNumber()) {
            resend = args[2]->Int32Value();
        }
        if (args[3]->IsNumber()) {
            nc = args[3]->Int32Value();
        }
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        Local<Number> ret = Number::New(isolate, ikcp_nodelay(thiz->kcp, nodelay, interval, resend, nc));
        args.GetReturnValue().Set(ret);
    }

}

