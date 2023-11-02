/**
 * @copyright Copyright (C) 2023 Neil Stansbury
 * All rights reserved.
 * 
 * Portions copyright (C) 2021 Keith Standiford
 * Portions copyright (C) 2021 Gary Sims
 * Portions copyright (C) 2017 Scott Nelson
 * Portions copyright (C) 2015-2018 National Cheng Kung University, Taiwan
 * Portions copyright (C) 2014-2017 Chris Stones
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Based on Piccolo OS, a simple cooperative multi-tasking OS originally written by 
 * [Gary Sims](https://github.com/garyexplains) as a teaching tool and later enhanced by
 * [Keith Standiford](https://github.com/KStandiford/Piccolo_OS_Plus) as Piccolo OS Plus
 */
#pragma once

namespace Postman {

  enum class Result {
    SUCCESS,
    FAILED,
    CONTINUE,
    ENDPOINT_DUPLICATE,
    ENDPOINT_NOT_AVAILABLE,
    ENDPOINT_BLOCKED,
    WORKER_NOT_AVAILABLE,
    WORKER_BOUND,
    WORKER_NOT_BOUND,
    TIMEOUT,
  };
}

#define INTERNAL_NS namespace { namespace NS {
#define END_INTERNAL }}


/**
 * @brief Number of Workers in a pool. 
 */
#define WORKER_POOL_SIZE 20

/**
 * @brief Number of concurrent Messages. 
 */
#define MESSAGE_BANK_SIZE 50


/**
 * @brief Size of a Worker stack in 32 bit words. 
 * @note Must be **even**, for exception frame stack alignment! 
 */
#define WORKER_STACK_SIZE 1024

/** Exception return behavior value **/
#define RETURN_THREAD_PSP 0xFFFFFFFD

/**
 * @brief The OS time slice, in microseconds 
 *
 * Setting time slice to zero will disable preemptive scheduling!
*/
#define WORKER_TIME_SLICE 1000
/**
 * @brief The maximum time that the dispatcher will sleep in the idle task (in usec).
 * 
 * This will influence the latency for tasks waking up for signals, 
 * for example. Setting this to zero will disable the idle task.
*/
#define DISPATCHER_MAX_IDLE_TIME 700

/**
 * @brief If true, the dispatcher will not idle (sleep) if tasks are blocking for signals.
 * 
 * If set to false, the scheduler will run the idle task for the minimum of
 * DISPATCHER_MAX_IDLE or the smallest time remaining for any task with a timeout
 * running. This will improve power consumption but potentially delay the response
 * of any task waiting for a signal. This may make good sense for applications without
 * serious response time concerns.
 * 
 */
#define DISPATCHER_NO_IDLE_FOR_SIGNALS true

/**
 * @brief Enable/disable multi-core scheduling
 * 
**/
#define DISPATCHER_MULTICORE true

