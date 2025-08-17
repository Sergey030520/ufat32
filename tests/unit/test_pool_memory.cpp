#include "CppUTest/TestHarness.h"           // Основной заголовок

extern "C" {
    #include "pool_memory.h"
}

TEST_GROUP(PoolMemoryTests){
    void setup(){
        pool_init();
    } 
    void teardown(){
       pool_free();
    }};

TEST(PoolMemoryTests, ShouldFailWhenTooMuchRequested){
    uint32_t *values = NULL;
    values = (uint32_t*)pool_alloc(sizeof(uint32_t)*34);
    CHECK_EQUAL(NULL, values);
}

TEST(PoolMemoryTests, ReturnsPointerWhenAllocatingWithinLimits){
    uint32_t *values = NULL;
    values = (uint32_t*)pool_alloc(sizeof(uint32_t)*32);
    CHECK(values != NULL);
}

TEST(PoolMemoryTests, SucceedsWhenFreeingAllocatedMemory){
    uint32_t *values = NULL;
    uint32_t size = sizeof(uint32_t)*32;
    values = (uint32_t*)pool_alloc(size);
    if(values == NULL){
        FAIL("Error allocated memory!");
    }
    
    CHECK_EQUAL(0, pool_free_region(values, size));
}