#include <linux/input.h>

#include <v8.h>
#include <node.h>
#include <nan.h>

#include <tslib.h>

using namespace v8;
using namespace node;

NAN_METHOD(Async);
void AsyncWork(uv_work_t* req);
void AsyncAfter(uv_work_t* req);

struct TouchInfo {
    NanCallback *callback;
    struct tsdev *ts;
    struct ts_sample *samp;

    bool error;
    std::string errorMessage;

    int read;

    int x;
    int y;
    int pressure;

    bool stop;
};

NAN_METHOD(Async) {
    NanScope();

    if (args[0]->IsUndefined()) {
        return NanThrowError("No parameters specified");
    }

    if (!args[0]->IsString()) {
        return NanThrowError("First parameter should be a string");
    }

    if (args[1]->IsUndefined()) {
        return NanThrowError("No callback function specified");
    }

    if (!args[1]->IsFunction()) {
        return NanThrowError("Second argument should be a function");
    }

    NanUtf8String *path = new NanUtf8String(args[0]);

    TouchInfo* touchInfo = new TouchInfo();
    touchInfo->ts = ts_open(**path, 0);
    touchInfo->callback = new NanCallback(args[1].As<Function>());
    touchInfo->samp = new ts_sample();
    touchInfo->error = false;
    touchInfo->stop = false;

    ts_load_module(touchInfo->ts, "input", " \t");
    ts_load_module(touchInfo->ts, "pthres", "pmin=2 \t");
    ts_load_module(touchInfo->ts, "variance", "delta=40 \t");
    ts_load_module(touchInfo->ts, "dejitter", "delta=100 \t");
    ts_load_module(touchInfo->ts, "linear", " \t");

    uv_work_t *req = new uv_work_t();
    req->data = touchInfo;

    int status = uv_queue_work(uv_default_loop(), req, AsyncWork, (uv_after_work_cb)AsyncAfter);

    assert(status == 0);

    NanReturnUndefined();
}

void AsyncWork(uv_work_t* req) {
    TouchInfo* touchInfo = static_cast<TouchInfo*>(req->data);

    touchInfo->read = ts_read(touchInfo->ts, touchInfo->samp, 1);

    if (touchInfo->read < 0) {
        touchInfo->error = true;
        touchInfo->errorMessage = "Read problem";
    }
}

void AsyncAfter(uv_work_t* req) {
    NanScope();

    TouchInfo* touchInfo = static_cast<TouchInfo*>(req->data);

    if (touchInfo->error) {
        Local<Value> err = Exception::Error(NanNew<String>(touchInfo->errorMessage.c_str()));

        const unsigned argc = 1;
        Local<Value> argv[argc] = { err };

        TryCatch try_catch;

        touchInfo->callback->Call(NanGetCurrentContext()->Global(), argc, argv);

        if (try_catch.HasCaught()) {
            FatalException(try_catch);
        }

    } else {

        Local<Object> touch = NanNew<Object>();
        touch->Set(NanNew<String>("x"), NanNew<Number>(touchInfo->samp->x));
        touch->Set(NanNew<String>("y"), NanNew<Number>(touchInfo->samp->y));
        touch->Set(NanNew<String>("pressure"), NanNew<Number>(touchInfo->samp->pressure));
        touch->Set(NanNew<String>("stop"), NanNew<Boolean>(touchInfo->stop));
        touch->Set(NanNew<String>("touch"), NanNew<Number>(touchInfo->read));

        const unsigned argc = 2;
        Local<Value> argv[argc] = { NanNull(), NanNew(touch) };

        TryCatch try_catch;

        touchInfo->callback->Call(NanGetCurrentContext()->Global(), argc, argv);
        touchInfo->stop = touch->Get(NanNew<String>("stop"))->BooleanValue();

        if (try_catch.HasCaught()) {
          FatalException(try_catch);
        }

        if (touchInfo->stop) {
            delete touchInfo->callback;
            delete touchInfo;
            delete req;
        } else {
            int status = uv_queue_work(uv_default_loop(), req, AsyncWork, (uv_after_work_cb)AsyncAfter);
            assert(status == 0);
        }

    }
}

void InitAll(Handle<Object> exports, Handle<Object> module) {
    NanScope();

    module->Set(NanNew("exports"),
      NanNew<FunctionTemplate>(Async)->GetFunction());
}

NODE_MODULE(ts, InitAll)
