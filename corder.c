#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pipewire/pipewire.h>
#include "corder.h"
#include "pipe.h"
#include "ui.h"

int rc;
float SAMPLE_LEFT;
float SAMPLE_RIGHT;
char* PATH;

pthread_t guiThread;
pthread_t pipeThread;
Transport transport;

float VOL_RATIO                 = .1f;
bool GUI_DISABLE                = false;
bool FILE_INITD                 = false;
bool MOUSE_ON_INPUT             = false;
char* SAVE_PATH                 = "/extra/Music/";

data Data                       = { 0 };
SF_INFO sfinfo                  = { 0 };

size_t charCount                = 0;
int samplerate                  = SAMPLING_RATE_48K;
int channels                    = STEREO;
char* rateStr                   = NULL;
char* streamTarget              = NULL;
char* path                      = NULL;
char fName[MAX_FILE_CHAR+1]     = "\0";

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

void sndFileInit(char* rateStr)
{
        if (!FILE_INITD) {
                Data.sfName = fName;

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

void argHandle(int argc, char* argv[])
{
        int opt;
        while ((opt = getopt_long(argc, argv, "hvfrotnc:", options, NULL)) != -1) {
                switch (opt) {
                        case 'h':
                                printf("Usage: ...\n");
                                return;
                        case 'v':
                                printf("Version: %s\n", version);
                                return;
                        case 'o':
                                path = optarg;
                                transport.armTrackBtn.isPressed = true;
                                transport.recTrackBtn.isPressed = true;
                                if (path) strncpy(fName, path, MAX_FILE_CHAR);
                                else strncpy(fName, "recording.wav", MAX_FILE_CHAR);
                                fName[MAX_FILE_CHAR] = '\0';
                                sndFileInit(rateStr);
                                break;
                        case 'r':
                                rateStr = optarg;
                                break;
                        case 't':
                                streamTarget = optarg;
                                break;
                        case 'n':
                                GUI_DISABLE = true;
                                break;
                        case 'c':
                                if (atoi(optarg) != 0)
                                        channels = atoi(optarg);
                                break;
                        case '?':
                                default:
                                break;
               }
        }

}

int main(int argc, char *argv[])
{
        argHandle(argc, argv);
        pw_init(&argc, &argv);
        pipeData *pd = malloc(sizeof(pipeData));
        pd->dat = Data;
        pd->target = streamTarget;
        pd->argc = argc;

        if (GUI_DISABLE) {
                piper(pd);
                if (Data.sf) sf_close(Data.sf);
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
        Texture2D backgroundTexture = LoadTexture("/usr/share/opencorder/assets/Background.png");
        Texture2D screenTexture     = LoadTexture("/usr/share/opencorder/assets/Screen.png");
        Texture2D transportTexture  = LoadTexture("/usr/share/opencorder/assets/Transport.png");
        Texture2D faderTexture      = LoadTexture("/usr/share/opencorder/assets/Fader.png");

        Texture2D armTrack          = LoadTexture("/usr/share/opencorder/assets/arm-track.png");
        Texture2D armTrackPressed   = LoadTexture("/usr/share/opencorder/assets/arm-track-pressed.png");
        Texture2D recTrack          = LoadTexture("/usr/share/opencorder/assets/play.png");
        Texture2D recTrackPressed   = LoadTexture("/usr/share/opencorder/assets/play-pressed.png");
        Texture2D stopTrack         = LoadTexture("/usr/share/opencorder/assets/stop-track.png");
        Texture2D stopTrackPressed  = LoadTexture("/usr/share/opencorder/assets/stop-track-pressed.png");
        Texture2D pauseTrack        = LoadTexture("/usr/share/opencorder/assets/pause-track.png");
        Texture2D pauseTrackPressed = LoadTexture("/usr/share/opencorder/assets/pause-track-pressed.png");

        Texture2D newFile           = LoadTexture("/usr/share/opencorder/assets/new.png");
        Texture2D newFilePressed    = LoadTexture("/usr/share/opencorder/assets/new-pressed.png");
        Texture2D saveFile          = LoadTexture("/usr/share/opencorder/assets/save.png");
        Texture2D saveFilePressed   = LoadTexture("/usr/share/opencorder/assets/save-pressed.png");

        TextureButton stopBtn   = newButton((Rectangle){ STOP_BTN_X,    HIGH_BTN_Y, BTN_W, BTN_H        },
                        stopTrack, stopTrackPressed, stopTrack, WHITE);
        TextureButton armBtn    = newButton((Rectangle){ ARM_BTN_X,     HIGH_BTN_Y, BTN_W, BTN_H        },
                        armTrack, armTrackPressed, armTrack, WHITE);
        TextureButton recBtn    = newButton((Rectangle){ REC_BTN_X,     HIGH_BTN_Y, BTN_W, BTN_H        },
                        recTrack, recTrackPressed, recTrack, WHITE);
        TextureButton pauseBtn  = newButton((Rectangle){ PAUSE_BTN_X,   HIGH_BTN_Y, BTN_W, BTN_H        },
                        pauseTrack, pauseTrackPressed, pauseTrack, WHITE);
        TextureButton newBtn    = newButton((Rectangle){ NEW_BTN_X,     HIGH_BTN_Y, BTN_W, SMALL_BTN_H  },
                        newFile, newFilePressed, newFile, WHITE);
        TextureButton saveBtn   = newButton((Rectangle){ SAVE_BTN_X,    LOW_BTN_Y, BTN_W,  SMALL_BTN_H  },
                        saveFile, saveFilePressed, saveFile, WHITE);

        transport.stopTrackBtn  = stopBtn;
        transport.armTrackBtn   = armBtn;
        transport.recTrackBtn   = recBtn;
        transport.pauseTrackBtn = pauseBtn;
        transport.saveFileBtn   = saveBtn;
        transport.newFileBtn    = newBtn;

        while (!WindowShouldClose() && !GUI_DISABLE) {
                Vector2 mousePos = GetMousePosition();

                bool filenameIsHovered = CheckCollisionPointRec(mousePos, fileNameInput);
                bool sampleRateIsHovered = CheckCollisionPointRec(mousePos, sampleRateInput);
                bool channelsIsHovered = CheckCollisionPointRec(mousePos, channelsInput);

                transport.newFileBtn.isHovered    = CheckCollisionPointRec(mousePos, transport.newFileBtn.bounds);
                transport.saveFileBtn.isHovered   = CheckCollisionPointRec(mousePos, transport.saveFileBtn.bounds);
                transport.pauseTrackBtn.isHovered = CheckCollisionPointRec(mousePos, transport.pauseTrackBtn.bounds);
                transport.armTrackBtn.isHovered   = CheckCollisionPointRec(mousePos, transport.armTrackBtn.bounds);
                transport.recTrackBtn.isHovered   = CheckCollisionPointRec(mousePos, transport.recTrackBtn.bounds);
                transport.stopTrackBtn.isHovered  = CheckCollisionPointRec(mousePos, transport.stopTrackBtn.bounds);
                transport.newFileBtn.isHovered    = CheckCollisionPointRec(mousePos, transport.newFileBtn.bounds);
                transport.saveFileBtn.isHovered   = CheckCollisionPointRec(mousePos, transport.saveFileBtn.bounds);
                transport.stopTrackBtn.isPressed  = false;
                transport.saveFileBtn.isPressed   = false;
                transport.newFileBtn.isPressed    = false;

                if (filenameIsHovered) updateFileName();
                else SetMouseCursor(MOUSE_CURSOR_DEFAULT);

                if (sampleRateIsHovered)
                        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

                if (sampleRateIsHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        // toggle sample rate for now
                        // sfinfo.samplerate = samplerate;
                        switch (samplerate) {
                                case SAMPLING_RATE_48K:
                                samplerate = SAMPLING_RATE_44K;
                                break;
                                case SAMPLING_RATE_44K:
                                samplerate = SAMPLING_RATE_22K;
                                break;
                                case SAMPLING_RATE_22K:
                                samplerate = SAMPLING_RATE_11K;
                                break;
                                default:
                                samplerate = SAMPLING_RATE_48K;
                                break;
                        }
                }

                if (channelsIsHovered)
                        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

                if (channelsIsHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        // sfinfo.channels = samplerate;
                        switch (channels) {
                                case STEREO:
                                channels = MONO;
                                break;
                                default:
                                channels = STEREO;
                                break;
                        }
                }

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

                if (transport.newFileBtn.isHovered)
                        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

                if (transport.newFileBtn.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        transport.newFileBtn.isPressed = true;
                        FILE_INITD = false;
                        memset(fName, 0, sizeof(fName) - 1);
                        charCount = 0;
                        sf_close(Data.sf);
                        samplerate = SAMPLING_RATE_48K;
                        channels = STEREO;
                }

                if (transport.saveFileBtn.isHovered)
                        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

                if (transport.saveFileBtn.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        transport.saveFileBtn.isPressed = true;
                }

                BeginDrawing();
                ClearBackground(BLACK);
                DrawTexture(backgroundTexture, 0, 0, WHITE);
                DrawTexture(screenTexture, SCREEN_X, SCREEN_Y, WHITE);
                DrawRectangleLines(SCREEN_X, SCREEN_Y, SCREEN_W, SCREEN_H, SCREEN_BORDER_C);
                drawInfo(&Data, sfinfo);
                drawVolumeMeters(faderTexture);
                drawVolumeValues();
                DrawTexture(transportTexture, TRANSPORT_X, TRANSPORT_Y, WHITE);
                DrawRectangleLines(TRANSPORT_X, TRANSPORT_Y, TRANSPORT_W, TRANSPORT_H, BLACK);
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
