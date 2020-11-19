#include <nuttx/config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//uROS libraries
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <std_msgs/msg/int32.h>
#include <rmw_uros/options.h>

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Aborting.\n",__LINE__,(int)temp_rc); return 1;}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Continuing.\n",__LINE__,(int)temp_rc);}}

#if defined(BUILD_MODULE)
int main(int argc, char *argv[])
#else
int uros_6lowpan_main(int argc, char* argv[])
#endif
{
    char buffer[256]; // Buffer to save configuration commands.

    if(3 > argc || 0 == atoi(argv[2]))
    {
        printf("usage: program [-h | --help] | ip port sub/pub [<max_topics>]\n");
        return 0;
    }
    //Check if the IP and the port are correct
    if(strlen(argv[1])>39 || strlen(argv[2])>4){
        printf("Error: IP or port size incorrect \r\n");
        return 0;
    }

    //6lowpan configuration process
    system("ifdown wpan0"); // Is necessary to bring down the network to configure.
    if(!strcmp(argv[3],"pub")){
        system("i8sak wpan0 startpan cd:ab"); //Set the radio as an endpoint.
        system("i8sak set chan 26"); //Set the radio channel.
        system("i8sak set panid cd:ab"); //Set network PAN ID.
        sprintf(buffer,"i8sak set saddr 42:%02x",CONFIG_UROS_6LOWPAN_EXAMPLE_ID); // Set the short address of the radio
        system(buffer);
        sprintf(buffer, "i8sak set eaddr 00:fa:de:00:de:ad:be:%02x", CONFIG_UROS_6LOWPAN_EXAMPLE_ID); // TODO: This won't work on the lastest version of NuttX
        system(buffer);
        system("i8sak acceptassoc");
        //6lowpan configuration finished
    } else {
        system("i8sak wpan0 set chan 26"); //Set the radio channel.
        system("i8sak set panid cd:ab"); //Set network PAN ID.
        sprintf(buffer,"i8sak set saddr 42:%02x",CONFIG_UROS_6LOWPAN_EXAMPLE_ID+128); // Set the short address of the radio
        system(buffer);
        sprintf(buffer, "i8sak set eaddr 00:fa:de:00:de:ad:bf:%02x", CONFIG_UROS_6LOWPAN_EXAMPLE_ID); // TODO: This won't work on the lastest version of NuttX
        system(buffer);
        system("i8sak assoc");
   }

    system("ifup wpan0"); // Bring up the network.
    system("mount -t procfs /proc");// Mount the proc file system to check the connection data.
    printf("Connection data\r\n");
    system("cat proc/net/wpan0");

    //Waiting for a user input to continue.
    printf("Press any key to continue\r\n");
    scanf("%s",buffer);

    rcl_ret_t rv;

    rcl_init_options_t options = rcl_get_zero_initialized_init_options();

    rv = rcl_init_options_init(&options, rcl_get_default_allocator());
    if (RCL_RET_OK != rv) {
        printf("rcl init options error: %s\n", rcl_get_error_string().str);
        return 1;
    }

    // Set the IP and the port of the Agent
    rmw_init_options_t* rmw_options = rcl_init_options_get_rmw_init_options(&options);
    rmw_uros_options_set_udp_address(argv[1], argv[2], rmw_options);

    rcl_context_t context = rcl_get_zero_initialized_context();
    rv = rcl_init(0, NULL, &options, &context);
    if (RCL_RET_OK != rv) {
        printf("rcl initialization error: %s\n", rcl_get_error_string().str);
        return 1;
    }

    if(!strcmp(argv[3],"pub")){
        printf("micro-ROS Publisher\r\n");

        rcl_node_options_t node_ops = rcl_node_get_default_options();
        rcl_node_t node = rcl_get_zero_initialized_node();
        rv = rcl_node_init(&node, "int32_publisher_rcl", "", &context, &node_ops);
        if (RCL_RET_OK != rv) {
            printf("Node initialization error: %s\n", rcl_get_error_string().str);
            return 1;
        }

        rcl_publisher_options_t publisher_ops = rcl_publisher_get_default_options();
        rcl_publisher_t publisher = rcl_get_zero_initialized_publisher();
        rv = rcl_publisher_init(&publisher, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "std_msgs_msg_Int32", &publisher_ops);
        if (RCL_RET_OK != rv) {
            printf("Publisher initialization error: %s\n", rcl_get_error_string().str);
            return 1;
        }

        std_msgs__msg__Int32 msg;
        const int num_msg = 1000;
        msg.data = 0;
        usleep(3000000); // As we are sending low number mensajes we need to wait discovery of the subscriber. (Do not have a notification on discovery)
        do {
            rv = rcl_publish(&publisher, (const void*)&msg, NULL);
            if (RCL_RET_OK == rv )
            {
                printf("Sent: '%i'\n", msg.data++);
                sleep(1);
            }
        } while (RCL_RET_OK == rv && msg.data < num_msg );
        printf("TOTAL sent: %i\n", num_msg);

        rv = rcl_publisher_fini(&publisher, &node);
        rv = rcl_node_fini(&node);

    }
    else if(!strcmp(argv[3],"sub")){
        printf("micro-ROS subscriber \r\n");

	rcl_node_options_t node_ops = rcl_node_get_default_options();
        rcl_node_t node = rcl_get_zero_initialized_node();
	RCCHECK(rcl_node_init(&node, "int32_subscriber_rcl", "", &context, &node_ops));

	rcl_subscription_options_t subscription_ops = rcl_subscription_get_default_options();
	rcl_subscription_t subscription = rcl_get_zero_initialized_subscription();
	RCCHECK(rcl_subscription_init(&subscription, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "std_msgs_msg_Int32", &subscription_ops));

	rcl_wait_set_t wait_set = rcl_get_zero_initialized_wait_set();
	RCCHECK(rcl_wait_set_init(&wait_set, 1, 0, 0, 0, 0, 0, &context, rcl_get_default_allocator()));

	do {
	  RCSOFTCHECK(rcl_wait_set_clear(&wait_set));

         size_t index;
         RCSOFTCHECK(rcl_wait_set_add_subscription(&wait_set, &subscription, &index));

	  RCSOFTCHECK(rcl_wait(&wait_set, RCL_MS_TO_NS(1000)));

	  if (wait_set.subscriptions[index]) {
	    std_msgs__msg__Int32 msg;

	    rcl_ret_t rc = rcl_take(wait_set.subscriptions[index], &msg, NULL, NULL);
	    if (RCL_RET_OK == rc) {
	      printf("I received: [%i]\n", msg.data);
	    }
	  }

	} while ( true );
	RCCHECK(rcl_subscription_fini(&subscription, &node));
	RCCHECK(rcl_node_fini(&node));

    }
    else{
        printf("Error. It must be pub (publisher) or sub (subscriber).\r\n");
    }

    printf("Closing Micro-ROS 6lowpan app\r\n");
    return 0;
}
