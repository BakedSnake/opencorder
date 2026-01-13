#include <stdio.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <sys/stat.h>

#include <spa/param/audio/format-utils.h>
#include <pipewire/pipewire.h>
#include <sndfile.h>

static char version[5] = "0.0.2";

static struct option options[] = {
  {"help",              no_argument, 		0, 'h'},
  {"version",           no_argument, 		0, 'v'},
  {"format",            required_argument, 	0, 'f'},
  {"rate",		required_argument, 	0, 'r'},
  {"output", 		required_argument, 	0, 'o'},
  {"target", 		required_argument, 	0, 't'},
  {0, 			0, 			0,  0 }
};

struct data {
        struct pw_main_loop *loop;
        struct pw_stream *stream;

        struct spa_audio_info format;
        unsigned move:1;
        SNDFILE *sf;
	char *sfName;
};

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
        if (data->move)
                fprintf(stdout, "%c[%dA", 0x1b, n_channels + 1);
        fprintf(stdout, "\n");
        for (c = 0; c < data->format.info.raw.channels; c++) {
                max = 0.0f;
                for (n = c; n < n_samples; n += n_channels) {
                        max = fmaxf(max, fabsf(samples[n]));
                }


                peak = (uint32_t)SPA_CLAMPF(max * 30, 0.f, 39.f);

                fprintf(stdout, "channel %d: |%*s%*s| peak:%f\n",
                                c, peak+1, "*", 40 - peak, "", max);
        }

        // Write sample data (total nr samples) to file.
        sf_write_float(data->sf, samples, n_samples);

        // Keep track of file size.
        FILE *file = fopen(data->sfName, "r");
        fseek(file, 0, SEEK_END);
        long fsize = ftell(file);
        fprintf(stdout, "File: %s | %ld bytes", data->sfName, fsize);
        rewind(file);

        data->move = true;
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

        fprintf(stdout, "source rate:%d channels:%d\n",
                        data->format.info.raw.rate, data->format.info.raw.channels);

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

int main(int argc, char *argv[])
{
        int opt;
	char* rateStr = NULL;
	char* streamTarget = NULL;
	char* path = NULL;

        while ((opt = getopt_long(argc, argv, "hvfrot:", options, NULL)) != -1) {
                switch (opt) {
                        case 'h':
                                printf("Usage: ...\n");
                                return 0;
                        case 'v':
                                printf("Version: %s\n", version);
                                return 0;
                        case 'o':
                                path = optarg;
                                break;
                        case 'r':
                                rateStr = optarg;
                        case 't':
                                streamTarget = optarg;
                        case '?':
                                default:
                                break;
                }
        }

        struct data data = { 0, };
        const struct spa_pod *params[1];
        uint8_t buffer[1024];
        struct pw_properties *props;
        struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

        data.sfName = path != NULL ? path : "out-recording.wav";
        const int channels = 2;
        const int samplerate = rateStr != NULL ? atoi(rateStr) : 48000;
        const int frames = samplerate;

        SF_INFO sfinfo = {0};
        sfinfo.samplerate = samplerate;
        sfinfo.channels = channels;
        sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_32;
        data.sf = sf_open(data.sfName, SFM_WRITE, &sfinfo);
        if (!data.sf) {
            fprintf(stderr, "Error opening file: %s\n", sf_strerror(NULL));
            return 1;
        }

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

        pw_stream_destroy(data.stream);
        pw_main_loop_destroy(data.loop);
        pw_deinit();

        return 0;
}
