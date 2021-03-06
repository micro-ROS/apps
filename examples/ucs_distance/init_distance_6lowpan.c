#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "init_distance_6lowpan.h"


int init_distance_6lowpan(void)
{
// When running on nsh prompt
#if (!defined(CONFIG_FS_ROMFS) || !defined(CONFIG_NSH_ROMFSETC))

    // //6lowpan configuration process
    char buffer[256]; 
    system("mount -t procfs /proc");// Mount the proc file system to check the connection data.
    system("ifdown wpan0"); // Is necessary to bring down the network to configure.

    // system("i8sak wpan0 startpan cd:ab"); //Set the radio as an endpoint.
   // sprintf(buffer,"i8sak wpan0 startpan %02x:%02x", DISTANCE_PAN_ID & 0xff, DISTANCE_PAN_ID >> 8); //Set the radio as an endpoint.  
   // system(buffer);

    // system("i8sak set chan 11"); //Set the radio channel.
    sprintf(buffer,"i8sak set chan %d",DISTANCE_CHANNEL); //Set the radio channel.   
    system(buffer);

    // system("i8sak set panid cd:ab"); //Set network PAN ID.
    sprintf(buffer,"i8sak set panid %02x:%02x", DISTANCE_PAN_ID & 0xff, DISTANCE_PAN_ID  >> 8); //Set network PAN ID
    system(buffer);


    // sprintf(buffer,"i8sak set saddr 42:%02x",CONFIG_UCS_DISTANCE_EXAMPLE_ID); // Set the short address of the radio
    sprintf(buffer,"i8sak set saddr 42:%02x",DISTANCE_DEVICE_ID); // Set the short address of the radio
    system(buffer);

    // sprintf(buffer, "i8sak set eaddr 00:fa:de:00:de:ad:be:%02x", CONFIG_UCS_DISTANCE_EXAMPLE_ID); // TODO: This won't work on the lastest version of NuttX
    sprintf(buffer, "i8sak set eaddr 00:fa:de:00:de:ad:be:%02x", DISTANCE_DEVICE_ID); // TODO: This won't work on the lastest version of NuttX
    system(buffer);

    // system("i8sak acceptassoc");
    system("ifup wpan0"); // Bring up the network.

    printf("Connection data\r\n");
    system("cat proc/net/wpan0");
    usleep(1000000);
#endif

    return 0;
}
