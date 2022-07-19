/* A simple SocketCAN example */

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>

#include <cstdint>
#include <chrono>
#include <mutex>
#include <thread>

int soc;
//int read_can_port;
bool is_running;
std::mutex mtx;


int open_port(const char *port)
{
    struct ifreq ifr;
    struct sockaddr_can addr;

    /* open socket */
    soc = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    //soc = socket(AF_CAN, SOCK_DGRAM, CAN_J1939);
    if (soc < 0) {
        return (-1);
    }

    addr.can_family = AF_CAN;
    strcpy(ifr.ifr_name, port);

    if (ioctl(soc, SIOCGIFINDEX, &ifr) < 0) {

        return (-1);
    }

    addr.can_ifindex = ifr.ifr_ifindex;

    fcntl(soc, F_SETFL, O_NONBLOCK);

    if (bind(soc, (struct sockaddr *)&addr, sizeof(addr)) < 0) {

        return (-1);
    }

    return 0;
}

int send_port(struct can_frame *frame)
{
    int retval;
    retval = write(soc, frame, sizeof(struct can_frame));
    if (retval != sizeof(struct can_frame)) {
        return (-1);
    } else {
        auto print_frame = [](struct can_frame frame) {
            mtx.lock();
            printf("TX id = 0x%08X,  dlc = %d, data = 0x ", frame.can_id, frame.can_dlc);
            for (int i=0; i<8; i++) {
                //printf("%.2X ", frame.data[i]);
                printf("%02X ", frame.data[i]);
            }
            printf("\n");
            mtx.unlock();
        };
        print_frame(*frame);
        return (0);
    }
}

/* this is just an example, run in a thread */
void read_port()
{
    struct can_frame frame_rd;
    int recvbytes = 0;

    memset(frame_rd.data, 0, sizeof(frame_rd.data));

    //read_can_port = 1;
    //while (read_can_port) {
    while (is_running) {
        struct timeval timeout = {1, 0};
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(soc, &readSet);

        if (select((soc + 1), &readSet, NULL, NULL, &timeout) >= 0) {
            //if (!read_can_port) {
            if (!is_running) {
                break;
            }
            if (FD_ISSET(soc, &readSet)) {
                recvbytes = read(soc, &frame_rd, sizeof(struct can_frame));
                if (recvbytes) {
                    bool is_extended_frame = frame_rd.can_id & CAN_EFF_FLAG;
                    //printf("is extended frame: %d\n", (int)is_extended_frame);
                    frame_rd.can_id &= 0x1FFFFFFF;
                    //frame_rd.can_id &= CAN_EFF_FLAG;
                    auto print_frame = [](struct can_frame frame) {
                        mtx.lock();
                        printf("RX id = 0x%08X,  dlc = %d, data = 0x ", frame.can_id, frame.can_dlc);
                        for (int i=0; i<8; i++) {
                            //printf("%.2X ", frame.data[i]);
                            printf("%02X ", frame.data[i]);
                        }
                        printf("\n");
                        mtx.unlock();
                    };
                    print_frame(frame_rd);
                    
                }
            }
        }
    }
}

int close_port()
{
    close(soc);
    return 0;
}

void send_frames() {
    struct can_frame frame;

    frame.can_id = 0x18FFA017 | CAN_EFF_FLAG;
    frame.can_dlc = 8;
    for (int i=0; i<8; i++)
        frame.data[i] = i;

    uint32_t cnt = 0;
    while (is_running) {

        //frame.can_id = 0x18FFA000 | CAN_EFF_FLAG;
        //frame.can_id += cnt++%255;

        int ret = send_port(&frame);
        if (ret < 0) {
            printf("error - could not send frame\n");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void signal_handler(int s) {
    printf("caught signal %d\n", s);
    is_running = false;
    //exit(1);
}

int main(void)
{
    is_running = true;

    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = signal_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);

    printf("socketcan test\n");
    open_port("can0");
    
    std::thread publisher(&send_frames);
    
    read_port();

    publisher.join();
    printf("publisher thread joined\n");
    close_port();
    printf("socket closed\n");

    printf("exiting\n");

    return 0;
}