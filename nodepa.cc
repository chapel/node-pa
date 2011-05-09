#include <v8.h>
#include <node.h>
#include <sndfile.h>
#include <portaudio.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;
using namespace node;
using namespace v8;

class PAudio : public ObjectWrap {
    static Persistent<FunctionTemplate> constructor_template;

public:

    static void Initialize(Handle<Object> target) {
        HandleScope scope;

        Local<FunctionTemplate> t = FunctionTemplate::New(New);
        constructor_template = Persistent<FunctionTemplate>::New(t);
        constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
        constructor_template->SetClassName(String::NewSymbol("PAudio"));

        NODE_SET_PROTOTYPE_METHOD(constructor_template, "play", Play);
        target->Set(String::NewSymbol("PAudio"), constructor_template->GetFunction());
      	
    }

    struct OurData {
      Persistent<Function> cb;
      SNDFILE *sndFile;
      SF_INFO sfInfo;
      PAudio *paudio;
      int position;
    };

    static Handle<Value> New(const Arguments &args) {
        HandleScope scope;

        PAudio *paudio = new PAudio();
        paudio->Wrap(args.This());
        return args.This();
    }
    
    static Handle<Value> Play(const Arguments &args) {
        HandleScope scope;
        
        Local<Function> cb = Local<Function>::Cast(args[1]);
        PAudio *paudio = ObjectWrap::Unwrap<PAudio>(args.This());
        const char *usage = "usage: play(file)";
        if (args.Length() < 1) {
          return ThrowException(Exception::Error(String::New(usage)));
        }
        String::AsciiValue fileName(args[0]->ToString());
        OurData *data = new OurData;
      	data->position = 0;
      	data->sfInfo.format = 0;
      	data->cb = Persistent<Function>::New(cb);
        data->paudio = paudio;
      	data->sndFile = sf_open(*fileName, SFM_READ, &data->sfInfo);
        Pa_Initialize();
        eio_custom(EIO_Play, EIO_PRI_DEFAULT, EIO_PlayAfter, data);

        ev_ref(EV_DEFAULT_UC);
        paudio->Ref();

        return Undefined();
    }

    static int EIO_Play(eio_req *req) {
        OurData *data = (OurData *)req->data;
        
        PaStream *stream;
      	PaError error;
      	PaStreamParameters outputParameters;
        outputParameters.device = Pa_GetDefaultOutputDevice();
      	outputParameters.channelCount = data->sfInfo.channels; 
      	outputParameters.sampleFormat = paInt32;
      	outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowInputLatency;
      	outputParameters.hostApiSpecificStreamInfo = 0; 
        
      	error = Pa_OpenStream(&stream,  
      						  0,
      						  &outputParameters,
      						  data->sfInfo.samplerate,
      						  paFramesPerBufferUnspecified,  
      						  paNoFlag, 
      						  Callback,
      						  data ); 

      	if (error) {
      	  Pa_Terminate();
          return 0;
      	}

      	Pa_StartStream(stream);
      	Pa_Sleep(2000);
      	Pa_StopStream(stream);
      	Pa_Terminate();
        return 0;
    }

    static int EIO_PlayAfter(eio_req *req) {
        HandleScope scope;
        OurData *data = (OurData *)req->data;
        ev_unref(EV_DEFAULT_UC);
        Local<Value> argv[1];
        argv[0] = Local<Value>::New(Null());
        TryCatch try_catch;
        data->cb->Call(Context::GetCurrent()->Global(), 1, argv);
        if (try_catch.HasCaught())
            FatalException(try_catch);
        data->cb.Dispose();
        data->paudio->Unref();
        free(data);
        return 0;
    }
    
    static int Callback (const void *input,
                         void *output,
                         unsigned long frameCount,
                         const PaStreamCallbackTimeInfo* paTimeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData) {
      OurData *data = (OurData *)userData;
      int *cursor;
      int *out = (int *)output;
      int thisSize = frameCount;
      int thisRead;

      cursor = out; 
      while (thisSize > 0)
      {
        sf_seek(data->sndFile, data->position, SEEK_SET);

        if (thisSize > (data->sfInfo.frames - data->position))
        {
          thisRead = data->sfInfo.frames - data->position;
          data->position = 0;
        }
        else
        {
          thisRead = thisSize;
          data->position += thisRead;
        }

        sf_readf_int(data->sndFile, cursor, thisRead);
        cursor += thisRead;
        thisSize -= thisRead;
      }

      return paContinue;
    }
};

Persistent<FunctionTemplate> PAudio::constructor_template;

extern "C" void
init(Handle<Object> target)
{
    HandleScope scope;

    PAudio::Initialize(target);
}