#include "ratsense_freertos_bridge.h"

static const ratsense_task_budget_t kBudgets[] = {
    {6U, 512U, 1U, 1U, "MotorTask"},
    {6U, 768U, 10U, 3U, "PressureTask"},
    {5U, 768U, 10U, 10U, "TrainingTask"},
    {4U, 1024U, 100U, 30U, "ThermalTask"},
    {3U, 768U, 200U, 50U, "DisplayTask"},
    {3U, 1024U, 100U, 50U, "StorageTask"},
    {4U, 768U, 20U, 5U, "CommsTask"},
    {7U, 512U, 100U, 2U, "MonitorTask"},
};

const ratsense_task_budget_t* ratsense_task_budgets(uint32_t* count) {
    if (count != 0) {
        *count = (uint32_t)(sizeof(kBudgets) / sizeof(kBudgets[0]));
    }
    return kBudgets;
}

void ratsense_freertos_isr_yield(bool higher_priority_woken) {
    (void)higher_priority_woken;
    /* Map to portYIELD_FROM_ISR only inside a checked FreeRTOS port. */
}

