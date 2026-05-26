#include <stdio.h>
#include <stdlib.h>
#include "corder.h"
#include "pipe.h"
#include "ui.h"

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

void drawInfo(struct data *data, SF_INFO sfinfo)
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

