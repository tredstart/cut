//
// Created by redstart on 10/29/21.
//
#include "buffer.h"
#include <assert.h>
#include <stdbool.h>

static Buffer *test_ringBuffer;

void test_buffer_init(void);
void test_buffer_is_empty(bool expected_result);
void test_buffer_is_full(bool expected_result);
Stats *test_buffer_add(void);
void test_buffer_get(Stats *expected_head);

void case_1(void);
Stats *case_2(void);
void case_3(Stats *expected_head);
void case_4(Stats *previous_head);
void case_5(void);

int main(){
    test_ringBuffer = calloc(1, sizeof(Buffer));
    test_ringBuffer->max = BUFFER_SIZE;
    case_1();
    Stats *test_head = case_2();
    case_3(test_head);
    case_4(test_head);
    case_5();
    printf("All tests passed\n");
    return 0;
}


// check buffer initialization
void case_1(void){
    test_buffer_init();
    test_buffer_is_empty(true);
    test_buffer_is_full(false);
}

// check adding first stat
Stats *case_2(void){
    Stats *stats = test_buffer_add();
    test_buffer_is_empty(false);
    test_buffer_is_full(false);
    return stats;
}

// check head after adding second parameter
void case_3(Stats *expected_head){
    test_buffer_add();
    test_buffer_is_empty(false);
    test_buffer_is_full(false);
    test_buffer_get(expected_head);
}

// check new head after removing
void case_4(Stats *previous_head){
    int previous_count = test_ringBuffer->count;
    remove_from_buffer(test_ringBuffer);
    assert(previous_head != test_ringBuffer->head);
    assert(test_ringBuffer->count == previous_count-1);
}

// check if the all buffered data is removed
void case_5(void){
    test_buffer_add();
    buffer_flush(&test_ringBuffer);
    assert(!test_ringBuffer);
}

void test_buffer_init(void){
    assert(test_ringBuffer != NULL);
}

void test_buffer_is_empty(bool expected_result){
    assert(expected_result == buffer_is_empty(test_ringBuffer));
}

void test_buffer_is_full(bool expected_result){
    assert(expected_result == buffer_is_full(test_ringBuffer));
}

Stats *test_buffer_add(void){
    Stats *test_stats = calloc(1, sizeof(Stats));
    test_stats->next = NULL;
    test_stats->data[0] = 3540820;
    test_stats->data[1] = 1148;
    test_stats->data[2] = 1060031;
    test_stats->data[3] = 9407974;
    test_stats->data[4] = 50447;
    test_stats->data[6] = 57604;
    int prev_count = test_ringBuffer->count;
    put_into_buffer(test_ringBuffer, test_stats);
    assert(test_ringBuffer->tail == test_stats);
    assert(test_ringBuffer->count == prev_count+1);
    return test_stats;
}

void test_buffer_get(Stats *expected_head){
    assert(expected_head == buffer_get(test_ringBuffer));
}
