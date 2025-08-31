// firmware/application/usb_disabled_stubs.cpp
// Stuby wyłączające stos USB/DFU – zapewniają brakujące symbole do linkowania.

#include <string>

// Dopasuj dokładnie do deklaracji z nagłówków projektowych:
#include "usb_serial.hpp"            // portapack::USBSerial
#include "usb_serial_asyncmsg.hpp"   // UsbSerialAsyncmsg, STRINGCOVER

namespace portapack {

// Odpowiada metodom zadeklarowanym w usb_serial.hpp
void USBSerial::initialize() {}
void USBSerial::dispatch() {}
void USBSerial::dispatch_transfer() {}

}  // namespace portapack

// ----------------------------------------------------------------------
// Wątek USB – zapewnij *out-of-line* dtor, żeby link wyemitował symbol.
class UsbSerialThread {
public:
    ~UsbSerialThread();
};
UsbSerialThread::~UsbSerialThread() {}

// ----------------------------------------------------------------------
// Asynchroniczne wiadomości – w nagłówku to *statyczna* metoda z aliasem
// STRINGCOVER. Definiujemy dokładnie tę sygnaturę, bez szablonów.
void UsbSerialAsyncmsg::asyncmsg(const STRINGCOVER& /*data*/) {}

// ----------------------------------------------------------------------
// i2c shell glue – dopasuj do deklaracji z i2cdev_ppmod.cpp
struct EventDispatcher;
void create_shell_i2c(EventDispatcher* /*evtd*/) {}

// ----------------------------------------------------------------------
// Widoki/ekrany, które wycięliśmy z builda (DFU, mass-storage USB, SubGhzD).
// Definiujemy minimalne konstruktory/dtory, żeby powstały vtable’e.
// (Zachowujemy przestrzeń nazw i nazwy klas z nagłówków projektu.)

namespace ui {

class NavigationView;

// DFU
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

// SubGhzD menu
class SubGhzDView {
public:
    SubGhzDView(NavigationView&) {}
    virtual ~SubGhzDView() = default;
};

}  // namespace ui
