#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

struct index_s {
    double time_mark;
    uint64_t recno;
};

struct index_hdr_s {
    uint64_t records;
    struct index_s* idx;
};

double generate_time_mark() {
    double min = 15020.0;
    double max = 2460436.5;
    return min + (double)rand() / ((double)RAND_MAX / (max - min));
}

uint64_t generate_recno() {
    return rand() + 1;
}

void generate_file(const char *filename, uint64_t num_records) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    struct index_hdr_s header;
    header.records = num_records;
    header.idx = (struct index_s*)malloc(num_records * sizeof(struct index_s));
    fwrite(&header, sizeof(header), 1, file);

    for (uint64_t i = 0; i < num_records; ++i) {
        struct index_s record;
        record.time_mark = generate_time_mark();
        record.recno = i + 1;
        fwrite(&record, sizeof(record), 1, file);
    }

    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <filename> <num_records>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *filename = argv[1];
    uint64_t num_records = strtoul(argv[2], NULL, 10);

    srand(time(NULL));

    generate_file(filename, num_records);

    return EXIT_SUCCESS;
}
