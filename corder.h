#include <getopt.h>
#include <raylib.h>

#define WIDTH 600
#define HEIGHT 350
#define MAX_FILE_CHAR 24

extern float SAMPLE_LEFT;
extern float SAMPLE_RIGHT;
extern float VOL_RATIO;

extern bool GUI_DISABLE;
extern bool FILE_INITD;
extern bool MOUSE_ON_INPUT;

static struct option options[] = {
  {"help",              no_argument,            0, 'h'},
  {"version",           no_argument,            0, 'v'},
  {"format",            required_argument,      0, 'f'},
  {"rate",              required_argument,      0, 'r'},
  {"output",            required_argument,      0, 'o'},
  {"target",            required_argument,      0, 't'},
  {"nogui",             no_argument,            0, 'n'},
  {0,                   0,                      0,  0 }
};

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
