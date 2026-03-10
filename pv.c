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
#include <unistd.h>

#define WIDTH 600
#define HEIGHT 350
#define MAX_FILE_CHAR 24

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

typedef struct Transport {
        TextureButton stopTrackBtn;
        TextureButton armTrackBtn;
        TextureButton recTrackBtn;
        TextureButton pauseTrackBtn;
        TextureButton saveFileBtn;
        TextureButton newFileBtn;
} Transport;

Transport transport;

float SAMPLE_LEFT;
float SAMPLE_RIGHT;
float VOL_RATIO = .1f;

bool FILE_INITD = false;
bool MOUSE_ON_INPUT = false;

data Data = { 0, };

int rc;

char* rateStr = NULL;
char* streamTarget = NULL;
char* path = NULL;

SF_INFO sfinfo = {0};

char fName[MAX_FILE_CHAR+1] = "\0";
size_t charCount = 0;

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
        //if (data->move)
        //        fprintf(stdout, "%c[%dA", 0x1b, n_channels + 1);

        if (transport.armTrackBtn.isPressed) {
                //fprintf(stdout, "\n");
                for (c = 0; c < data->format.info.raw.channels; c++) {
                        max = 0.0f;
                        for (n = c; n < n_samples; n += n_channels) {
                                max = fmaxf(max, fabsf(samples[n]));
                        }

                        SAMPLE_LEFT = c == 0 ? samples[0] / VOL_RATIO : SAMPLE_LEFT;
                        SAMPLE_RIGHT = c == 1 ? samples[1] / VOL_RATIO : SAMPLE_RIGHT;

                        //peak = (uint32_t)SPA_CLAMPF(max * 30, 0.f, 39.f);

                        /*fprintf(stdout, "channel %d: |%*s%*s| peak:%f\n",
                                        c, peak+1, "*", 40 - peak, "", max);*/
                }
        }

        // Write sample data (total nr samples) to file.
        if (transport.armTrackBtn.isPressed && transport.recTrackBtn.isPressed) {
                if (FILE_INITD && Data.sf != NULL) sf_write_float(Data.sf, samples, n_samples);

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
        DrawRectangleLines(107, 23, 135, 18, RED);
        DrawRectangle(107, 41, 135, 18, BLACK);
        DrawRectangleLines(107, 41, 135, 18, BEIGE);
        DrawRectangle(107, 59, 135, 18, BLACK);
        DrawRectangleLines(107, 59, 135, 18, BEIGE);
}

void updateFileName()
{
        SetMouseCursor(MOUSE_CURSOR_IBEAM);
        int key = GetCharPressed();

        while (key > 0) {
                if ((key >= 32) && (key <= 125) && (charCount < MAX_FILE_CHAR)) {
                        fName[charCount] = (char)key;
                        fName[charCount+1] = '\0';
                        charCount++;
                }

                key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE)) {
                charCount--;
                if (charCount < 0) charCount = 0;
                fName[charCount] = '\0';
        }
}

void drawInfo(data data, SF_INFO sfinfo)
{
        drawheader();
        char* filename = fName;
        DrawText(filename, 115, 25, 14, BEIGE);

        char rate[9];
        int sp = sfinfo.samplerate ? sfinfo.samplerate : 48000;
        snprintf(rate, 9, "%d Hz", sp);
        DrawText(rate, 115, 43, 14, BEIGE);

        char master[7];
        int chans = sfinfo.channels ? sfinfo.channels : 2;
        if (chans == 2)
                snprintf(master, 7, "%s", "Stereo");

        if (chans == 1)
                snprintf(master, 7, "%s", "Mono  ");

        DrawText(master, 115, 62, 14, BEIGE);
}

void drawVolumeMeters(Texture2D faderTex)
{
        DrawTexture(faderTex, 500, 20, WHITE);
        DrawTexture(faderTex, 550, 20, WHITE);

        float leftFull = SAMPLE_LEFT * 280;
        float rightFull = SAMPLE_RIGHT * 280;

        for (size_t j = 0; j < (280); ++j) {
                if (j < leftFull)
                        DrawRectangle(WIDTH-100, 299-j, 30, 1, GREEN);

                if (j < rightFull)
                        DrawRectangle(WIDTH-50, 299-j, 30, 1, GREEN);
        }

        DrawRectangleLines(550, 20, 30, 280, BLACK);
        DrawRectangleLines(500, 20, 30, 280, BLACK);
}

void drawVolumeValues()
{
        char* left = malloc(sizeof(SAMPLE_LEFT));
        char* right = malloc(sizeof(SAMPLE_RIGHT));

        snprintf(left, sizeof(SAMPLE_LEFT)+1, "%.2f", SAMPLE_LEFT);
        snprintf(right, sizeof(SAMPLE_RIGHT)+1, "%.2f", SAMPLE_RIGHT);

        DrawText(left, 505, 305, 14, RED);
        DrawText(right, 555, 305, 14, RED);

        free(left);
        free(right);
}

TextureButton newButton(Rectangle bounds, Texture2D texture, Texture2D pressedTexture, Texture2D hoverTexture, Color tint)
{
        TextureButton new = {
                bounds,
                texture,
                pressedTexture,
                hoverTexture,
                tint,
                false,
                false,
        };

        return new;
}

void drawControls()
{
        Texture2D currArmTrackTexture = transport.armTrackBtn.texture;
        if (transport.armTrackBtn.isPressed)
                currArmTrackTexture = transport.armTrackBtn.pressedTexture;
        else if (transport.armTrackBtn.isHovered)
                currArmTrackTexture = transport.armTrackBtn.hoverTexture;

        Texture2D currRecTrackTexture = transport.recTrackBtn.texture;
        if (transport.recTrackBtn.isPressed)
                currRecTrackTexture = transport.recTrackBtn.pressedTexture;
        else if (transport.recTrackBtn.isHovered)
                currRecTrackTexture = transport.recTrackBtn.hoverTexture;

        Texture2D currStopTrackTexture = transport.stopTrackBtn.texture;
        if (transport.stopTrackBtn.isPressed)
                currStopTrackTexture = transport.stopTrackBtn.pressedTexture;
        else if (transport.stopTrackBtn.isHovered)
                currStopTrackTexture = transport.stopTrackBtn.hoverTexture;

        Texture2D currPauseTrackTexture = transport.pauseTrackBtn.texture;
        if (transport.pauseTrackBtn.isPressed)
                currPauseTrackTexture = transport.pauseTrackBtn.pressedTexture;
        else if (transport.pauseTrackBtn.isHovered)
                currPauseTrackTexture = transport.pauseTrackBtn.hoverTexture;

        Texture2D currNewFileTexture = transport.newFileBtn.texture;
        if (transport.newFileBtn.isPressed)
                currNewFileTexture = transport.newFileBtn.pressedTexture;
        else if (transport.newFileBtn.isHovered)
                currNewFileTexture = transport.newFileBtn.hoverTexture;

        Texture2D currSaveFileTexture = transport.saveFileBtn.texture;
        if (transport.saveFileBtn.isPressed)
                currSaveFileTexture = transport.saveFileBtn.pressedTexture;
        else if (transport.saveFileBtn.isHovered)
                currSaveFileTexture = transport.saveFileBtn.hoverTexture;

        DrawTextureEx(currNewFileTexture,       (Vector2){310, 235}, .0f, .75f, WHITE);
        DrawTextureEx(currSaveFileTexture,      (Vector2){310, 260}, .0f, .75f, WHITE);
        DrawTextureEx(currStopTrackTexture,     (Vector2){35 , 235}, .0f, .75f, WHITE);
        DrawTextureEx(currArmTrackTexture,      (Vector2){82 , 235}, .0f, .75f, WHITE);
        DrawTextureEx(currRecTrackTexture,      (Vector2){129, 235}, .0f, .75f, WHITE);
        DrawTextureEx(currPauseTrackTexture,    (Vector2){176, 235}, .0f, .75f, WHITE);
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

void sndFileInit(char* rateStr)
{
        if (!FILE_INITD) {
                Data.sfName = fName;
                const int channels = 2;
                const int samplerate = rateStr != NULL ? atoi(rateStr) : 48000;
                const int frames = samplerate;

                sfinfo.samplerate = samplerate;
                sfinfo.channels = channels;
                sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_32;
                Data.sf = sf_open(Data.sfName, SFM_WRITE, &sfinfo);
                if (!Data.sf) {
                    fprintf(stderr, "Error opening file: %s\n", sf_strerror(NULL));
                    return;
                }

                FILE_INITD = true;
        }
}

void argHandle()
{
        //int opt;
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

}

int main(int argc, char *argv[])
{

        InitWindow(600, 350, "frecorder");
        SetTargetFPS(60);
        Texture2D backgroundTexture = LoadTexture("./assets/Background.png");
        Texture2D screenTexture = LoadTexture("./assets/Screen.png");
        Texture2D transportTexture = LoadTexture("./assets/Transport.png");
        Texture2D faderTexture = LoadTexture("./assets/Fader.png");

        Texture2D armTrack = LoadTexture("./assets/arm-track.png");
        Texture2D armTrackPressed = LoadTexture("./assets/arm-track-pressed.png");
        Texture2D recTrack = LoadTexture("./assets/play.png");
        Texture2D recTrackPressed = LoadTexture("./assets/play-pressed.png");
        Texture2D stopTrack = LoadTexture("./assets/stop-track.png");
        Texture2D stopTrackPressed = LoadTexture("./assets/stop-track-pressed.png");
        Texture2D pauseTrack = LoadTexture("./assets/pause-track.png");
        Texture2D pauseTrackPressed = LoadTexture("./assets/pause-track-pressed.png");

        Texture2D newFile = LoadTexture("./assets/new.png");
        Texture2D newFilePressed = LoadTexture("./assets/new-pressed.png");
        Texture2D saveFile = LoadTexture("./assets/save.png");
        Texture2D saveFilePressed = LoadTexture("./assets/save-pressed.png");

        TextureButton stopBtn   = newButton((Rectangle){  35, 235, 60, 60 }, stopTrack, stopTrackPressed, stopTrack, WHITE);
        TextureButton armBtn    = newButton((Rectangle){  82, 235, 60, 60 }, armTrack, armTrackPressed, armTrack, WHITE);
        TextureButton recBtn    = newButton((Rectangle){ 129, 235, 60, 60 }, recTrack, recTrackPressed, recTrack, WHITE);
        TextureButton pauseBtn  = newButton((Rectangle){ 176, 235, 60, 60 }, pauseTrack, pauseTrackPressed, pauseTrack, WHITE);
        TextureButton newBtn    = newButton((Rectangle){ 310, 235, 60, 30 }, newFile, newFilePressed, newFile, WHITE);
        TextureButton saveBtn   = newButton((Rectangle){ 310, 260, 60, 30 }, saveFile, saveFilePressed, saveFile, WHITE);

        transport.stopTrackBtn = stopBtn;
        transport.armTrackBtn = armBtn;
        transport.recTrackBtn = recBtn;
        transport.pauseTrackBtn = pauseBtn;
        transport.saveFileBtn = saveBtn;
        transport.newFileBtn = newBtn;

        pw_init(&argc, &argv);
        pipeData *pd = malloc(sizeof(pipeData));
        pd->dat = Data;
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

                bool filenameIsHovered = CheckCollisionPointRec(mousePos, (Rectangle){ 107, 23, 135, 18 });

                transport.newFileBtn.isHovered = CheckCollisionPointRec(mousePos, transport.newFileBtn.bounds);
                transport.saveFileBtn.isHovered = CheckCollisionPointRec(mousePos, transport.saveFileBtn.bounds);
                transport.pauseTrackBtn.isHovered = CheckCollisionPointRec(mousePos, transport.pauseTrackBtn.bounds);
                transport.armTrackBtn.isHovered = CheckCollisionPointRec(mousePos, transport.armTrackBtn.bounds);
                transport.recTrackBtn.isHovered = CheckCollisionPointRec(mousePos, transport.recTrackBtn.bounds);
                transport.stopTrackBtn.isHovered = CheckCollisionPointRec(mousePos, transport.stopTrackBtn.bounds);
                transport.stopTrackBtn.isPressed = false;
                transport.saveFileBtn.isPressed = false;
                transport.newFileBtn.isPressed = false;

                if (filenameIsHovered) updateFileName();
                else SetMouseCursor(MOUSE_CURSOR_DEFAULT);

                if (transport.armTrackBtn.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {

                        if (transport.armTrackBtn.isPressed) {
                                transport.armTrackBtn.isPressed = false;
                        } else {
                                transport.armTrackBtn.isPressed = true;
                                sndFileInit(rateStr);
                        }
                }

                if (transport.recTrackBtn.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (transport.recTrackBtn.isPressed) {
                                transport.recTrackBtn.isPressed = false;
                        } else {
                                transport.recTrackBtn.isPressed = true;
                        }
                }

                if (transport.pauseTrackBtn.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (!transport.pauseTrackBtn.isPressed) {
                                if (transport.armTrackBtn.isPressed)
                                        transport.armTrackBtn.isPressed = false;

                                if (transport.recTrackBtn.isPressed)
                                        transport.recTrackBtn.isPressed = false;

                                transport.pauseTrackBtn.isPressed = true;
                        } else {
                                if (transport.armTrackBtn.isPressed)
                                        transport.armTrackBtn.isPressed = true;

                                if (transport.recTrackBtn.isPressed)
                                        transport.recTrackBtn.isPressed = true;

                                transport.pauseTrackBtn.isPressed = false;
                        }
                }

                if (transport.stopTrackBtn.isHovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                        transport.stopTrackBtn.isPressed = true;
                }

                if (transport.stopTrackBtn.isPressed) {
                        transport.armTrackBtn.isPressed = false;
                        transport.recTrackBtn.isPressed = false;
                        transport.pauseTrackBtn.isPressed = false;
                        FILE_INITD = false;
                }

                if (transport.saveFileBtn.isHovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
                        transport.saveFileBtn.isPressed = true;

                if (transport.newFileBtn.isHovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
                        transport.newFileBtn.isPressed = true;

                BeginDrawing();
                ClearBackground(BLACK);
                DrawTexture(backgroundTexture, 0, 0, WHITE);
                DrawTexture(screenTexture, 20, 20, ORANGE);
                DrawRectangleLines(20, 20, 425, 150, BLACK);
                drawInfo(Data, sfinfo);
                drawVolumeMeters(faderTexture);
                drawVolumeValues();
                DrawTexture(transportTexture, 20, 220, WHITE);
                DrawRectangleLines(20, 220, 425, 75, BLACK);
                drawControls();
                EndDrawing();
        }


        pthread_detach(guiThread);
        pthread_join(pipeThread, NULL);
        pw_stream_destroy(Data.stream);
        pw_main_loop_destroy(Data.loop);
        pw_deinit();
        UnloadTexture(stopTrack);
        UnloadTexture(armTrack);
        UnloadTexture(recTrack);
        CloseWindow();
        return 0;
}
