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

#include <nan.h>
#include <nan_object_wrap.h>
#include "kcp/ikcp.h"

namespace node_kcp {

    class KCPObject : public Nan::ObjectWrap
    {
        public:
            static NAN_MODULE_INIT(Init);

        private:
            explicit KCPObject(IUINT32 conv);
            ~KCPObject();

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
            static int kcp_output(const char *buf, int len, ikcpcb *kcp, void *user);
            ikcpcb* kcp;
            Nan::Persistent<v8::Function> output;
            Nan::Persistent<v8::Object> context;
            char *recvBuff = NULL;
            unsigned int recvBuffSize = 1024;
    };

}

#endif
