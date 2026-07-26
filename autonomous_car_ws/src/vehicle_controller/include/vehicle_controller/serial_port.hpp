#pragma once

#include <string>
#include <vector>

class SerialPort
{
public:

    SerialPort();

    ~SerialPort();

    bool openPort(
        const std::string &device,
        int baudrate);

    void closePort();

    bool isOpen() const;

    bool writeBytes(
        const void *data,
        size_t length);

    int readBytes(
        void *buffer,
        size_t length);

private:

    int fd_;
};