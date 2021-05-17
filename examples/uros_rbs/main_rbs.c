// Copyright (c) 2020 Robert Bosch GmbH
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <nuttx/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <time.h>


#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>

#define STRING_BUFFER_LEN 100

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){\
   printf("Failed status on line %d: %d: %s. Aborting.\n",__LINE__,(int)temp_rc, rcl_get_error_string().str);\
   rcutils_reset_error(); return 1;}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d:\
   %s. Continuing.\n",__LINE__,(int)temp_rc, rcl_get_error_string().str);rcutils_reset_error();}}
#define RCUNUSED(fn) { rcl_ret_t temp_rc __attribute__((unused)); temp_rc = fn; }

// static unsigned int sleep_ms;

rcl_subscription_t high_ping_subscription_;
std_msgs__msg__Int32 high_ping_msg_;
rcl_publisher_t high_pong_publisher_;
rcl_subscription_t low_ping_subscription_;
std_msgs__msg__Int32 low_ping_msg_;
rcl_publisher_t low_pong_publisher_;

static void my_mdelay(unsigned int milliseconds)
{
  volatile unsigned int i;
  volatile unsigned int j;

  for (i = 0; i < milliseconds; i++)
    {
      for (j = 0; j < CONFIG_BOARD_LOOPSPERMSEC; j++)
        {
        }
    }
}

static void my_mdelay2(unsigned int milliseconds)
{
  volatile unsigned int i;
  volatile unsigned int j;

  for (i = 0; i < milliseconds; i++)
    {
      for (j = 0; j < CONFIG_BOARD_LOOPSPERMSEC; j++)
        {
        }
    }
}
/*
void delay_test(void)
{
  struct timespec startTimeP;
  struct timespec endTimeP;
  unsigned int delay = 10;
  long measuredDuration = 0;
  long sumDuration = 0;

  unsigned int iter = 10;
  for (int i=0;i<iter;i++){
    clock_gettime(CLOCK_REALTIME, &startTimeP);
    my_mdelay(delay);
    clock_gettime(CLOCK_REALTIME, &endTimeP);
    measuredDuration = (endTimeP.tv_sec - startTimeP.tv_sec) * 1000000000 + 
                            (endTimeP.tv_nsec - startTimeP.tv_nsec);
    sumDuration += measuredDuration;
    printf("duration %ld\n", measuredDuration);
  }
  printf("sum-duration %ld \n", sumDuration);
  printf("time test duration planned %d real %f \n", delay, (float)sumDuration/((float)iter*1000000));

  clock_gettime(CLOCK_REALTIME, &startTimeP);
  usleep(sleep_ms*1000);;
  clock_gettime(CLOCK_REALTIME, &endTimeP);
  measuredDuration = (endTimeP.tv_sec - startTimeP.tv_sec) * 1000000000 + 
                      (endTimeP.tv_nsec - startTimeP.tv_nsec);
  printf("usleep test: %d ms duration %ld\n", sleep_ms, measuredDuration/1000000);

}
*/
/*
void burn_cpu_cycles_high(long duration)
{
  if (duration > 0) {
    clockid_t clockId;
    clockId = CLOCK_REALTIME;
    // clock_getcpuclockid(pthread_self(), &clockId);
    // NuttX does not implement this function
    // -lrt  is not available , I did not find this function in 
    // NuttX subfolders
    // adding EXTRA_LIBS to the Makefile did not resolve the error.
    struct timespec startTimeP;
    clock_gettime(clockId, &startTimeP);
    int x = 0;
    bool doAgain = true;
    while (doAgain) {
      while (x != rand() && x % 10 != 0) {
        x++;
      }
      struct timespec currentTimeP;
      clock_gettime(clockId, &currentTimeP);
      long currentDuration = (currentTimeP.tv_sec - startTimeP.tv_sec) * 1000000000 + (currentTimeP.tv_nsec - startTimeP.tv_nsec);
      doAgain = (currentDuration < duration);
    }
  }
}

void burn_cpu_cycles_low(long duration)
{
  if (duration > 0) {
    clockid_t clockId;
    clockId = CLOCK_REALTIME;
    // clock_getcpuclockid(pthread_self(), &clockId);
    // NuttX does not implement this function
    // -lrt  is not available , I did not find this function in 
    // NuttX subfolders
    // adding EXTRA_LIBS to the Makefile did not resolve the error.
    struct timespec startTimeP;
    clock_gettime(clockId, &startTimeP);
    int x = 0;
    bool doAgain = true;
    while (doAgain) {
      while (x != rand() && x % 10 != 0) {
        x++;
      }
      struct timespec currentTimeP;
      clock_gettime(clockId, &currentTimeP);
      long currentDuration = (currentTimeP.tv_sec - startTimeP.tv_sec) * 1000000000 + (currentTimeP.tv_nsec - startTimeP.tv_nsec);
      doAgain = (currentDuration < duration);
    }
  }
}
*/

void high_ping_received(const void * pong_msg)
{
  my_mdelay(10);  // 10ms TODO: Get this value from parameter 'high_busyloop'.
  // printf("high ping received.\n");
  RCUNUSED(rclc_executor_publish(&high_pong_publisher_, pong_msg, NULL));
}


void low_ping_received(const void * pong_msg)
{
  my_mdelay(10);
  // usleep(sleep_ms*1000);
  // printf("low ping received.\n");
  RCUNUSED(rclc_executor_publish(&low_pong_publisher_, pong_msg, NULL));

/*
  unsigned int seconds = 15;
  while(seconds>0){
    my_mdelay2(1000);
    seconds--; 
  }
*/
}

#if defined(BUILD_MODULE)
int main(int argc, char *argv[])
#else
int uros_rbs_main(int argc, char* argv[])
#endif
{
  rcl_allocator_t allocator = rcl_get_default_allocator();
  rclc_support_t support;
  int ret;
  int budget_ms = 30;
  // sleep_ms = 0;
  unsigned int timeout_ms = 100;


  if (argc ==2){
    // budget_ms = atoi(argv[1]);
    // sleep_ms = atoi(argv[1]);  
    timeout_ms = atoi(argv[1]);
  }
  // period_ms
  // low_pong_policy
  // high_ping_prio
  // low_ping_prio
  printf("high_pong budget %d ms\n", budget_ms);
  // printf("low_pong sleep %d ms\n", sleep_ms);
  printf("parameter rcl_wait timeout %d ms\n", timeout_ms);
  // tests
  // delay_test();

  // Set the executor thread priority 
  struct sched_param exe_param;
  exe_param.sched_priority = 110;
  ret = sched_setparam(0, &exe_param);
  if (ret < 0)
  {
    printf("uros_rbs: sched_setparam failed: %d\n" , ret);
    return 1;
  }

  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
  rcl_node_t node = rcl_get_zero_initialized_node();
  RCCHECK(rclc_node_init_default(&node, "pong_rclc_node", "", &support));
  RCCHECK(rclc_publisher_init_best_effort(&high_pong_publisher_, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "high_pong"));
  RCCHECK(rclc_subscription_init_best_effort(&high_ping_subscription_, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "high_ping"));
  RCCHECK(rclc_publisher_init_best_effort(&low_pong_publisher_, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "low_pong"));
  RCCHECK(rclc_subscription_init_best_effort(&low_ping_subscription_, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "low_ping"));

  rclc_executor_t executor = rclc_executor_get_zero_initialized_executor();
  RCCHECK(rclc_executor_init(&executor, &support.context, 2, &allocator));
  RCCHECK(rclc_executor_set_timeout(&executor, RCL_MS_TO_NS(timeout_ms)));
  printf("main: executor->timeout_ns %d\n", executor.timeout_ns);
  RCCHECK(rclc_executor_set_timeout(&executor, timeout_ms));
  printf("main: executor->timeout_ns %d\n", executor.timeout_ns);
  RCCHECK(rclc_executor_set_timeout(&executor, 420420420));
  printf("main: executor->timeout_ns %d\n", executor.timeout_ns);

  // sparam_high.priority = sched_get_priority_max(SCHED_FIFO);
  // sparam_low.priority = sched_get_priority_min(SCHED_FIFO);
  // printf("max prio %d min prio %d\n", SCHED_PRIORITY_MAX, SCHED_PRIORITY_MIN); => [0, 255]
  rclc_executor_sched_parameter_t sparam_high;
  sparam_high.policy = SCHED_SPORADIC;
  sparam_high.param.sched_priority               = 60;
  sparam_high.param.sched_ss_low_priority        = 6;
  sparam_high.param.sched_ss_repl_period.tv_sec  = 0;
  sparam_high.param.sched_ss_repl_period.tv_nsec = 100000000;  // 100ms
  sparam_high.param.sched_ss_init_budget.tv_sec  = 0;
  sparam_high.param.sched_ss_init_budget.tv_nsec = budget_ms*1000000;
  sparam_high.param.sched_ss_max_repl            = CONFIG_SCHED_SPORADIC_MAXREPL;

  rclc_executor_sched_parameter_t sparam_low;
  sparam_low.policy = SCHED_FIFO;
  sparam_low.param.sched_priority               = 50;
  /*
  sparam_low.param.sched_ss_low_priority        = 2;
  sparam_low.param.sched_ss_repl_period.tv_sec  = 0;
  sparam_low.param.sched_ss_repl_period.tv_nsec = 10000000;   // 10ms
  sparam_low.param.sched_ss_init_budget.tv_sec  = 0;
  sparam_low.param.sched_ss_init_budget.tv_nsec = 4000000;    // 4ms
  sparam_low.param.sched_ss_max_repl            = CONFIG_SCHED_SPORADIC_MAXREPL;
  */
  printf("high_pong prio %d low_pong prio %d\n", sparam_high.param.sched_priority, sparam_low.param.sched_priority);
  RCCHECK(rclc_executor_add_subscription_sched(&executor, &high_ping_subscription_, &high_ping_msg_, &high_ping_received, ON_NEW_DATA, &sparam_high));
  RCCHECK(rclc_executor_add_subscription_sched(&executor, &low_ping_subscription_, &low_ping_msg_, &low_ping_received, ON_NEW_DATA, &sparam_low));
  RCCHECK(rclc_executor_start_multi_threading_for_nuttx(&executor));

  printf("clean up \n");
  RCCHECK(rclc_executor_fini(&executor));
  RCCHECK(rcl_subscription_fini(&high_ping_subscription_, &node));
  RCCHECK(rcl_publisher_fini(&high_pong_publisher_, &node));
  RCCHECK(rcl_subscription_fini(&low_ping_subscription_, &node));  
  RCCHECK(rcl_publisher_fini(&low_pong_publisher_, &node));
  RCCHECK(rcl_node_fini(&node));
  return 0;
}

