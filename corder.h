#include <getopt.h>
#include <raylib.h>

#define WIDTH 600
#define HEIGHT 350
#define MAX_FILE_CHAR 24

static struct option options[] = {
  {"help",              no_argument,            0, 'h'},
  {"version",           no_argument,            0, 'v'},
  {"format",            required_argument,      0, 'f'},
  {"rate",              required_argument,      0, 'r'},
  {"output",            required_argument,      0, 'o'},
  {"target",            required_argument,      0, 't'},
  {"gui",               no_argument,            0, 'g'},
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

