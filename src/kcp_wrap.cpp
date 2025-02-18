/**
 * Copyright 2025 leenjewel
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
#ifdef USE_NAPI
#include <napi.h>
#include "kcp/ikcp.h"

class KCPWrap : public Napi::ObjectWrap<KCPWrap> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "KCP", {
      InstanceMethod("send", &KCPWrap::Send),
      InstanceMethod("recv", &KCPWrap::Recv),
      InstanceMethod("update", &KCPWrap::Update),
      InstanceMethod("input", &KCPWrap::Input),
      InstanceMethod("flush", &KCPWrap::Flush),
      InstanceMethod("peeksize", &KCPWrap::PeekSize),
      InstanceMethod("setmtu", &KCPWrap::SetMtu),
      InstanceMethod("wndsize", &KCPWrap::WndSize),
      InstanceMethod("waitsnd", &KCPWrap::WaitSnd),
      InstanceMethod("nodelay", &KCPWrap::NoDelay),
      InstanceMethod("context", &KCPWrap::GetContext),
      InstanceMethod("stream", &KCPWrap::Stream),
      InstanceMethod("output", &KCPWrap::Output),
      InstanceAccessor("mtu", &KCPWrap::GetMtu, &KCPWrap::SetMtu)
    });

    exports.Set("KCP", func);
    return exports;
  }
  KCPWrap(const Napi::CallbackInfo& info) : Napi::ObjectWrap<KCPWrap>(info) {
    uint32_t conv = info[0].As<Napi::Number>().Uint32Value();
    Napi::Object user = info[1].As<Napi::Object>();

    kcp_ = ikcp_create(conv, this);
    kcp_->output = [](const char* buf, int len, ikcpcb* kcp, void* user) {
      KCPWrap* wrap = static_cast<KCPWrap*>(user);
      wrap->HandleOutput(buf, len);
      return len;
    };

    user_ref_ = Napi::Persistent(user);
  }
  ~KCPWrap() {
    ikcp_release(kcp_);
    user_ref_.Reset();
  }
private:
  ikcpcb* kcp_;
  Napi::ObjectReference user_ref_;
  void HandleOutput(const char* buf, int len) {
    Napi::Env env = user_ref_.Env();
    Napi::HandleScope scope(env);

    Napi::Buffer<char> buffer = Napi::Buffer<char>::Copy(env, buf, len);
    user_ref_.Value().As<Napi::Object>().Get("onoutput").As<Napi::Function>()
      .Call(user_ref_.Value() , { buffer });
  }
  Napi::Value Send(const Napi::CallbackInfo& info) {
    if (info[0].IsString()) {
      Napi::String str = info[0].As<Napi::String>();
      std::string data = str.Utf8Value();
      int ret = ikcp_send(kcp_, data.c_str(), data.length());
      return Napi::Number::New(info.Env(), ret);
    } else if (info[0].IsBuffer()) {
      Napi::Buffer<char> buffer = info[0].As<Napi::Buffer<char>>();
      int ret = ikcp_send(kcp_, buffer.Data(), buffer.Length());
      return Napi::Number::New(info.Env(), ret);
    }
    Napi::Error::New(info.Env(), "Send method only accepts String or Buffer").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  Napi::Value Recv(const Napi::CallbackInfo& info) {
    char buffer[4096];
    int len = ikcp_recv(kcp_, buffer, sizeof(buffer));

    if (len < 0) return info.Env().Null();
    return Napi::Buffer<char>::Copy(info.Env(), buffer, len);
  }
  Napi::Value Update(const Napi::CallbackInfo& info) {
    uint32_t current = info[0].As<Napi::Number>().Uint32Value();
    ikcp_update(kcp_, current);
    return info.Env().Undefined();
  }
  Napi::Value Input(const Napi::CallbackInfo& info) {
    if (info[0].IsString()) {
      Napi::String str = info[0].As<Napi::String>();
      std::string data = str.Utf8Value();
      int ret = ikcp_input(kcp_, data.c_str(), data.length());
      return Napi::Number::New(info.Env(), ret);
    } else if (info[0].IsBuffer()) {
      Napi::Buffer<char> buffer = info[0].As<Napi::Buffer<char>>();
      int ret = ikcp_input(kcp_, buffer.Data(), buffer.Length());
      return Napi::Number::New(info.Env(), ret);
    }
    Napi::Error::New(info.Env(), "Input method only accepts String or Buffer").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  Napi::Value GetMtu(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), kcp_->mtu);
  }
  Napi::Value Flush(const Napi::CallbackInfo& info) {
    ikcp_flush(kcp_);
    return info.Env().Undefined();
  }
  Napi::Value PeekSize(const Napi::CallbackInfo& info) {
    int size = ikcp_peeksize(kcp_);
    return Napi::Number::New(info.Env(), size);
  }
  Napi::Value SetMtu(const Napi::CallbackInfo& info) {
    int mtu = info[0].As<Napi::Number>().Int32Value();
    int ret = ikcp_setmtu(kcp_, mtu);
    return Napi::Number::New(info.Env(), ret);
  }
  Napi::Value WndSize(const Napi::CallbackInfo& info) {
    int sndwnd = info[0].As<Napi::Number>().Int32Value();
    int rcvwnd = info[1].As<Napi::Number>().Int32Value();
    int ret = ikcp_wndsize(kcp_, sndwnd, rcvwnd);
    return Napi::Number::New(info.Env(), ret);
  }
  Napi::Value WaitSnd(const Napi::CallbackInfo& info) {
    int count = ikcp_waitsnd(kcp_);
    return Napi::Number::New(info.Env(), count);
  }
  Napi::Value NoDelay(const Napi::CallbackInfo& info) {
    int nodelay = info[0].As<Napi::Number>().Int32Value();
    int interval = info[1].As<Napi::Number>().Int32Value();
    int resend = info[2].As<Napi::Number>().Int32Value();
    int nc = info[3].As<Napi::Number>().Int32Value();
    int ret = ikcp_nodelay(kcp_, nodelay, interval, resend, nc);
    return Napi::Number::New(info.Env(), ret);
  }
  void SetMtu(const Napi::CallbackInfo& info, const Napi::Value& value) {
    kcp_->mtu = value.As<Napi::Number>().Uint32Value();
  }
  Napi::Value GetContext(const Napi::CallbackInfo& info) {
    return user_ref_.Value();
  }
  Napi::Value Stream(const Napi::CallbackInfo& info) {
    if (info.Length() > 0 && info[0].IsNumber()) {
      kcp_->stream = info[0].As<Napi::Number>().Int32Value();
    }
    return Napi::Number::New(info.Env(), kcp_->stream);
  }
  Napi::Value Output(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    env.Global().Get("console").As<Napi::Object>().Get("warn").As<Napi::Function>().Call({Napi::String::New(env, "This method 'output' is deprecated. Please define onoutput method when initializing")});
    return env.Undefined();
  }
};

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  return KCPWrap::Init(env, exports);
}

NODE_API_MODULE(kcp_native, Init)
#endif