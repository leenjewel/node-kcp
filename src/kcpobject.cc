/**
 * Copyright 2021 leenjewel
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

namespace node_kcp
{
#ifdef USE_NAPI
    using namespace Napi;
#else
    using v8::Context;
    using v8::Function;
    using Nan::FunctionCallbackInfo;
    using v8::FunctionTemplate;
    using v8::Local;
    using Nan::MaybeLocal;
    using v8::Number;
    using v8::Integer;
    using v8::Object;
    using Nan::Persistent;
    using v8::String;
    using v8::Value;
    using v8::Exception;
    using Nan::Null;
    using Nan::Callback;
    using Nan::GetFunction;
    using Nan::Set;
    using Nan::To;

    Persistent<Function> KCPObject::constructor;
#endif

    int KCPObject::kcp_output(const char *buf, int len, ikcpcb *kcp, void *user)
    {
        KCPObject* thiz = (KCPObject*)user;
#ifdef USE_NAPI
        if (!thiz->outputFunctionHasSet) {
            return len;
        }
        if (thiz->contextObjectHasSet) {
            thiz->output.NonBlockingCall
        }
#else
        if (thiz->output.IsEmpty()) {
            return len;
        }
        if (thiz->context.IsEmpty()) {
            const unsigned argc = 2;
            Local<Value> argv[argc] = {
                Nan::CopyBuffer(buf, len).ToLocalChecked(),
                Nan::New<Number>(len)
            };
            Callback callback(Nan::New<Function>(thiz->output));
            Nan::Call(callback, argc, argv);
        } else {
            const unsigned argc = 3;
            Local<Value> argv[argc] = {
                Nan::CopyBuffer(buf, len).ToLocalChecked(),
                Nan::New<Number>(len),
                Nan::New<Object>(thiz->context)
            };
            Callback callback(Nan::New<Function>(thiz->output));
            Nan::Call(callback, argc, argv);
        }
#endif
        return len;
    }

    KCPObject::~KCPObject()
    {
        if (kcp) {
            ikcp_release(kcp);
            kcp = NULL;
        }
        if (recvBuff) {
            free(recvBuff);
            recvBuff = NULL;
        }
        if (!context.IsEmpty()) {
            context.Reset();
        }
#ifdef USE_NAPI
#else
        if (!output.IsEmpty()) {
            output.Reset();
        }
#endif
    }

#ifdef USE_NAPI
    KCPObject::KCPObject(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<KCPObject>(info)
    {
        Napi::Env env = info.Env();
        int length = info.Length();
        if (length <= 0 || !info[0].IsNumber()) {
            Napi::TypeError::New(env, "kcp.KCP 1 arg must be number").ThrowAsJavaScriptException();
            return;
        }
        Napi::Number value = info[0].As<Napi::Number>();
        kcp = ikcp_create(value.Int32Value(), this);
        kcp->output = KCPObject::kcp_output;
        recvBuff = (char*)realloc(recvBuff, recvBuffSize);
        if (length > 1 && info[1].IsObject()) {
            this->context = Napi::Persistent(info[1].As<Napi::Object>());
            this->contextObjectHasSet = true;
        }
    }

    Napi::Object KCPObject::Init(Napi::Env env, Napi::Object exports)
    {
        Napi::Function func = DefineClass(env,
            "KCP",
            {
                InstanceMethod("release", &KCPObject::Release),
                InstanceMethod("context", &KCPObject::GetContext),
                InstanceMethod("recv", &KCPObject::Recv),
                InstanceMethod("send", &KCPObject::Send),
                InstanceMethod("input", &KCPObject::Input),
                InstanceMethod("output", &KCPObject::Output),
                InstanceMethod("update", &KCPObject::Update),
                InstanceMethod("check", &KCPObject::Check),
                InstanceMethod("flush", &KCPObject::Flush),
                InstanceMethod("peeksize", &KCPObject::Peeksize),
                InstanceMethod("setmtu", &KCPObject::Setmtu),
                InstanceMethod("wndsize", &KCPObject::Wndsize),
                InstanceMethod("waitsnd", &KCPObject::Waitsnd),
                InstanceMethod("nodelay", &KCPObject::Nodelay),
                InstanceMethod("stream", &KCPObject::Stream)
            }
        );

        Napi::FunctionReference* constructor = new Napi::FunctionReference();
        *constructor = Napi::Persistent(func);
        env.SetInstanceData(constructor);

        exports.Set("KCP", func);
        return exports;
    }
#else
    KCPObject::KCPObject(IUINT32 conv)
    {
        kcp = ikcp_create(conv, this);
        kcp->output = KCPObject::kcp_output;
        recvBuff = (char*)realloc(recvBuff, recvBuffSize);
    }

    NAN_MODULE_INIT(KCPObject::Init)
    {
        Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
        tpl->SetClassName(Nan::New("KCPObject").ToLocalChecked());
        tpl->InstanceTemplate()->SetInternalFieldCount(1);

        SetPrototypeMethod(tpl, "release", Release);
        SetPrototypeMethod(tpl, "context", GetContext);
        SetPrototypeMethod(tpl, "recv", Recv);
        SetPrototypeMethod(tpl, "send", Send);
        SetPrototypeMethod(tpl, "input", Input);
        SetPrototypeMethod(tpl, "output", Output);
        SetPrototypeMethod(tpl, "update", Update);
        SetPrototypeMethod(tpl, "check", Check);
        SetPrototypeMethod(tpl, "flush", Flush);
        SetPrototypeMethod(tpl, "peeksize", Peeksize);
        SetPrototypeMethod(tpl, "setmtu", Setmtu);
        SetPrototypeMethod(tpl, "wndsize", Wndsize);
        SetPrototypeMethod(tpl, "waitsnd", Waitsnd);
        SetPrototypeMethod(tpl, "nodelay", Nodelay);
        SetPrototypeMethod(tpl, "stream", Stream);

        constructor.Reset(GetFunction(tpl).ToLocalChecked());
        Set(target, Nan::New("KCP").ToLocalChecked(), GetFunction(tpl).ToLocalChecked());
    }

    NAN_METHOD(KCPObject::New)
    {
        if (!info[0]->IsNumber()) {
            Nan::ThrowTypeError("kcp.KCP 1 arg must be number");
            return;
        }
        if (info.IsConstructCall()) {
            uint32_t conv = To<uint32_t>(info[0]).FromJust();
            KCPObject* kcpobj = new KCPObject(conv);
            if (info[1]->IsObject()) {
                kcpobj->context.Reset(Local<Object>::Cast(info[1]));
            }
            kcpobj->Wrap(info.This());
            info.GetReturnValue().Set(info.This());
        } else {
            Local<Value> argv[2] = {
                info[0],
                info[1]
            };
            Local<Function> cons = Nan::New(constructor);
            info.GetReturnValue().Set(Nan::NewInstance(cons, 2, argv).ToLocalChecked());
        }
    }
#endif

/////////////////////////////////////////////////////////////////////
// KCPObject::GetContext
/////////////////////////////////////////////////////////////////////
#ifdef USE_NAPI
    Napi::Value KCPObject::GetContext(const Napi::CallbackInfo& info)
    {
        if (this->contextObjectHasSet) {
            return this->context.Value();
        }
        return info.Env().Null();
    }
#else
    NAN_METHOD(KCPObject::GetContext)
    {
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
        if (!thiz->context.IsEmpty()) {
            info.GetReturnValue().Set(Nan::New<Object>(thiz->context));
        }
    }
#endif

/////////////////////////////////////////////////////////////////////
// KCPObject::Release
/////////////////////////////////////////////////////////////////////
#ifdef USE_NAPI
    Napi::Value KCPObject::Release(const Napi::CallbackInfo& info) {
        auto thiz = this;
#else
    NAN_METHOD(KCPObject::Release)
    {
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
#endif
        if (thiz->kcp) {
            ikcp_release(thiz->kcp);
            thiz->kcp = NULL;
        }
        if (thiz->recvBuff) {
            free(thiz->recvBuff);
            thiz->recvBuff = NULL;
        }
#ifdef USE_NAPI
        return info.Env().Undefined();
#endif
    }
/////////////////////////////////////////////////////////////////////
// KCPObject::Recv
/////////////////////////////////////////////////////////////////////
#ifdef USE_NAPI
    Napi::Value KCPObject::Recv(const Napi::CallbackInfo& info) {
        auto thiz = this;
#else
    NAN_METHOD(KCPObject::Recv)
    {
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
#endif
        int bufsize = 0;
        unsigned int allsize = 0;
        int buflen = 0;
        int len = 0;
        while(1) {
            bufsize = ikcp_peeksize(thiz->kcp);
            if (bufsize <= 0) {
                break;
            }
            allsize += bufsize;
            if (allsize > thiz->recvBuffSize) {
                int align = allsize % 4;
                if (align) {
                    allsize += 4 - align;
                }
                thiz->recvBuffSize = allsize;
                thiz->recvBuff = (char*)realloc(thiz->recvBuff, thiz->recvBuffSize);
                if (!thiz->recvBuff) {
#ifdef USE_NAPI
                    Napi::TypeError::New(info.Env(),
                    "realloc error").ThrowAsJavaScriptException();
#else
                    Nan::ThrowError("realloc error");
#endif
                    len = 0;
                    break;
                }
            }

            buflen = ikcp_recv(thiz->kcp, thiz->recvBuff + len, bufsize);
            if (buflen <= 0) {
                break;
            }
            len += buflen;
            if (thiz->kcp->stream == 0) {
                break;
            }
        }
        if (len > 0) {
#ifdef USE_NAPI
            return Napi::String::New(info.Env(), thiz->recvBuff, len);
#else
            info.GetReturnValue().Set(
                Nan::CopyBuffer((const char*)thiz->recvBuff, len).ToLocalChecked()
            );
#endif
        }
#ifdef USE_NAPI
        return info.Env().Null();
#endif
    }
/////////////////////////////////////////////////////////////////////
// KCPObject::Input
/////////////////////////////////////////////////////////////////////
#ifdef USE_NAPI
    Napi::Value KCPObject::Input(const Napi::CallbackInfo& info) {
        Napi::Value arg0 = info[0];
        if (arg0.IsString()) {
            std::string data = arg0.As<Napi::String>().Utf8Value();
            if (0 == data.length()) {
                Napi::TypeError::New(info.Env(),
                "Input is empty").ThrowAsJavaScriptException();
                return;
            }
            int t = ikcp_input(this->kcp, data.c_str(), data.length());
            return Napi::Number::New(info.Env(), t);
        } else if (arg0.IsBuffer()) {
            Napi::Buffer<> buffer = arg0.As<Napi::Buffer>();
        }
    }
#else
    NAN_METHOD(KCPObject::Input)
    {
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
        char* buf = NULL;
        int len = 0;
        Local<Value> arg0 = info[0];
        if (arg0->IsString()) {
            Nan::Utf8String data(arg0);
            len = data.length();
            if (0 == len) {
                Nan::ThrowError("INput Nan Utf8String error");
                return;
            }
            int t = ikcp_input(thiz->kcp, *data, len);
            Local<Number> ret = Nan::New(t);
            info.GetReturnValue().Set(ret);
            free(buf);
        } else if (node::Buffer::HasInstance(arg0)) {
            Nan::MaybeLocal<v8::Object> maybeObj = Nan::To<v8::Object>(arg0);
            v8::Local<Object> obj;
            if (!maybeObj.ToLocal(&obj)) {
                return;
            }
            len = node::Buffer::Length(obj);
            if (0 == len) {
                return;
            }
            buf = node::Buffer::Data(obj);
            int t = ikcp_input(thiz->kcp, (const char*)buf, len);
            Local<Number> ret = Nan::New(t);
            info.GetReturnValue().Set(ret);
        }
    }
#endif

    NAN_METHOD(KCPObject::Send)
    {
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
        char* buf = NULL;
        int len = 0;
        Local<Value> arg0 = info[0];
        if (arg0->IsString()) {
            Nan::Utf8String data(arg0);
            len = data.length();
            if (0 == len) {
                Nan::ThrowError("Send Nan Utf8String error");
                return;
            }
            int t = ikcp_send(thiz->kcp, *data, len);
            Local<Number> ret = Nan::New(t);
            info.GetReturnValue().Set(ret);
            free(buf);
        } else if (node::Buffer::HasInstance(arg0)) {
            Nan::MaybeLocal<v8::Object> maybeObj = Nan::To<v8::Object>(arg0);
            v8::Local<Object> obj;
            if (!maybeObj.ToLocal(&obj)) {
                return;
            }
            // len = node::Buffer::Length(arg0->ToObject(isolate));
            len = node::Buffer::Length(obj);
            if (0 == len) {
                return;
            }
            // buf = node::Buffer::Data(arg0->ToObject(isolate));
            buf = node::Buffer::Data(obj);
            int t = ikcp_send(thiz->kcp, (const char*)buf, len);
            Local<Number> ret = Nan::New(t);
            info.GetReturnValue().Set(ret);
        }
    }

    NAN_METHOD(KCPObject::Output)
    {
        if (!info[0]->IsFunction()) {
            return;
        }
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
        thiz->output.Reset(Local<Function>::Cast(info[0]));
    }

    NAN_METHOD(KCPObject::Update)
    {
        if (!info[0]->IsNumber()) {
            Nan::ThrowTypeError("KCP update first argument must be number");
            return;
        }
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
        int64_t arg0 = To<int64_t>(info[0]).FromJust();
        IUINT32 current = (IUINT32)(arg0 & 0xfffffffful);
        ikcp_update(thiz->kcp, current);
    }

    NAN_METHOD(KCPObject::Check)
    {
        if (!info[0]->IsNumber()) {
            Nan::ThrowTypeError("KCP check first argument must be number");
            return;
        }

        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
        int64_t arg0 = To<int64_t>(info[0]).FromJust();
        IUINT32 current = (IUINT32)(arg0 & 0xfffffffful);
        IUINT32 ret = ikcp_check(thiz->kcp, current) - current;
        Local<Integer> num = Nan::New((uint32_t)(ret>0?ret:0));
        info.GetReturnValue().Set(num);
    }

    NAN_METHOD(KCPObject::Flush)
    {
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
        ikcp_flush(thiz->kcp);
    }

    NAN_METHOD(KCPObject::Peeksize)
    {
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
        Local<v8::Int32> ret = Nan::New(ikcp_peeksize(thiz->kcp));
        info.GetReturnValue().Set(ret);
    }

    NAN_METHOD(KCPObject::Setmtu)
    {
        int mtu = 1400;
        if (info[0]->IsNumber()) {
            mtu = To<int>(info[0]).FromJust();
        }
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
        Local<v8::Int32> ret = Nan::New(ikcp_setmtu(thiz->kcp, mtu));
        info.GetReturnValue().Set(ret);
    }

    NAN_METHOD(KCPObject::Wndsize)
    {
        int sndwnd = 32;
        int rcvwnd = 32;
        if (info[0]->IsNumber()) {
            sndwnd = To<int>(info[0]).FromJust();
        }
        if (info[1]->IsNumber()) {
            rcvwnd = To<int>(info[1]).FromJust();
        }
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
        Local<v8::Int32> ret = Nan::New(ikcp_wndsize(thiz->kcp, sndwnd, rcvwnd));
        info.GetReturnValue().Set(ret);
    }

    NAN_METHOD(KCPObject::Waitsnd)
    {
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
        Local<v8::Int32> ret = Nan::New(ikcp_waitsnd(thiz->kcp));
        info.GetReturnValue().Set(ret);
    }

    NAN_METHOD(KCPObject::Nodelay)
    {
        int nodelay = 0;
        int interval = 100;
        int resend = 0;
        int nc = 0;
        if (info[0]->IsNumber()) {
            nodelay = To<int>(info[0]).FromJust();
        }
        if (info[1]->IsNumber()) {
            interval = To<int>(info[1]).FromJust();
        }
        if (info[2]->IsNumber()) {
            resend = To<int>(info[2]).FromJust();
        }
        if (info[3]->IsNumber()) {
            nc = To<int>(info[3]).FromJust();
        }
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
        Local<v8::Int32> ret = Nan::New(ikcp_nodelay(thiz->kcp, nodelay, interval, resend, nc));
        info.GetReturnValue().Set(ret);
    }

    NAN_METHOD(KCPObject::Stream)
    {
        if (info[0]->IsNumber()) {
            int stream = To<int>(info[0]).FromJust();
            KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
            thiz->kcp->stream = stream;
            Local<v8::Int32> ret = Nan::New(thiz->kcp->stream);
            info.GetReturnValue().Set(ret);
        }
    }

}

