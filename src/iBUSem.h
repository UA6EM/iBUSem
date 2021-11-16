#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
//#include <SoftwareSerial.h>
#include <AltSoftSerial.h>
#include <CustomSoftwareSerial.h>

class iBus
{
public:

    /**
     * Constructor for the class. Takes either a HardwareSerial or SoftwareSerial class
     */
    iBus(HardwareSerial& serial);
 //   iBus(SoftwareSerial& serial);
    iBus(AltSoftSerial& serial);
    iBus(CustomSoftwareSerial& serial);
    
    /**
     * A getter to get received channel values
     * @param ch An integer value to pick channel, 0-13
     */
    int get_channel(int ch);

    /**
     * A method to set transmitted channel values
     * @param ch An integer value to pick channel, 0-13
     * @param val An integer value to send on this channel
     */
    void set_channel(int ch, int val);

    /**
     * A method to get transmitted channel values
     * @param ch An integer value to pick channel, 0-13
     */
    int get_tx_channel(int ch);

    /**
     * A method to set the minimum time between sending ibus packets
     * @param val The minimum time in ms between transmitting ibus packets
     */
    void set_tx_period(unsigned int val);

    /**
     * A method to set the maximum time between receiving ibus packets to 
     * consider the connection active
     * @param timeout The maximum time in ms between receiving ibus packets
     */
    void set_alive_timeout(unsigned int timeout);

    /**
     * @return true if valid packet has been received recently, defined by m_timeout. Otherwise false. 
     */
    bool is_alive();

    /**
     * The main ticker function for the class. Handles any buffered incoming bytes, 
     */
    void handle(unsigned int timeout = 10);

    /**
     * @return The time since last valid iBUS packet
     */
    uint32_t time_since_last();

private:

    Stream& m_ser;

    // The extra bytes used for non-data in the packets.
    // One start byte and 2 CRC bytes
    const static int m_packet_overhead = 3;

    // Number of channels sent per packet
    const static int m_channels_per_packet = 14;

    // Size of a packet
    const static int m_packet_size = 2*m_channels_per_packet + m_packet_overhead;

    // Max time between packets
    unsigned int m_timeout = 25; // If no iBUS packet has been received in this time, consider the TX off
    uint32_t m_last_iBus_packet = 0;

    // Buffer for receiving packets
    uint8_t m_packet[m_packet_size];
    
    // Array of received channel values
    int m_channel[m_channels_per_packet];

    // Array of channel values to send
    int m_channel_out[m_channels_per_packet];

    // Flag signifying if currently in a packet
    bool m_in_packet = false;
    
    // Current index of packet
    int m_packet_offset = 0;

    // Returns sum of all channel values
    int  m_get_checksum(int ch[]);

    // Checks if checksum matches received data
    bool m_checksum_check(uint8_t packet[]);

    // Parses channel values from raw packet
    void m_parse_channels(uint8_t packet[], int ch[]);

    // Rate limiting for sending packets in ms
    unsigned int m_minimum_packet_spacing = 10;

    uint32_t m_last_packet_sent = 0;

    // Builds packet and sends it
    void m_send_packet(int ch[]);
    
};
