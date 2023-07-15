#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <complex.h> 
#include <fftw3.h>
#include <time.h>
#include <pthread.h>

#include <spa/param/audio/format-utils.h>

#include <pipewire/pipewire.h>

// #include <kaiser.h>
#include <ringBuffer.h>
#include <properFFTalgorithm.h>

double *real_in;
fftw_complex *complex_out;
fftw_plan p;

pthread_t readStuff;

int capturedSamples = 0;

void deinterleaveArrays(const float* interleavedData, float** outputArrays, int numArrays, int length) {
    for (int i = 0; i < numArrays; i++) {
        for (int j = 0; j < length; j++) {
            outputArrays[i][j] = interleavedData[j * numArrays + i];
        }
    }
}

struct data {
        struct pw_main_loop *loop;
        struct pw_stream *stream;
 
        struct spa_audio_info format;
        unsigned move:1;
};
 
/* our data processing function is in general:
 *
 *  struct pw_buffer *b;
 *  b = pw_stream_dequeue_buffer(stream);
 *
 *  .. consume stuff in the buffer ...
 *
 *  pw_stream_queue_buffer(stream, b);
 */
static void on_process(void *userdata)
{
        struct data *data = userdata;
        struct pw_buffer *b;
        struct spa_buffer *buf;
        float *samples, min, max;
        uint32_t c, n, n_samples, peak;
        static int n_channels = 0;
 
        if ((b = pw_stream_dequeue_buffer(data->stream)) == NULL) {
                pw_log_warn("out of buffers: %m");
                return;
        }
 
        buf = b->buffer;
        if ((samples = buf->datas[0].data) == NULL)
                return;
        
        n_channels = data->format.info.raw.channels;

        n_samples = buf->datas[0].chunk->size / sizeof(float) / n_channels;
        if (getBufferWriteSpace(&allBuffer) >= n_samples)
        {
            struct winsize w;
            ioctl(0, TIOCGWINSZ, &w);

            double *audio = calloc(n_samples, sizeof(double));
            for (int c = 0; c < n_channels; c++)
            {
                for (int i = 0; i < n_samples; i++)
                {
                    audio[i] = ((float*)buf->datas[0].data)[(i * n_channels + c)];
                }
                // mvprintw(w.ws_row - 2, 0, "started writing");
                // refresh();
                writeBuffer(&allBuffer, (uint8_t*)audio, c, n_samples);
                // mvprintw(w.ws_row - 2, 0, "writing done   ");
                // refresh();
            }
            increaseBufferWriteIndex(&allBuffer, n_samples);
            free(audio);
        }
        // writeBuffer(&allBuffer, (uint8_t*)buf->datas[0].data, 0, n_samples);


 
        /* move cursor up */
        static int frame = 0;
        capturedSamples = n_samples;
        
        data->move = true;
        // fflush(stdout);
 
        pw_stream_queue_buffer(data->stream, b);
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

        
        if (sampleRate != data->format.info.raw.rate)
        {
            sampleRate = data->format.info.raw.rate;
            // setNewSampleRate = sampleRate;
        }
        if (channels != data->format.info.raw.channels)
        {
            channels = data->format.info.raw.channels;
            // setNewChannels(channels);
            // fprintf(stdout, "channels changed to %d\n", channels);
            initializeBufferWithChunksSize(&allBuffer, channels, 65536, sizeof(double), NUMBER_OF_FFT_SAMPLES);
            allBuffer.partialRead = true;
            printw("initialized the buffer with %d size", allBuffer.size);
            refresh;
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

#ifndef THIS_IS_A_TEST
int main(int argc, char *argv[])
{
        struct data data = { 0, };
        const struct spa_pod *params[1];
        uint8_t buffer[1024];
        struct pw_properties *props;
        struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

        double colorR, colorG, colorB;
        printf("\033]0;Acid Analyzer\007");
        setlocale(LC_ALL, "");
        initscr();
        curs_set(0);
        start_color();
        srand(time(NULL));
        // HSLtoRGB((double)rand() / RAND_MAX, 1.0, 0.5, &colorR, &colorG, &colorB);
        // mvprintw(1, 0, "%lf, %lf, %lf", colorR, colorG, colorB);
        printw("program started \u2581\u2582\u2583\u2584\u2585\u2586\u2587\u2588");
        refresh;
        refresh();
        // nanosleep(&(struct timespec){.tv_sec = 10}, NULL);
        // init_color(COLOR_WHITE, 900, 0, 0);
        // init_color(COLOR_BLACK, 0, 0, 0);
        // init_pair(1, COLOR_WHITE, COLOR_BLACK);
        // init_pair(1, 1 + rand() % 15, COLOR_BLACK);
        init_color(COLOR_BLUE, 0, 0, 1000);
        init_color(COLOR_CYAN, 300, 300, 1000);
        init_pair(1, COLOR_BLUE, COLOR_BLACK);
        init_pair(2, COLOR_BLACK, COLOR_CYAN);
        color_set(1, NULL);
        // attron(COLOR_PAIR(1));

        // fprintf(stdout, "sizeof(fftw_complex) = %d\n", sizeof(fftw_complex));
 
        /* and wait while we let things run */
        pthread_create(&readStuff, NULL, threadFunction, NULL);
        sleep(1);
        
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
        
        pw_main_loop_run(data.loop);
 
        pw_stream_destroy(data.stream);
        pw_main_loop_destroy(data.loop);
        pw_deinit();
 
        return 0;
}
#endif
