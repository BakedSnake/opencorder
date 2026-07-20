#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "config.h"
#include "corder.h"
#include "pipe.h"
#include "ui.h"

Rectangle FILENAME_INPUT         = (Rectangle){ HEADER_INPUT_X, HEADER_INPUT_Y,   HEADER_INPUT_WIDTH, HEADER_H };
Rectangle SAMPLERATE_INPUT       = (Rectangle){ HEADER_INPUT_X, HEADER_RATE_INPUT_Y,   HEADER_INPUT_WIDTH, HEADER_H };
Rectangle CHANNELS_INPUTBASE     = (Rectangle){ WIDTH-PUSH_BTN_W-50, HEADER_MASTER_Y, PUSH_BSE_W, PUSH_BSE_H };
Rectangle CHANNELS_INPUT         = (Rectangle){ WIDTH-PUSH_BTN_W-50, HEADER_MASTER_Y, PUSH_BTN_W, PUSH_BTN_H };
Rectangle LEFT_REEL              = (Rectangle){ LEFT_REEL_X, REEL_Y, REEL_W, REEL_H };
Rectangle RIGHT_REEL             = (Rectangle){ RIGHT_REEL_X, REEL_Y, REEL_W, REEL_H };
Texture2D REEL;
float ROTATION                   = 0.;

ButtonSwitch newPushButton(Rectangle bounds, Rectangle measure)
{
        ButtonSwitch new = { bounds, measure, { false, false } };
        return new;
}

void drawPushButton(ButtonSwitch btn)
{
        DrawRectangle(btn.bounds.x, btn.bounds.y, PUSH_BSE_W, PUSH_BSE_H, PUSH_BSE_C);
        DrawRectangle(btn.measure.x, btn.measure.y, PUSH_BTN_W, PUSH_BTN_H, PUSH_BTN_C);
}

void drawTitle(void)
{
    char title[24];
    snprintf(title, 24, "Opencorder v%s", VERSION);
    DrawText(title, 5, 5, 16, RAYWHITE);
    DrawLine(3, 24, CONTROLLER_W, 24, RAYWHITE);
}

void drawFilterPanel(void)
{
    DrawLineEx((Vector2){5, 90},(Vector2){CONTROLLER_W, 90}, 2, RAYWHITE);
    DrawCircle(CONTROLLER_W-40, 55, 20, DARKGRAY);
    DrawCircle(CONTROLLER_W-40, 68, 3, BLACK);
}

void drawheader(void)
{
        DrawRectangle(HEADER_X, HEADER_FILE_Y, HEADER_W, HEADER_H, HEADER_BG_COLOR);
        DrawRectangleLines(HEADER_X, HEADER_FILE_Y, HEADER_W, HEADER_H, HEADER_BG_COLOR);
        DrawText("File name", HEADER_TEXT_X, HEADER_FILE_TEXT_Y, HEADER_TEXT_SIZE, HEADER_FG_COLOR);
        DrawRectangle(HEADER_X, HEADER_RATE_Y, HEADER_W, HEADER_H, HEADER_BG_COLOR);
        DrawRectangleLines(HEADER_X, HEADER_RATE_Y, HEADER_W, HEADER_H, HEADER_BG_COLOR);
        DrawText("Sample rate", HEADER_TEXT_X, HEADER_RATE_TEXT_Y, HEADER_TEXT_SIZE, HEADER_FG_COLOR);

        // filename
        DrawRectangle(HEADER_INPUT_X, HEADER_INPUT_Y, HEADER_INPUT_WIDTH, HEADER_H, HEADER_FG_COLOR);
        DrawRectangleLines(
                        FILENAME_INPUT.x,
                        FILENAME_INPUT.y,
                        FILENAME_INPUT.width,
                        FILENAME_INPUT.height,
                        HEADER_BORDER_COLOR
        );
        // sample rate
        DrawRectangle(HEADER_INPUT_X, HEADER_RATE_INPUT_Y, HEADER_INPUT_WIDTH, HEADER_H, HEADER_FG_COLOR);
        DrawRectangleLines(SAMPLERATE_INPUT.x,
                        SAMPLERATE_INPUT.y,
                        SAMPLERATE_INPUT.width,
                        SAMPLERATE_INPUT.height,
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

        DrawText(master, HEADER_X, MASTER_TEXT_Y, INFO_TEXT_SIZE, BEIGE);

        ButtonSwitch masterBtn = newPushButton(CHANNELS_INPUTBASE, CHANNELS_INPUT);
        drawPushButton(masterBtn);
}

void drawController(void)
{
        DrawRectangle(CONTROLLER_X, CONTROLLER_Y, CONTROLLER_W, CONTROLLER_H, BLACK);
}

void drawVolumeSection(void)
{
        DrawRectangle(WIDTH-200, 0, 200, HEIGHT, BLACK);
        DrawLineEx((Vector2){WIDTH-200, 5}, (Vector2){WIDTH-200, HEIGHT-5}, 2, RAYWHITE);
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

void drawVolumeValues(void)
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

void drawVuMeters(void)
{
        drawVolumeSection();
        DrawRectangle(LEFT_VU_X, LEFT_VU_Y, VU_W, VU_H, RAYWHITE);
        DrawRectangle(RIGHT_VU_X, RIGHT_VU_Y, VU_W, VU_H, RAYWHITE);

        float langle = -45.f + SAMPLE_LEFT * 90.f;
        if (langle > 45) langle = 45;
        float lrad = langle * DEG2RAD;

        DrawRing((Vector2){LEFT_NEEDLE_X, LEFT_NEEDLE_Y}, 60, 70, -135, -41, 24, Fade(RED, .4f));
        DrawRing((Vector2){LEFT_NEEDLE_X, LEFT_NEEDLE_Y}, 60, 70, -69, -41, 24, RED);

        Vector2 lstart = {LEFT_NEEDLE_X, LEFT_NEEDLE_Y};
        Vector2 lend = {
                lstart.x + NEEDLE_LEN * sinf(lrad),
                lstart.y - NEEDLE_LEN * cosf(lrad)
        };
        DrawLineEx(lstart, lend, NEEDLE_THICKNESS, DARKBLUE);
        DrawCircle(LEFT_NEEDLE_X, LEFT_NEDL_HOLD_Y, NEEDLE_HOLD_THIC, BLACK);

        float rangle = -45.f + SAMPLE_RIGHT * 90.f;
        if (rangle > 45) rangle = 45;
        float rrad = rangle * DEG2RAD;

        DrawRing((Vector2){RIGHT_NEEDLE_X, RIGHT_NEEDLE_Y}, 60, 70, -135, -41, 24, Fade(RED, .4f));
        DrawRing((Vector2){RIGHT_NEEDLE_X, RIGHT_NEEDLE_Y}, 60, 70, -69, -41, 24, RED);
        Vector2 rstart = {RIGHT_NEEDLE_X, RIGHT_NEEDLE_Y};
        Vector2 rend = {
                rstart.x + NEEDLE_LEN * sinf(rrad),
                rstart.y - NEEDLE_LEN * cosf(rrad)
        };
        DrawLineEx(rstart, rend, NEEDLE_THICKNESS, DARKBLUE);
        DrawCircle(RIGHT_NEEDLE_X, RIGHT_NEDL_HOLD_Y, NEEDLE_HOLD_THIC, BLACK);

        drawVolumeValues();
        drawInfo(&DATA, SFINFO);
}

void drawReel(Texture2D tex, Rectangle bounds)
{
        Vector2 origin = { bounds.width/2., bounds.height/2. };
        DrawTexturePro(tex, (Rectangle){ 0, 0, bounds.width, bounds.height}, bounds, origin, ROTATION, WHITE);
}

void drawReels(void)
{
        drawReel(REEL, LEFT_REEL);
        drawReel(REEL, RIGHT_REEL);
        DrawRectangle(LEFT_REEL_X-REEL_W/2., REEL_Y-REEL_H/2., 122, 60, GetColor(0x000000AA));
        DrawLineEx((Vector2){LEFT_REEL_X-REEL_W/2.-5, REEL_Y-REEL_H/2.},
                        (Vector2) {LEFT_REEL_X-REEL_W/2.-5, REEL_Y+REEL_H/2.}, 2, RAYWHITE);
        DrawLineEx((Vector2){RIGHT_REEL_X+REEL_W/2.+5, REEL_Y-REEL_H/2.},
                        (Vector2) {RIGHT_REEL_X+REEL_W/2.+5, REEL_Y+REEL_H/2.}, 2, RAYWHITE);
        if (transport.recTrackBtn.isPressed) ROTATION++;

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

void drawTransportControls(void)
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

        DrawTextureEx(currNewFileTexture,       (Vector2){NEW_BTN_X,    NEW_BTN_Y}, .0f, BTN_SCALE, WHITE);
        DrawTextureEx(currSaveFileTexture,      (Vector2){SAVE_BTN_X,   SAVE_BTN_Y }, .0f, BTN_SCALE, WHITE);
        DrawTextureEx(currStopTrackTexture,     (Vector2){STOP_BTN_X ,  HIGH_BTN_Y}, .0f, BTN_SCALE, WHITE);
        DrawTextureEx(currArmTrackTexture,      (Vector2){ARM_BTN_X ,   HIGH_BTN_Y}, .0f, BTN_SCALE, WHITE);
        DrawTextureEx(currRecTrackTexture,      (Vector2){REC_BTN_X,    HIGH_BTN_Y}, .0f, BTN_SCALE, WHITE);
        DrawTextureEx(currPauseTrackTexture,    (Vector2){PAUSE_BTN_X,  HIGH_BTN_Y}, .0f, BTN_SCALE, WHITE);
}

void drawTimer(void)
{
        DrawRectangle(TIMER_X, TIMER_Y, TIMER_W, TIMER_H, TIMER_C);
        if (transport.recTrackBtn.isPressed) {
                double diff = difftime(time(NULL), CLOCK);
                int totalSeconds = (int)diff;
                int minutes = totalSeconds / 60;
                int seconds = totalSeconds % 60;
                snprintf(TIME, 64, "%02d:%02d", minutes, seconds);
                Vector2 dims = MeasureTextEx(GetFontDefault(), TIME, TIMER_FONT_S, 0);
                int len = MeasureText(TIME, TIMER_FONT_S);
                DrawText(TIME, TIMER_TEXT_X(len), TIMER_TEXT_Y(dims.y), TIMER_FONT_S, TIMER_FONT_C);
        } else if (transport.pauseTrackBtn.isPressed) {
                int len = MeasureText(TIME, TIMER_FONT_S);
                Vector2 dims = MeasureTextEx(GetFontDefault(), TIME, TIMER_FONT_S, 0);
                DrawText(TIME, TIMER_TEXT_X(len), TIMER_TEXT_Y(dims.y), TIMER_FONT_S, TIMER_FONT_C);
        } else {
                char *time = "00:00";
                int len = MeasureText(time, TIMER_FONT_S);
                Vector2 dims = MeasureTextEx(GetFontDefault(), time, TIMER_FONT_S, 0);
                DrawText(time, TIMER_TEXT_X(len), TIMER_TEXT_Y(dims.y), TIMER_FONT_S, TIMER_FONT_C);
        }
}

void drawTransport(void)
{
        DrawRectangle(TRANSPORT_X, TRANSPORT_Y, TRANSPORT_W, TRANSPORT_H, BLACK);
        DrawLineEx((Vector2){TRANSPORT_X+5, TRANSPORT_Y}, (Vector2){400, TRANSPORT_Y}, 2, RAYWHITE);
        drawTransportControls();
        drawTimer();
}

void updateFileName(void)
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
