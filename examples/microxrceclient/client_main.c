/****************************************************************************
 * examples/microxrceclient/client_main.c
 *
 * Copyright 2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "ShapeType.h"
#include <nuttx/config.h>
#include <uxr/client/client.h>
#include <ucdr/microcdr.h>

#include <stdio.h>
#include <fcntl.h>  // O_RDWR, O_NOCTTY, O_NONBLOCK
#include <stdlib.h> // atoi
#include <string.h> // strcpy
#include <termios.h>

#include <sys/select.h>
#include <sys/time.h>

// Colors for highlight the print
#define GREEN_CONSOLE_COLOR "\x1B[1;32m"
#define RED_CONSOLE_COLOR   "\x1B[1;31m"
#define RESTORE_COLOR       "\x1B[0m"

// Getting the max MTU
#define MAX_TRANSPORT_MTU UXR_CONFIG_SERIAL_TRANSPORT_MTU

// Stream buffers
#define MAX_HISTORY        2
#define MAX_BUFFER_SIZE    MAX_TRANSPORT_MTU * MAX_HISTORY

static bool run_command(const char* command, uxrSession* session, uxrStreamId* stream_id);
static bool compute_command(uxrSession* session, uxrStreamId* stream_id, int length, const char* name,
                            uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, const char* topic_color);
static bool compute_print_command(uxrSession* session, uxrStreamId* stream_id, int length, const char* name,
                            uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, const char* topic_color);
static void on_topic(uxrSession* session, uxrObjectId object_id, uint16_t request_id, uxrStreamId stream_id, struct ucdrBuffer* serialization, void* args);
static void on_status(uxrSession* session, uxrObjectId object_id, uint16_t request_id, uint8_t status, void* args);
static void print_ShapeType_topic(const ShapeType* topic);
static void print_status(uint8_t status);
static void print_help(void);
static void print_commands(void);
static int check_input(void);

/****************************************************************************
 * hello_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int args, FAR char *argv[])
#else
int client_main(int args, char *argv[])
#endif
{
    uxrSession session;
    uxrSerialTransport serial;
    uxrSerialPlatform serial_platform;

    uxrCommunication* comm;

    if (args >= 3 && strcmp(argv[1], "--serial") == 0)
    {
        char* device = argv[2];
        int fd = open(device, O_RDWR | O_NOCTTY);
        if (0 < fd)
        {
            struct termios tty_config;
            memset(&tty_config, 0, sizeof(tty_config));
            if (0 == tcgetattr(fd, &tty_config))
            {
                /* Setting CONTROL OPTIONS. */
                tty_config.c_cflag |= CREAD;    // Enable read.
                tty_config.c_cflag |= CLOCAL;   // Set local mode.
                tty_config.c_cflag &= ~PARENB;  // Disable parity.
                tty_config.c_cflag &= ~CSTOPB;  // Set one stop bit.
                tty_config.c_cflag &= ~CSIZE;   // Mask the character size bits.
                tty_config.c_cflag |= CS8;      // Set 8 data bits.
                tty_config.c_cflag &= ~CRTSCTS; // Disable hardware flow control.

                /* Setting LOCAL OPTIONS. */
                tty_config.c_lflag &= ~ICANON;  // Set non-canonical input.
                tty_config.c_lflag &= ~ECHO;    // Disable echoing of input characters.
                tty_config.c_lflag &= ~ECHOE;   // Disable echoing the erase character.
                tty_config.c_lflag &= ~ISIG;    // Disable SIGINTR, SIGSUSP, SIGDSUSP and SIGQUIT signals.

                /* Setting INPUT OPTIONS. */
                tty_config.c_iflag &= ~IXON;    // Disable output software flow control.
                tty_config.c_iflag &= ~IXOFF;   // Disable input software flow control.
                tty_config.c_iflag &= ~INPCK;   // Disable parity check.
                tty_config.c_iflag &= ~ISTRIP;  // Disable strip parity bits.
                tty_config.c_iflag &= ~IGNBRK;  // No ignore break condition.
                tty_config.c_iflag &= ~IGNCR;   // No ignore carrier return.
                tty_config.c_iflag &= ~INLCR;   // No map NL to CR.
                tty_config.c_iflag &= ~ICRNL;   // No map CR to NL.

                /* Setting OUTPUT OPTIONS. */
                tty_config.c_oflag &= ~OPOST;   // Set raw output.

                /* Setting OUTPUT CHARACTERS. */
                tty_config.c_cc[VMIN] = 1;
                tty_config.c_cc[VTIME] = 1;

                /* Setting BAUD RATE. */
                cfsetispeed(&tty_config, B115200);
                cfsetospeed(&tty_config, B115200);

                if (0 == tcsetattr(fd, TCSANOW, &tty_config))
                {
                    if(!uxr_init_serial_transport(&serial, &serial_platform, fd, 0, 1))
                    {
                        printf("%sCan not create a serial connection%s\n", RED_CONSOLE_COLOR, RESTORE_COLOR);
                        return 1;
                    }
                }
            }
            comm = &serial.comm;
            printf("Serial mode => dev: %s\n", device);
        }

    }
    else
    {
        print_help();
        return 1;
    }

    printf("Running Shapes Demo Client...\n");

    uint32_t key = 0xAABBCCDD;
    uint16_t history = 2;

    // Init session and streams
    uxr_init_session(&session, comm, key);
    uxr_set_topic_callback(&session, on_topic, NULL);
    uxr_set_status_callback(&session, on_status, NULL);

    uint8_t output_best_effort_stream_buffer[MAX_TRANSPORT_MTU * UXR_CONFIG_MAX_OUTPUT_BEST_EFFORT_STREAMS];
    uint8_t output_reliable_stream_buffer[MAX_BUFFER_SIZE * UXR_CONFIG_MAX_OUTPUT_RELIABLE_STREAMS];
    uint8_t input_reliable_stream_buffer[MAX_BUFFER_SIZE * UXR_CONFIG_MAX_INPUT_RELIABLE_STREAMS];

    for(int i = 0; i < UXR_CONFIG_MAX_OUTPUT_BEST_EFFORT_STREAMS; ++i)
    {
        (void) uxr_create_output_best_effort_stream(&session, output_best_effort_stream_buffer + MAX_TRANSPORT_MTU * i, comm->mtu);
    }
    for(int i = 0; i < UXR_CONFIG_MAX_INPUT_BEST_EFFORT_STREAMS; ++i)
    {
        (void) uxr_create_input_best_effort_stream(&session);
    }
    for(int i = 0; i < UXR_CONFIG_MAX_OUTPUT_RELIABLE_STREAMS; ++i)
    {
        (void) uxr_create_output_reliable_stream(&session, output_reliable_stream_buffer + MAX_BUFFER_SIZE * i, comm->mtu * history, history);
    }
    for(int i = 0; i < UXR_CONFIG_MAX_INPUT_RELIABLE_STREAMS; ++i)
    {
        (void) uxr_create_input_reliable_stream(&session, input_reliable_stream_buffer + MAX_BUFFER_SIZE * i, comm->mtu * history, history);
    }

    uxrStreamId default_output = uxr_stream_id(0, UXR_RELIABLE_STREAM, UXR_OUTPUT_STREAM);

    // Waiting user commands
    char command_stdin_line[256];
    bool running = true;
    while (running)
    {
        if (!check_input())
        {
            (void) uxr_run_session_time(&session, 100);
        }
        else if (fgets(command_stdin_line, 256, stdin))
        {
            running = run_command(command_stdin_line, &session, &default_output);
        }
    }

//    if(&udp.comm == comm)
//    {
//        uxr_close_udp_transport(&udp);
//    }
//    else if(&tcp.comm == comm)
//    {
//        uxr_close_tcp_transport(&tcp);
//    }
//#if !defined(WIN32)
//    else if(&serial.comm == comm)
//    {
//        uxr_close_serial_transport(&serial);
//    }
//#endif

    uxr_close_serial_transport(&serial);

    return 0;
}

bool run_command(const char* command, uxrSession* session, uxrStreamId* stream_id)
{
    char name[128];
    uint32_t arg1 = 0;
    uint32_t arg2 = 0;
    uint32_t arg3 = 0;
    uint32_t arg4 = 0;
    uint32_t arg5 = 0;
    char topic_color[128] = "";
    int length = sscanf(command, "%s %u %u %u %u %u %s", name, &arg1, &arg2, &arg3, &arg4, &arg5, topic_color);
    if(length == 7 && topic_color[0] == '\0')
    {
        length = length - 1; //some implementations of sscanfs add 1 to length if color is empty.
    }

    return compute_command(session, stream_id, length, name, arg1, arg2, arg3, arg4, arg5, topic_color);
}

bool compute_command(uxrSession* session, uxrStreamId* stream_id, int length, const char* name,
                     uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, const char* topic_color)
{
    if(0 == strcmp(name, "create_session") && 1 == length)
    {
        (void) uxr_create_session(session);
        print_status(session->info.last_requested_status);
    }
    else if(0 == strcmp(name, "create_participant") && 2 == length)
    {
        uxrObjectId participant_id = uxr_object_id((uint16_t)arg1, UXR_PARTICIPANT_ID);
        const char* participant_ref = "default_xrce_participant";
        (void) uxr_buffer_create_participant_ref(session, *stream_id, participant_id, 0, participant_ref, 0);
    }
    else if(0 == strcmp(name, "create_topic") && 3 == length)
    {
        uxrObjectId topic_id = uxr_object_id((uint16_t)arg1, UXR_TOPIC_ID);
        uxrObjectId participant_id = uxr_object_id((uint16_t)arg2, UXR_PARTICIPANT_ID);
        const char* topic_ref = "shapetype_topic";
        (void) uxr_buffer_create_topic_ref(session, *stream_id, topic_id, participant_id, topic_ref, 0);
    }
    else if(0 == strcmp(name, "create_publisher") && 3 == length)
    {
        uxrObjectId publisher_id = uxr_object_id((uint16_t)arg1, UXR_PUBLISHER_ID);
        uxrObjectId participant_id = uxr_object_id((uint16_t)arg2, UXR_PARTICIPANT_ID);
        const char* publisher_xml = "";
        (void) uxr_buffer_create_publisher_xml(session, *stream_id, publisher_id, participant_id, publisher_xml, 0);
    }
    else if(0 == strcmp(name, "create_subscriber") && 3 == length)
    {
        uxrObjectId subscriber_id = uxr_object_id((uint16_t)arg1, UXR_SUBSCRIBER_ID);
        uxrObjectId participant_id = uxr_object_id((uint16_t)arg2, UXR_PARTICIPANT_ID);
        const char* subscriber_xml = "";
        (void) uxr_buffer_create_subscriber_xml(session, *stream_id, subscriber_id, participant_id, subscriber_xml, 0);
    }
    else if(0 == strcmp(name, "create_datawriter") && 3 == length)
    {
        uxrObjectId datawriter_id = uxr_object_id((uint16_t)arg1, UXR_DATAWRITER_ID);
        uxrObjectId publisher_id = uxr_object_id((uint16_t)arg2, UXR_PUBLISHER_ID);
        const char* datawriter_ref = "shapetype_data_writer";
        (void) uxr_buffer_create_datawriter_ref(session, *stream_id, datawriter_id, publisher_id, datawriter_ref, 0);
    }
    else if(0 == strcmp(name, "create_datareader") && 3 == length)
    {
        uxrObjectId datareader_id = uxr_object_id((uint16_t)arg1, UXR_DATAREADER_ID);
        uxrObjectId subscriber_id = uxr_object_id((uint16_t)arg2, UXR_SUBSCRIBER_ID);
        const char* datareader_ref = "shapetype_data_reader";
        (void) uxr_buffer_create_datareader_ref(session, *stream_id, datareader_id, subscriber_id, datareader_ref, 0);
    }
    else if(0 == strcmp(name, "write_data") && 3 <= length)
    {
        ShapeType topic = {"GREEN", 100 , 100, 50};
        if(3 < length)
        {
            strncpy(topic.color, topic_color, sizeof(topic.color));
            topic.x = arg3;
            topic.y = arg4;
            topic.shapesize = arg5;
        }

        uxrObjectId datawriter_id = uxr_object_id((uint16_t)arg1, UXR_DATAWRITER_ID);
        uxrStreamId output_stream_id = uxr_stream_id_from_raw((uint8_t)arg2, UXR_INPUT_STREAM);

        ucdrBuffer mb;
        uint32_t topic_size = ShapeType_size_of_topic(&topic, 0);
        uxr_prepare_output_stream(session, output_stream_id, datawriter_id, &mb, topic_size);
        ShapeType_serialize_topic(&mb, &topic);

        printf("Sending... ");
        print_ShapeType_topic(&topic);
    }
    else if(0 == strcmp(name, "request_data") && 4 == length)
    {
        uxrDeliveryControl delivery_control;
        delivery_control.max_samples = (uint16_t)arg3;
        delivery_control.max_elapsed_time = UXR_MAX_ELAPSED_TIME_UNLIMITED;
        delivery_control.max_bytes_per_second = UXR_MAX_BYTES_PER_SECOND_UNLIMITED;
        delivery_control.min_pace_period = 0;

        uxrObjectId datareader_id = uxr_object_id((uint16_t)arg1, UXR_DATAREADER_ID);
        uxrStreamId input_stream_id = uxr_stream_id_from_raw((uint8_t)arg2, UXR_INPUT_STREAM);
        (void) uxr_buffer_request_data(session, *stream_id, datareader_id, input_stream_id, &delivery_control);
    }
    else if(0 == strcmp(name, "cancel_data") && 2 == length)
    {
        uxrObjectId datareader_id = uxr_object_id((uint16_t)arg1, UXR_DATAREADER_ID);
        (void) uxr_buffer_cancel_data(session, *stream_id, datareader_id);
    }
    else if(0 == strcmp(name, "delete") && 3 == length)
    {
        uxrObjectId entity_id = uxr_object_id((uint16_t)(arg1 & 0xFFF0) >> 4, arg1 & 0x0F);
        (void) uxr_buffer_delete_entity(session, *stream_id, entity_id);
    }
    else if((0 == strcmp(name, "default_output_stream") || 0 == strcmp(name, "stream")) && 2 == length)
    {
        *stream_id = uxr_stream_id_from_raw((uint8_t)arg1, UXR_OUTPUT_STREAM);
    }
    else if(0 == strcmp(name, "delete_session") && 1 == length)
    {
        (void) uxr_delete_session(session);
        print_status(session->info.last_requested_status);
    }
    else if(0 == strcmp(name, "exit") && 1 == length)
    {
        return false;
    }
    else if((0 == strcmp(name, "entity_tree") || 0 == strcmp(name, "tree")) && 2 == length)
    {
        (void) compute_print_command(session, stream_id, 2, "create_participant", arg1, 0, 0, 0, 0, "");
        (void) uxr_run_session_time(session, 20);
        (void) compute_print_command(session, stream_id, 3, "create_topic"      , arg1, arg1, 0, 0, 0, "");
        (void) uxr_run_session_time(session, 20);
        (void) compute_print_command(session, stream_id, 3, "create_publisher"  , arg1, arg1, 0, 0, 0, "");
        (void) uxr_run_session_time(session, 20);
        (void) compute_print_command(session, stream_id, 3, "create_subscriber" , arg1, arg1, 0, 0, 0, "");
        (void) uxr_run_session_time(session, 20);
        (void) compute_print_command(session, stream_id, 3, "create_datawriter" , arg1, arg1, 0, 0, 0, "");
        (void) uxr_run_session_time(session, 20);
        (void) compute_print_command(session, stream_id, 3, "create_datareader" , arg1, arg1, 0, 0, 0, "");
        (void) uxr_run_session_time(session, 20);
    }
    else if(0 == strcmp(name, "list") || 0 == strcmp(name, "l"))
    {
        print_commands();
    }
    else
    {
        printf("%sUnknown command error, write 'l' or 'list' to show the command list.%s\n", RED_CONSOLE_COLOR, RESTORE_COLOR);
    }

    return true;
}


static bool compute_print_command(uxrSession* session, uxrStreamId* stream_id, int length, const char* name,
                            uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, const char* topic_color)
{
    printf("%s\n", name);
    return compute_command(session, stream_id, length, name, arg1, arg2, arg3, arg4, arg5, topic_color);
}

void on_status(uxrSession* session, uxrObjectId object_id, uint16_t request_id, uint8_t status, void* args)
{
    (void) session; (void) object_id; (void) request_id; (void) args;
    print_status(status);
}

void on_topic(uxrSession* session, uxrObjectId object_id, uint16_t request_id, uxrStreamId stream_id, struct ucdrBuffer* serialization, void* args)
{
    (void) session; (void) object_id; (void) request_id; (void) stream_id; (void) serialization; (void) args;

    ShapeType topic;
    ShapeType_deserialize_topic(serialization, &topic);

    printf("Receiving... ");
    print_ShapeType_topic(&topic);
}

void print_ShapeType_topic(const ShapeType* shape_topic)
{
    printf("%s[SHAPE_TYPE | color: %s | x: %u | y: %u | size: %u]%s\n",
           "\x1B[1;34m",
           shape_topic->color,
           shape_topic->x,
           shape_topic->y,
           shape_topic->shapesize,
           "\x1B[0m");
}

void print_status(uint8_t status)
{
    char status_name[128];
    if(UXR_STATUS_OK == status)
    {
        strcpy(status_name, "OK");
    }
    else if(UXR_STATUS_OK_MATCHED == status)
    {
        strcpy(status_name, "OK_MATCHED");
    }
    else if(UXR_STATUS_ERR_DDS_ERROR == status)
    {
        strcpy(status_name, "ERR_DDS_ERROR");
    }
    else if(UXR_STATUS_ERR_MISMATCH == status)
    {
        strcpy(status_name, "ERR_MISMATCH");
    }
    else if(UXR_STATUS_ERR_ALREADY_EXISTS == status)
    {
        strcpy(status_name, "ERR_ALREADY_EXISTS");
    }
    else if(UXR_STATUS_ERR_DENIED == status)
    {
        strcpy(status_name, "ERR_DDS_DENIED");
    }
    else if(UXR_STATUS_ERR_UNKNOWN_REFERENCE == status)
    {
        strcpy(status_name, "UNKNOWN_REFERENCE");
    }
    else if(UXR_STATUS_ERR_INVALID_DATA == status)
    {
        strcpy(status_name, "ERR_INVALID_DATA");
    }
    else if(UXR_STATUS_ERR_INCOMPATIBLE == status)
    {
        strcpy(status_name, "ERR_INCOMPATIBLE");
    }
    else if(UXR_STATUS_ERR_RESOURCES == status)
    {
        strcpy(status_name, "ERR_RESOURCES");
    }
    else if(UXR_STATUS_NONE == status)
    {
        strcpy(status_name, "NONE");
    }

    const char* color = (UXR_STATUS_OK == status || UXR_STATUS_OK_MATCHED == status) ? GREEN_CONSOLE_COLOR : RED_CONSOLE_COLOR;
    printf("%sStatus: %s%s\n", color, status_name, RESTORE_COLOR);
}

void print_help(void)
{
    printf("Usage: program --help\n");
    printf("       program <transport> [--key <number>] [--history <number>]\n");
    printf("List of available transports:\n");
    printf("    --serial <device>\n");
    printf("    --udp <agent-ip> <agent-port>\n");
    printf("    --tcp <agent-ip> <agent-port>\n");
}

void print_commands(void)
{
    printf("usage: <command> [<args>]\n");
    printf("    create_session\n");
    printf("        Creates a Session, if exists, reuse it.\n");
    printf("    create_participant <participant id>:\n");
    printf("        Creates a Participant on the current session.\n");
    printf("    create_topic       <topic id> <participant id>:\n");
    printf("        Register a Topic using <participant id> participant.\n");
    printf("    create_publisher   <publisher id> <participant id>:\n");
    printf("        Creates a Publisher on <participant id> participant.\n");
    printf("    create_subscriber  <subscriber id> <participant id>:\n");
    printf("        Creates a Subscriber on <participant id> participant.\n");
    printf("    create_datawriter  <datawriter id> <publisher id>:\n");
    printf("        Creates a DataWriter on the publisher <publisher id>.\n");
    printf("    create_datareader  <datareader id> <subscriber id>:\n");
    printf("        Creates a DataReader on the subscriber <subscriber id>.\n");
    printf("    write_data <datawriter id> <stream id> [<x> <y> <size> <color>]:\n");
    printf("        Write data into a <stream id> using <data writer id> DataWriter.\n");
    printf("    request_data       <datareader id> <stream id> <samples>:\n");
    printf("        Read <sample> topics from a <stream id> using <datareader id> DataReader.\n");
    printf("    cancel_data        <datareader id>:\n");
    printf("        Cancel any previous request data of <datareader id> DataReader.\n");
    printf("    delete             <id_prefix> <type>:\n");
    printf("        Removes object with <id prefix> and <type>.\n");
    printf("    stream, default_output_stream <stream_id>:\n");
    printf("        Change the default output stream for all messages except of write data.\n");
    printf("        <stream_id> can be 1-127 for best effort and 128-255 for reliable.\n");
    printf("        The streams must be initially configured.\n");
    printf("    exit:\n");
    printf("        Close session and exit.\n");
    printf("    tree, entity_tree        <id>:\n");
    printf("        Create the necessary entities for a complete publisher and subscriber.\n");
    printf("        All entities will have the same <id> as id.\n");
    printf("    h, help:\n");
    printf("        Shows this message.\n");
}

int check_input(void)
{
    struct timeval tv = {0, 0};
    fd_set fds = {0};
    FD_ZERO(&fds);
    FD_SET(0, &fds); //STDIN 0
    select(1, &fds, NULL, NULL, &tv);
    return FD_ISSET(0, &fds);
}
   
