#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdint.h>
#include <semaphore.h>
#include <fcntl.h>
#include <iostream>
#include <math.h>

using namespace std;

typedef struct handler_info
{
    int signalType;
    int threadCount;
}handler_info;

typedef struct genDone
{
    int gen1done;
    int gen2done;
    int handlerdone;
}genDone;

typedef struct handlerswitch
{
    int sig1hand1;
    int sig1hand2;
    int sig2hand1;
    int sig2hand2;
}handlerswitch;

void randomNum(int low, int high);
void sigOneReporter(int sig);
void sigTwoReporter (int sig);
void *handleSignal (void *signal);
void generateSig(int sigGenerator, pid_t *handle);
void terminator (int sig);