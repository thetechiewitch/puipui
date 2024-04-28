#ifndef _SERIAL_H_
#define _SERIAL_H_

int serial_init(std::string tty);
int serial_write(const std::vector<uint8_t>& data, int start);
int serial_read(std::vector<uint8_t>& data);
int serial_close();
int serial_CTSbit(bool value);

#endif