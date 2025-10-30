#include <Nvic/NvicVectorTable.h>
#include <rtems/score/armv7m.h>
#include <bsp/armv7m-irq.h>
#include <bsp/start.h>

#define VECTOR_TABLE_ALIGNMENT 128

extern const char _ISR_Stack_area_end[];

void Fault_Handler();

__attribute__((aligned(VECTOR_TABLE_ALIGNMENT)))
const Nvic_VectorTable exception_table = {
  // configure stack pointer using linker symbols
  .initialStackPointer = (void*)_ISR_Stack_area_end,

  // clang-format off
  .resetHandler        = _start,
  .nmiHandler          = _ARMV7M_Exception_default,
  .hardFaultHandler    = Fault_Handler,
  .memManageHandler    = Fault_Handler,
  .busFaultHandler     = Fault_Handler,
  .usageFaultHandler   = Fault_Handler,
  .reserved0 = {
    _ARMV7M_Exception_default,
    _ARMV7M_Exception_default,
    _ARMV7M_Exception_default,
    _ARMV7M_Exception_default,
  },
  .svcHandler          = _ARMV7M_Supervisor_call,
  .debugMonHandler     = _ARMV7M_Exception_default,
  .reserved1           = _ARMV7M_Exception_default,
  .pendSvHandler       = _ARMV7M_Pendable_service_call,
  .sysTickHandler      = _ARMV7M_Clock_handler,
  // clang-format on

  .irqHandlers = {
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 0  Supply Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 1  Reset Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                      // 2  Real-time Clock
    _ARMV7M_NVIC_Interrupt_dispatch,                      // 3  Real-time Timer
    _ARMV7M_NVIC_Interrupt_dispatch,                      // 4  Watchdog Timer
    _ARMV7M_NVIC_Interrupt_dispatch,                      // 5  Power Management Controller
#if defined(N7S_TARGET_SAMV71Q21)
    _ARMV7M_NVIC_Interrupt_dispatch,                      // 6  Embedded Flash Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 7  Universal Asynchronous Receiver Transmitter
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 8  Universal Asynchronous Receiver Transmitter
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 9  Reserved
#elif defined(N7S_TARGET_SAMRH71F20) || defined(N7S_TARGET_SAMRH707F18)
    _ARMV7M_NVIC_Interrupt_dispatch,                  // 6  Matrix 0
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 7  FLEXCOM 0
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 8  FLEXCOM 1
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 9  NMI Controller
#endif
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 10 Parallel Input/Output Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 11 Parallel Input/Output Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 12 Parallel Input/Output Controller
#if defined(N7S_TARGET_SAMV71Q21)
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 13 Universal Synchronous Asynchronous Receiver Transmitter
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 14 Universal Synchronous Asynchronous Receiver Transmitter
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 15 Universal Synchronous Asynchronous Receiver Transmitter
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 16 Parallel Input/Output Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 17 Parallel Input/Output Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 18 High Speed MultiMedia Card Interface
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 19 Two-wire Interface High Speed
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 20 Two-wire Interface High Speed
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 21 Serial Peripheral Interface
    _ARMV7M_NVIC_Interrupt_dispatch,                      // 22 Synchronous Serial Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 23 Timer Counter 0 Channel 0
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 24 Timer Counter 0 Channel 1
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 25 Timer Counter 0 Channel 2
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 26 Timer Counter 1 Channel 0
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 27 Timer Counter 1 Channel 1
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 28 Timer Counter 1 Channel 2
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 29 Analog Front-End Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 30 Digital-to-Analog Converter Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 31 Pulse Width Modulation Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                      // 32 Integrity Check Monitor
    _ARMV7M_NVIC_Interrupt_dispatch,                      // 33 Analog Comparator Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 34 USB High-Speed Interface
    _ARMV7M_NVIC_Interrupt_dispatch,               // 35 MCAN 0 IRQ 0
    _ARMV7M_NVIC_Interrupt_dispatch,               // 36 MCAN 0 IRQ 1
    _ARMV7M_NVIC_Interrupt_dispatch,               // 37 MCAN 1 IRQ 0
    _ARMV7M_NVIC_Interrupt_dispatch,               // 38 MCAN 1 IRQ 1
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 39 Gigabit Ethernet MAC
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 40 Analog Front-End Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 41 Two-wire Interface High Speed
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 42 Serial Peripheral Interface
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 43 Quad Serial Peripheral Interface
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 44 Universal Asynchronous Receiver Transmitter
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 45 Universal Asynchronous Receiver Transmitter
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 46 Universal Asynchronous Receiver Transmitter
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 47 Timer Counter 2 Channel 0
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 48 Timer Counter 2 Channel 1
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 49 Timer Counter 2 Channel 2
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 50 Timer Counter 3 Channel 0
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 51 Timer Counter 3 Channel 1
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 52 Timer Counter 3 Channel 2
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 53 MediaLB IRQ 0
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 54 MediaLB IRQ 1
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 55 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                      // 56 Advanced Encryption Standard
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 57 True Random Number Generator
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 58 Extensible DMA Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                      // 59 Image Sensor Interface
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 60 Pulse Width Modulation Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                  // 61 ARM FPU
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 62 SDRAM Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 63 Reinforced Safety Watchdog Timer
    _ARMV7M_NVIC_Interrupt_dispatch,                  // 64 ARM Cache ECC Warning
    _ARMV7M_NVIC_Interrupt_dispatch,                  // 65 ARM Cache ECC Fault
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 66 GMAC Queue 1
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 67 GMAC Queue 2
    _ARMV7M_NVIC_Interrupt_dispatch,                  // 68 FPU Interrupt IXC associated with cumulative exception
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 69 Inter-IC Sound Controller 0
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 70 Inter-IC Sound Controller 1
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 71 GMAC Queue 3
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 72 GMAC Queue 4
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 73 GMAC Queue 5
#elif defined(N7S_TARGET_SAMRH71F20)
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 13 FLEXCOM 2
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 14 FLEXCOM 3
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 15 FLEXCOM 4
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 16 Parallel Input/Output Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 17 Parallel Input/Output Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                  // 18 ARM Cache ECC Warning
    _ARMV7M_NVIC_Interrupt_dispatch,                  // 19 ARM Cache ECC Fault
    _ARMV7M_NVIC_Interrupt_dispatch,                  // 20 FPU except IXC
    _ARMV7M_NVIC_Interrupt_dispatch,                  // 21 FPU Interrupt IXC associated with cumulative exception
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 22 FLEXCOM 5
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 23 FLEXCOM 6
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 24 FLEXCOM 7
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 25 Timer Counter Channel 0
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 26 Timer Counter Channel 1
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 27 Timer Counter Channel 2
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 28 Timer Counter Channel 3
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 29 Timer Counter Channel 4
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 30 Timer Counter Channel 5
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 31 Pulse Width Modulation 0
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 32 Pulse Width Modulation 1
    _ARMV7M_NVIC_Interrupt_dispatch,                      // 33 Integrity Check Monitor
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 34 Parallel I/O Controller F
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 35 Parallel I/O Controller G
    _ARMV7M_NVIC_Interrupt_dispatch,               // 36 MCAN 0 IRQ 0
    _ARMV7M_NVIC_Interrupt_dispatch,               // 37 MCAN 0 IRQ 1
    _ARMV7M_NVIC_Interrupt_dispatch,               // 38 MCAN 1 IRQ 0
    _ARMV7M_NVIC_Interrupt_dispatch,               // 39 MCAN 1 IRQ 1
    _ARMV7M_NVIC_Interrupt_dispatch,               // 40 TCM RAM - HECC Fixable error detected
    _ARMV7M_NVIC_Interrupt_dispatch,             // 41 TCM RAM - HECC Un-Fixable error detected
    _ARMV7M_NVIC_Interrupt_dispatch,              // 42 FlexRAM - HECC Fixable error detected
    _ARMV7M_NVIC_Interrupt_dispatch,            // 43 FlexRAM - HECC Un-Fixable error detected
    _ARMV7M_NVIC_Interrupt_dispatch,                      // 44 Secure Hash Algorithm
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 45 FLEXCOM 8
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 46 FLEXCOM 9
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 47 Watchdog Timer 1
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 48 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 49 Quad I/O Serial Peripheral Interface
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 50 Hardened Embedded Flash Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 51 Hardened Embedded Flash Controller Fixable error
    _ARMV7M_NVIC_Interrupt_dispatch,               // 52 Hardened Embedded Flash Controller Un-Fixable error
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 53 Timer Counter Channel 6
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 54 Timer Counter Channel 7
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 55 Timer Counter Channel 8
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 56 Timer Counter Channel 9
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 57 Timer Counter Channel 10
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 58 Timer Counter Channel 11
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 59 HEMC-SDRAM Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 60 HEMC-HECC Fixable Error detected
    _ARMV7M_NVIC_Interrupt_dispatch,               // 61 HEMC-HECC Unfixable Error detected
    _ARMV7M_NVIC_Interrupt_dispatch,                      // 62 Special Function Register
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 63 True Random Generator
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 64 eXtended DMA Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 65 Space Wire
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 66 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 67 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 68 MIL 1553
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 69 Ethernet MAC
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 70 GMAC Priority Queue 1
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 71 GMAC Priority Queue 2
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 72 GMAC Priority Queue 3
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 73 GMAC Priority Queue 4
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 74 GMAC Priority Queue 5
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 75 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 76 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 77 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 78 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 79 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 80 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 81 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 82 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 83 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 84 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 85 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 86 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 87 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 88 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 89 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 90 Reserved
#elif defined(N7S_TARGET_SAMRH707F18)
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 13 FLEXCOM 2
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 14 FLEXCOM 3
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 15 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 16 Parallel I/O Controller D
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 17 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                  // 18 ARM Cache ECC Warning
    _ARMV7M_NVIC_Interrupt_dispatch,                  // 19 ARM Cache ECC Fault
    _ARMV7M_NVIC_Interrupt_dispatch,                  // 20 Floating Point Unit except IXC
    _ARMV7M_NVIC_Interrupt_dispatch,                  // 21 Floating Point Unit Interrupt IXC
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 22 CRCCU_HCBDMA
    _ARMV7M_NVIC_Interrupt_dispatch,                      // 23 ADC controller
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 24 DAC Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 25 Timer Counter Channel 0
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 26 Timer Counter Channel 1
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 27 Timer Counter Channel 2
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 28 Timer Counter Channel 3
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 29 Timer Counter Channel 4
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 30 Timer Counter Channel 5
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 31 Pulse Width Modulation 0
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 32 Pulse Width Modulation 1
    _ARMV7M_NVIC_Interrupt_dispatch,                      // 33 Integrity Check Monitor
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 34 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 35 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,               // 36 MCAN Controller 0
    _ARMV7M_NVIC_Interrupt_dispatch,               // 37 MCAN 0 IRQ 1
    _ARMV7M_NVIC_Interrupt_dispatch,               // 38 MCAN Controller 1
    _ARMV7M_NVIC_Interrupt_dispatch,               // 39 MCAN 1 IRQ 1
    _ARMV7M_NVIC_Interrupt_dispatch,               // 40 TCM RAM - HECC Fixable error detected
    _ARMV7M_NVIC_Interrupt_dispatch,             // 41 TCM RAM - HECC Un-Fixable error detected
    _ARMV7M_NVIC_Interrupt_dispatch,              // 42 FlexRAM - HECC Fixable error detected
    _ARMV7M_NVIC_Interrupt_dispatch,            // 43 FlexRAM - HECC Un-Fixable error detected
    _ARMV7M_NVIC_Interrupt_dispatch,                      // 44 Secure Hash Algorithm
    _ARMV7M_NVIC_Interrupt_dispatch,                      // 45 PCC controller
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 46 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 47 Watchdog Timer 1
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 48 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 49 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 50 HEFC INT0
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 51 HEFC Fixable Error
    _ARMV7M_NVIC_Interrupt_dispatch,               // 52 HEFC Unfixable Error
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 53 Timer Counter Channel 6
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 54 Timer Counter Channel 7
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 55 Timer Counter Channel 8
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 56 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 57 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 58 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 59 Hardened Embedded Flash Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 60 Hardened Embedded Flash Controller Fixable error
    _ARMV7M_NVIC_Interrupt_dispatch,               // 61 Hardened Embedded Flash Controller Un-Fixable error
    _ARMV7M_NVIC_Interrupt_dispatch,                      // 62 Special Function Register
    _ARMV7M_NVIC_Interrupt_dispatch,                     // 63 True Random Generator
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 64 eXtended DMA Controller
    _ARMV7M_NVIC_Interrupt_dispatch,                 // 65 Space Wire
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 66 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 67 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                   // 68 MIL 1553
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 69 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 70 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 71 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 72 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 73 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 74 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 75 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 76 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 77 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 78 Reserved
    _ARMV7M_NVIC_Interrupt_dispatch,                    // 79 Reserved
#endif
  },
};