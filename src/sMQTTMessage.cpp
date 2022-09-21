#include "sMQTTMessage.h"
#include "sMQTTBroker.h"

sMQTTMessage::sMQTTMessage()
{
	reset();
};
sMQTTMessage::sMQTTMessage(Type t, unsigned char bits_d3_d0)
{
	create(t);
	buffer[0] |= bits_d3_d0;
};
void sMQTTMessage::getString(const char* &buff, unsigned short &len)
{
	len = (buff[0] << 8) | (buff[1]);
	buff += 2;
};
void sMQTTMessage::reset()
{
	state = FixedHeader;
	buffer.clear();
	size = 0;
};
void sMQTTMessage::incoming(char in_byte)
{
	buffer.push_back(in_byte);
	switch (state)
	{
	case FixedHeader:
		size = 0;
		multiplyer = 1;
		state = Length;
		break;
	case Length:
		{
			unsigned char encoded = in_byte & 0x7f;
			size += encoded*multiplyer;
			multiplyer *= 0x80;
			if ((in_byte & 0x80) == 0)
			{
				vheader = buffer.size();
				if (size == 0)
					state = Complete;
				else
				{
					buffer.reserve(size);
					state = VariableHeader;
				}
			}
#if 0
			if (size == 0)
				size = in_byte & 0x7F;
			else if (size < 128)
				size += static_cast<uint16_t>(in_byte & 0x7F) << 7;
			else
				state = Error;  // Really don't want to handle msg with length > 16k
								/*if (size > MaxBufferLength)
								{
								state = Error;
								}
								else*/ if ((in_byte & 0x80) == 0)
								{
									vheader = buffer.size();
									if (size == 0)
										state = Complete;
									else if (size > 500)	// TODO magic
									{
										state = Error;
									}
									else
									{
										buffer.reserve(size);
										state = VariableHeader;
									}
								}
#endif
		}
		break;
	case VariableHeader:
	case PayLoad:
		--size;
		if (size == 0)
		{
			state = Complete;
			// hexdump("rec");
		}
		break;
	case Create:
		size++;
		break;
	case Complete:
	default:
		reset();
		break;
	}
};
int sMQTTMessage::encodeLength(char* msb, int length) const
{
	int count = 0;
	do
	{
		unsigned char encoded(length & 0x7f);
		length >>= 7;
		if (length) encoded |= 0x80;
		//*msb++ = encoded;
		*msb = encoded;
		msb++;
		count++;
	} while (length);
	return count;
};
sMQTTError sMQTTMessage::sendTo(sMQTTClient *client,bool needRecalc)
{
	if (buffer.size())
	{
		if (needRecalc)
		{
			//debug("sending " << buffer.size() << " bytes");
			char *bytes = (char*)buffer.data();
			char size[4];
			size[0] = bytes[1];
			if (encodeLength(size, buffer.size() - vheader/*2*/) > 1)
			{
				buffer.insert(buffer.begin() + 2, size[1]);
			}
			buffer[1] = size[0];
		}
		// hexdump("snd");
		client->write(buffer.data(), buffer.size());
	}
	else
	{
		//debug("??? Invalid send");
		return sMQTTInvalidMessage;
	}
	return sMQTTOk;
};