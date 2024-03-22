//embedded system HW2
#include "mbed.h"
#include "wifi_helper.h"
#include "mbed-trace/mbed_trace.h"
#include "stm32l475e_iot01_tsensor.h"
#include "stm32l475e_iot01_hsensor.h"
#include "stm32l475e_iot01_psensor.h"
#include "stm32l475e_iot01_magneto.h"
#include "stm32l475e_iot01_gyro.h"
#include "stm32l475e_iot01_accelero.h"

#if MBED_CONF_APP_USE_TLS_SOCKET
#include "root_ca_cert.h"

#ifndef DEVICE_TRNG
#error "mbed-os-example-tls-socket requires a device which supports TRNG"
#endif
#endif // MBED_CONF_APP_USE_TLS_SOCKET

int16_t pDataXYZ[3] = {0};
float gDataXYZ[3] = {0};
float SCALE_MULTIPLIER = 0.5;
InterruptIn button(BUTTON1);
int volatile button_switch = 0;
void button_pressed()
{
    ++button_switch;
}

class SocketDemo {
    static constexpr size_t MAX_NUMBER_OF_ACCESS_POINTS = 10;
    static constexpr size_t MAX_MESSAGE_RECEIVED_LENGTH = 100;

#if MBED_CONF_APP_USE_TLS_SOCKET
    static constexpr size_t REMOTE_PORT = 443; // tls port
#else
    static constexpr size_t REMOTE_PORT = 7414; // standard HTTP port
#endif // MBED_CONF_APP_USE_TLS_SOCKET

public:
    SocketDemo() : _net(NetworkInterface::get_default_instance())
    {
    }

    ~SocketDemo()
    {
        if (_net) {
            _net->disconnect();
        }
    }

    void run()
    {
        if (!_net) {
            printf("Error! No network interface found.\r\n");
            return;
        }

        /* if we're using a wifi interface run a quick scan */
        if (_net->wifiInterface()) {
            /* the scan is not required to connect and only serves to show visible access points */
            wifi_scan();

            /* in this example we use credentials configured at compile time which are used by
             * NetworkInterface::connect() but it's possible to do this at runtime by using the
             * WiFiInterface::connect() which takes these parameters as arguments */
        }

        /* connect will perform the action appropriate to the interface type to connect to the network */

        printf("Connecting to the network...\r\n");

        nsapi_size_or_error_t result = _net->connect();
        if (result != 0) {
            printf("Error! _net->connect() returned: %d\r\n", result);
            return;
        }

        print_network_info();

        /* opening the socket only allocates resources */
        result = _socket.open(_net);
        if (result != 0) {
            printf("Error! _socket.open() returned: %d\r\n", result);
            return;
        }

#if MBED_CONF_APP_USE_TLS_SOCKET
        result = _socket.set_root_ca_cert(root_ca_cert);
        if (result != NSAPI_ERROR_OK) {
            printf("Error: _socket.set_root_ca_cert() returned %d\n", result);
            return;
        }
        _socket.set_hostname(MBED_CONF_APP_HOSTNAME);
#endif // MBED_CONF_APP_USE_TLS_SOCKET

        /* now we have to find where to connect */

        SocketAddress address;

        if (!resolve_hostname(address)) {
            return;
        }

        address.set_port(REMOTE_PORT);

        /* we are connected to the network but since we're using a connection oriented
         * protocol we still need to open a connection on the socket */

        printf("Opening connection to remote port %d\r\n", REMOTE_PORT);

        result = _socket.connect(address);
       
        if (result != 0) {
            printf("Error! _socket.connect() returned: %d\r\n", result);
            return;
        }
        
        char acc_json[500];
        int sample_num = 0;
        BSP_ACCELERO_Init();
        BSP_GYRO_Init();
        
        while (1){
            button.fall(&button_pressed);
            if(button_switch!=0) break;
            ++sample_num;
            BSP_ACCELERO_AccGetXYZ(pDataXYZ);
            BSP_GYRO_GetXYZ(gDataXYZ);
            float x = pDataXYZ[0]*SCALE_MULTIPLIER, y = pDataXYZ[1]*SCALE_MULTIPLIER, z = pDataXYZ[2]*SCALE_MULTIPLIER, 
                gx = gDataXYZ[0]*SCALE_MULTIPLIER, gy = gDataXYZ[1]*SCALE_MULTIPLIER, gz = gDataXYZ[2]*SCALE_MULTIPLIER;
            int len = std::sprintf(acc_json,"{\"x\":%f,\"y\":%f,\"z\":%f,\"gx\":%f,\"gy\":%f,\"gz\":%f,\"s\":%d}",(float)((int)(x*10000))/10000,
                                (float)((int)(y*10000))/10000, (float)((int)(z*10000))/10000, gx, gy, gz, sample_num);
            nsapi_size_or_error_t response = _socket.send(acc_json,len);
            if (0 >= response){
            printf("Error seding: %d\n", response);
            break;
            }
            ThisThread::sleep_for(100ms);
        } 
        printf("Demo concluded successfully \r\n");
    }

private:
    bool resolve_hostname(SocketAddress &address)
    {
        const char hostname[] = MBED_CONF_APP_HOSTNAME;

        /* get the host address */
        printf("\nResolve hostname %s\r\n", hostname);
        nsapi_size_or_error_t result = _net->gethostbyname(hostname, &address);
        if (result != 0) {
            printf("Error! gethostbyname(%s) returned: %d\r\n", hostname, result);
            return false;
        }

        printf("%s address is %s\r\n", hostname, (address.get_ip_address() ? address.get_ip_address() : "None") );

        return true;
    }

    void wifi_scan()
    {
        WiFiInterface *wifi = _net->wifiInterface();

        WiFiAccessPoint ap[MAX_NUMBER_OF_ACCESS_POINTS];

        /* scan call returns number of access points found */
        int result = wifi->scan(ap, MAX_NUMBER_OF_ACCESS_POINTS);

        if (result <= 0) {
            printf("WiFiInterface::scan() failed with return value: %d\r\n", result);
            return;
        }

        printf("%d networks available:\r\n", result);

        for (int i = 0; i < result; i++) {
            printf("Network: %s secured: %s BSSID: %hhX:%hhX:%hhX:%hhx:%hhx:%hhx RSSI: %hhd Ch: %hhd\r\n",
                   ap[i].get_ssid(), get_security_string(ap[i].get_security()),
                   ap[i].get_bssid()[0], ap[i].get_bssid()[1], ap[i].get_bssid()[2],
                   ap[i].get_bssid()[3], ap[i].get_bssid()[4], ap[i].get_bssid()[5],
                   ap[i].get_rssi(), ap[i].get_channel());
        }
        printf("\r\n");
    }

    void print_network_info()
    {
        /* print the network info */
        SocketAddress a;
        _net->get_ip_address(&a);
        printf("IP address: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
        _net->get_netmask(&a);
        printf("Netmask: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
        _net->get_gateway(&a);
        printf("Gateway: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
    }

private:
    NetworkInterface *_net;

#if MBED_CONF_APP_USE_TLS_SOCKET
    TLSSocket _socket;
#else
    TCPSocket _socket;
#endif // MBED_CONF_APP_USE_TLS_SOCKET
};

int main() {
    printf("\r\nStarting socket demo\r\n\r\n");

#ifdef MBED_CONF_MBED_TRACE_ENABLE
    mbed_trace_init();
#endif

    SocketDemo *example = new SocketDemo();
    MBED_ASSERT(example);
    example->run();

    return 0;
}
