#include <iBUSem.h>

iBus::iBus(HardwareSerial& serial):
m_ser(serial)
{
	serial.begin(115200);
	this->m_ser = serial;
}
/*
iBus::iBus(SoftwareSerial& serial):
m_ser(serial)
{
	serial.begin(115200);
	this->m_ser = serial;
}
*/

iBus::iBus(AltSoftSerial& serial):
m_ser(serial)
{
	serial.begin(115200);
	this->m_ser = serial;
}

/*
iBus::iBus(CustomSoftwareSerial& serial):
m_ser(serial)
{
	serial.begin(115200);
	this->m_ser = serial;
}
int iBus::get_channel(int ch)
{
	return m_channel[ch];
}
*/

void iBus::set_channel(int ch, int val)
{
	m_channel_out[ch] = val;
}

int iBus::get_tx_channel(int ch)
{
	return m_channel_out[ch];
}

void iBus::set_tx_period(unsigned int val)
{
	m_minimum_packet_spacing = val;
}

void iBus::set_alive_timeout(unsigned int timeout)
{
	m_timeout = timeout;
}

bool iBus::is_alive()
{
	if(millis() - m_last_iBus_packet < m_timeout)
	{
		return true;
	}
	return false;
}

void iBus::handle(unsigned int timeout)
{

	// If bytes are waiting in the serial buffer, process them
	// but don't loop for more than timeout ms
	uint32_t start = millis();
	while(m_ser.available() && millis()-start < timeout)
	{
		uint8_t c = m_ser.read();

		// If header byte received outside of packet, start a new packet
		if(!m_in_packet && c == 0x55) 
		{
			m_in_packet = true;
			m_packet_offset = 0;
			
			// If the next byte is also 0x55, it's most likely an error
			// but there are 4 valid values it could be.
			if(m_ser.peek() == 0x55)
			{
				m_ser.read();
				if(m_ser.peek() >= 4 && m_ser.peek() <=7)
				{ // Valid packet, let's clean up a bit
					m_packet[0] = 0x55;
					m_packet[1] = 0x55;
					m_packet_offset = 2;
				}
			}
		}

		// If m_packet_size bytes have been received, end packet and parse it
		if(m_packet_offset >= m_packet_size-1 && m_in_packet)
		{
			m_in_packet = false;
			m_packet[m_packet_size-1] = c;
		
			if(m_checksum_check(m_packet))
			{
				m_parse_channels(m_packet, m_channel);
				m_last_iBus_packet = millis();
			}
					
			m_packet[0] = 0x00; // Meant to signify error/no packet
		}
		
		// If in a packet, store byte and increment m_packet_offset
		if(m_in_packet)
		{
			m_packet[m_packet_offset++] = c;
		}
	}

	// If the link is alive (received valid packets recently), 
	// and over m_minimum_packet_spacing since last sent packet. Send a packet
	if(is_alive() && millis() - m_last_packet_sent > m_minimum_packet_spacing)
	{
		m_last_packet_sent = millis();
		m_send_packet(m_channel_out);
	}
}

void iBus::m_send_packet(int ch[])
{
	// Set up buffer and set header byte
	uint8_t buff[m_packet_size];
	buff[0] = 0x55;

	// For each channel, unpack 16 bit integer into bytes
	// The values are sent LSB first, MSb first.
	for(int i=0; i<m_channels_per_packet; i++)
	{
		buff[i*2+1] = (ch[i] & 0x00FF);
		buff[i*2+2] = (ch[i] >> 8);
	}
	int checksum = m_get_checksum(ch);

	buff[29] = (checksum & 0x00FF);
	buff[30] = (checksum >> 8);

	m_ser.write(buff, m_packet_size);
}

// Checksum is a simple sum of channel values
int iBus::m_get_checksum(int ch[])
{
	int sum = 0;
	for(int i=0; i<m_channels_per_packet; i++)
	{
		sum += ch[i];
	}
	return sum;
}

bool iBus::m_checksum_check(uint8_t packet[])
{
	int recieved_checksum = packet[30] << 8 | packet[29];
	int ch[m_channels_per_packet];
	m_parse_channels(packet, ch);

	return (recieved_checksum == m_get_checksum(ch));
}

void iBus::m_parse_channels(uint8_t packet[], int ch[])
{
	// For each channel, store 2 bytes in 16bit integer
	// The values are sent MSb first, LSB first
	for(int i=0; i<m_channels_per_packet; i++)
	{
		ch[i] = packet[i*2+2] << 8 | packet[i*2+1];
	}
}

uint32_t iBus::time_since_last()
{
	return millis() - m_last_iBus_packet;
}
