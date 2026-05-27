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

extern float SAMPLE_LEFT;
extern float SAMPLE_RIGHT;
extern float VOL_RATIO;

extern bool GUI_DISABLE;
extern bool FILE_INITD;
extern bool MOUSE_ON_INPUT;

extern char fName[MAX_FILE_CHAR+1];
extern size_t charCount;
extern int samplerate;
extern int channels;

static char version[6] = "0.0.8";

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

