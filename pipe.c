/*
 * The functions in this file were taken and adapted from
 * the pipewire example:
 *
 * - https://docs.pipewire.org/audio-capture_8c-example.html
 */

#include <pipewire-0.3/pipewire/thread-loop.h>
#include <pipewire/pipewire.h>
#include <math.h>
#include <signal.h>

#include "corder.h"
#include "pipe.h"
#include "ui.h"

static void on_process(void *userdata)
{
        struct data *data = userdata;
        struct pw_buffer *b;
        struct spa_buffer *buf;
        float *samples, max;
        uint32_t c, n, n_channels, n_samples, peak;

        if ((b = pw_stream_dequeue_buffer(data->stream)) == NULL) {
                pw_log_warn("out of buffers: %m");
                return;
        }

        buf = b->buffer;
        if ((samples = buf->datas[0].data) == NULL)
                return;

        n_channels = data->format.info.raw.channels;
        n_samples = buf->datas[0].chunk->size / sizeof(float);

        /* move cursor up */
        if (GUI_DISABLE) {
          if (data->move)
                  fprintf(stdout, "%c[%dA", 0x1b, n_channels + 1);
        }

        if (transport.armTrackBtn.isPressed) {
                if (GUI_DISABLE)
                        fprintf(stdout, "\n");

                for (c = 0; c < data->format.info.raw.channels; c++) {
                        max = 0.0f;
                        for (n = c; n < n_samples; n += n_channels) {
                                max = fmaxf(max, fabsf(samples[n]));
                        }

                        SAMPLE_LEFT = c == 0 ? samples[0] / VOL_RATIO : SAMPLE_LEFT;
                        SAMPLE_RIGHT = c == 1 ? samples[1] / VOL_RATIO : SAMPLE_RIGHT;

                        peak = (uint32_t)SPA_CLAMPF(max * 30, 0.f, 39.f);

                        if (GUI_DISABLE) {
                                fprintf(stdout, "channel %d: |", c);
                                for (size_t i = 0; i <= peak+1; i++) fputs("\u2592", stdout);
                                fprintf(stdout, "%*s| peak:%f\n", 40 - peak, "", max);
                        }
                }
        }

        // Write sample data (total nr samples) to file.
        if (transport.armTrackBtn.isPressed && transport.recTrackBtn.isPressed) {
                if (FILE_INITD && Data.sf != NULL) sf_write_float(Data.sf, samples, n_samples);

                long fsize = 0;
                FILE *file = fopen(data->sfName, "r");
                if (file != NULL) {
                        fseek(file, 0, SEEK_END);
                        fsize = ftell(file);
                        fclose(file);
                }

                if (GUI_DISABLE)
                        fprintf(stdout, "%-33sFile: %s | %ld bytes", "", data->sfName, fsize);
        }


        data->move = true;
        if (GUI_DISABLE)
                fflush(stdout);

        pw_stream_queue_buffer(data->stream, b);
}

/*
 * Notify when the stream param changes. ( format ).
 */
static void
on_stream_param_changed(void *_data, uint32_t id, const struct spa_pod *param)
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

        if (GUI_DISABLE) {
                char separator[2] = "-";
                size_t sepLength = 69;
                fprintf(stdout, "[OpenCorder] >>> {Source Rate: %dHz Channels: %d}\n",
                        data->format.info.raw.rate, data->format.info.raw.channels);
                for (size_t i = 0; i < sepLength; i++) fputs(separator, stdout);
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

void* piper(void* arg)
{
        pipeData *pdPtr = (pipeData*)arg;
        data data = pdPtr->dat;
        char* streamTarget = pdPtr->target;
        int argc = pdPtr->argc;
        const struct spa_pod *params[1];
        uint8_t buffer[1024];
        struct pw_properties *props;
        struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

        /* make a main loop. If you already have another main loop, you can add
         * the fd of this pipewire mainloop to it. */
        data.loop = pw_main_loop_new(NULL);

        pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGINT, do_quit, &data);
        pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGTERM, do_quit, &data);

        /* Create a simple stream, the simple stream manages the core and remote
         * objects for you if you don't need to deal with them.
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
        if (argc > 1 && streamTarget != NULL)
                /* Set stream target if given on command line */
                pw_properties_set(props, PW_KEY_TARGET_OBJECT, streamTarget);
        /* uncomment if you want to capture from the sink monitor ports */
        /* pw_properties_set(props, PW_KEY_STREAM_CAPTURE_SINK, "true"); */

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
        pw_stream_connect(data.stream,
                          PW_DIRECTION_INPUT,
                          PW_ID_ANY,
                          PW_STREAM_FLAG_AUTOCONNECT |
                          PW_STREAM_FLAG_MAP_BUFFERS |
                          PW_STREAM_FLAG_RT_PROCESS,
                          params, 1);

        /* and wait while we let things run */
        pw_main_loop_run(data.loop);

        /* Cleanup resources owned by this thread. This is done here to ensure
         * the thread fully finishes its work before the thread exits. */
        if (data.stream) {
                pw_stream_destroy(data.stream);
        }
        if (data.loop) {
                pw_main_loop_destroy(data.loop);
        }
        /* Note: sf handle closing is managed by the thread if used. */

        free(pdPtr);
        return NULL;
}

