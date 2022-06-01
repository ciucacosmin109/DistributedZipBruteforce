#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <mpi.h>
#include <string.h>
#include <fstream>

#define SIZE 4

using namespace std;

clock_t start, stop;
unsigned int microsecond = 1000000;

inline bool nextPass(int* pass, int size, const char* allowedChars) {

    int allowedCharsLen = strlen(allowedChars);

    for(int i = size - 1; i >= 0; i--) {
        // advance the last non final character
        if(pass[i] < allowedCharsLen - 1) {
            pass[i]++;
            // reset the chars to the right
            for(int j = i + 1; j < size; j++) {
                pass[j] = 0;
            }
            return true;
        }
    }

    return false;
}
inline bool nextPass(int* pass, int size, const char* allowedChars, int step) {
    for (int i = 0; i < step; i++) {
        if(!nextPass(pass, size, allowedChars)) {
            return false;
        }
    }
    return true;
}
inline const char* getPassString(int* pass, int size, const char* allowedChars) {
    char* passStr = new char[size + 1]; 
    for(int j = 0; j < size; j++) {
        passStr[j] = allowedChars[pass[j]];
    }
    passStr[size] = 0;
    return passStr;
}
inline void printResult(const char* passStr, long clicks, const char* file) {
    printf("The password is: %s\n", passStr);
    printf("It took %ld clicks (%7.2f seconds) \n\n", clicks, ((float)clicks)/CLOCKS_PER_SEC); 
    
    printf("File content: \n");
    string text; 
    ifstream ifs(file); 
    while (getline (ifs, text)) { 
        printf("%s", text.c_str());
    } 
    ifs.close(); 
    printf("\n");
}

int main(int argc, char **argv)
{
    if(argc != 2) {
        printf("USE: %s <protected .zip file> \n", argv[0]);
        exit(1);
    }

    // allowed chars
    const char* CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const int PASS_LEN = 4;
    
    // Initialize the MPI environment
    MPI_Init(NULL, NULL); 
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size); 
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // validate the world size
    if (world_size != SIZE && world_rank == 0) { 
        printf("You must specify %d processors. Terminating.\n", SIZE);
        exit(2);
    }

    // generate config
    int config[SIZE][2];
    if(world_rank == 0) { 
        for (int i = 0; i < SIZE; i++) {
            config[i][0] = SIZE - i - 1; // password offset
            config[i][1] = world_size; // step
        }
    }
    
    // scatter the configurations
    int myConfig[2]; 
    MPI_Scatter(
        config, 2, MPI_INT, 
        myConfig, 2, MPI_INT, 
        0, MPI_COMM_WORLD
    );
    int offset = myConfig[0];
    int step = myConfig[1];

	start = clock();
    printf("Processing on process %d (offset: %d, step: %d) ...\n", world_rank, offset, step);

    // crack here
    {
        // the password will contain indexes from CHARS
        int* password = new int[PASS_LEN];
        for(int i=0; i < PASS_LEN; i++){
            password[i] = 0; /// 15 = begin from PPPP to save some time (P09s is the pass)
        }
        //password[1] = 51;
        // P1 will start from AAAA, P2 from AAAB, P3 from AAAC, P4 from AAAD
        password[PASS_LEN - 1] += offset; 
        
        bool validPass = true;
        while(validPass) {
            const char* passStr = getPassString(password, PASS_LEN, CHARS);
            //printf("%s - %d \n", passStr, world_rank);
            //usleep(2 * microsecond); // debug

            char command[128];
            sprintf(command, "unzip -oP %s %s >>/dev/null 2>>/dev/null", passStr, argv[1]);

            if(system(command) == 0){
	            stop = clock();
                printResult(passStr, stop - start, "test.txt");
                MPI_Abort(MPI_COMM_WORLD, 0);
            }

            // All processes will advance 'step' steps. P1 will advance to AAAE, P2 to AAAF, P3 to AAAG, P4 to AAAH 
            validPass = nextPass(password, PASS_LEN, CHARS, step);
        }
    }

    // Finalize the MPI environment.
    MPI_Finalize(); 
    return 0;
}