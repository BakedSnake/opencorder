#include <stdio.h>
#include <stdlib.h>
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

void drawVolumeMeters(Texture2D faderTex)
{
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

        DrawTextureEx(currNewFileTexture,       (Vector2){NEW_BTN_X,    HIGH_BTN_Y}, .0f, BTN_SCALE, WHITE);
        DrawTextureEx(currSaveFileTexture,      (Vector2){SAVE_BTN_X,   LOW_BTN_Y }, .0f, BTN_SCALE, WHITE);
        DrawTextureEx(currStopTrackTexture,     (Vector2){STOP_BTN_X ,  HIGH_BTN_Y}, .0f, BTN_SCALE, WHITE);
        DrawTextureEx(currArmTrackTexture,      (Vector2){ARM_BTN_X ,   HIGH_BTN_Y}, .0f, BTN_SCALE, WHITE);
        DrawTextureEx(currRecTrackTexture,      (Vector2){REC_BTN_X,    HIGH_BTN_Y}, .0f, BTN_SCALE, WHITE);
        DrawTextureEx(currPauseTrackTexture,    (Vector2){PAUSE_BTN_X,  HIGH_BTN_Y}, .0f, BTN_SCALE, WHITE);
}

