// firmware/application/usb_disabled_stubs.cpp
// Stuby do builda bez pełnego USB/DFU – dostarczają brakujące symbole.

#include <string>
#include <vector>

#include "usb_serial.hpp"            // portapack::USBSerial
#include "usb_serial_asyncmsg.hpp"   // UsbSerialAsyncmsg

namespace portapack {

// Dokładnie jak w usb_serial.hpp:
void USBSerial::initialize() {}
void USBSerial::dispatch() {}
void USBSerial::dispatch_transfer() {}

}  // namespace portapack

// ----------------------------------------------------------------------
// Wątek USB – zapewnij symbol destruktora (jest odwołanie w kodzie).
class UsbSerialThread {
public:
    ~UsbSerialThread();
};
UsbSerialThread::~UsbSerialThread() {}

// ----------------------------------------------------------------------
// Asynchroniczne wiadomości – zdefiniuj szablony *i* jawne instancje
// dla typów używanych w logach (std::string, vector<uint8_t> itp.).

template <typename STRINGCOVER>
void UsbSerialAsyncmsg::asyncmsg(const STRINGCOVER&) {}

template <typename VECTORCOVER>
void UsbSerialAsyncmsg::asyncmsg(const std::vector<VECTORCOVER>&) {}

void UsbSerialAsyncmsg::asyncmsg(const char*) {}

// Jawne instancjonowania – rozszerz w razie potrzeby gdyby link wołał o kolejne typy.
template void UsbSerialAsyncmsg::asyncmsg<std::string>(const std::string&);
template void UsbSerialAsyncmsg::asyncmsg<unsigned char>(const std::vector<unsigned char>&);
template void UsbSerialAsyncmsg::asyncmsg<uint8_t>(const std::vector<uint8_t>&);

// ----------------------------------------------------------------------
// i2c shell glue – sygnatura zgodna z i2cdev_ppmod.cpp.
struct EventDispatcher;
void create_shell_i2c(EventDispatcher* /*evtd*/) {}

// ----------------------------------------------------------------------
// Minimalne definicje konstruktorów/destruktorów dla widoków, których
// implementacje są pomijane w tym wariancie buildu (DFU, SD over USB, SubGhzD).
// Dzięki temu powstają vtable i znikają "undefined reference".

namespace ui {

class NavigationView;

// DFU menu
class DfuMenu {
public:
    DfuMenu(NavigationView&) {}
    virtual ~DfuMenu() = default;
};
class DfuMenu2 {
public:
    DfuMenu2(NavigationView&) {}
    virtual ~DfuMenu2() = default;
};

// SD over USB (mass storage)
class SdOverUsbView {
public:
    SdOverUsbView(NavigationView&) {}
    virtual ~SdOverUsbView() = default;
};

// SubGhzD
class SubGhzDView {
public:
    SubGhzDView(NavigationView&) {}
    virtual ~SubGhzDView() = default;
};

}  // namespace ui
