#include <raylib.h>
#include <sndfile.h>

extern Rectangle fileNameInput;
extern Rectangle sampleRateInput;
extern Rectangle channelsInput;
extern Rectangle newFileInput;
extern Rectangle saveFileInput;

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

extern Transport transport;

void drawheader();

void drawInfo(struct data *data, SF_INFO sfinfo);

void drawVolumeMeters(Texture2D faderTex);

void drawVolumeValues();

TextureButton newButton(Rectangle bounds, Texture2D texture, Texture2D pressedTexture, Texture2D hoverTexture, Color tint);

void drawControls();
