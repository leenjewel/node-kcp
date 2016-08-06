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

#ifndef KCPOBJECT_H
#define KCPOBJECT_H

#include <node.h>
#include <node_object_wrap.h>
#include "kcp/ikcp.h"

namespace node_kcp {

    class KCPObject : public node::ObjectWrap
    {
        public:
            static void Init(v8::Local<v8::Object> exports);

        private:
            explicit KCPObject(IUINT32 conv, void* user);
            ~KCPObject();

            static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
            static void Release(const v8::FunctionCallbackInfo<v8::Value>& args);
            static void GetContext(const v8::FunctionCallbackInfo<v8::Value>& args);
            static void Recv(const v8::FunctionCallbackInfo<v8::Value>& args);
            static void Send(const v8::FunctionCallbackInfo<v8::Value>& args);
            static void Output(const v8::FunctionCallbackInfo<v8::Value>& args);
            static void Input(const v8::FunctionCallbackInfo<v8::Value>& args);
            static void Update(const v8::FunctionCallbackInfo<v8::Value>& args);
            static void Check(const v8::FunctionCallbackInfo<v8::Value>& args);
            static void Flush(const v8::FunctionCallbackInfo<v8::Value>& args);
            static void Peeksize(const v8::FunctionCallbackInfo<v8::Value>& args);
            static void Setmtu(const v8::FunctionCallbackInfo<v8::Value>& args);
            static void Wndsize(const v8::FunctionCallbackInfo<v8::Value>& args);
            static void Waitsnd(const v8::FunctionCallbackInfo<v8::Value>& args);
            static void Nodelay(const v8::FunctionCallbackInfo<v8::Value>& args);
            static v8::Persistent<v8::Function> constructor;
            static int kcp_output(const char *buf, int len, ikcpcb *kcp, void *user);
            ikcpcb* kcp;
            v8::Persistent<v8::Function> output;
            v8::Persistent<v8::Object> context;
    };

}

#endif
