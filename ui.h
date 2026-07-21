#include <raylib.h>
#include <sndfile.h>

/* Main */
#define FPS                     60
#define WIDTH                   600
#define HEIGHT                  350
#define SCREEN_X                20
#define SCREEN_Y                20
#define SCREEN_W                425
#define SCREEN_H                150
#define SCREEN_BORDER_C         BLACK
#define TRANSPORT_W             400
#define TRANSPORT_H             75
#define TRANSPORT_X             0
#define TRANSPORT_Y             HEIGHT - TRANSPORT_H
#define TRANSPORT_BORDER_C      BLACK
/* Header */
#define HEADER_H                18
#define HEADER_W                140
#define HEADER_X                (400 + 200/2.) - HEADER_W/2.
#define HEADER_TEXT_X           HEADER_X+2
#define HEADER_TEXT_SIZE        14
#define HEADER_FILE_Y           243
#define HEADER_FILE_TEXT_Y      245
#define HEADER_RATE_Y           243+(HEADER_H*2)
#define HEADER_RATE_TEXT_Y      243+(HEADER_H*2)+1
#define HEADER_RATE_INPUT_Y     243+(HEADER_H*3)
#define HEADER_MASTER_Y         243+(HEADER_H*4)+8
#define HEADER_MASTER_TEXT_Y    52
#define HEADER_BG_COLOR         LIGHTGRAY
#define HEADER_FG_COLOR         BLACK
#define HEADER_BORDER_COLOR     RAYWHITE
#define HEADER_INPUT_X          HEADER_X
#define HEADER_INPUT_Y          HEADER_FILE_Y+HEADER_H
#define HEADER_INPUT_WIDTH      140
/* Info */
#define INFO_TEXT_SIZE          14
#define INFO_TEXT_X             HEADER_X+2
#define RATE_TEXT_LENGTH        9
#define RATE_TEXT_Y             HEADER_RATE_INPUT_Y+2
#define MASTER_TEXT_LENGTH      7
#define MASTER_TEXT_Y           243+(HEADER_H*4)+12
#define FILE_NAME_Y             HEADER_INPUT_Y+2
/* Controller */
#define CONTROLLER_W            400
#define CONTROLLER_H            HEIGHT-TRANSPORT_H
#define CONTROLLER_X            0
#define CONTROLLER_Y            0
/* Meters */
#define FADER_HEIGHT            280
#define FADER_WIDTH             30
#define LEFT_FADER_X            450
#define RIGHT_FADER_X           520
#define FADER_Y                 35
#define FADER_LINE_COLOR        BLACK
#define LEFT_METER_X            450
#define RIGHT_METER_X           520
#define METER_PADDING           34
#define METER_THICKNESS         1
#define METER_COLOR             GREEN
/* VU Meters */
#define VU_W                    140
#define VU_H                    90
#define LEFT_VU_X               (400 + 200/2.) - VU_W/2.
#define LEFT_VU_Y               20
#define RIGHT_VU_X              (400 + 200/2.) - VU_W/2.
#define RIGHT_VU_Y              VU_H + 40
#define VU_A                    0.f
#define NEEDLE_LEN              70
#define LEFT_NEEDLE_X           LEFT_VU_X+VU_W/2.
#define LEFT_NEEDLE_Y           108
#define RIGHT_NEEDLE_X          RIGHT_VU_X+VU_W/2.
#define RIGHT_NEEDLE_Y          218
#define NEEDLE_THICKNESS        2
#define NEEDLE_HOLD_THIC        8.f
#define LEFT_NEDL_HOLD_Y        110
#define RIGHT_NEDL_HOLD_Y       220
/* Volume Values */
#define LEFT_VOLUME_VALUE_X     540
#define LEFT_VOLUME_VALUE_Y     90
#define LEFT_VOLUME_VALUE_S     14
#define LEFT_VOLUME_VALUE_C     RED
#define RIGHT_VOLUME_VALUE_X    540
#define RIGHT_VOLUME_VALUE_Y    200
#define RIGHT_VOLUME_VALUE_S    14
#define RIGHT_VOLUME_VALUE_C    RED
/* Reels */
#define REEL_H                  60
#define REEL_W                  60
#define REEL_Y                  (HEIGHT-TRANSPORT_H) - (REEL_H/2.) - 2
#define LEFT_REEL_X             233
#define RIGHT_REEL_X            295
/* Timer */
#define TIMER_W                 120
#define TIMER_H                 45
#define TIMER_X                 205
#define TIMER_Y                 HEIGHT - 60
#define TIMER_FONT_S            36
#define TIMER_TEXT_X(len)       (TIMER_X-2)+(TIMER_W/2.-(len)/2.)
#define TIMER_TEXT_Y(hi)        (TIMER_Y+2)+((TIMER_H-2)/2.-(hi)/2.)
#define TIMER_C                 GetColor(0xAC3B4AFF)
#define TIMER_FONT_C            GetColor(0x62222AFF)
//* Buttons *//
#define BTN_W                   60
#define BTN_H                   50
#define SMALL_BTN_H             30
#define HIGH_BTN_Y              HEIGHT - BTN_H
#define LOW_BTN_Y               HEIGHT - BTN_H + 30
#define NEW_BTN_X               340
#define NEW_BTN_Y               HEIGHT - BTN_H - 10
#define SAVE_BTN_X              340
#define SAVE_BTN_Y              HEIGHT - (BTN_H/2.) - 10
#define STOP_BTN_X              10
#define STOP_BTN_Y              HEIGHT - BTN_H
#define ARM_BTN_X               57
#define ARM_BTN_Y               HEIGHT - BTN_H
#define REC_BTN_X               104
#define REC_BTN_Y               HEIGHT - BTN_H
#define PAUSE_BTN_X             151
#define PAUSE_BTN_Y             HEIGHT - BTN_H
#define BTN_SCALE               1.f

#define PUSH_BTN_W              20
#define PUSH_BTN_H              20
#define PUSH_BTN_C              GetColor(0x202020FF)
#define PUSH_BSE_W              40
#define PUSH_BSE_H              20
#define PUSH_BSE_C              GetColor(0x333333FF)

/* Filter */
#define FLT_BTN_Y               40
#define LF_BTN_X                5
#define MF_BTN_X                55
#define HF_BTN_X                105
#define FLT_SEP_X               5
#define FLT_SEP_Y               90
#define FLT_SEP_W               CONTROLLER_W
#define FLT_SEP_T               2
#define FLT_WHEEL_X             FLT_SEP_W-40
#define FLT_WHEEL_Y             55
#define FLT_WHEEL_S             20
#define FLT_WHEEL_S             20
#define FLT_WHEEL_M_X           FLT_SEP_W-40
#define FLT_WHEEL_M_Y           68
#define FLT_WHEEL_M_S           3

extern Rectangle FILENAME_INPUT;
extern Rectangle SAMPLERATE_INPUT;
extern Rectangle CHANNELS_INPUT;
extern Rectangle CHANNELS_INPUTBASE;
extern Rectangle NEWFILE_INPUT;
extern Rectangle SAVEFILE_INPUT;
extern Rectangle LEFT_REEL;
extern Rectangle RIGHT_REEL;
extern Texture2D REEL;
extern Texture2D LOWPASS;
extern Texture2D MIDPASS;
extern Texture2D HIHPASS;
extern float ROTATION;

typedef struct TextureButton {
        Rectangle bounds;
        Texture2D texture;
        Texture2D pressedTexture;
        Texture2D hoverTexture;
        Color tint;
        bool isPressed;
        bool isHovered;
} TextureButton;

typedef struct ButtonSwitch {
        Rectangle bounds;
        Rectangle measure;
        Color color;
        bool isPressed;
        bool isHovered;
} ButtonSwitch;

typedef struct Transport {
        TextureButton stopTrackBtn;
        TextureButton armTrackBtn;
        TextureButton recTrackBtn;
        TextureButton pauseTrackBtn;
        TextureButton saveFileBtn;
        TextureButton newFileBtn;
} Transport;

extern Transport transport;

void drawTitle(void);

void drawFilterPanel(void);

void drawheader(void);

void drawInfo(struct data *data, SF_INFO sfinfo);

void drawVolumeMeters(Texture2D faderTex);

void drawVolumeValues(void);

void drawVuMeters(void);

void drawReels(void);

TextureButton newButton(Rectangle bounds, Texture2D texture, Texture2D pressedTexture, Texture2D hoverTexture, Color tint);

void drawTransportControls(void);

void drawTransport(void);

void drawController(void);

void updateFileName(void);
