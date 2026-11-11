#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t priority;
    uint32_t stack_words;
    uint32_t period_ms;
    uint32_t max_block_ms;
    const char* name;
} ratsense_task_budget_t;

const ratsense_task_budget_t* ratsense_task_budgets(uint32_t* count);
void ratsense_freertos_isr_yield(bool higher_priority_woken);

