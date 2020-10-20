#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <values.h>


int num_of_threads_random = 62;
int num_of_threads_agregate = 108;
int size = 198 * 1024 * 1024;
int part = 0;
char *array;
int size_of_file = 148 * 1024 * 1024;
int size_of_block = 45;
int num_of_files;
int fd[];
int max_value = MININT;

typedef struct args_for_random_tag {
    int id;
} args_for_random;

typedef struct args_for_agr_tag {
    int id;
    int max;
} args_for_agr;

void *fill_array(void *arg) {
    args_for_random *args = (args_for_random *) arg;
    int one_thread_should_write = size / num_of_threads_random;
    int start = args->id * one_thread_should_write;
    int randomData = open("/dev/urandom", O_RDONLY);
    if (randomData < 0) {
        printf("could not access urandom\n");
    } else {
        ssize_t result;
        if (part != num_of_threads_random)
            result = read(randomData, &array[start], one_thread_should_write);
        else
            result = read(randomData, &array[start], size - (one_thread_should_write * (num_of_threads_random - 1)));
        if (result < 0) {
            printf("could not read from urandom\n");
        }
    }
    close(randomData);
    return 0;
}

void *agr_array(void *arg) {
    args_for_agr *args = (args_for_agr *) arg;
    int max = MININT;

    for (int i = 0; i < num_of_files; i++) {
        if (i == num_of_files - 1) {
            int data_size;
            int file_act_size = size - size_of_file * (num_of_files - 1);
            if (args->id != num_of_threads_agregate - 1) {
                data_size = file_act_size / num_of_threads_agregate;
            } else {
                data_size = file_act_size - ((num_of_threads_agregate - 1) * (file_act_size / num_of_threads_agregate));
            }
            char buf[data_size];
            ssize_t bytes_read = pread(fd[i], buf, data_size, (file_act_size / num_of_threads_agregate) * args->id);
            for (int j = 0; j < sizeof(buf); j++) {
                if (buf[j] > max)
                    max = buf[j];
            }
            if (args->id == num_of_threads_agregate - 1) {
                flock(fd[i], LOCK_UN);
                close(fd[i]);
            }
        } else {
            int data_size;
            if (args->id != num_of_threads_agregate - 1) {
                data_size = size_of_file / num_of_threads_agregate;
            } else {
                data_size = size_of_file - ((num_of_threads_agregate - 1) * (size_of_file / num_of_threads_agregate));
            }
            char buf[data_size];
            ssize_t bytes_read = pread(fd[i], buf, data_size, (size_of_file / num_of_threads_agregate) * args->id);
            for (int j = 0; j < sizeof(buf); j++) {
                if (buf[j] > max)
                    max = buf[j];
            }
            if (args->id == num_of_threads_agregate - 1) {
                flock(fd[i], LOCK_UN);
                close(fd[i]);
            }
        }
    }
    args->max = max;
    return 0;
}

int main() {
    while (1) {
        //выделяем область памяти
        array = (char *) malloc(size);

        //заполняем область памяти случайными числами
        pthread_t threads[num_of_threads_random];
        args_for_random args_read[num_of_threads_random];

        for (int i = 0; i < num_of_threads_random; i++) {
            args_read[i].id = i;
            pthread_create(&threads[i], NULL, fill_array, (void *) &args_read[i]);
        }
        for (int i = 0; i < num_of_threads_random; i++)
            pthread_join(threads[i], NULL);

        //сохраняем область памяти в файлы
        num_of_files;
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
            int file = open(filename, O_RDWR | O_CREAT, 0666);
            flock(file, LOCK_EX);
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
            flock(file, LOCK_UN);
            wrote = 0;
            close(file);
        }

        //деаллокация
        free(array);

        //подсчитываем агрегированную характеристику, читая из файлов в несколько потоков
        pthread_t threads_agr[num_of_threads_agregate];
        args_for_agr args_agr[num_of_threads_agregate];

        for (int i = 1; i < num_of_files + 1; i++) {
            char filename[sizeof "file1"];
            sprintf(filename, "file%d", i);
            int file = open(filename, O_RDWR);
            flock(file, LOCK_SH);
            fd[i - 1] = file;
        }

        for (int i = 0; i < num_of_threads_agregate; i++) {
            args_agr[i].id = i;
            pthread_create(&threads_agr[i], NULL, agr_array, (void *) &args_agr[i]);
        }

        for (int i = 0; i < num_of_threads_agregate; i++)
            pthread_join(threads_agr[i], NULL);

        //ищем максимум
        for (int i = 0; i < num_of_threads_agregate; i++) {
            if (args_agr[i].max > max_value) {
                max_value = args_agr[i].max;
            }
        }

        /*int local_max = MININT;
        for (int i = 0; i < size; i++){
            if(array[i] > local_max){
                local_max = array[i];
            }
        }
        printf("%d \n", local_max);
        printf("%d \n", max_value);*/

    }
    return 0;
}




