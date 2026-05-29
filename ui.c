#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "config.h"
#include "corder.h"
#include "pipe.h"
#include "ui.h"

Rectangle fileNameInput         = (Rectangle){ HEADER_INPUT_X, HEADER_FILE_Y,   HEADER_INPUT_WIDTH, HEADER_HEIGHT };
Rectangle sampleRateInput       = (Rectangle){ HEADER_INPUT_X, HEADER_RATE_Y,   HEADER_INPUT_WIDTH, HEADER_HEIGHT };
Rectangle channelsInput         = (Rectangle){ HEADER_INPUT_X, HEADER_MASTER_Y, HEADER_INPUT_WIDTH, HEADER_HEIGHT };

void drawheader()
{
        DrawRectangle(HEADER_X, HEADER_FILE_Y, HEADER_WIDTH, HEADER_HEIGHT, HEADER_BG_COLOR);
        DrawRectangleLines(HEADER_X, HEADER_FILE_Y, HEADER_WIDTH, HEADER_HEIGHT, HEADER_FG_COLOR);
        DrawText("File name", HEADER_TEXT_X, HEADER_FILE_TEXT_Y, HEADER_TEXT_SIZE, HEADER_FG_COLOR);
        DrawRectangle(HEADER_X, HEADER_RATE_Y, HEADER_WIDTH, HEADER_HEIGHT, HEADER_BG_COLOR);
        DrawRectangleLines(HEADER_X, HEADER_RATE_Y, HEADER_WIDTH, HEADER_HEIGHT, HEADER_FG_COLOR);
        DrawText("Sample rate", HEADER_TEXT_X, HEADER_RATE_TEXT_Y, HEADER_TEXT_SIZE, HEADER_FG_COLOR);
        DrawRectangle(HEADER_X, HEADER_MASTER_Y, HEADER_WIDTH, HEADER_HEIGHT, HEADER_BG_COLOR);
        DrawRectangleLines(HEADER_X, HEADER_MASTER_Y, HEADER_WIDTH, HEADER_HEIGHT, HEADER_FG_COLOR);
        DrawText("Master", HEADER_TEXT_X, HEADER_MASTER_TEXT_Y, HEADER_TEXT_SIZE, HEADER_FG_COLOR);

        // filename
        DrawRectangle(HEADER_INPUT_X, HEADER_FILE_Y, HEADER_INPUT_WIDTH, HEADER_HEIGHT, HEADER_FG_COLOR);
        DrawRectangleLines(
                        fileNameInput.x,
                        fileNameInput.y,
                        fileNameInput.width,
                        fileNameInput.height,
                        HEADER_BORDER_COLOR
        );
        // sample rate
        DrawRectangle(HEADER_INPUT_X, HEADER_RATE_Y, HEADER_INPUT_WIDTH, HEADER_HEIGHT, HEADER_FG_COLOR);
        DrawRectangleLines(sampleRateInput.x,
                        sampleRateInput.y,
                        sampleRateInput.width,
                        sampleRateInput.height,
                        HEADER_BORDER_COLOR
        );
        // channels
        DrawRectangle(HEADER_INPUT_X, HEADER_MASTER_Y, HEADER_INPUT_WIDTH, HEADER_HEIGHT, HEADER_FG_COLOR);
        DrawRectangleLines(channelsInput.x,
                        channelsInput.y,
                        channelsInput.width,
                        channelsInput.height,
                        HEADER_BORDER_COLOR
        );
}

void drawInfo(struct data *data, SF_INFO sfinfo)
{
        drawheader();
        char* filename = FNAME;
        DrawText(filename, INFO_TEXT_X, FILE_NAME_Y, INFO_TEXT_SIZE, BEIGE);

        char rate[RATE_TEXT_LENGTH];
        int sp = SAMPLE_RATE;
        snprintf(rate, RATE_TEXT_LENGTH, "%d Hz", sp);
        DrawText(rate, INFO_TEXT_X, RATE_TEXT_Y, INFO_TEXT_SIZE, BEIGE);

        char master[MASTER_TEXT_LENGTH];
        int chans = CHANNELS;
        if (chans == 2)
                snprintf(master, MASTER_TEXT_LENGTH, "%s", "Stereo");

        if (chans == 1)
                snprintf(master, MASTER_TEXT_LENGTH, "%s", "Mono  ");

        DrawText(master, INFO_TEXT_X, MASTER_TEXT_Y, INFO_TEXT_SIZE, BEIGE);
}

void drawController()
{
#define CONTROLLER_W 400
#define CONTROLLER_H HEIGHT-TRANSPORT_H
#define CONTROLLER_X 0
#define CONTROLLER_Y 0
        DrawRectangle(CONTROLLER_X, CONTROLLER_Y, CONTROLLER_W, CONTROLLER_H, BLACK);
        drawInfo(&DATA, SFINFO);
}

void drawVolumeSection()
{
        DrawRectangle(WIDTH-200, 0, 200, HEIGHT, BLACK);
        DrawLineEx((Vector2){WIDTH-200, 0}, (Vector2){WIDTH-200, HEIGHT}, 2, RAYWHITE);
}

void drawVolumeMeters(Texture2D faderTex)
{
        drawVolumeSection();
        DrawTexture(faderTex, LEFT_FADER_X, FADER_Y, WHITE);
        DrawTexture(faderTex, RIGHT_FADER_X, FADER_Y, WHITE);

        float leftFull = SAMPLE_LEFT * FADER_HEIGHT;
        float rightFull = SAMPLE_RIGHT * FADER_HEIGHT;

        for (size_t j = 0; j < FADER_HEIGHT; ++j) {
                if (j < leftFull)
                        DrawRectangle(LEFT_METER_X, FADER_HEIGHT+METER_PADDING-j, FADER_WIDTH, METER_THICKNESS, METER_COLOR);

                if (j < rightFull)
                        DrawRectangle(RIGHT_METER_X, FADER_HEIGHT+METER_PADDING-j, FADER_WIDTH, METER_THICKNESS, METER_COLOR);
        }

        DrawRectangleLines(RIGHT_FADER_X, FADER_Y, FADER_WIDTH, FADER_HEIGHT, FADER_LINE_COLOR);
        DrawRectangleLines(LEFT_FADER_X, FADER_Y, FADER_WIDTH, FADER_HEIGHT, FADER_LINE_COLOR);
}

void drawVolumeValues()
{
        char* left = malloc(sizeof(SAMPLE_LEFT));
        char* right = malloc(sizeof(SAMPLE_RIGHT));

        snprintf(left, sizeof(SAMPLE_LEFT)+1, "%.2f", SAMPLE_LEFT);
        snprintf(right, sizeof(SAMPLE_RIGHT)+1, "%.2f", SAMPLE_RIGHT);

        DrawText(left, LEFT_VOLUME_VALUE_X, LEFT_VOLUME_VALUE_Y, LEFT_VOLUME_VALUE_S, LEFT_VOLUME_VALUE_C);
        DrawText(right, RIGHT_VOLUME_VALUE_X, RIGHT_VOLUME_VALUE_Y, RIGHT_VOLUME_VALUE_S, RIGHT_VOLUME_VALUE_C);

        free(left);
        free(right);
}

void drawVuMeters()
{
        drawVolumeSection();
        DrawRectangle(LEFT_VU_X, LEFT_VU_Y, VU_W, VU_H, RAYWHITE);
        DrawRectangle(RIGHT_VU_X, RIGHT_VU_Y, VU_W, VU_H, RAYWHITE);

        float angle = -45.f + SAMPLE_LEFT * 90.f;
        if (angle > 45) angle = 45;
        float radius = angle * DEG2RAD;

        DrawRing((Vector2){LEFT_NEEDLE_X, LEFT_NEEDLE_Y}, 60, 70, -135, -41, 24, Fade(RED, .4f));
        DrawRing((Vector2){LEFT_NEEDLE_X, LEFT_NEEDLE_Y}, 60, 70, -69, -41, 24, RED);

        Vector2 lstart = {LEFT_NEEDLE_X, LEFT_NEEDLE_Y};
        Vector2 lend = {
                lstart.x + NEEDLE_LEN * sinf(radius),
                lstart.y - NEEDLE_LEN * cosf(radius)
        };
        DrawLineEx(lstart, lend, NEEDLE_THICKNESS, DARKBLUE);
        DrawCircle(LEFT_NEEDLE_X, LEFT_NEDL_HOLD_Y, NEEDLE_HOLD_THIC, BLACK);

        DrawRing((Vector2){RIGHT_NEEDLE_X, RIGHT_NEEDLE_Y}, 60, 70, -135, -41, 24, Fade(RED, .4f));
        DrawRing((Vector2){RIGHT_NEEDLE_X, RIGHT_NEEDLE_Y}, 60, 70, -69, -41, 24, RED);
        Vector2 rstart = {RIGHT_NEEDLE_X, RIGHT_NEEDLE_Y};
        Vector2 rend = {
                rstart.x + NEEDLE_LEN * sinf(radius),
                rstart.y - NEEDLE_LEN * cosf(radius)
        };
        DrawLineEx(rstart, rend, NEEDLE_THICKNESS, DARKBLUE);
        DrawCircle(RIGHT_NEEDLE_X, RIGHT_NEDL_HOLD_Y, NEEDLE_HOLD_THIC, BLACK);

        drawVolumeValues();
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

void drawTransportControls()
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

        DrawTextureEx(currNewFileTexture,       (Vector2){NEW_BTN_X,    HIGH_BTN_Y}, .0f, BTN_SCALE, WHITE);
        DrawTextureEx(currSaveFileTexture,      (Vector2){SAVE_BTN_X,   LOW_BTN_Y }, .0f, BTN_SCALE, WHITE);
        DrawTextureEx(currStopTrackTexture,     (Vector2){STOP_BTN_X ,  HIGH_BTN_Y}, .0f, BTN_SCALE, WHITE);
        DrawTextureEx(currArmTrackTexture,      (Vector2){ARM_BTN_X ,   HIGH_BTN_Y}, .0f, BTN_SCALE, WHITE);
        DrawTextureEx(currRecTrackTexture,      (Vector2){REC_BTN_X,    HIGH_BTN_Y}, .0f, BTN_SCALE, WHITE);
        DrawTextureEx(currPauseTrackTexture,    (Vector2){PAUSE_BTN_X,  HIGH_BTN_Y}, .0f, BTN_SCALE, WHITE);
}

void drawTimer()
{
#define TIMER_W 120
#define TIMER_H 45
#define TIMER_X 205
#define TIMER_Y HEIGHT - 60
        DrawRectangle(TIMER_X, TIMER_Y, TIMER_W, TIMER_H, BEIGE);
        if (transport.recTrackBtn.isPressed) {
                double diff = difftime(time(NULL), CLOCK);
                int totalSeconds = (int)diff;
                int minutes = totalSeconds / 60;
                int seconds = totalSeconds % 60;
                snprintf(TIME, 64, "%02d:%02d", minutes, seconds);
                Vector2 dims = MeasureTextEx(GetFontDefault(), TIME, 36, 0);
                int len = MeasureText(TIME, 36);
                DrawText(TIME, (TIMER_X-2)+(TIMER_W/2.-(len/2.)), (TIMER_Y+2)+(TIMER_H-2)/2.-dims.y/2, 36, BLACK);
        } else if (transport.pauseTrackBtn.isPressed) {
                int len = MeasureText(TIME, 36);
                Vector2 dims = MeasureTextEx(GetFontDefault(), TIME, 36, 0);
                DrawText(TIME, (TIMER_X-2)+(TIMER_W/2.-(len/2.)), (TIMER_Y+2)+(TIMER_H-2)/2.-dims.y/2, 36, BLACK);
        } else {
                char *time = "00:00";
                int len = MeasureText(time, 36);
                Vector2 dims = MeasureTextEx(GetFontDefault(), time, 36, 0);
                DrawText(time, (TIMER_X-2)+(TIMER_W/2.-(len/2.)), (TIMER_Y+2)+(TIMER_H-2)/2.-dims.y/2, 36, BLACK);
        }
}

void drawTransport()
{
        DrawRectangle(TRANSPORT_X, TRANSPORT_Y, TRANSPORT_W, TRANSPORT_H, BLACK);
        DrawLineEx((Vector2){TRANSPORT_X, TRANSPORT_Y}, (Vector2){400, TRANSPORT_Y}, 2, RAYWHITE);
        drawTransportControls();
        drawTimer();
}

void updateFileName()
{
        SetMouseCursor(MOUSE_CURSOR_IBEAM);
        int key = GetCharPressed();

        while (key > 0) {
                if ((key >= 32) && (key <= 125) && (CHAR_COUNT < MAX_FILE_CHAR)) {
                        FNAME[CHAR_COUNT] = (char)key;
                        FNAME[CHAR_COUNT+1] = '\0';
                        CHAR_COUNT++;
                }

                key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE)) {
                CHAR_COUNT--;
                if (CHAR_COUNT < 0) CHAR_COUNT = 0;
                FNAME[CHAR_COUNT] = '\0';
        }
}
