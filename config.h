extern char* CONFIG_PATH;
extern char* SAVE_PATH;
extern char* RATESTR;

extern int SAMPLE_RATE;
extern int CHANNELS;

void getConfig();

char* parseConfigLine(char buf[256], char *target);
