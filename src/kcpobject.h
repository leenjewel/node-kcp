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

#ifndef KCPOBJECT_H
#define KCPOBJECT_H

#include "kcp/ikcp.h"

#ifdef USE_NAPI
#include <napi.h>
#define NAPI_METHOD(m) Napi::Value m(const Napi::CallbackInfo& info)
#else
#include <nan.h>
#include <nan_object_wrap.h>
#endif

namespace node_kcp {

#ifdef USE_NAPI
    class KCPObject : public Napi::ObjectWrap<KCPObject>
#else
    class KCPObject : public Nan::ObjectWrap
#endif
    {
        public:
#ifdef USE_NAPI
            KCPObject(const Napi::CallbackInfo&);
            static Napi::Object Init(Napi::Env env, Napi::Object exports);
#else
            static NAN_MODULE_INIT(Init);
            explicit KCPObject(IUINT32 conv);
#endif
        private:
            ~KCPObject();
            static int kcp_output(const char *buf, int len, ikcpcb *kcp, void *user);
            ikcpcb* kcp;
            char *recvBuff = NULL;
            unsigned int recvBuffSize = 1024;
#ifdef USE_NAPI
            Napi::ThreadSafeFunction output;
            bool outputFunctionHasSet = false;
            Napi::ObjectReference context;
            bool contextObjectHasSet = false;

            NAPI_METHOD(Release);
            NAPI_METHOD(GetContext);
            NAPI_METHOD(Recv);
            NAPI_METHOD(Send);
            NAPI_METHOD(Output);
            NAPI_METHOD(Input);
            NAPI_METHOD(Update);
            NAPI_METHOD(Check);
            NAPI_METHOD(Flush);
            NAPI_METHOD(Peeksize);
            NAPI_METHOD(Setmtu);
            NAPI_METHOD(Wndsize);
            NAPI_METHOD(Waitsnd);
            NAPI_METHOD(Nodelay);
            NAPI_METHOD(Stream);
#else
            static NAN_METHOD(New);
            static NAN_METHOD(Release);
            static NAN_METHOD(GetContext);
            static NAN_METHOD(Recv);
            static NAN_METHOD(Send);
            static NAN_METHOD(Output);
            static NAN_METHOD(Input);
            static NAN_METHOD(Update);
            static NAN_METHOD(Check);
            static NAN_METHOD(Flush);
            static NAN_METHOD(Peeksize);
            static NAN_METHOD(Setmtu);
            static NAN_METHOD(Wndsize);
            static NAN_METHOD(Waitsnd);
            static NAN_METHOD(Nodelay);
            static NAN_METHOD(Stream);
            static Nan::Persistent<v8::Function> constructor;
            Nan::Persistent<v8::Function> output;
            Nan::Persistent<v8::Object> context;
#endif
    };

}

#endif
