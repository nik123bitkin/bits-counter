#include <stdbool.h>
#include <libgen.h>
#include <stdio.h>
#include <sys/stat.h>
#include <memory.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <semaphore.h>
#include <errno.h>
#include <ctype.h>

#define VALID_ARGS 3

#define MISSING_ARG "Missing argument\n"
#define DIR_ERR "Directory not found/ not a directory\n"
#define FILE_ERR "File cannot be opened"
#define MEM_ERR "Memory allocation error"
#define OPEN_DIR_ERR "Unable to open dir"
#define PID_CREATE_ERR "Unable to create process\n"
#define PROC_ERR "Number of max processes is not an integer\n"
#define PROC_COUNT_ERR "Number of max processes must be greater than 1"
#define COMMAND_ERR "Unable to run command\n"

#define EXIT_ERR(M)     do{ throwError(M); return 1;} while(0)
#define CMP(A, B)            _Generic((A), char*:   strcmp(A, B),\
                                          float: ((abs((A) - (B)) > 0.0001) - (abs((A) - (B)) < 0.0001)),\
                                          double: ((abs((A) - (B)) > 0.0001) - (abs((A) - (B)) < 0.0001)),\
                                          default: (((A) > (B))-((A) < (B))))
#define string            char*
#define const_string      const char*
#define MALLOC_STR(S)     (char *)malloc((S))
#define REALLOC_STR(P, S) do{free(P); P = MALLOC_STR((S));} while(0)

void throwError(const_string msg);

bool validateDir(const_string path);

void parseDir(const_string path);

string getFullPath(const_string path, string name);

pid_t tryFork();

string PROG_NAME;
FILE *out;
long max_processes;
int proc_counter = 1;
unsigned char hash[256] =
        {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2,
         3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2,
         3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4,
         5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2,
         3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3,
         4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4,
         5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3,
         4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2,
         3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4,
         5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3,
         4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5,
         6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4,
         5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};


int main(int argc, string argv[], string envp[]) {
    PROG_NAME = basename(argv[0]);

    if (argc < VALID_ARGS)
        EXIT_ERR(MISSING_ARG);

    if (!validateDir(argv[1]))
        EXIT_ERR(DIR_ERR);

    if (!(out = fopen(argv[2], "w+")))
        EXIT_ERR(FILE_ERR);

    if (!(max_processes = strtol(argv[3], NULL, 10))) {
        EXIT_ERR(PROC_ERR);
    }

    if (max_processes < 2) {
        EXIT_ERR(PROC_COUNT_ERR);
    }

    parseDir(argv[1]);

    while (proc_counter > 1) {
        wait(NULL);
        proc_counter--;
    }

    fclose(out);

    return 0;
}

inline void throwError(const_string msg) {
    fprintf(stderr, "%s: %s\n", PROG_NAME, msg);
}

inline bool validateDir(const_string path) {
    struct stat statbuf;
    return stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode);
}

string getFullPath(const_string path, string name) {
    size_t pathLength = sizeof(char) * (strlen(path) + strlen(name) + 2);
    string fullPath = MALLOC_STR(pathLength);
    if (fullPath) {
        strcpy(fullPath, path);
        strcpy(fullPath + strlen(path), "/");
        strcpy(fullPath + strlen(path) + 1, name);
    }
    return fullPath;
}

pid_t tryFork() {
    if (proc_counter == max_processes) {
        wait(NULL);
        proc_counter--;
    }
    bool success = false;
    pid_t pid;
    while (!success) {
        pid = fork();
        if (pid == -1) {
            throwError(PID_CREATE_ERR);
            continue;
        }
        success = !success;
    }
    proc_counter++;
    return pid;
}

bool counter(char *file, long *count) {
    FILE *fp = fopen(file, "rb");
    unsigned char *buf, *this;
    if (fp) {
        while (!feof(fp)) {
            if ((buf = (unsigned char *) malloc(1048576))) {
                size_t res = fread(buf, 1, 1048576, fp);
                this = buf;
                for (int i = 0; i < res; i++) {
                    *count += hash[*this];
                    this++;
                }
            } else {
                fprintf(stderr, "%s: %s %s %d\n", PROG_NAME, MEM_ERR, "(file buffer)", getpid());
            }
        }
        if(buf)
            free(buf);
        fclose(fp);
        return true;
    }
    return false;
}

void parseDir(const_string path) {
    DIR *dir;
    if ((dir = opendir(path))) {
        struct dirent *direntbuf;
        long size;
        while ((direntbuf = readdir(dir))) {
            if (CMP(direntbuf->d_name, ".") && CMP(direntbuf->d_name, "..") && direntbuf->d_type != DT_UNKNOWN) {
                if (direntbuf->d_type == DT_DIR) {
                    string name = getFullPath(path, direntbuf->d_name);
                    parseDir(name);
                    free(name);
                } else if (direntbuf->d_type == DT_REG) {
                    struct stat statbuf;
                    string name = getFullPath(path, direntbuf->d_name);
                    if (!name) {
                        fprintf(stderr, "%s: %s %s %d\n", PROG_NAME, MEM_ERR, path, getpid());
                    } else {
                        if (stat(name, &statbuf) == 0 && S_ISREG(statbuf.st_mode)) {
                            pid_t pid = tryFork();
                            if (!pid) {

                                size = statbuf.st_size;
                                long count = 0;

                                if (counter(name, &count)) {

                                    printf("%d %s %ld %ld %ld\n", getpid(), name, size, count, (size * 8) - count);
                                    fprintf(out, "%d %s %ld %ld %ld\n", getpid(), name, size, count, (size * 8) - count);

                                } else {

                                    fprintf(stderr, "%s: %s %s %d\n", PROG_NAME, FILE_ERR, name, getpid());

                                }

                                exit(0);
                            }

                        }
                        free(name);
                    }
                }
            }
        }
        closedir(dir);
    } else {
        fprintf(stderr, "%s: %s %s %d\n", PROG_NAME, OPEN_DIR_ERR, path, getpid());
        return;
    }
}
