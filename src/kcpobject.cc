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

#include <string.h>
#include <stdio.h>
#include "kcpobject.h"

#define RECV_BUFFER_SIZE 4194304

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
    using v8::Object;
    using v8::Persistent;
    using v8::String;
    using v8::Value;
    using v8::Exception;
    using v8::Null;

    Persistent<Function> KCPObject::constructor;

    int KCPObject::kcp_output(const char *buf, int len, ikcpcb *kcp, void *user)
    {
        KCPObject* thiz = (KCPObject*)user;
        const char* data = new char[len]{0};
        memcpy((void*)data, (void*)buf, len);
        const unsigned argc = 1;
        Local<Value> argv[argc] = {
            String::NewFromUtf8(Isolate::GetCurrent(), data)
        };
        thiz->output->Call(Null(Isolate::GetCurrent()), argc, argv);
        delete []data;
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
    }

    void KCPObject::Init(Local<Object> exports)
    {
        Isolate* isolate = exports->GetIsolate();

        Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
        tpl->SetClassName(String::NewFromUtf8(isolate, "KCPObject"));
        tpl->InstanceTemplate()->SetInternalFieldCount(1);

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
        NODE_SET_PROTOTYPE_METHOD(tpl, "rcvbufcount", RcvbufCount);
        NODE_SET_PROTOTYPE_METHOD(tpl, "sndbufcount", SndbufCount);

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
            kcpobj->Wrap(args.This());
            args.GetReturnValue().Set(args.This());
        } else {
            isolate->ThrowException(Exception::Error(
                String::NewFromUtf8(isolate, "Must to use new")
            ));
            return;
        }
    }

    void KCPObject::Recv(const v8::FunctionCallbackInfo<v8::Value>& args)
    {
        Isolate* isolate = args.GetIsolate();
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        char* buf = new char[RECV_BUFFER_SIZE]{0};
        int t = ikcp_recv(thiz->kcp, buf, RECV_BUFFER_SIZE);
        if (t >= 0) {
            args.GetReturnValue().Set(
                String::NewFromUtf8(isolate, (const char*)buf)
            );
        }
        delete []buf;
    }

    void KCPObject::Input(const v8::FunctionCallbackInfo<v8::Value>& args)
    {
        if (!args[0]->IsString()) {
            return;
        }
        String::Utf8Value data(args[0]);
        int len = data.length();
        if (0 == len) {
            return;
        }
        Isolate* isolate = args.GetIsolate();
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        char* buf = new char[len+1]{0};
        strcpy(buf, *data);
        int t = ikcp_input(thiz->kcp, (const char*)buf, len);
        Local<Number> ret = Number::New(isolate, t);
        args.GetReturnValue().Set(ret);
        delete []buf;
    }

    void KCPObject::Send(const v8::FunctionCallbackInfo<v8::Value>& args)
    {
        if (!args[0]->IsString()) {
            return;
        }
        String::Utf8Value data(args[0]);
        int len = data.length();
        if (0 == len) {
            return;
        }
        Isolate* isolate = args.GetIsolate();
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        char* buf = new char[len+1]{0};
        strcpy(buf, *data);
        int t = ikcp_send(thiz->kcp, (const char*)buf, len);
        Local<Number> ret = Number::New(isolate, t);
        args.GetReturnValue().Set(ret);
        delete []buf;
    }

    void KCPObject::Output(const FunctionCallbackInfo<Value>& args)
    {
        if (!args[0]->IsFunction()) {
            return;
        }
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        thiz->output = Local<Function>::Cast(args[0]);
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
        uint32_t current = args[0]->Uint32Value();
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
        uint32_t current = args[0]->Uint32Value();
        IUINT32 ret = ikcp_check(thiz->kcp, current);
        Local<Number> num = Number::New(isolate, ret);
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

    void KCPObject::RcvbufCount(const v8::FunctionCallbackInfo<v8::Value>& args)
    {
        Isolate* isolate = args.GetIsolate();
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        Local<Number> ret = Number::New(isolate, ikcp_rcvbuf_count(thiz->kcp));
        args.GetReturnValue().Set(ret);
    }

    void KCPObject::SndbufCount(const v8::FunctionCallbackInfo<v8::Value>& args)
    {
        Isolate* isolate = args.GetIsolate();
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(args.Holder());
        Local<Number> ret = Number::New(isolate, ikcp_sndbuf_count(thiz->kcp));
        args.GetReturnValue().Set(ret);
    }

}

