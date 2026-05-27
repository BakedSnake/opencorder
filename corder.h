#include <getopt.h>
#include <stdio.h>
#include <stdbool.h>

#define MONO                    1
#define STEREO                  2
#define MAX_FILE_CHAR           24
#define SAMPLING_RATE_48K       48000
#define SAMPLING_RATE_44K       44100
#define SAMPLING_RATE_22K       22050
#define SAMPLING_RATE_11K       11025

extern char* PATH;

extern float SAMPLE_LEFT;
extern float SAMPLE_RIGHT;
extern float VOL_RATIO;

extern bool GUI_DISABLE;
extern bool FILE_INITD;
extern bool MOUSE_ON_INPUT;

extern char SEPARATOR[2];
extern size_t CLILEN;

extern char FNAME[MAX_FILE_CHAR+1];
extern size_t CHAR_COUNT;
extern int SAMPLE_RATE;
extern int CHANNELS;

static char VERSION[6] = "0.0.8";

static struct option options[] = {
  {"help",              no_argument,            0, 'h'},
  {"version",           no_argument,            0, 'v'},
  {"format",            required_argument,      0, 'f'},
  {"rate",              required_argument,      0, 'r'},
  {"output",            required_argument,      0, 'o'},
  {"target",            required_argument,      0, 't'},
  {"nogui",             no_argument,            0, 'n'},
  {"channels",          required_argument,      0, 'c'},
  {0,                   0,                      0,  0 }
};

