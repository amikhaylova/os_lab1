#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int num_of_threads = 62;
int size = 198 * 1024 * 1024;
int part = 0;
char *array;
int size_of_file = 148 * 1024 * 1024;
int size_of_block = 45;

void *fill_array(void *arg) {
    int thread_part = part++;
    int one_thread_should_write = size / num_of_threads;
    int start = thread_part * one_thread_should_write;
    int randomData = open("/dev/urandom", O_RDONLY);
    if (randomData < 0) {
        printf("could not access urandom\n");
    } else {
        if (part != num_of_threads) {
            ssize_t result = read(randomData, &array[start], one_thread_should_write);
            if (result < 0) {
                printf("could not read from urandom\n");
            }
        } else {
            ssize_t result = read(randomData, &array[start], size - (one_thread_should_write * (num_of_threads - 1)));
            if (result < 0) {
                printf("could not read from urandom\n");
            }
        }
    }
    close(randomData);
    return 0;
}

int main() {
    array = (char *) malloc(size);

    pthread_t threads[num_of_threads];

    for (int i = 0; i < num_of_threads; i++)
        pthread_create(&threads[i], NULL, fill_array, (void *) NULL);

    for (int i = 0; i < num_of_threads; i++)
        pthread_join(threads[i], NULL);

    int num_of_files;
    if (size % size_of_file == 0) {
        num_of_files = size / size_of_file;
    } else {
        num_of_files = size / size_of_file + 1;
    }

    size_t wrote = 0;
    size_t sum = 0;
    int j = 0;
    for (int i = 1; i < num_of_files + 1; i++) {
        char filename[sizeof "file1"];
        sprintf(filename, "file%d", i);
        int file = open(filename, O_WRONLY | O_CREAT);
        while ((wrote < size_of_file) && (sum < size)) {
            size_t bytes_wrote;
            if (wrote + size_of_block > size_of_file)
                bytes_wrote = write(file, &array[j * size_of_block], size_of_file - wrote);
            else if (sum + size_of_block > size)
                bytes_wrote = write(file, &array[j * size_of_block], size - sum);
            else
                bytes_wrote = write(file, &array[j * size_of_block], size_of_block);
            if (bytes_wrote >= 0) {
                wrote += bytes_wrote;
                sum += bytes_wrote;
            } else {
                printf("something went wrong during writing");
            }
            j++;
        }
        wrote = 0;
        close(file);
    }
    return 0;
}




