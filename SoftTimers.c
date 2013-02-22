/**
* @file		SoftTimers.c
* @brief	Contains all functions support for handling software timers.
*
* @version	0.1
* @date		22 February 2013
* @author	Mauro Gamba
*
***********************************************************************/

#include <string.h>
#include "timer.h"
#include "SoftTimers.h"

#define MAX_TIMERS      5
#define TIMER_ACTIVE_MASK               0x8000
#define TIMER_CREATED_MASK              0x4000
#define TIMER_CREATED_AND_ACTIVE_AMSK   0xC000
#define TIMER_TIMEOUT_MASK              0x3FFF

static timer_t TimerTable[MAX_TIMERS];

void Timer_Init(void)
{
    memset(&TimerTable, 0, sizeof(TimerTable));
}

/**
 * @fn uint8_t Timer_Create(uint16_t timeout, func_pnt_t FunctionPointer)
 * @brief Create a software timer.
 * @param FunctionPointer The function to call on timeout.
 * @return The timer id or the error code ERR_TIMER_NOT_AVAILABLE.
 */
uint8_t Timer_Create(timer_handler_t FunctionPointer)
{
    uint8_t idx;

    idx = 0;
    while ((idx < MAX_TIMERS) && (TimerTable[idx].TimeoutTick & TIMER_CREATED_MASK))
    {
        idx++;
    }
    if (idx == MAX_TIMERS)
    {
        idx = ERR_TIMER_NOT_AVAILABLE;
    }
    else
    {
        /* Disable timer interrupt */
        DIS_TIMER_INT_RX;
        /* Initialize timer */
        TimerTable[idx].TimerHandler = FunctionPointer;
        TimerTable[idx].TimeoutTick = TIMER_CREATED_MASK;
        /* Enable timer interrupt */
        EN_TIMER_INT_RX;
    }
    return idx;
}

/**
 * @fn void Timer_Destroy(uint8_t timerId)
 * @brief Destroy the timer passed as parameter
 * @param timerId The id of the timer to destroy.
 */
void Timer_Destroy(uint8_t timerId)
{
    if (timerId < MAX_TIMERS)
    {
        TimerTable[timerId].TimeoutTick = 0;
    }
}

/**
 * @fn void Timer_Start(uint8_t timerId)
 * @brief Start the timer.
 * @param timerId The id of the timer to start.
 * @param timeout The timeout in tens of milliseconds. Max timeout value is 16383 (14 bits available).
 */
void Timer_Start(uint8_t timerId, uint16_t timeout)
{
    if (timerId < MAX_TIMERS)
    {
        /* Disable timer interrupt */
        DIS_TIMER_INT_RX;
        /* Set the timer active flag */
        TimerTable[timerId].TimeoutTick |= (TIMER_ACTIVE_MASK + (timeout & TIMER_TIMEOUT_MASK));
        /* Enable timer interrupt */
        EN_TIMER_INT_RX;
    }
}

/**
 * @fn void Timer_Stop(uint8_t timerId)
 * @brief Stop the timer.
 * @param timerId The id of the timer to stop.
 */
void Timer_Stop(uint8_t timerId)
{
    if (timerId < MAX_TIMERS)
    {
        /* Disable timer interrupt */
        DIS_TIMER_INT_RX;
        /* Reset the timer active flag */
        TimerTable[timerId].TimeoutTick &= ~TIMER_ACTIVE_MASK;
        /* Enable timer interrupt */
        EN_TIMER_INT_RX;
    }
}

/**
 * @fn void Timer_Update(void)
 * @brief Update active timers. Note that it's called in interrupt contest.
 */
void Timer_Update(void)
{
    uint8_t idx;

    for (idx=0; idx<MAX_TIMERS; idx++)
    {
        if ((TimerTable[idx].TimeoutTick & TIMER_CREATED_AND_ACTIVE_AMSK) == TIMER_CREATED_AND_ACTIVE_AMSK)
        {
            TimerTable[idx].TimeoutTick--;
            if ((TimerTable[idx].TimeoutTick & TIMER_TIMEOUT_MASK) == 0)
            {
                /* Disable the timer */
                TimerTable[idx].TimeoutTick &= ~TIMER_ACTIVE_MASK;

                /* Call the timer handler */
                TimerTable[idx].TimerHandler(idx);
            }
        }
    }
}
