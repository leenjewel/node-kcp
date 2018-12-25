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

    int KCPObject::kcp_output(const char *buf, int len, ikcpcb *kcp, void *user)
    {
        KCPObject* thiz = (KCPObject*)user;
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
            callback.Call(argc, argv);
        } else {
            const unsigned argc = 3;
            Local<Value> argv[argc] = {
                Nan::CopyBuffer(buf, len).ToLocalChecked(),
                Nan::New<Number>(len),
                Nan::New<Object>(thiz->context)
            };
            Callback callback(Nan::New<Function>(thiz->output));
            callback.Call(argc, argv);
        }
        return len;
    }

    KCPObject::KCPObject(IUINT32 conv)
    {
        kcp = ikcp_create(conv, this);
        kcp->output = KCPObject::kcp_output;
        recvBuff = (char*)realloc(recvBuff, recvBuffSize);
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
        if (!output.IsEmpty()) {
            output.Reset();
        }
        if (!context.IsEmpty()) {
            context.Reset();
        }
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

    NAN_METHOD(KCPObject::GetContext)
    {
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
        if (!thiz->context.IsEmpty()) {
            info.GetReturnValue().Set(Nan::New<Object>(thiz->context));
        }
    }

    NAN_METHOD(KCPObject::Release)
    {
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
        if (thiz->kcp) {
            ikcp_release(thiz->kcp);
            thiz->kcp = NULL;
        }
        if (thiz->recvBuff) {
            free(thiz->recvBuff);
            thiz->recvBuff = NULL;
        }
    }

    NAN_METHOD(KCPObject::Recv)
    {
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
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
                    Nan::ThrowError("realloc error");
                    len = 0;
                    break;
                }
            }

            buflen = ikcp_recv(thiz->kcp, thiz->recvBuff + len, bufsize);
            if (buflen <= 0) {
                break;
            }
            len += buflen;
        }
        if (len > 0) {
            info.GetReturnValue().Set(
                Nan::CopyBuffer((const char*)thiz->recvBuff, len).ToLocalChecked()
            );
        }
    }

    NAN_METHOD(KCPObject::Input)
    {
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
        char* buf = NULL;
        int len = 0;
        Local<Value> arg0 = info[0];
        if (arg0->IsString()) {
            String::Value data(arg0);
            len = data.length();
            if (0 == len) {
                return;
            }
            if (!(buf = (char*)malloc(len))) {
                Nan::ThrowError("malloc error");
                return;
            }
            string2char(data, len, buf);
            int t = ikcp_input(thiz->kcp, (const char*)buf, len);
            Local<Number> ret = Nan::New(t);
            info.GetReturnValue().Set(ret);
            free(buf);
        } else if (node::Buffer::HasInstance(arg0)) {
            len = node::Buffer::Length(arg0->ToObject());
            if (0 == len) {
                return;
            }
            buf = node::Buffer::Data(arg0->ToObject());
            int t = ikcp_input(thiz->kcp, (const char*)buf, len);
            Local<Number> ret = Nan::New(t);
            info.GetReturnValue().Set(ret);
        }
    }

    NAN_METHOD(KCPObject::Send)
    {
        KCPObject* thiz = ObjectWrap::Unwrap<KCPObject>(info.Holder());
        char* buf = NULL;
        int len = 0;
        Local<Value> arg0 = info[0];
        if (arg0->IsString()) {
            String::Value data(arg0);
            len = data.length();
            if (0 == len) {
                return;
            }
            if (!(buf = (char*)malloc(len))) {
                Nan::ThrowError("malloc error");
                return;
            }
            string2char(data, len, buf);
            int t = ikcp_send(thiz->kcp, (const char*)buf, len);
            Local<Number> ret = Nan::New(t);
            info.GetReturnValue().Set(ret);
            free(buf);
        } else if (node::Buffer::HasInstance(arg0)) {
            len = node::Buffer::Length(arg0->ToObject());
            if (0 == len) {
                return;
            }
            buf = node::Buffer::Data(arg0->ToObject());
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

}

