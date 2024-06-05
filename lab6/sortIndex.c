#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>


pthread_barrier_t barrier;

pthread_mutex_t map_mutex = PTHREAD_MUTEX_INITIALIZER;

int* block_map;

struct thread_args {
    uint8_t *file_map;
    uint64_t sizeBlock;
    uint64_t num;
    uint64_t num_blocks;
    uint64_t threads;
    uint64_t memsize;
};

struct data_block {
    double time_mark;
    uint64_t recno;
};

struct data_block_header {
    uint64_t records;
    struct data_block* idx;
};

struct block_info {
    struct data_block *data;
    int size;
};

struct block_info *block_info;

void* merge_blocks(void *args);

int compare_records(const void *a, const void *b) {
    const uint8_t *record1 = (const uint8_t *)a;
    const uint8_t *record2 = (const uint8_t *)b;
    // Сравнение временных меток (double - 8 байт)
    double time1 = *((double *)(record1));
    double time2 = *((double *)(record2));
    if (time1 < time2) return -1;
    if (time1 > time2) return 1;
    // Если временные метки равны, сравниваем первичные индексы (uint64_t - 8 байт)
    uint64_t index1 = *((uint64_t *)(record1 + sizeof(double)));
    uint64_t index2 = *((uint64_t *)(record2 + sizeof(double)));
    if (index1 < index2) return -1;
    if (index1 > index2) return 1;
    return 0;
}

void* sort_blocks_with_map(void *args) {
    struct thread_args *t_args = (struct thread_args *)args;

    fflush(stdout);
    pthread_barrier_wait(&barrier);

    pthread_mutex_lock(&map_mutex);
    uint8_t *file_map = t_args->file_map + t_args->num * (t_args->sizeBlock / sizeof(struct data_block)) * sizeof(struct data_block);
    block_map[t_args->num] = 1;
    pthread_mutex_unlock(&map_mutex);

    while(1) {
        qsort(file_map,
            t_args->sizeBlock / sizeof(struct data_block),
            sizeof(struct data_block),
            compare_records);

        pthread_mutex_lock(&map_mutex);
        int block_index = -1;
        for (int i = 0; i < t_args->num_blocks; i++) {
            if (block_map[i] == 0) {
                block_index = i;
                block_map[i] = 1;
                break;
            }
        }
        if (block_index == -1) {
            pthread_mutex_unlock(&map_mutex);
            break;
        }
        file_map = t_args->file_map + block_index * (t_args->sizeBlock / sizeof(struct data_block)) * sizeof(struct data_block);
        pthread_mutex_unlock(&map_mutex);
    }

    pthread_barrier_wait(&barrier);

    merge_blocks(args);

    return NULL;
}

void* merge_blocks(void *args) {
    struct thread_args *t_args = (struct thread_args *)args;
    while(t_args->num_blocks > 1) {

        for (int id = t_args->num; id < (t_args->num_blocks / 2); id += t_args->threads) {

            int block1 = 2 * id;
            int block2 = 2 * id + 1;

            if (block1 < t_args->num_blocks && block2 < t_args->num_blocks) {

                struct data_block *merged_block = malloc((block_info[block1].size + block_info[block2].size) * sizeof(struct data_block));
                int i = 0, j = 0, k = 0;

                while (i < block_info[block1].size && j < block_info[block2].size) {
                    if (compare_records(&block_info[block1].data[i], &block_info[block2].data[j]) < 0) {
                        merged_block[k] = block_info[block1].data[i];
                        i++;
                    } else {
                        merged_block[k] = block_info[block2].data[j];
                        j++;
                    }
                    k++;
                }

                while (i < block_info[block1].size) {
                    merged_block[k] = block_info[block1].data[i];
                    i++;
                    k++;
                }

                while (j < block_info[block2].size) {
                    merged_block[k] = block_info[block2].data[j];
                    j++;
                    k++;
                }

                if(k != t_args->memsize / sizeof(struct data_block)) {
                    pthread_barrier_wait(&barrier);
                }
                block_info[block1 / 2].data = merged_block;
                block_info[block1 / 2].size = k;

            }
        }
        t_args->num_blocks /= 2;
        if(block_info[0].size != t_args->memsize / sizeof(struct data_block)) {
            pthread_barrier_wait(&barrier);
        }
    }
    pthread_barrier_wait(&barrier);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        printf("Usage: %s memsize granul threads filename\n", argv[0]);
        return EXIT_FAILURE;
    }

    uint64_t memsize = strtoull(argv[1], NULL, 10); // размер файла
    uint64_t blocks = strtoull(argv[2], NULL, 10); // количество блоков
    uint64_t threads = strtoull(argv[3], NULL, 10); // количество потоков
    char *filename = argv[4];
    uint64_t offset = strtoull(argv[5], NULL, 10); // смещение

    struct data_block_header header;

    pthread_barrier_init(&barrier, NULL, threads + 1);

    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("Failed to open file");
        return EXIT_FAILURE;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) {
        perror("Failed to determine file size");
        close(fd);
        return EXIT_FAILURE;
    }

    uint8_t *file_map = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (file_map == MAP_FAILED) {
        perror("Failed to map file into memory");
        close(fd);
        return EXIT_FAILURE;
    }

    file_map += sizeof(struct data_block_header);
    file_map += offset;
    block_info = (struct block_info*)malloc(blocks * sizeof(struct block_info));
    for(int i = 0; i < blocks; i++) {
        block_info[i].size = memsize / blocks / sizeof(struct data_block);
        block_info[i].data = (struct data_block*)(file_map + i * (block_info[i].size) * sizeof(struct data_block));
    }

    block_map = (int*)malloc(blocks * sizeof(int));
    for(int i = 0; i < blocks; i++) {
        block_map[i] = 0;
    }

    pthread_t sort_threads[threads];
    struct thread_args args[threads];
    for (uint64_t i = 0; i < threads; ++i) {
        fflush(stdout);
        args[i].file_map = file_map;
        args[i].sizeBlock = memsize / blocks;
        args[i].num_blocks = blocks;
        args[i].threads = threads + 1;
        args[i].memsize = memsize;
        args[i].num = i + 1;
        pthread_create(&sort_threads[i], NULL, sort_blocks_with_map, (void *)&args[i]);
    }

    struct thread_args argsMain;
    argsMain.file_map = file_map;
    argsMain.sizeBlock = memsize / blocks;
    argsMain.num_blocks = blocks;
    argsMain.threads = threads + 1;
    argsMain.memsize = memsize;
    argsMain.num = 0;
    sort_blocks_with_map((void *)&argsMain);

    struct data_block *block = (struct data_block*)(file_map);
    for(int i = 0; i < memsize / sizeof(struct data_block); i++) {
        block[i].time_mark = block_info[0].data[i].time_mark;
        block[i].recno = block_info[0].data[i].recno;
    }

    munmap(file_map, file_size);
    close(fd);

    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return EXIT_FAILURE;
    }
    fread(&header, sizeof(struct data_block_header), 1, file);
    header.idx = (struct data_block*)malloc(header.records * sizeof(struct data_block));
    fread(header.idx, sizeof(struct data_block), header.records, file);
    printf("File %s opened with %lu records.\n", filename, header.records);
    for(int i = 0; i < header.records; i++) {
        printf("%f %lu\n", header.idx[i].time_mark, header.idx[i].recno);
    }
    fclose(file);

    free(header.idx);
    free(block_info);
    free(block_map);

    return EXIT_SUCCESS;
}
