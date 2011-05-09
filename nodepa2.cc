#include <v8.h>
#include <node.h>
#include <string.h>
#include <sndfile.h>
#include <portaudio.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;
using namespace node;
using namespace v8;

static Handle<Value> Play (const Arguments&);
static int EIO_Play (eio_req *);
static int EIO_PlayAfter (eio_req *);
static int Callback (const void *input,
                     void *output,
                     unsigned long frameCount,
                     const PaStreamCallbackTimeInfo*,
                     PaStreamCallbackFlags,
                     void *userData);
extern "C" void init (Handle<Object>);

struct PlayData {
  //const char * fileName;
  SNDFILE *sndFile;
  SF_INFO sfInfo;
  Persistent<Function> cb;
  int position;
};

static Handle<Value> Play (const Arguments& args) {
  HandleScope scope;
  const char *usage = "usage: doSomething(file, cb)";
  if (args.Length() < 1) {
    return ThrowException(Exception::Error(String::New(usage)));
  }
  String::AsciiValue fileName(args[0]->ToString());
  Local<Function> cb = Local<Function>::Cast(args[1]);

  PlayData *data = (PlayData *)
    malloc(sizeof(struct PlayData) + fileName.length() + 1);
  data->position = 0;
	data->sfInfo.format = 0;
  data->cb = Persistent<Function>::New(cb);
  data->sndFile = sf_open(*fileName, SFM_READ, &data->sfInfo);
  //data->fileName = *fileName;

  eio_custom(EIO_Play, EIO_PRI_DEFAULT, EIO_PlayAfter, data);
  ev_ref(EV_DEFAULT_UC);
  return Undefined();
}

static int EIO_Play (eio_req *req) {
  struct PlayData * data = (struct PlayData *)req->data;
  PaStream *stream;
	PaError error;
	PaStreamParameters outputParameters;
	Pa_Initialize();
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
  return 0;
}

static int EIO_PlayAfter (eio_req *req) {
  HandleScope scope;
  ev_unref(EV_DEFAULT_UC);
  struct PlayData * data = (struct PlayData *)req->data;
  Local<Value> argv[1];
  argv[0] = Local<Value>::New(Null());
  TryCatch try_catch;
  data->cb->Call(Context::GetCurrent()->Global(), 1, argv);
  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }
  data->cb.Dispose();
  free(data);
  return 0;
}

static int Callback (const void *input,
                     void *output,
                     unsigned long frameCount,
                     const PaStreamCallbackTimeInfo* paTimeInfo,
                     PaStreamCallbackFlags statusFlags,
                     void *userData) {
  PlayData *data = (PlayData *)userData;
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

extern "C" void init (Handle<Object> target) {
  HandleScope scope;
  NODE_SET_METHOD(target, "play", Play);
}