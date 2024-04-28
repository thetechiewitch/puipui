#include <chrono>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <vector>

#define MAX_READ 2000

static int serial_port;

int serial_init(std::string tty_name) {
    serial_port = open(tty_name.c_str(), (O_RDWR | O_NDELAY | O_NOCTTY));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    tcflush(serial_port, TCIOFLUSH);

    if (serial_port == -1) {
        std::cerr << "cannot open serial port\n";
        return -1;
    } else {
        fcntl(serial_port, F_SETFL, 0);
    }

    struct termios tty;

    if (tcgetattr(serial_port, &tty) != 0) {
        std::cout << "Error no " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
        return -1;
    }

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;

    tty.c_cc[VTIME] = 1;
    tty.c_cc[VMIN] = 0;

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        std::cout << "Error no " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
        return -1;
    }

    return 0;
}

int serial_write(const std::vector<uint8_t>& data, int start) {
    int written_bytes = write(serial_port, data.data() + start, data.size() - start);
    if (written_bytes != data.size() - start) {
        std::cerr << written_bytes << " written, of " << data.size() - start << " bytes" << std::endl;
    }
    return written_bytes;
}

int serial_read(std::vector<uint8_t>& data) {
    uint8_t read_buf[MAX_READ];
    int num_bytes = read(serial_port, &read_buf, sizeof(read_buf));

    if (num_bytes < 0) {
        printf("Error reading: %s", strerror(errno));
        return -1;
    }
    if (num_bytes > 0) {
        copy(read_buf, &read_buf[num_bytes], back_inserter(data));
    }
    return num_bytes;
}

int serial_close() { return close(serial_port); }

int serial_CTSbit(bool value) {
    int modem_ctrl = 0;

    if (ioctl(serial_port, TIOCMGET, &modem_ctrl) != 0) {
        return -1;
    }

    if (value) {
        modem_ctrl |= TIOCM_CTS;
    } else {
        modem_ctrl &= ~TIOCM_CTS;
    }

    if (ioctl(serial_port, TIOCMSET, &modem_ctrl) != 0) {
        return -2;
    }

    return 0;
}
