#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

#include "device.h"
#include "common.h"

//-----------------------------------------------------------------------------

Device::Device(uint32_t baseAddr, unsigned int regCnt)
    : m_fd(-1), m_baseAddr(baseAddr), m_regCnt(regCnt), m_mapBase(nullptr),
      m_mappedFlag(false)
{
    // get offset
    m_offset = (unsigned int)(m_baseAddr & (m_pagesize - 1));

    //-------------------------------------------------------------------------

    // allocate memory for buffer with device register values
    m_region = (struct register_t *) malloc(sizeof (struct register_t) *
                                            m_regCnt);
    if (!m_region)
    {
        printerr("Can't allocate buffer for device registers\n");
        exit(1);
    }

    //-------------------------------------------------------------------------

    // fill region with addresses (they will be always the same)
    for (unsigned int i = 0; i < m_regCnt; i++)
        m_region[i].addr = m_baseAddr + i * m_step;

    //-------------------------------------------------------------------------

    // map device
    m_mappedFlag = m_mapRegion();
    if(!m_mappedFlag)
        printerr("Cannot map device\n");
}

//-----------------------------------------------------------------------------

Device::~Device()
{
    // unmap device
    if (m_mapBase != MAP_FAILED)
    {
        if (munmap(m_mapBase, m_pagesize) != 0)
            printerr("Cannot unmap device registers\n");
    }

    //-------------------------------------------------------------------------

    // free buffer with device registerts
    free(m_region);

    //-------------------------------------------------------------------------

    // close character device
    if (m_fd != -1)
        close(m_fd);
}

//-----------------------------------------------------------------------------

bool Device::readRegion()
{
    // check if device is mapped
    if (!m_mappedFlag)
    {
        printerr("Device isn't remapped\n");
        return false;
    }

    // read device registers
    m_readRegion();

    return true;
}

//-----------------------------------------------------------------------------

void Device::showRegion()
{
    for (unsigned int i = 0; i < m_regCnt; i++)
        printf("0x%08X - 0x%08X\n", m_region[i].addr, m_region[i].val);
}

//-----------------------------------------------------------------------------

bool Device::m_mapRegion()
{
    // register count isn't specified
    if (!m_regCnt)
        return false;

    //-------------------------------------------------------------------------

    // open character device
    m_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (m_fd == -1)
    {
        printerr("Error opening /dev/mem (%d) : %s\n", errno, strerror(errno));
        return false;
    }

    //-------------------------------------------------------------------------

    // map device
    m_mapBase = mmap(0, m_pagesize, PROT_READ | PROT_WRITE, MAP_SHARED,
                    m_fd,
                    m_baseAddr & ~((typeof(m_baseAddr))m_pagesize - 1));

    if (m_mapBase == MAP_FAILED)
    {
        printerr("Error mapping (%d) : %s\n", errno, strerror(errno));
        return false;
    }

    //-------------------------------------------------------------------------

    return true;
}

//-----------------------------------------------------------------------------

void Device::m_readRegion()
{
    void * virt_addr;
    uint32_t tmpOffset;
    uint32_t val;

    tmpOffset = m_offset;
    // read memory region from device
    for (unsigned int i = 0; i < m_regCnt; i++)
    {
        virt_addr = m_mapBase + tmpOffset;
        m_region[i].val = *((uint32_t *) virt_addr);
        tmpOffset += m_step;
    }
}

//=============================================================================

MultipleDevice::MultipleDevice(int argc, char ** argv)
    : m_argc(argc), m_argv(argv)
{
    unsigned int argBit = 0;

    // incorrect using of program
    if (m_argc < 5)
        m_showHelp();

    // parse devices
    argBit = m_getArgs();
    m_args = m_parseArgs(argBit);

    // create devices
    m_createDevices();
}

//-----------------------------------------------------------------------------

MultipleDevice::~MultipleDevice()
{
    for (unsigned int i = 0; i < m_args.addrs.addr_count; i++)
        delete m_devs[i];

    delete [] m_devs;
}

//-----------------------------------------------------------------------------

void MultipleDevice::showRegions()
{
    // read all regions
    for (unsigned int i = 0; i < m_args.addrs.addr_count; i++)
        m_devs[i]->readRegion();

    // show all regions
    for (unsigned int i = 0; i < m_args.addrs.addr_count; i++)
        m_devs[i]->showRegion();
}

//-----------------------------------------------------------------------------

void MultipleDevice::m_showHelp()
{
    printf(
                "\n Usage: fastread -a [base addresses] -c [register count]\n\n"
                " Fastread v. 1.1                                   \n"
                " Arguments:                                        \n"
                "    -a         Array of base addresses             \n"
                "    -c         Count of registers (4 byte each)    \n"
                "    -h         Show help message and quit          \n\n");
    exit(EXIT_SUCCESS);
}

//-----------------------------------------------------------------------------

int MultipleDevice::m_getArgs()
{
    const char * input_arg[INPUT_ARGUMENTS_MAX] =
    {
        "-a", "-c", "-h"
    };

    unsigned int arg_bit = 0;

    //------------------------------------------------------------------------

    for (int i = 1; i < m_argc; i++)
    {
        for (int j = 0; j < INPUT_ARGUMENTS_MAX; j++)
        {
            if (!strcmp(m_argv[i], input_arg[j]))
            {
                if (j == 0)         /* argument -a */
                {
                    arg_bit |= 1;
                }
                else if (j == 1)    /* argument -c */
                {
                    arg_bit |= 2;
                }
                else if (j == 2)    /* argument -h */
                {
                    arg_bit = 425;
                    break;
                }
            }
        }
    }

    //------------------------------------------------------------------------

    if (arg_bit != 425)
    {
        if (arg_bit != 3)
        {
            m_showHelp();
            exit(1);
        }
    }

    return arg_bit;
}

//-----------------------------------------------------------------------------

unsigned int MultipleDevice::m_getRegCount()
{
    unsigned int reg_count = 0;

    for (int i = 1; i < m_argc; i++)
    {
        for (int j = 0; j < INPUT_ARGUMENTS_MAX; j++)
        {
            if (!strcmp(m_argv[i], "-c"))
            {
                ++i;
                if (i == m_argc)
                {
                    fprintf(stderr, "No register count is specified\n"
                                    "Use argument -h for full details\n\n");
                    exit(EXIT_FAILURE);
                }

                reg_count = atoi(m_argv[i]);

                if (!reg_count)
                {
                    std::cout << "Register count is 0. "
                              << "Specify please register count."
                              << std::endl;
                    exit(1);
                }

                return reg_count;
            }
        }
    }

    return reg_count;
}

//-----------------------------------------------------------------------------

MultipleDevice::m_addrs_t MultipleDevice::m_getAddrs()
{
    struct m_addrs_t addrs;
    unsigned int addrs_count = 0;

    //-------------------------------------------------------------------------

    for (int i = 1; i < m_argc; i++)
    {
        for (int j = 0; j < INPUT_ARGUMENTS_MAX; j++)
        {
            if (!strcmp(m_argv[i], "-a"))
            {
                ++i;
                if (i == m_argc)
                {
                    fprintf(stderr, "No base addresses are specified\n"
                                    "Use argument -h for full details\n\n");
                    exit(EXIT_FAILURE);
                }

                // get addrs
                for (int k = i; k < m_argc; k++)
                {
                    if (!strcmp(m_argv[k], "-c"))
                    {
                        if (!addrs_count)
                        {
                            fprintf(stderr, "No base addresses "
                                            "are specified\n"
                                            "Use argument -h for "
                                            "full details\n\n");
                            exit(EXIT_FAILURE);
                        }
                        else
                        {
                            // first addr
                            addrs.addrs = &m_argv[i];
                            // the hole count of addrs
                            addrs.addr_count = addrs_count;

                            return addrs;
                        }
                    }

                    addrs_count++;
                }

                // first addr
                addrs.addrs = &m_argv[i];
                // the hole count of addrs
                addrs.addr_count = addrs_count;

                return addrs;
            }
        }
    }

    return addrs;
}

//-----------------------------------------------------------------------------

MultipleDevice::m_args_t MultipleDevice::m_parseArgs(unsigned int argBit)
{
    struct m_args_t args;

    //-------------------------------------------------------------------------

    // show help message and quit
    if (argBit == 425)
    {
        m_showHelp();
        exit(0);
    }

    //-------------------------------------------------------------------------

    // argument -a
    if (argBit & 1)
        args.addrs = m_getAddrs();

    //-------------------------------------------------------------------------

    // argument -c
    if (argBit & 2)
        args.reg_count = m_getRegCount();

    //-------------------------------------------------------------------------

    return args;
}

//-----------------------------------------------------------------------------

void MultipleDevice::m_createDevices()
{
    uint32_t target;
    char * endp = NULL;

    // allocate memory for device array
    m_devs = new Device * [m_args.addrs.addr_count];
    // (Device **) malloc(sizeof (Device *) * m_args.addrs.addr_count);

    for (unsigned int i = 0; i < m_args.addrs.addr_count; i++)
    {
        // get addr
        target = strtoull(m_args.addrs.addrs[i], &endp, 0);
        if (errno != 0 || (endp && 0 != *endp))
        {
            printerr("Invalid memory address: %s\n", m_args.addrs.addrs[i]);
            exit(1);
        }

        // create device here
        m_devs[i] = new Device(target, m_args.reg_count);
    }
}
