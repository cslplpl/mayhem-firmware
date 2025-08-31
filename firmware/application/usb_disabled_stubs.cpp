// Minimalne stuby, gdy USB/DFU jest wyłączone w buildzie.
#include <string>

// --- USBSerial (statyczne wołania w portapack.cpp / event_m0.cpp)
namespace portapack {
struct USBSerial {
  static void initialize();
  static void dispatch();
  static void dispatch_transfer();
};
void USBSerial::initialize() {}
void USBSerial::dispatch() {}
void USBSerial::dispatch_transfer() {}
} // namespace portapack

// --- Wątki/async msg używane warunkowo przez niektóre widoki
class UsbSerialThread {
public:
  ~UsbSerialThread() {}  // dtor dla linkera
};

class UsbSerialAsyncmsg {
public:
  template <typename T>
  void asyncmsg(const T&) {}  // no-op
};
// jawna instancja używana w BLE
template void UsbSerialAsyncmsg::asyncmsg<std::string>(const std::string&);

// --- create_shell_i2c wywoływane z i2cdev_ppmod.cpp
extern "C" void create_shell_i2c() {}

// --- Widoki, które rejestruje navigation, ale których implementacje wycięliśmy
namespace ui {
class NavigationView;

// Deklaracje są w nagłówkach w innych TU; tu wystarczą out-of-line definicje ctor/dtor:
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
class SdOverUsbView {
public:
  SdOverUsbView(NavigationView&) {}
  virtual ~SdOverUsbView() = default;
};
class SubGhzDView {
public:
  SubGhzDView(NavigationView&) {}
  virtual ~SubGhzDView() = default;
};
} // namespace ui
