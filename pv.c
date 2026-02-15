#include <pipewire-0.3/pipewire/thread-loop.h>
#include <sched.h>
#include <stdio.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <sys/stat.h>
#include <pthread.h>

#include <spa/param/audio/format-utils.h>
#include <pipewire/pipewire.h>
#include <sndfile.h>
#include <raylib.h>

#define WIDTH 600
#define HEIGHT 350

static char version[5] = "0.0.3";

static struct option options[] = {
  {"help",              no_argument,            0, 'h'},
  {"version",           no_argument,            0, 'v'},
  {"format",            required_argument,      0, 'f'},
  {"rate",              required_argument,      0, 'r'},
  {"output",            required_argument,      0, 'o'},
  {"target",            required_argument,      0, 't'},
  {"gui",               no_argument,            0, 'g'},
  {0,                   0,                      0,  0 }
};

typedef struct data {
        struct pw_main_loop *loop;
        struct pw_stream *stream;

        struct spa_audio_info format;
        unsigned move:1;
        SNDFILE *sf;
        char *sfName;
} data;

typedef struct pipeData {
        struct data dat;
        char* target;
        int argc;
} pipeData;

typedef struct State {
        bool isRecording;
        bool isArmed;
        bool isPaused;
} State;

typedef struct TextureButton {
        Rectangle bounds;
        Texture2D texture;
        Texture2D pressedTexture;
        Texture2D hoverTexture;
        Color tint;
        bool isPressed;
        bool isHovered;
} TextureButton;

TextureButton stopTrackBtn;
TextureButton armTrackBtn;
TextureButton recTrackBtn;
TextureButton pauseTrackBtn;
TextureButton saveFileBtn;

float SAMPLE_LEFT;
float SAMPLE_RIGHT;
pthread_t guiThread;
pthread_t pipeThread;

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

        if (armTrackBtn.isPressed) {
                //fprintf(stdout, "\n");
                for (c = 0; c < data->format.info.raw.channels; c++) {
                        max = 0.0f;
                        for (n = c; n < n_samples; n += n_channels) {
                                max = fmaxf(max, fabsf(samples[n]));
                        }

                        SAMPLE_LEFT = c == 0 ? samples[0] : SAMPLE_LEFT;
                        SAMPLE_RIGHT = c == 1 ? samples[1] : SAMPLE_RIGHT;

                        //peak = (uint32_t)SPA_CLAMPF(max * 30, 0.f, 39.f);

                        /*fprintf(stdout, "channel %d: |%*s%*s| peak:%f\n",
                                        c, peak+1, "*", 40 - peak, "", max);*/
                }
        }

        // Write sample data (total nr samples) to file.
        if (armTrackBtn.isPressed && recTrackBtn.isPressed) {
                sf_write_float(data->sf, samples, n_samples);

                // Keep track of file size.
                long fsize = 0;
                FILE *file = fopen(data->sfName, "r");
                if (file != NULL) {
                        fseek(file, 0, SEEK_END);
                        fsize = ftell(file);
                        fclose(file);
                }
                //fprintf(stdout, "File: %s | %ld bytes", data->sfName, fsize);
        }


        data->move = true;
        //fflush(stdout);
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

        /*fprintf(stdout, "source rate:%d channels:%d\n",
                        data->format.info.raw.rate, data->format.info.raw.channels);*/

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

void drawheader()
{
        DrawRectangle(23, 23, 85, 18, LIGHTGRAY);
        DrawRectangleLines(23, 23, 85, 18, BLACK);
        DrawText("File name", 25, 25, 14, BLACK);
        DrawRectangle(23, 41, 85, 18, LIGHTGRAY);
        DrawRectangleLines(23, 41, 85, 18, BLACK);
        DrawText("Sample rate", 25, 43, 14, BLACK);
        DrawRectangle(23, 59, 85, 18, LIGHTGRAY);
        DrawRectangleLines(23, 59, 85, 18, BLACK);
        DrawText("Master", 25, 62, 14, BLACK);

        DrawRectangle(107, 23, 135, 18, BLACK);
        DrawRectangleLines(107, 23, 135, 18, BEIGE);
        DrawRectangle(107, 41, 135, 18, BLACK);
        DrawRectangleLines(107, 41, 135, 18, BEIGE);
        DrawRectangle(107, 59, 135, 18, BLACK);
        DrawRectangleLines(107, 59, 135, 18, BEIGE);
}

void drawInfo(data data, SF_INFO sfinfo)
{
        drawheader();
        char* filename = data.sfName;
        DrawText(filename, 115, 25, 14, BEIGE);

        char rate[9];
        snprintf(rate, 9, "%d Hz", sfinfo.samplerate);
        DrawText(rate, 115, 43, 14, BEIGE);

        char master[7];
        int chans = sfinfo.channels;
        if (chans == 2)
                snprintf(master, 7, "%s", "Stereo");

        if (chans == 1)
                snprintf(master, 7, "%s", "Mono  ");

        DrawText(master, 115, 62, 14, BEIGE);
}

void drawVolumeMeters()
{
        DrawRectangle(550, 20, 30, 305, RED);
        DrawRectangle(500, 20, 30, 305, RED);

        float leftFull = SAMPLE_LEFT * 305;
        float rightFull = SAMPLE_RIGHT * 305;

        for (size_t j = 0; j < (305); ++j) {
                if (j < leftFull)
                        DrawRectangle(WIDTH-100, 325-j, 30, 1, GREEN);

                if (j < rightFull)
                        DrawRectangle(WIDTH-50, 325-j, 30, 1, GREEN);
        }

        DrawRectangleLines(550, 20, 30, 305, WHITE);
        DrawRectangleLines(500, 20, 30, 305, WHITE);
}

void drawVolumeValues()
{
        char* left = malloc(sizeof(SAMPLE_LEFT));
        char* right = malloc(sizeof(SAMPLE_RIGHT));

        snprintf(left, sizeof(SAMPLE_LEFT)+1, "%.2f", SAMPLE_LEFT);
        snprintf(right, sizeof(SAMPLE_RIGHT)+1, "%.2f", SAMPLE_RIGHT);

        DrawText(left, 505, 330, 14, RED);
        DrawText(right, 555, 330, 14, RED);

        free(left);
        free(right);
}

void drawControls()
{
        Texture2D currArmTrackTexture = armTrackBtn.texture;
        if (armTrackBtn.isPressed)
                currArmTrackTexture = armTrackBtn.pressedTexture;
        else if (armTrackBtn.isHovered)
                currArmTrackTexture = armTrackBtn.hoverTexture;

        Texture2D currRecTrackTexture = recTrackBtn.texture;
        if (recTrackBtn.isPressed)
                currRecTrackTexture = recTrackBtn.pressedTexture;
        else if (recTrackBtn.isHovered)
                currRecTrackTexture = recTrackBtn.hoverTexture;

        Texture2D currStopTrackTexture = stopTrackBtn.texture;
        if (stopTrackBtn.isPressed)
                currStopTrackTexture = stopTrackBtn.pressedTexture;
        else if (stopTrackBtn.isHovered)
                currStopTrackTexture = stopTrackBtn.hoverTexture;

        Texture2D currPauseTrackTexture = pauseTrackBtn.texture;
        if (pauseTrackBtn.isPressed)
                currPauseTrackTexture = pauseTrackBtn.pressedTexture;
        else if (pauseTrackBtn.isHovered)
                currPauseTrackTexture = pauseTrackBtn.hoverTexture;

        Texture2D currSaveFileTexture = saveFileBtn.texture;
        if (saveFileBtn.isPressed)
                currSaveFileTexture = saveFileBtn.pressedTexture;
        else if (saveFileBtn.isHovered)
                currSaveFileTexture = saveFileBtn.hoverTexture;

        DrawTextureEx(currSaveFileTexture, (Vector2){300, HEIGHT - 60}, .0f, .75f, WHITE);
        DrawTextureEx(currStopTrackTexture, (Vector2){25, HEIGHT - 60}, .0f, .75f, WHITE);
        DrawTextureEx(currArmTrackTexture, (Vector2){72, HEIGHT - 60}, .0f, .75f, WHITE);
        DrawTextureEx(currRecTrackTexture, (Vector2){119, HEIGHT - 60}, .0f, .75f, WHITE);
        DrawTextureEx(currPauseTrackTexture, (Vector2){166, HEIGHT - 60}, .0f, .75f, WHITE);
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

int main(int argc, char *argv[])
{
        int opt;
        char* rateStr = NULL;
        char* streamTarget = NULL;
        char* path = NULL;
        int rc;

        /* while ((opt = getopt_long(argc, argv, "hvfrotg:", options, NULL)) != -1) {
                switch (opt) {
                        case 'h':
                                printf("Usage: ...\n");
                                return 0;
                        case 'v':
                                printf("Version: %s\n", version);
                                return 0;
                        case 'g':
                                rc = pthread_create(&guiThread, NULL, guiStart, NULL);
                                if (rc < 0) {
                                        fprintf(stdout, "\n");
                                        return 1;
                                }
                                break;
                        case 'o':
                                path = optarg;
                                break;
                        case 'r':
                                rateStr = optarg;
                                break;
                        case 't':
                                streamTarget = optarg;
                                break;
                        case '?':
                                default:
                                break;
                }
        }*/

        data data = { 0, };

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

        InitWindow(600, 350, "frecorder");
        SetTargetFPS(60);

        Texture2D armTrack = LoadTexture("./assets/arm-track.png");
        Texture2D armTrackPressed = LoadTexture("./assets/arm-track-pressed.png");
        Texture2D recTrack = LoadTexture("./assets/play.png");
        Texture2D recTrackPressed = LoadTexture("./assets/play-pressed.png");
        Texture2D stopTrack = LoadTexture("./assets/stop-track.png");
        Texture2D stopTrackPressed = LoadTexture("./assets/stop-track-pressed.png");
        Texture2D pauseTrack = LoadTexture("./assets/pause-track.png");
        Texture2D pauseTrackPressed = LoadTexture("./assets/pause-track-pressed.png");

        Texture2D saveFile = LoadTexture("./assets/save.png");
        Texture2D saveFilePressed = LoadTexture("./assets/save-pressed.png");

        TextureButton stopBtn = {
                .bounds = { 25, HEIGHT-60, 60, 60 },  // x, y, width, height
                .texture = stopTrack,
                .pressedTexture = stopTrackPressed,
                .hoverTexture = stopTrack,
                .tint = WHITE,
                .isHovered = false,
                .isPressed = false
        };
        TextureButton armBtn = {
                .bounds = { 72, HEIGHT-60, 60, 60 },  // x, y, width, height
                .texture = armTrack,
                .pressedTexture = armTrackPressed,
                .hoverTexture = armTrack,
                .tint = WHITE,
                .isHovered = false,
                .isPressed = false
        };
        TextureButton recBtn = {
                .bounds = { 119, HEIGHT-60, 60, 60 },  // x, y, width, height
                .texture = recTrack,
                .pressedTexture = recTrackPressed,
                .hoverTexture = recTrack,
                .tint = WHITE,
                .isHovered = false,
                .isPressed = false
        };
        TextureButton pauseBtn = {
                .bounds = { 166, HEIGHT-60, 60, 60 },  // x, y, width, height
                .texture = pauseTrack,
                .pressedTexture = pauseTrackPressed,
                .hoverTexture = pauseTrack,
                .tint = WHITE,
                .isHovered = false,
                .isPressed = false
        };

        TextureButton saveBtn = {
                .bounds = { 300, HEIGHT-60, 60, 30 },  // x, y, width, height
                .texture = saveFile,
                .pressedTexture = saveFilePressed,
                .hoverTexture = saveFile,
                .tint = WHITE,
                .isHovered = false,
                .isPressed = false
        };

        stopTrackBtn = stopBtn;
        armTrackBtn = armBtn;
        recTrackBtn = recBtn;
        pauseTrackBtn = pauseBtn;
        saveFileBtn = saveBtn;

        pw_init(&argc, &argv);
        pipeData *pd = malloc(sizeof(pipeData));
        pd->dat = data;
        pd->target = streamTarget;
        pd->argc = argc;
        rc = pthread_create(&pipeThread, NULL, piper, pd);
        if (rc < 0) {
                fprintf(stdout, "\n");
                free(pd);
                return 1;
        }

        while (!WindowShouldClose()) {
                Vector2 mousePos = GetMousePosition();

                saveFileBtn.isHovered = CheckCollisionPointRec(mousePos, saveFileBtn.bounds);
                pauseTrackBtn.isHovered = CheckCollisionPointRec(mousePos, pauseTrackBtn.bounds);
                armTrackBtn.isHovered = CheckCollisionPointRec(mousePos, armTrackBtn.bounds);
                recTrackBtn.isHovered = CheckCollisionPointRec(mousePos, recTrackBtn.bounds);
                stopTrackBtn.isHovered = CheckCollisionPointRec(mousePos, stopTrackBtn.bounds);
                stopTrackBtn.isPressed = false;
                saveFileBtn.isPressed = false;

                if (armTrackBtn.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (armTrackBtn.isPressed) {
                                armTrackBtn.isPressed = false;
                        } else {
                                armTrackBtn.isPressed = true;
                        }
                }

                if (recTrackBtn.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (recTrackBtn.isPressed) {
                                recTrackBtn.isPressed = false;
                        } else {
                                recTrackBtn.isPressed = true;
                        }
                }

                if (pauseTrackBtn.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (!pauseTrackBtn.isPressed) {
                                if (armTrackBtn.isPressed)
                                        armTrackBtn.isPressed = false;

                                if (recTrackBtn.isPressed)
                                        recTrackBtn.isPressed = false;

                                pauseTrackBtn.isPressed = true;
                        } else {
                                if (armTrackBtn.isPressed)
                                        armTrackBtn.isPressed = true;

                                if (recTrackBtn.isPressed)
                                        recTrackBtn.isPressed = true;

                                pauseTrackBtn.isPressed = false;
                        }
                }

                if (stopTrackBtn.isHovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                        stopTrackBtn.isPressed = true;
                }

                if (stopTrackBtn.isPressed) {
                        armTrackBtn.isPressed = false;
                        recTrackBtn.isPressed = false;
                        pauseTrackBtn.isPressed = false;
                }

                if (saveFileBtn.isHovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                        saveFileBtn.isPressed = true;
                }


                BeginDrawing();
                ClearBackground(BLACK);
                DrawRectangle(20, 20, 425, 150, ORANGE);
                DrawRectangleLines(20, 20, 425, 150, WHITE);
                drawInfo(data, sfinfo);
                drawVolumeMeters();
                drawVolumeValues();
                drawControls();
                EndDrawing();
        }


        pthread_detach(guiThread);
        pthread_join(pipeThread, NULL);
        pw_stream_destroy(data.stream);
        pw_main_loop_destroy(data.loop);
        pw_deinit();
        UnloadTexture(stopTrack);
        UnloadTexture(armTrack);
        UnloadTexture(recTrack);
        CloseWindow();
        return 0;
}
