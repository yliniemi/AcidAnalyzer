#include <readPipewire.h>


// #include <pthread.h>

#include <pipewire/pipewire.h>

#include <ringBuffer.h>

#include <defaults.h>

#include <locale.h>
#include <wchar.h>
#include <ncurses.h>



void deinterleaveArrays(const float* interleavedData, float** outputArrays, int64_t numArrays, int64_t length) {
    for (int64_t i = 0; i < numArrays; i++) {
        for (int64_t j = 0; j < length; j++) {
            outputArrays[i][j] = interleavedData[j * numArrays + i];
        }
    }
}

/* our data processing function is in general:
 *
 *  struct pw_buffer *b;
 *  b = pw_stream_dequeue_buffer(stream);
 *
 *  .. consume stuff in the buffer ...
 *
 *  pw_stream_queue_buffer(stream, b);
 */
static void on_process(void *userData)
{
        struct data *data = userData;
        struct pw_buffer *pipeWireBuffer;
        struct spa_buffer *spaBuffer;
        uint32_t n_samples;
        static int64_t n_channels = 0;
        
        if ((pipeWireBuffer = pw_stream_dequeue_buffer(data->stream)) == NULL) {
                pw_log_warn("out of buffers: %m");
                return;
        }
        
        spaBuffer = pipeWireBuffer->buffer;
        if (spaBuffer->datas[0].data == NULL)
                return;
        
        n_channels = data->format.info.raw.channels;
        
        n_samples = spaBuffer->datas[0].chunk->size / sizeof(float) / n_channels;
        if (n_samples > global.mostCapturedSamples) global.mostCapturedSamples = n_samples;
        if (getBufferWriteSpace(&global.allBuffer) >= n_samples)
        {
            double *audio = calloc(n_samples, sizeof(double));
            for (int64_t channel = 0; channel < n_channels; channel++)
            {
                for (int64_t i = 0; i < n_samples; i++)
                {
                    audio[i] = ((float*)spaBuffer->datas[0].data)[(i * n_channels + channel)];
                }
                // mvprintw(w.ws_row - 2, 0, "started writing");
                // refresh();
                writeBuffer(&global.allBuffer, (uint8_t*)audio, channel, n_samples);
                // mvprintw(w.ws_row - 2, 0, "writing done   ");
                // refresh();
            }
            increaseBufferWriteIndex(&global.allBuffer, n_samples);
            free(audio);
            // printw("d");
        }
        // writeBuffer(&global.allBuffer, (uint8_t*)buf->datas[0].data, 0, n_samples);
        
        
        /* move cursor up */
        static int64_t frame = 0;
        
        data->move = true;
        // fflush(stdout);
        
        pw_stream_queue_buffer(data->stream, pipeWireBuffer);
}
 
/* Be notified when the stream param changes. We're only looking at the
 * format changes.
 */

void on_stream_param_changed(void *_data, uint32_t id, const struct spa_pod *param)
{
        struct data *data = _data;
        
        /* NULL means to clear the format */
        if (param == NULL || id != SPA_PARAM_Format)
                return;
                
        if (spa_format_parse(param, &data->format.media_type, &data->format.media_subtype) < 0)
                return;
                
        /* only accept raw audio */
        if (data->format.media_type != SPA_MEDIA_TYPE_audio ||
            data->format.media_subtype != SPA_MEDIA_SUBTYPE_raw)
                return;
                
         /* call a helper function to parse the format for us. */
        spa_format_audio_raw_parse(param, &data->format.info.raw);
        
        
        if (global.sampleRate != data->format.info.raw.rate)
        {
            global.sampleRate = data->format.info.raw.rate;
            // setNewSampleRate = sampleRate;
        }
        if (global.channels != data->format.info.raw.channels)
        {
            global.channels = data->format.info.raw.channels;
            initializeBufferWithChunksSize(&global.allBuffer, global.channels, global.bufferExtra, sizeof(double), global.FFTsize);
            global.allBuffer.partialRead = true;
            if (global.usingNcurses)
            {
                printw("initialized the buffer with %lld size and %lld channels", global.allBuffer.size, global.channels);
                refresh();
            }
            else
            {
                printf("initialized the buffer with %lld size and %lld channels\n", global.allBuffer.size, global.channels);
            }
        }
        
}
 
static const struct pw_stream_events stream_events = {
        PW_VERSION_STREAM_EVENTS,
        .param_changed = on_stream_param_changed,
        .process = on_process,
};
 
static void do_quit(void *userdata, int signal_number)
{
        struct data *data = userdata;
        pw_main_loop_quit(data->loop);
}


void startPipewire(int argc, char *argv[])
{
    struct data data = { 0, };
    const struct spa_pod *params[1];
    uint8_t buffer[1024];
    struct pw_properties *props;
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    
    pw_init(&argc, &argv);
    
    /* make a main loop. If you already have another main loop, you can add
     * the fd of this pipewire mainloop to it. */
    data.loop = pw_main_loop_new(NULL);
    
    pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGINT, do_quit, &data);
    pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGTERM, do_quit, &data);
    
    /* Create a simple stream, the simple stream manages the core and remote
     * objects for you if you don't need to deal with them.
     *
     * If you plan to autoconnect your stream, you need to provide at least
     * media, category and role properties.
     *
     * Pass your events and a user_data pointer as the last arguments. This
     * will inform you about the stream state. The most important event
     * you need to listen to is the process event where you need to produce
     * the data.
     */
     
    props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
                    PW_KEY_MEDIA_CATEGORY, "Capture",
                    PW_KEY_MEDIA_ROLE, "Music",
                    NULL);
    if (argc > 1)
            /* Set stream target if given on command line */
            pw_properties_set(props, PW_KEY_TARGET_OBJECT, argv[1]);
    /* uncomment if you want to capture from the sink monitor ports */
    pw_properties_set(props, PW_KEY_STREAM_CAPTURE_SINK, "true");

    data.stream = pw_stream_new_simple(
                    pw_main_loop_get_loop(data.loop),
                    "audio-capture",
                    props,
                    &stream_events,
                    &data);
                    
    /* Make one parameter with the supported formats. The SPA_PARAM_EnumFormat
     * id means that this is a format enumeration (of 1 value).
     * We leave the channels and rate empty to accept the native graph
     * rate and channels. */
    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat,
                    &SPA_AUDIO_INFO_RAW_INIT(
                       .format = SPA_AUDIO_FORMAT_F32));
    
    /* Now connect this stream. We ask that our process function is
     * called in a realtime thread. */
    /*
    pw_stream_connect(data.stream,
                      PW_DIRECTION_INPUT,
                      PW_ID_ANY,
                      PW_STREAM_FLAG_AUTOCONNECT |
                      PW_STREAM_FLAG_MAP_BUFFERS |
                      PW_STREAM_FLAG_RT_PROCESS,
                      params, 1);
    */
    pw_stream_connect(data.stream,
                      PW_DIRECTION_INPUT,
                      PW_ID_ANY,
                      PW_STREAM_FLAG_AUTOCONNECT |
                      PW_STREAM_FLAG_MAP_BUFFERS |
                      PW_STREAM_FLAG_RT_PROCESS,
                      params, 1);
    
    if (global.usingNcurses)
    {
        printw("initialized audio capture\n");
        refresh();
    }
    else
    {
        printf("initialized audio capture\n");
    }
    
    pw_main_loop_run(data.loop);
    
    // maybe I should do these when I quit
    pw_stream_destroy(data.stream);
    pw_main_loop_destroy(data.loop);
    pw_deinit();
}
