#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>

size_t get_file_size(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fclose(file);
    return size;
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        printf("Usage: %s memsize blocks threads filenаme num_records\n", argv[0]);
        return EXIT_FAILURE;
    }

    size_t memsize = strtoull(argv[1], NULL, 10);
    size_t blocks = strtoull(argv[2], NULL, 10);
    size_t threads = strtoull(argv[3], NULL, 10);
    const char *filename = argv[4];
    uint64_t num_records = strtoull(argv[5], NULL, 10);

    if (blocks < threads) {
        printf("Count of blocks should be greater than threads.\n");
        return EXIT_FAILURE;
    }

    if (blocks == 0 || (blocks & (blocks - 1)) != 0) {
        printf("Count of blocks should be a power of two.\n");
        return EXIT_FAILURE;
    }

    if (memsize == 0 || (memsize & (memsize - 1)) != 0) {
        printf("Memsize should be a power of two.\n");
        return EXIT_FAILURE;
    }

    pid_t pidGen = fork();

    if (pidGen == -1) {
        perror("Failed to fork");
        return EXIT_FAILURE;
    } else if (pidGen == 0) {
        execl("./genFile", "generateFile", argv[4], argv[5], (char *)NULL);

        perror("Failed to execute generate_file");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pidGen, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
            printf("File %s generated with %lu records.\n", filename, num_records);
        } else {
            printf("Failed to generate file.\n");
            return EXIT_FAILURE;
        }
    }

    pid_t pidSort = fork();

    if (pidSort == -1) {
        perror("Failed to fork");
        return EXIT_FAILURE;
    } else if (pidSort == 0) {

        int offset = 0;
        char memsizeChar[21];
        char blocksChar[21];
        char threadsChar[21];
        char offsetChar[21];

        snprintf(memsizeChar, sizeof(memsizeChar), "%lu", memsize);
        snprintf(blocksChar, sizeof(blocksChar), "%lu", blocks);
        snprintf(threadsChar, sizeof(threadsChar), "%lu", threads);
        snprintf(offsetChar, sizeof(offsetChar), "%d", offset);
        
        execl("./sortIndex", "sortIndex", memsizeChar, blocksChar, threadsChar, filename, offsetChar, (char *)NULL);

        perror("Failed to execute sortIndex");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pidSort, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
            printf("File %s sorted.\n", filename);
        } else {
            printf("Failed to sort file.\n");
            return EXIT_FAILURE;
        }

        size_t filesize = get_file_size(filename) - 16;
        size_t count = memsize;
        while (count < filesize) {
            pid_t pidSortDop = fork(); 

            if (pidSort == -1) {
                perror("Failed to fork");
                return EXIT_FAILURE;
            } else if (pidSortDop == 0) {

                char memsizeChar[21];
                char blocksChar[21];
                char threadsChar[21];
                char offsetChar[21];

                snprintf(memsizeChar, sizeof(memsizeChar), "%zu", memsize);
                snprintf(offsetChar, sizeof(offsetChar), "%lu", count);
                snprintf(blocksChar, sizeof(blocksChar), "%lu", blocks);
                snprintf(threadsChar, sizeof(threadsChar), "%lu", threads);
                printf("Offset: %s\n", offsetChar);

                execl("./sortIndex", "sortIndex", memsizeChar, blocksChar, threadsChar, filename, offsetChar, (char *)NULL);
                perror("execl failed");
                return EXIT_FAILURE;
            } else {
                waitpid(pidSortDop, &status, 0);
                filesize = get_file_size(filename) - 16;

                if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
                    printf("File %s sorted.\n", filename);
                } else {
                    printf("Failed to sort file.\n");
                    return EXIT_FAILURE;
                }
            }
            count += memsize;
        }

    }

}
