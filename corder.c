#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pipewire/pipewire.h>
#include "config.h"
#include "corder.h"
#include "pipe.h"
#include "ui.h"

data    DATA                    = { 0 };
SF_INFO SFINFO                  = { 0 };

float   SAMPLE_LEFT;
float   SAMPLE_RIGHT;

float   VOL_RATIO               = 1.f;
bool    GUI_DISABLE             = false;
bool    FILE_INITD              = false;
bool    MOUSE_ON_INPUT          = false;
char    SEPARATOR[2]            = "-";
size_t  CLILEN                  = 69;

time_t  CLOCK                   = 0;
time_t  CLOCK_P                 = 0;
size_t  CHAR_COUNT              = 0;
int     SAMPLE_RATE             = SAMPLING_RATE_48K;
int     CHANNELS                = STEREO;
char*   RATESTR                 = NULL;
char*   STREAM_TARGET           = NULL;
char*   SAVE_PATH               = NULL;
char*   PATH                    = NULL;
char    TIME[64]                = "";
char    FNAME[MAX_FILE_CHAR+1]  = "\0";

int rc;
pthread_t guiThread;
pthread_t pipeThread;
Transport transport;
Filter filter;

void sndFileInit(char* rateStr)
{
        if (!FILE_INITD) {
                DATA.sfName = FNAME;

                SFINFO.samplerate = SAMPLE_RATE;
                SFINFO.channels = CHANNELS;
                SFINFO.format = SF_FORMAT_WAV | SF_FORMAT_PCM_32;
                DATA.sf = sf_open(DATA.sfName, SFM_WRITE, &SFINFO);
                if (!DATA.sf) {
                    fprintf(stderr, "Error opening file: %s\n", sf_strerror(NULL));
                    return;
                }

                FILE_INITD = true;
        }
}

int main(int argc, char *argv[])
{
        getConfig();
        int e = argHandle(argc, argv);
        if (e != 0) return 0;

        pw_init(&argc, &argv);
        pipeData *pd = malloc(sizeof(pipeData));
        pd->dat = DATA;
        pd->target = STREAM_TARGET;
        pd->argc = argc;

        if (GUI_DISABLE) {
                piper(pd);
                if (DATA.sf) sf_close(DATA.sf);
                pw_deinit();
                return 0;
        } else {
                rc = pthread_create(&pipeThread, NULL, piper, pd);
                if (rc < 0) {
                        fprintf(stdout, "\n");
                        free(pd);
                        return 1;
                }

        }

        InitWindow(WIDTH, HEIGHT, "opencorder");
        SetTargetFPS(FPS);

        Texture2D backgroundTexture     = LoadTexture("/usr/share/opencorder/assets/Background.png");
        Texture2D screenTexture         = LoadTexture("/usr/share/opencorder/assets/Screen.png");
        Texture2D transportTexture      = LoadTexture("/usr/share/opencorder/assets/Transport.png");
        Texture2D faderTexture          = LoadTexture("/usr/share/opencorder/assets/Fader.png");

        // Transport button textures
        Texture2D armTrack              = LoadTexture("/usr/share/opencorder/assets/arm-button-t.png");
        Texture2D armTrackPressed       = LoadTexture("/usr/share/opencorder/assets/arm-button-t-pressed.png");
        Texture2D recTrack              = LoadTexture("/usr/share/opencorder/assets/play-button-t.png");
        Texture2D recTrackPressed       = LoadTexture("/usr/share/opencorder/assets/play-button-t-pressed.png");
        Texture2D stopTrack             = LoadTexture("/usr/share/opencorder/assets/stop-button-t.png");
        Texture2D stopTrackPressed      = LoadTexture("/usr/share/opencorder/assets/stop-button-pressed.png");
        Texture2D pauseTrack            = LoadTexture("/usr/share/opencorder/assets/pause-button-t.png");
        Texture2D pauseTrackPressed     = LoadTexture("/usr/share/opencorder/assets/pause-button-t-pressed.png");

        Texture2D newFile               = LoadTexture("/usr/share/opencorder/assets/new-button-t.png");
        Texture2D newFilePressed        = LoadTexture("/usr/share/opencorder/assets/new-button-t-pressed.png");
        Texture2D saveFile              = LoadTexture("/usr/share/opencorder/assets/save-button-t.png");
        Texture2D saveFilePressed       = LoadTexture("/usr/share/opencorder/assets/save-button-t-pressed.png");

        // Filter button textures
        LOWPASS_BTN_TEX                 = LoadTexture("/usr/share/opencorder/assets/low-pass-button.png");
        LOWPASS_BTN_TEX_PRESSED         = LoadTexture("/usr/share/opencorder/assets/low-pass-button-pressed.png");
        MIDPASS_BTN_TEX                 = LoadTexture("/usr/share/opencorder/assets/mid-pass-button.png");
        MIDPASS_BTN_TEX_PRESSED         = LoadTexture("/usr/share/opencorder/assets/mid-pass-button-pressed.png");
        HIHPASS_BTN_TEX                 = LoadTexture("/usr/share/opencorder/assets/high-pass-button.png");
        HIHPASS_BTN_TEX_PRESSED         = LoadTexture("/usr/share/opencorder/assets/hi-pass-button-pressed.png");

        REEL                            = LoadTexture("/usr/share/opencorder/assets/reel.png");

        // Transport buttons
        TextureButton stopBtn   = newButton((Rectangle){ STOP_BTN_X,    HIGH_BTN_Y, BTN_W, BTN_H        },
                        stopTrack, stopTrackPressed, stopTrack, WHITE);
        TextureButton armBtn    = newButton((Rectangle){ ARM_BTN_X,     HIGH_BTN_Y, BTN_W, BTN_H        },
                        armTrack, armTrackPressed, armTrack, WHITE);
        TextureButton recBtn    = newButton((Rectangle){ REC_BTN_X,     HIGH_BTN_Y, BTN_W, BTN_H        },
                        recTrack, recTrackPressed, recTrack, WHITE);
        TextureButton pauseBtn  = newButton((Rectangle){ PAUSE_BTN_X,   HIGH_BTN_Y, BTN_W, BTN_H        },
                        pauseTrack, pauseTrackPressed, pauseTrack, WHITE);
        TextureButton newBtn    = newButton((Rectangle){ NEW_BTN_X,     NEW_BTN_Y, BTN_W, SMALL_BTN_H  },
                        newFile, newFilePressed, newFile, WHITE);
        TextureButton saveBtn   = newButton((Rectangle){ SAVE_BTN_X,    SAVE_BTN_Y, BTN_W,  SMALL_BTN_H  },
                        saveFile, saveFilePressed, saveFile, WHITE);

        transport.stopTrackBtn  = stopBtn;
        transport.armTrackBtn   = armBtn;
        transport.recTrackBtn   = recBtn;
        transport.pauseTrackBtn = pauseBtn;
        transport.saveFileBtn   = saveBtn;
        transport.newFileBtn    = newBtn;

        // Filter buttons
        TextureButton lopaBtn   = newButton((Rectangle){ LF_BTN_X, FLT_BTN_Y, BTN_W, BTN_H },
                LOWPASS_BTN_TEX, LOWPASS_BTN_TEX_PRESSED, LOWPASS_BTN_TEX, WHITE);
        TextureButton mipaBtn   = newButton((Rectangle){ MF_BTN_X, FLT_BTN_Y, BTN_W, BTN_H },
                MIDPASS_BTN_TEX, MIDPASS_BTN_TEX_PRESSED, MIDPASS_BTN_TEX, WHITE);
        TextureButton hipaBtn   = newButton((Rectangle){ HF_BTN_X, FLT_BTN_Y, BTN_W, BTN_H },
                HIHPASS_BTN_TEX, HIHPASS_BTN_TEX_PRESSED, HIHPASS_BTN_TEX, WHITE);

        // Set filter buttons
        filter.loPassBtn        = lopaBtn;
        filter.miPassBtn        = mipaBtn;
        filter.hiPassBtn        = hipaBtn;

        while (!WindowShouldClose() && !GUI_DISABLE) {
                Vector2 mousePos = GetMousePosition();

                bool filenameIsHovered              = CheckCollisionPointRec(mousePos, FILENAME_INPUT);
                bool sampleRateIsHovered            = CheckCollisionPointRec(mousePos, SAMPLERATE_INPUT);
                bool channelsIsHovered              = CheckCollisionPointRec(mousePos, CHANNELS_INPUTBASE);

                transport.newFileBtn.isHovered      = CheckCollisionPointRec(mousePos, transport.newFileBtn.bounds);
                transport.saveFileBtn.isHovered     = CheckCollisionPointRec(mousePos, transport.saveFileBtn.bounds);
                transport.pauseTrackBtn.isHovered   = CheckCollisionPointRec(mousePos, transport.pauseTrackBtn.bounds);
                transport.armTrackBtn.isHovered     = CheckCollisionPointRec(mousePos, transport.armTrackBtn.bounds);
                transport.recTrackBtn.isHovered     = CheckCollisionPointRec(mousePos, transport.recTrackBtn.bounds);
                transport.stopTrackBtn.isHovered    = CheckCollisionPointRec(mousePos, transport.stopTrackBtn.bounds);
                transport.newFileBtn.isHovered      = CheckCollisionPointRec(mousePos, transport.newFileBtn.bounds);
                transport.saveFileBtn.isHovered     = CheckCollisionPointRec(mousePos, transport.saveFileBtn.bounds);
                transport.stopTrackBtn.isPressed    = false;
                transport.saveFileBtn.isPressed     = false;
                transport.newFileBtn.isPressed      = false;

                filter.loPassBtn.isHovered          = CheckCollisionPointRec(mousePos, filter.loPassBtn.bounds);
                filter.miPassBtn.isHovered          = CheckCollisionPointRec(mousePos, filter.miPassBtn.bounds);
                filter.hiPassBtn.isHovered          = CheckCollisionPointRec(mousePos, filter.hiPassBtn.bounds);

                if (filenameIsHovered) updateFileName();
                else SetMouseCursor(MOUSE_CURSOR_DEFAULT);

                if (sampleRateIsHovered)
                        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

                if (sampleRateIsHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        switch (SAMPLE_RATE) {
                                case SAMPLING_RATE_48K:
                                SAMPLE_RATE = SAMPLING_RATE_44K;
                                break;
                                case SAMPLING_RATE_44K:
                                SAMPLE_RATE = SAMPLING_RATE_22K;
                                break;
                                case SAMPLING_RATE_22K:
                                SAMPLE_RATE = SAMPLING_RATE_11K;
                                break;
                                default:
                                SAMPLE_RATE = SAMPLING_RATE_48K;
                                break;
                        }
                }

                if (channelsIsHovered)
                        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

                if (channelsIsHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        switch (CHANNELS) {
                                case STEREO:
                                CHANNELS = MONO;
                                CHANNELS_INPUT.x += 20;
                                break;
                                default:
                                CHANNELS = STEREO;
                                CHANNELS_INPUT.x -= 20;
                                break;
                        }
                }

                if (transport.armTrackBtn.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {

                        if (transport.armTrackBtn.isPressed) {
                                transport.armTrackBtn.isPressed = false;
                        } else {
                                transport.armTrackBtn.isPressed = true;
                                sndFileInit(RATESTR);
                        }
                }

                if (transport.recTrackBtn.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (transport.recTrackBtn.isPressed) {
                                transport.recTrackBtn.isPressed = false;
                        } else {
                                if (!transport.pauseTrackBtn.isPressed) {
                                        CLOCK = time(NULL);
                                } else {
                                        CLOCK += time(NULL) - CLOCK_P;
                                        CLOCK_P = 0;
                                }

                                transport.recTrackBtn.isPressed = true;
                                transport.pauseTrackBtn.isPressed = false;
                        }
                }

                if (transport.pauseTrackBtn.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (!transport.pauseTrackBtn.isPressed) {
                                CLOCK_P = time(NULL);
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
                        SAMPLE_LEFT = 0;
                        SAMPLE_RIGHT = 0;
                }

                if (transport.newFileBtn.isHovered)
                        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

                if (transport.newFileBtn.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        transport.newFileBtn.isPressed = true;
                        FILE_INITD = false;
                        memset(FNAME, 0, sizeof(FNAME) - 1);
                        CHAR_COUNT = 0;
                        sf_close(DATA.sf);
                        SAMPLE_RATE = SAMPLING_RATE_48K;
                        CHANNELS = STEREO;
                }

                if (transport.saveFileBtn.isHovered)
                        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

                if (transport.saveFileBtn.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        transport.saveFileBtn.isPressed = true;
                }

                if (filter.loPassBtn.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (filter.loPassBtn.isPressed) {
                        filter.loPassBtn.isPressed = false;
                    } else {
                        filter.loPassBtn.isPressed = true;
                    }
                }

                if (filter.miPassBtn.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (filter.miPassBtn.isPressed) {
                        filter.miPassBtn.isPressed = false;
                    } else {
                        filter.miPassBtn.isPressed = true;
                    }
                }

                if (filter.hiPassBtn.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (filter.hiPassBtn.isPressed) {
                        filter.hiPassBtn.isPressed = false;
                    } else {
                        filter.hiPassBtn.isPressed = true;
                    }
                }

                BeginDrawing();
                ClearBackground(BLACK);
                DrawTexture(backgroundTexture, 0, 0, WHITE);
                drawController();
                drawTransport();
                drawVolumeValues();
                drawVuMeters();
                drawReels();
                drawTitle();
                drawFilterPanel();
                EndDrawing();
        }

        pthread_detach(guiThread);
        pthread_join(pipeThread, NULL);
        pw_stream_destroy(DATA.stream);
        pw_main_loop_destroy(DATA.loop);
        pw_deinit();
        UnloadTexture(stopTrack);
        UnloadTexture(armTrack);
        UnloadTexture(recTrack);
        CloseWindow();
        return 0;
}

int argHandle(int argc, char* argv[])
{
        int opt;
        while ((opt = getopt_long(argc, argv, "hvf:r:o:t:nc:", options, NULL)) != -1) {
                switch (opt) {
                        case 'h':
                                printf("[OpenCorder] %56s\n", "USAGE:");
                                for (size_t i = 0; i < CLILEN; ++i) fputs("-", stdout);
                                printf("\n-n, %21s, %42s\n",    "--nogui",      "No GUI, CLI only.");
                                printf("-r, %21s, %42s\n",      "--rate",       "Specify sampling rate.");
                                printf("-c, %21s, %42s\n",      "--channels",   "Specify number of channels.");
                                printf("-o, %21s, %42s\n",      "--output",     "Output file path.");
                                printf("-v, %21s, %42s\n",      "--version",    "Show version.");
                                printf("-h, %21s, %42s\n",      "--help",       "Show help.");
                                return 1;
                        case 'v':
                                printf("[OpenCorder] %56s\n", "MIT 2026");
                                for (size_t i = 0; i < CLILEN; ++i) fputs("-", stdout);
                                printf("\nVersion: %60s\n", VERSION);
                                return 1;
                        case 'o':
                                PATH = optarg;
                                transport.armTrackBtn.isPressed = true;
                                transport.recTrackBtn.isPressed = true;
                                if (PATH) strncpy(FNAME, PATH, MAX_FILE_CHAR);
                                else strncpy(FNAME, "recording.wav", MAX_FILE_CHAR);
                                FNAME[MAX_FILE_CHAR] = '\0';
                                sndFileInit(RATESTR);
                                break;
                        case 'r':
                                RATESTR = optarg;
                                if (atoi(RATESTR) != 0)
                                        SAMPLE_RATE = atoi(RATESTR);
                                break;
                        case 't':
                                STREAM_TARGET = optarg;
                                break;
                        case 'n':
                                GUI_DISABLE = true;
                                break;
                        case 'c':
                                if (atoi(optarg) != 0)
                                        CHANNELS = atoi(optarg);
                                break;
                        case '?':
                                default:
                                break;
               }
        }

        return 0;
}

void copyFile(char* targetPath)
{
        char ch;
        FILE *source, *target;

        source = fopen(PATH, "r");
        if (source == NULL) {
                fprintf(stderr, "Source file could not be opened.\n");
                return;
        }

        target = fopen(targetPath, "w");
        if (target == NULL) {
                fclose(source);
                fprintf(stderr, "Target file could not be opened.\n");
                return;
        }

        while ((ch = fgetc(source)) != EOF) {
                fputc(ch, target);
        }

        printf("File copied successfully.\n");
        fclose(source);
        fclose(target);
}

