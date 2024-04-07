#pragma once

#include <stdint.h>
#include <spa/param/audio/format-utils.h>

extern struct Global global;


void startPipewire(int argc, char *argv[]);

void deinterleaveArrays(const float* interleavedData, float** outputArrays, int64_t numArrays, int64_t length);

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
static void on_process(void *userData);
 
/* Be notified when the stream param changes. We're only looking at the
 * format changes.
 */

void on_stream_param_changed(void *_data, uint32_t id, const struct spa_pod *param);
 
static void do_quit(void *userdata, int signal_number);


