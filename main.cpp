// https://github.com/FrankBau/raspi-repo-manifest/wiki/OpenCV
// g++ -W -Wall -pthread netcvc1.cpp -o netcvc.out -I/usr/local/include -L/usr/local/lib -lopencv_core -lopencv_imgproc -lopencv_videoio -lopencv_highgui

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>
#include <string.h>
#include <ctime>


#include <iostream>
#include <pthread.h>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;

#define SERVERPORT 8887

VideoCapture    capture;

Mat             img0, img1;
int             is_data_ready = 0;
int             clientSock;
//char*     	server_ip;
//int       	server_port;
vector<uchar> buf;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

void* streamClient(void* arg);
void  quit(string msg, int retval);

void errno_abort(const char* header)
{
    perror(header);
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    pthread_t   thread_c;
    int         key;

    //if (argc < 3) {
    //    quit("Usage: netcv_client <server_ip> <server_port> <input_file>(optional)", 0);
    //}
    //if (argc == 4) {
    //      capture.open(argv[3]);
    //} else {
    //      capture.open(0);
    //}

    cv::VideoCapture videoCap(0);
    videoCap()
    if (!videoCap.isOpened()) {
        quit("\n--> cvCapture failed", 1);
    }

    //server_ip   = argv[1];
    //server_port = atoi(argv[2]);

    videoCap  >> img0;
    img1 = Mat::zeros(img0.rows, img0.cols ,CV_8UC1);

    // run the streaming client as a separate thread
    if (pthread_create(&thread_c, NULL, streamClient, NULL)) {
        quit("\n--> pthread_create failed.", 1);
    }

    cout << "\n--> Press 'q' to quit. \n\n" << endl;

    //flip(img0, img0, 1);
    cvtColor(img0, img1, cv::COLOR_BGR2GRAY);

    while(key != 'q') {
        /* get a frame from camera */
        videoCap  >> img0;
        if (img0.empty()) break;

        pthread_mutex_lock(&mutex1);

        //flip(img0, img0, 1);
        cvtColor(img0, img1, cv::COLOR_BGR2GRAY);
        imencode(".jpg",img1,buf);
        is_data_ready = 1;

        pthread_mutex_unlock(&mutex1);
        usleep(10);

    }

    /* user has pressed 'q', terminate the streaming client */
    if (pthread_cancel(thread_c)) {
        quit("\n--> pthread_cancel failed.", 1);
    }

    /* free memory */
    //destroyWindow("stream_client");
    quit("\n--> NULL", 0);
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * This is the streaming client, run as separate thread
 */
void* streamClient(void* arg)
{
    struct sockaddr_in send_addr;
    int trueflag = 1;
    int fd;
    if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        errno_abort("socket");
        //

    /* make this thread cancellable using pthread_cancel() */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &trueflag, sizeof trueflag) < 0)
    {
        errno_abort("setsockopt"); // one of them..
        quit("\n--> socket() failed.", 1);
    }

    memset(&send_addr, 0, sizeof send_addr);
    send_addr.sin_family = AF_INET;

    //htons() converts the unsigned short integer hostshort from host byte order to network byte order.
    send_addr.sin_port = (in_port_t) htons(SERVERPORT);

    //inet_aton() converts the Internet host address cp from the IPv4 numbers-and-dots notation into binary form (in network byte order) and stores it in the structure that inp points to.

    //if (inet_aton("192.168.2.117", &send_addr.sin_addr) == 0)
    //    std::cout<<"Inet Aton Failed"<<endl;

    if (inet_aton("127.0.0.1", &send_addr.sin_addr) == 0)
        std::cout<<"Inet Aton Failed"<<endl;

    //std::string boundary = "--OPENCV_UDP_BOUNDARY \r\n";

    int counter = 0;
    /* start sending images */
    while(1)
    {
        /* send the grayscaled frame, thread safe */
        if (is_data_ready)
        {
            pthread_mutex_lock(&mutex1);
            //std::string header ="--SomeBoundary\r\nContent-Type: image/jpeg\r\nContent-Length: ";
            //header+= std::to_string(buf.size()*sizeof(uchar));
            //header+= "\r\n\r\n";

//            std::string header ="Content-type: image/jpeg\r\nContent-Length: ";
//            header+= std::to_string(buf.size());
//            header+= "\r\n";
//            header+= "X-Timestamp: ";
//            header+= std::to_string(counter);
//            header += "\r\n";
//            buf.insert(buf.begin(), header.begin(), header.end());
//            buf.insert(buf.end(), boundary.begin(), boundary.end());

            //std::string header ="\r\nContent-type: image/jpeg\r\n";
            //buf.insert(buf.begin(), header.begin(), header.end());

            int sendSum = 0;
            int i = 0;
            int sendSize = 0;
            while( buf.size() > 0 )
            {
                if ( ( buf.size() - (i+1) ) >= 1452)
                {
                    sendSize = 1452;
                }
                else
                {
                    sendSize = (buf.size()-(i+1));
                }
                cout << "\n Buf Iterator " << i <<endl;
                cout << "\n Sending Size " << sendSize <<endl;

                if (sendto(fd, &buf[i], sendSize, 0, (struct sockaddr*) &send_addr, sizeof send_addr) < 0)
                {
                //cerr << "\n--> bytes = " << bytes << endl;
                    quit("\n--> send() failed", 1);
                    errno_abort("send");
                }
                else {
                    //we did it..
                    if (sendSize<1452)
                    {
                        counter++;

                        std::ios oldState(nullptr);
                        oldState.copyfmt(std::cout);
                        for(int k=0; k<buf.size(); ++k)
                            std::cout << std::hex << (int)buf[k];
                        std::cout<<endl;
                        std::cout.copyfmt(oldState);


                        std::cout<< "Iterator i reached: " << i << "and buf.size() was: " << buf.size()<<endl;
                        if (sendSum == buf.size())
                        {
                            sendSum = 0;
                            sendSize = 0;
                            break;
                        }
                    }
                    else {
                        //i = i+1452;
                        i = i + sendSize;
                    }
                }
            }
            is_data_ready = 0;
            buf.clear();

            pthread_mutex_unlock(&mutex1);
            cout << "\n This was the " << counter++ << "--> th package\n\n" <<endl;

//            if (counter%50 ==0)
//            {
//                std::string StreamHeader = "Content-Type: multipart/x-mixed-replace; boundary=--OPENCV_UDP_BOUNDARY\r\n";
//                int StreamHeaderSize = StreamHeader.length();
//                if (sendto(fd, &StreamHeader[0], StreamHeaderSize, 0, (struct sockaddr*) &send_addr, sizeof send_addr) < 0)
//                {
//                //cerr << "\n--> bytes = " << bytes << endl;
//                    quit("\n--> send() failed", 1);
//                    errno_abort("send");
//                }

//            }

            //memset(&serverAddr, 0x0, serverAddrLen);
        }

        /* have we terminated yet? */
        pthread_testcancel();

        /* no, take a rest for a while */
        usleep(1);   //1 Micro Sec
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * this function provides a way to exit nicely from the system
 */
void quit(string msg, int retval)
{
    if (retval == 0) {
        cout << (msg == "NULL" ? "" : msg) << "\n" << endl;
    } else {
        cerr << (msg == "NULL" ? "" : msg) << "\n" << endl;
    }
    if (clientSock){
        close(clientSock);
    }
    if (capture.isOpened()){
        capture.release();
    }
    if (!(img0.empty())){
        (~img0);
    }
    if (!(img1.empty())){
        (~img1);
    }

    pthread_mutex_destroy(&mutex1);
    exit(retval);
}




