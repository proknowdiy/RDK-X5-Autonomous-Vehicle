#include "vehicle_controller/serial_port.hpp"

#include <unistd.h>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <iostream>
#include <cstring>
#include <cerrno>

SerialPort::SerialPort()
    : fd_(-1)
{
}

SerialPort::~SerialPort()
{
    closePort();
}

bool SerialPort::isOpen() const
{
    return fd_ >= 0;
}

void SerialPort::closePort()
{
    if (fd_ >= 0)
    {
        close(fd_);
        fd_ = -1;
    }
}

bool SerialPort::writeBytes(
    const void *data,
    size_t length)
{
    if (!isOpen())
        return false;

    const auto *ptr =
        static_cast<const uint8_t *>(data);

    ssize_t written =
        write(fd_, ptr, length);

    return written ==
        static_cast<ssize_t>(length);
}

int SerialPort::readBytes(
    void *buffer,
    size_t length)
{
    if (!isOpen())
        return -1;

    return read(
        fd_,
        buffer,
        length);
}

bool SerialPort::openPort(
    const std::string &device,
    int baudrate)
{
    closePort();

    fd_ = open(device.c_str(), O_RDWR | O_NOCTTY | O_SYNC);

    if (fd_ < 0)
    {
        std::cerr
            << "Failed to open "
            << device
            << " : "
            << strerror(errno)
            << std::endl;

        return false;
    }

    struct termios tty {};

    if (tcgetattr(fd_, &tty) != 0)
    {
        std::cerr
            << "tcgetattr() failed: "
            << strerror(errno)
            << std::endl;

        closePort();
        return false;
    }

    speed_t speed;

    switch (baudrate)
    {
        case 9600:
            speed = B9600;
            break;

        case 57600:
            speed = B57600;
            break;

        case 115200:
            speed = B115200;
            break;

        default:
            closePort();
            return false;
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    tty.c_cflag |= (CLOCAL | CREAD);

    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    tty.c_iflag = 0;
    tty.c_oflag = 0;
    tty.c_lflag = 0;

    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd_, TCSANOW, &tty) != 0)
    {
        std::cerr
            << "tcsetattr() failed: "
            << strerror(errno)
            << std::endl;

        closePort();
        return false;
    }

    tcflush(fd_, TCIOFLUSH);

    return true;
}