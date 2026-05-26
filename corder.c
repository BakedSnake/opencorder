#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <pipewire/pipewire.h>

#include "corder.h"
#include "pipe.h"

static char version[5] = "0.0.4";

float SAMPLE_LEFT;
float SAMPLE_RIGHT;
float VOL_RATIO = .1f;

bool GUI_DISABLE = false;
bool FILE_INITD = false;
bool MOUSE_ON_INPUT = false;

Transport transport;

data Data = { 0 };

int rc;

char* PATH;
char* SAVE_PATH="/extra/Music/";
char* rateStr = NULL;
char* streamTarget = NULL;
char* path = NULL;

SF_INFO sfinfo = {0};

char fName[MAX_FILE_CHAR+1] = "\0";
size_t charCount = 0;
int samplerate = 48000;
int channels = 2;

pthread_t guiThread;
pthread_t pipeThread;

Rectangle fileNameInput = (Rectangle){ 107, 23, 135, 18 };
Rectangle sampleRateInput = (Rectangle){ 107, 41, 135, 18 };
Rectangle channelsInput = (Rectangle){ 107, 59, 135, 18 };

Rectangle newFileInput = (Rectangle){ 310, 235, 60, 30 };
Rectangle saveFileInput = (Rectangle){ 310, 260, 60, 30 };

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

        // filename
        DrawRectangle(107, 23, 135, 18, BLACK);
        DrawRectangleLines(
                        fileNameInput.x,
                        fileNameInput.y,
                        fileNameInput.width,
                        fileNameInput.height,
                        BEIGE
        );
        // sample rate
        DrawRectangle(107, 41, 135, 18, BLACK);
        DrawRectangleLines(sampleRateInput.x,
                        sampleRateInput.y,
                        sampleRateInput.width,
                        sampleRateInput.height,
                        BEIGE
        );
        // channels
        DrawRectangle(107, 59, 135, 18, BLACK);
        DrawRectangleLines(channelsInput.x,
                        channelsInput.y,
                        channelsInput.width,
                        channelsInput.height,
                        BEIGE
        );
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
        int sp = samplerate;
        snprintf(rate, 9, "%d Hz", sp);
        DrawText(rate, 115, 43, 14, BEIGE);

        char master[7];
        int chans = channels;
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
        while ((opt = getopt_long(argc, argv, "hvfrotn:", options, NULL)) != -1) {
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

        InitWindow(600, 350, "opencorder");
        SetTargetFPS(60);
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

        TextureButton stopBtn   = newButton((Rectangle){  35, 235, 60, 60 }, stopTrack, stopTrackPressed, stopTrack, WHITE);
        TextureButton armBtn    = newButton((Rectangle){  82, 235, 60, 60 }, armTrack, armTrackPressed, armTrack, WHITE);
        TextureButton recBtn    = newButton((Rectangle){ 129, 235, 60, 60 }, recTrack, recTrackPressed, recTrack, WHITE);
        TextureButton pauseBtn  = newButton((Rectangle){ 176, 235, 60, 60 }, pauseTrack, pauseTrackPressed, pauseTrack, WHITE);
        TextureButton newBtn    = newButton((Rectangle){ 310, 235, 60, 30 }, newFile, newFilePressed, newFile, WHITE);
        TextureButton saveBtn   = newButton((Rectangle){ 310, 260, 60, 30 }, saveFile, saveFilePressed, saveFile, WHITE);

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
                                case 48000:
                                samplerate = 44100;
                                break;
                                case 44100:
                                samplerate = 22050;
                                break;
                                case 22050:
                                samplerate = 11025;
                                break;
                                default:
                                samplerate = 48000;
                                break;
                        }
                }

                if (channelsIsHovered)
                        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

                if (channelsIsHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        // sfinfo.channels = samplerate;
                        switch (channels) {
                                case 2:
                                channels = 1;
                                break;
                                default:
                                channels = 2;
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
                        samplerate = 48000;
                        channels = 2;
                }

                if (transport.saveFileBtn.isHovered)
                        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

                if (transport.saveFileBtn.isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        transport.saveFileBtn.isPressed = true;
                }

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
