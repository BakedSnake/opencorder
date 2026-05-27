#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

char* CONFIG_PATH = "/.config/opencorder/opencorder.conf";

void getConfig()
{
        char buf[256];
        char* home = getenv("HOME");
        char* fullConfigPath = strcat(home, CONFIG_PATH);
        FILE *fp = fopen(fullConfigPath, "a+");
        if (!fp) printf("%m\n");

        rewind(fp);
        while (fgets(buf, sizeof(buf), fp) != NULL) {
                char* dir = parseConfigLine(buf, "Directory");
                if (dir != NULL) SAVE_PATH = dir;

                char* sr = parseConfigLine(buf, "SamplingRate");
                if (sr != NULL) {
                        RATESTR = sr;

                        if (atoi(RATESTR) != 0)
                                SAMPLE_RATE = atoi(RATESTR);
                }
        }

        fclose(fp);
}

char* parseConfigLine(char buf[256], char *target)
{
        char* result = NULL;
        if (strstr(buf, target) != 0) {
                char* token = strtok(buf, " = ");
                size_t count = 0;

                while (token != NULL) {
                        if (count > 0) result = token;
                        token = strtok(NULL, " = ");
                        count++;
                }
        }

        return result;
}
