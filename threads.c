//
// Created by redstart on 10/28/21.
//

#include "threads.h"

volatile sig_atomic_t done = 0;

void term(int signum)
{
    done = 1;
    printf("Received SIGTERM, flushing\n");
}



void *Reader(void* arg) {
    const char* filename = "/proc/stat";
    FILE *file;

    regex_t regex;
    regcomp(&regex, "cpu", 0);

    while (!done){
        file = fopen(filename, "r");
        if (!file){
            return NULL;
        }

        char *name = calloc(NAME_SIZE, sizeof(char ));
        fscanf(file, "%s", name);
        sem_wait(&emptyBuffer);
        pthread_mutex_lock(&mutex);
        read_to_buffer(name, &regex, file);
        pthread_mutex_unlock(&mutex);
        sem_post(&fillBuffer);

        free(name);
        name = NULL;
        fclose(file);
        file = NULL;
        sleep(1);
    }
    regfree(&regex);
    printf("exiting reader\n");
    return NULL;
}

void read_to_buffer(char *name, regex_t *regex, FILE *file){
    while (regexec(regex, name, 0, NULL, 0) == 0){

        Stats *stats = calloc(1, sizeof(Stats));
        for (int i = 0; i < DATA_LENGTH; ++i) {
            fscanf(file, "%ld", &stats->data[i]);
        }

        for (int i = 0; i < NAME_SIZE; ++i) {
            stats->name[i] = *(name+i);
        }

        put_into_buffer(raw_data, stats);
        fscanf(file, "%s", name);

    }

}

void *Analyzer(void *arg){
    unsigned long Idle, NonIdle, Total;
    prev_data *prevData = NULL;
    printf("Please wait, reading previous cpu data\n");
    analyze(prevData, &Idle, &NonIdle, &Total);
    return NULL;
}

void analyze(prev_data *prevData, unsigned long *Idle, unsigned long *NonIdle, unsigned long *Total){
    while (!done){
        sleep(1);
        Stats *stats = buffer_get(raw_data);
        if(!stats) continue;

        prevData = &all_data[get_index(stats)];
        if (!prevData->PrevTotal && !prevData->PrevNonIdle && !prevData->PrevIdle){
            sem_wait(&fillBuffer);
            pthread_mutex_lock(&mutex);
            write_previous_data(prevData, stats);
            pthread_mutex_unlock(&mutex);
            sem_post(&emptyBuffer);
        }else{

            sem_wait(&emptyCopy);
            sem_wait(&fillBuffer);
            pthread_mutex_lock(&mutex);

            calculate_total(Idle, NonIdle, Total, stats);
            prevData->PrevNonIdle = *NonIdle;

            Stats *to_print = calloc(1, sizeof(Stats));
            stats_copy(to_print, stats);
            calculate_percentage(prevData, Idle, Total, to_print);
            pthread_mutex_unlock(&mutex);
            sem_post(&emptyBuffer);
            sem_post(&fillCopy);

        }
    }
}

int get_index(Stats *stats){
    if(stats->name[3] == '\0'){
        return 0;
    }else{
        return (stats->name[3])-47;
    }
}

int write_previous_data(prev_data *prevData, Stats *stats){
    if (!prevData->PrevTotal && !prevData->PrevNonIdle && !prevData->PrevIdle){
        calculate_total(&prevData->PrevIdle, &prevData->PrevNonIdle, &prevData->PrevTotal, stats);
        remove_from_buffer(raw_data);
        return 0;
    }
    return 1;
}

void calculate_total(unsigned long *Idle, unsigned long *NonIdle, unsigned long*Total, Stats *stats){
    *Idle = stats->data[3] + stats->data[4];
    *NonIdle = 0;
    for (int i = 0; i < 3; ++i) {
        *NonIdle += stats->data[i];
    }
    for (int i = 5; i < DATA_LENGTH; ++i) {
        *NonIdle += stats->data[i];
    }
    *Total = *Idle + *NonIdle;
}

void calculate_percentage(prev_data *prevData, const unsigned long *Idle, const unsigned long *Total, Stats *stats){
    unsigned long totald = *Total - prevData->PrevTotal;
    unsigned long idled = *Idle - prevData->PrevIdle;


    prevData->PrevTotal = *Total;
    prevData->PrevIdle = *Idle;

    stats->cpu_percentage = (double )(totald - idled) / (double )totald;
    stats->cpu_percentage *= 100;

    put_into_buffer(copy_data, stats);
    remove_from_buffer(raw_data);

}

void *Printer(void* arg) {
    while (!done){
        sleep(1);
        sem_wait(&fillCopy);
        pthread_mutex_lock(&mutex);
        printf("%s: %.2f%%\n", copy_data->head->name, copy_data->head->cpu_percentage);
        remove_from_buffer(copy_data);
        pthread_mutex_unlock(&mutex);
        sem_post(&emptyCopy);
    }
    printf("exiting printer\n");
    return NULL;
}
