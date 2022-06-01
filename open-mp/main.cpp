#include <string.h>
#include <sstream>
#include <omp.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <fstream>

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

int main(int argc, char *argv[])
{
    if(argc != 2) {
        printf("USE: %s <protected .zip file> \n", argv[0]);
        exit(1);
    }

    // allowed chars
    const char* CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const int PASS_LEN = 4;

	start = clock();
    bool done = false;

#pragma omp parallel shared(done)
    {
        const int NR_THREADS = omp_get_num_threads();
        const int tn = omp_get_thread_num();
        printf("Processing on thread %d ...\n", tn);
        
        // the password will contain indexes from CHARS
        int* password = new int[PASS_LEN];
        for(int i=0; i<PASS_LEN; i++){
            password[i] = 0; /// 15 = begin from PPPP to save some time (P09s is the pass)
        }
        // T1 will start from AAAA, T2 from AAAB, T3 from AAAC, T4 from AAAD
        password[PASS_LEN - 1] += tn; 

        bool validPass = true;
        while(validPass && !done) {
            const char* passStr = getPassString(password, PASS_LEN, CHARS);
            //printf("%s - %d \n", passStr, tn);
            //usleep(2 * microsecond); // debug

            char command[128];
            sprintf(command, "unzip -oP %s %s >>/dev/null 2>>/dev/null", passStr, argv[1]);

            if(system(command) == 0){
	            stop = clock();
                done = true;
                printResult(passStr, stop - start, "test.txt");
            }

            // All threads will advance 'NR_THREADS' steps. T1 will advance to AAAE, T2 to AAAF, T3 to AAAG, T4 to AAAH 
            validPass = nextPass(password, PASS_LEN, CHARS, NR_THREADS);
        }
    }

}
