#pragma once

#include <Ethernet.h>
#include <EthernetUdp.h>
#include "trajectory.h"
#include "../include/definitions.h"
#include "../include/ErrorDef.h"
#include "logger.h"

class NetworkHandler {
public:
    enum Protocol {
        UDP,
        TCP
    };

    NetworkHandler() = default;

    Error_t init(void(*packetAvailableCallback)(char* pPacket, size_t packetSize)) {
        m_packetAvailableCallback = packetAvailableCallback;
        Ethernet.init(CS_PIN);

        IPAddress ip;
        ip.fromString(IP_ADDR);

        // start the Ethernet
        Ethernet.begin(mac, ip);

        // Check for Ethernet hardware present
        if (Ethernet.hardwareStatus() == EthernetNoHardware) {
            LOG_ERROR("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
            return kFileOpenError;  // similar to linux hardware us treated as a file
        }

        if (Ethernet.linkStatus() == LinkOFF) {
            LOG_ERROR("Ethernet cable is not connected.");
            return kFileOpenError;
        }

        // start UDP
        m_socket.begin(PORT);
        LOG_LOG("Server Listening...");
        return kNoError;
    }

    void close() {
        m_socket.stop();
        m_client.stop();
    }

    void poll() {
        int packetSize = m_socket.parsePacket();
        if (packetSize) {
            // read the packet into packetBuffer and callback
            if (m_socket.read(m_packetBuffer, packetSize)) {
                m_packetAvailableCallback(m_packetBuffer, packetSize);
            }
        }
    }

    void requestData() {
        send(Command::REQUEST_DATA);
    }

    void send(Command cmd, Protocol protocol = TCP) {
        switch (protocol) {
        case TCP:
            if (m_client.connected()) {
                m_client.write((int) cmd);
            }
            break;

        case UDP:
            break;
        }
    }

    void send(int32_t* motorData, Protocol protocol = UDP) {
        if (!motorData)
            return;
        const size_t length = NUM_MOTORS * 4; //*NUM_BYTES_PER_VALUE;
        uint8_t buf[length];


        switch (protocol) {
        case TCP:
            break;

        case UDP:
            m_socket.write(buf, length);
            break;
        }
    }

    int connectToMaster() {
        masterIp.fromString(MASTER_ADDR);
        return m_client.connect(masterIp, PORT);
    }

private:
    // Enter a MAC address and IP address for your controller below.
    // The IP address will be dependent on your local network:
    byte mac[6] = {
      0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
    };

    // An EthernetUDP instance to let us send and receive packets over UDP
    EthernetUDP m_socket;
    EthernetClient m_client;

    IPAddress masterIp;

    // buffers for receiving and sending data
    char m_packetBuffer[MAX_BUFFER_SIZE];   // buffer to hold incoming packet,
    char m_requestBuffer[1] = { REQUEST_BUFFER_FLAG };    // a string to send back

    void (*m_packetAvailableCallback)(char* pPacket, size_t packetSize);
};