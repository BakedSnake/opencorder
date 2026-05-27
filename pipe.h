#include <sndfile.h>
#include <spa/param/audio/format-utils.h>

typedef struct data {
        struct pw_main_loop *loop;
        struct pw_stream *stream;

        struct spa_audio_info format;
        unsigned move:1;
        SNDFILE *sf;
        char *sfName;
} data;

extern data DATA;

typedef struct pipeData {
        struct data dat;
        char* target;
        int argc;
} pipeData;

typedef struct State {
        bool isRecording;
        bool isArmed;
        bool isPaused;
} State;

void* piper(void* arg);
