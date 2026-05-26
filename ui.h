#include <raylib.h>
#include <sndfile.h>

/* Header */
#define HEADER_X                23
#define HEADER_TEXT_X           25
#define HEADER_WIDTH            85
#define HEADER_HEIGHT           18
#define HEADER_TEXT_SIZE        14
#define HEADER_FILE_Y           23
#define HEADER_FILE_TEXT_Y      25
#define HEADER_RATE_Y           41
#define HEADER_RATE_TEXT_Y      43
#define HEADER_MASTER_Y         59
#define HEADER_MASTER_TEXT_Y    62
#define HEADER_BG_COLOR         LIGHTGRAY
#define HEADER_FG_COLOR         BLACK
#define HEADER_BORDER_COLOR     BEIGE
#define HEADER_INPUT_X          107
#define HEADER_INPUT_WIDTH      135
/* Info */
#define INFO_TEXT_SIZE          14
#define INFO_TEXT_X             115
#define RATE_TEXT_LENGTH        9
#define RATE_TEXT_Y             43
#define MASTER_TEXT_LENGTH      7
#define MASTER_TEXT_Y           62
#define FILE_NAME_Y             25
/* Meters */
#define FADER_HEIGHT            280
#define FADER_WIDTH             30
#define LEFT_FADER_X            500
#define RIGHT_FADER_X           550
#define FADER_Y                 20
#define FADER_LINE_COLOR        BLACK
#define LEFT_METER_X            WIDTH-100
#define RIGHT_METER_X           WIDTH-50
#define METER_PADDING           19
#define METER_THICKNESS         1
#define METER_COLOR             GREEN
/* Volume Values */
#define LEFT_VOLUME_VALUE_X     505
#define LEFT_VOLUME_VALUE_Y     305
#define LEFT_VOLUME_VALUE_S     14
#define LEFT_VOLUME_VALUE_C     RED
#define RIGHT_VOLUME_VALUE_X    555
#define RIGHT_VOLUME_VALUE_Y    305
#define RIGHT_VOLUME_VALUE_S    14
#define RIGHT_VOLUME_VALUE_C    RED
//* Buttons *//
#define HIGH_BTN_Y              235
#define LOW_BTN_Y               260
#define NEW_BTN_X               310
#define NEW_BTN_Y               HIGH_BTN_Y
#define SAVE_BTN_X              310
#define SAVE_BTN_Y              LOW_BTN_Y
#define STOP_BTN_X              35
#define STOP_BTN_Y              HIGH_BTN_Y
#define ARM_BTN_X               82
#define ARM_BTN_Y               HIGH_BTN_Y
#define REC_BTN_X               129
#define REC_BTN_Y               HIGH_BTN_Y
#define PAUSE_BTN_X             176
#define PAUSE_BTN_Y             HIGH_BTN_Y
#define BTN_SCALE               .75f

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
