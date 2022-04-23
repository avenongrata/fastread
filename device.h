#ifndef DEVICE_H
#define DEVICE_H

#include <iostream>
#include <unistd.h>

//-----------------------------------------------------------------------------

class Device
{
public:

    Device(uint32_t baseAddr, unsigned int regCnt);
    ~Device();

    //-------------------------------------------------------------------------

    bool readRegion();
    void showRegion();

    //-------------------------------------------------------------------------

private:

    // to save addresses and its values
    struct register_t
    {
        uint32_t addr;
        uint32_t val;
    } __attribute__((__aligned__));

    // character device descriptor
    int m_fd;
    // device base address
    uint32_t m_baseAddr;
    // count of registers to mmap
    unsigned int m_regCnt;
    // maped address
    void * m_mapBase;
    // offset in page ?
    uint32_t m_offset;
    // region will be saved to this buffer
    struct register_t * m_region;
    // flag to detect is region mapped or not
    bool m_mappedFlag;
    // offset to next register
    const unsigned int m_step = sizeof(uint32_t);
    // page size
    const unsigned int m_pagesize = (unsigned)getpagesize();

    //-------------------------------------------------------------------------

    bool m_mapRegion();
    void m_readRegion();
};

//=============================================================================

class MultipleDevice
{
public:

    MultipleDevice(int argc, char ** argv);
    ~MultipleDevice();

    //-------------------------------------------------------------------------

    void showRegions();

    //-------------------------------------------------------------------------

private:

    struct m_addrs_t
    {
        char ** addrs;
        unsigned int addr_count;
    };

    struct m_args_t
    {
        struct m_addrs_t addrs;
        unsigned int reg_count;
    };

    //-------------------------------------------------------------------------

    Device ** m_devs;
    int m_argc;
    char ** m_argv;
    struct m_args_t m_args;

    //-------------------------------------------------------------------------

    void m_showHelp();
    int m_getArgs();
    unsigned int m_getRegCount();
    struct m_addrs_t m_getAddrs();
    struct m_args_t m_parseArgs(unsigned int argBit);
    void m_createDevices();
};

//-----------------------------------------------------------------------------

#endif // DEVICE_H
