#include "processmakerheader.h"

// switches for each signal handler thread
int sig1hand1 = 0;
int sig1hand2 = 0;
int sig2hand1 = 0;
int sig2hand2 = 0;

// semaphores for signal generation
sem_t *sigGen1 = NULL;
sem_t *sigGen2 = NULL;

int sig1 = SIGUSR1;
int sig2 = SIGUSR2;

int terminateSwitch = 0;

int main()
{
    srand(time(NULL));
    
    // 2 handler for when signals are to be terminated
    struct sigaction first;
    struct sigaction second;

    pid_t threadGen1;
    pid_t threadGen2;
    pid_t sigHandle;

    int waitval = 0;
    int gen1done = 0;
    int gen2done = 0;
    int handlerdone = 0;

    // sa_handler points to signal catching function
    second.sa_handler = &terminator;
    sigemptyset(&(second.sa_mask)); // initialize set to empty
    second.sa_flags = 0; // set all flags to 0

    int checksigact = sigaction(SIGTERM, &second, &first);
    if (checksigact== -1) {
        cerr << "couldn't perform sigaction" << endl;
        exit(EXIT_FAILURE);
    }

    // Initialize semaphores
    sigGen1 = sem_open("/sigGen1sem", O_CREAT, 0777, 0);
    sigGen2 = sem_open("/sigGen2sem", O_CREAT, 0777, 0);

    // fork to create signal handling process
    if ((sigHandle = fork()) == -1) 
    {
        cerr<< "Could not create signal handler" << endl;
        exit(EXIT_FAILURE);
    } 
    
    //Signal Handler Process
    else if (sigHandle == 0) 
    {
        handler_info handlerInfoArray[4]; // array of signal handler relevant info structs
        pthread_t handlerThreads[4]; // array of threads

        // Create four signal handling threads, two for both types of signal
        for (int i = 0; i < 4; i++) 
        {
            handlerInfoArray[i].threadCount = i + 1; // label to identify which signal handling thread

            if ( i <= 1) // 2 SIGUSR1 threads
            {
                handlerInfoArray[i].signalType = sig1;
            }

            else // 2 SIGUSR2 threads
            {
                handlerInfoArray[i].signalType = sig2;
            }

            // check for thread creation success
            int pthread_check = pthread_create(&(handlerThreads[i]), 0, handleSignal, (void*) &(handlerInfoArray[i]) );
            if (pthread_check != 0)
            {
                cerr << "Could not create thread (handler)" << endl;
                exit(EXIT_FAILURE);
            }

            // while any of the thread handlers are available, sleep for a sec
            while (sig1hand1 == 0 || sig1hand2 == 0 || sig2hand1 == 0 || sig2hand2 == 0)
            {
                sleep(1);
            }

            // unlock semaphore
            sem_post(sigGen1);
            sem_post(sigGen2);

            // pthread_join threads
            pthread_join(handlerThreads[1], 0);
            pthread_join(handlerThreads[2], 0);
            pthread_join(handlerThreads[3], 0);
            pthread_join(handlerThreads[4], 0);
        }
    }

    // Signal generation process
    else
    {
        // Fork to create first signal generation process and check to make sure it was forked
        threadGen1 = fork();
        if (threadGen1 == -1)
        {
            cerr << "could not create threadGen1 for signal generation" << endl;
        }
        
        else if (threadGen1 == 0)
        {
            generateSig(1, &sigHandle); // 1 for first signal generator 
        } 

        // Second signal generation thread
         else
        {
            threadGen2 = fork();
            if (threadGen2 == -1)
            {
                cerr << "could not create threadGen2 for signal generation" << endl;
            }
 
            else if (threadGen2 == 0)
            {
                generateSig(2, &sigHandle);
            }
 
            else
            {
                sleep(1); // back at the parent process by this point
 
                // kill threads
                kill(threadGen1, SIGTERM);
                kill(threadGen2, SIGTERM);
                kill(sigHandle, SIGKILL);
 
                // while any of signal generator or handler threads are still running
                while (threadGen1 == 0 || threadGen2 == 0 || sigHandle == 0)
                {
                    waitval = waitpid(-1, NULL, 0);
 
                    if (waitval == threadGen1) // if first signal generator is complete
                    {
                        gen1done = 1;
                    }
 
                    else if (waitval == threadGen2) // if second signal generator is complete
                    {
                        gen2done = 1;
                    }
 
                    else if (waitval == sigHandle) // if handler is complete
                    {
                        handlerdone = 1;
                    }
                }
            }
        }
    }

    sem_close(sigGen1);
    sem_close(sigGen2);
}

double randomNum(double lowerBound, double upperBound) 
{
    int r = rand();
    double randomNum = (fmod((double)r, upperBound*100) + lowerBound*100);
    return randomNum/100;
}

void sigOneReporter(int sig)
{
    cout << "Handling a new SIGUSR1" << endl;
}

void sigTwoReporter (int sig)
{
    cout << "Handling a new SIGUSR2" << endl;
}

void *handleSignal (void *signal)
{
    //create masks
    sigset_t mask1; 
    sigset_t mask2;

    handler_info *sig = (handler_info *) signal;

    struct sigaction sigact1;
    struct sigaction sigact2;

    if ((*sig).signalType == sig2)
    {
        sigact2.sa_handler = &sigTwoReporter;
    }

    else if ((*sig).signalType == sig1)
    {
        sigact2.sa_handler = &sigOneReporter;
    }

    sigemptyset(&(sigact2.sa_mask));
    sigact2.sa_flags = 0;

    int sethandler = sigaction((*sig).signalType, &sigact2, &sigact1);
    if (sethandler == -1)
    {
        cout<< "sigaction failed" << endl;
    }

    sigemptyset(&mask2);
    sigaddset(&mask2, (*sig).signalType);

    if ((*sig).threadCount == 1)
    {
        sig1hand1 = 1;
        cout << "started sig1hand1" << endl;
    }

    else if ((*sig).threadCount == 2)
    {
        sig1hand2 = 1;
        cout << "started sig1hand2" << endl;
    }

    else if ((*sig).threadCount == 3)
    {
        sig2hand1 = 1;
        cout << "started sig2hand1" << endl;
    }

    else if ((*sig).threadCount == 4)
    {
        sig2hand2 = 1;
        cout << "started sig2hand2" << endl;
    }

    sigprocmask(SIG_BLOCK, &mask2, &mask1);

    while (terminateSwitch == 0)
    {    
    }

    sigprocmask(SIG_UNBLOCK, &mask2, &mask1);

    return 0;

    
}

void generateSig(int sigGenerator, pid_t *handle)
{
    // srand(time(NULL));

    struct timespec delay;
    int eitheror; // int for randomly choosing what signal
    int randomSignal; // int for what signal was chosen

    if (sigGenerator == 1) // if first signal generating thread
    {
        sem_wait(sigGen1);
    }
    
    else if (sigGenerator == 2) // if first signal generating thread
    {
        sem_wait(sigGen2);
    }  

        while(terminateSwitch == 0) // artificial signal delay
    {
        delay.tv_sec = 0;
        delay.tv_nsec = randomNum(.01, .1) * 1000000000L; // .01 seconds, .1 seconds in nanoseconds
 
        eitheror = rand() % 2; // randomly determine what signal to use
       
        if (eitheror == 0)
        {
            randomSignal = SIGUSR1;
        }
 
        else if (eitheror == 1)
        {
            randomSignal = SIGUSR2;
        }
 
        nanosleep(&delay, NULL); // delay for random time
        kill(*handle, randomSignal); // send random signal to handler thread
    }
}


void terminator (int sig)
{
    terminateSwitch = 1;
}