# (C) PortaPack / Mayhem — application CMake (USB/DFU disabled build)

enable_language(C CXX ASM)
include(CheckCXXCompilerFlag)
project(application)

# ---- toolchain & opts -------------------------------------------------
set(USE_OPT "-Os -g --specs=nano.specs -specs=nosys.specs")
set(USE_COPT "-std=gnu99")
check_cxx_compiler_flag("-std=c++20" cpp20_supported)
if(cpp20_supported)
  set(USE_CPPOPT "-std=c++20")
else()
  set(USE_CPPOPT "-std=c++17")
endif()
set(USE_CPPOPT "${USE_CPPOPT} -fno-rtti -fno-exceptions -Weffc++ -Wuninitialized -Wno-volatile")
set(USE_LINK_GC yes)
set(USE_LDOPT)
set(USE_LTO no)
set(USE_THUMB yes)
set(USE_VERBOSE_COMPILE no)
set(USE_FPU no)

# ---- imported ChibiOS/board stuff ------------------------------------
include(${CHIBIOS_PORTAPACK}/boards/PORTAPACK_APPLICATION/board.cmake)
include(${CHIBIOS_PORTAPACK}/os/hal/platforms/LPC43xx_M0/platform.cmake)
include(${CHIBIOS}/os/hal/hal.cmake)
include(${CHIBIOS_PORTAPACK}/os/ports/GCC/ARMCMx/LPC43xx_M0/port.cmake)
include(${CHIBIOS}/os/kernel/kernel.cmake)
include(${CHIBIOS_PORTAPACK}/os/various/fatfs_bindings/fatfs.cmake)
include(${CHIBIOS}/test/test.cmake)
include(external/external.cmake)

# ---- CPLD data --------------------------------------------------------
set(CPLD_20150901_SVF_PATH ${HARDWARE_PATH}/portapack_h1/cpld/20150901/output_files/portapack_h1_cpld.svf)
set(CPLD_20150901_DATA_CPP ${CMAKE_CURRENT_BINARY_DIR}/portapack_cpld_20150901_data.cpp)
set(CPLD_20170522_SVF_PATH ${HARDWARE_PATH}/portapack_h1/cpld/20170522/output_files/portapack_h1_cpld.svf)
set(CPLD_20170522_DATA_CPP ${CMAKE_CURRENT_BINARY_DIR}/portapack_cpld_20170522_data.cpp)
set(HACKRF_CPLD_DATA_HPP ${CMAKE_CURRENT_BINARY_DIR}/hackrf_cpld_data.hpp)
set(HACKRF_CPLD_DATA_CPP ${CMAKE_CURRENT_BINARY_DIR}/hackrf_cpld_data.cpp)

# ---- linker script ----------------------------------------------------
set(LDSCRIPT ${PORTLD}/LPC43xx_M0.ld)

# ---- C sources (USB removed) -----------------------------------------
set(CSRC
  ${PORTSRC}
  ${KERNSRC}
  ${TESTSRC}
  ${HALSRC}
  ${PLATFORMSRC}
  ${BOARDSRC}
  ${FATFSSRC}
  firmware_info.c
  i2c_device_to_host.c
  ${CHIBIOS}/os/various/chprintf.c
  # USB serial .c files removed:
  # usb_serial_cdc.c
  # usb_serial_descriptor.c
  # usb_serial_endpoints.c
  # usb_serial_device_to_host.c
  # HackRF USB glue removed:
  # ${HACKRF_PATH}/firmware/hackrf_usb/usb_device.c
  # ${HACKRF_PATH}/firmware/common/usb_request.c
  # ${HACKRF_PATH}/firmware/common/usb_standard_request.c
)

# ---- auto-collect i2cdev_* -------------------------------------------
file(GLOB I2CDEV_SOURCES ${COMMON}/i2cdev_*.cpp)

# ---- C++ sources (USB/DFU screens removed) ---------------------------
set(CPPSRC
  main.cpp
  shell.cpp
  ${COMMON}/adsb.cpp
  ${COMMON}/adsb_frame.cpp
  ${COMMON}/ais_baseband.cpp
  ${COMMON}/ais_packet.cpp
  ${COMMON}/ak4951.cpp
  ${COMMON}/backlight.cpp
  ${COMMON}/baseband_cpld.cpp
  ${COMMON}/bch_code.cpp
  ${COMMON}/buffer.cpp
  ${COMMON}/buffer_exchange.cpp
  ${COMMON}/chibios_cpp.cpp
  ${COMMON}/cpld_max5.cpp
  ${COMMON}/cpld_update.cpp
  ${COMMON}/cpld_xilinx.cpp
  debug.cpp
  ${COMMON}/ert_packet.cpp
  ${COMMON}/event.cpp
  ${COMMON}/gcc.cpp
  ${COMMON}/hackrf_hal.cpp
  ${COMMON}/i2c_pp.cpp
  ${COMMON}/jtag.cpp
  ${COMMON}/jtag_tap.cpp
  ${COMMON}/lcd_ili9341.cpp
  ${COMMON}/lfsr_random.cpp
  ${COMMON}/manchester.cpp
  ${COMMON}/message_queue.cpp
  ${COMMON}/morse.cpp
  ${COMMON}/png_writer.cpp
  ${COMMON}/pocsag.cpp
  ${COMMON}/pocsag_packet.cpp
  ${COMMON}/aprs_packet.cpp
  ${COMMON}/portapack_io.cpp
  ${COMMON}/portapack_persistent_memory.cpp
  ${COMMON}/portapack_shared_memory.cpp
  ${COMMON}/sonde_packet.cpp
  ${COMMON}/tpms_packet.cpp
  ${COMMON}/ui.cpp
  ${COMMON}/ui_focus.cpp
  ${COMMON}/ui_painter.cpp
  ${COMMON}/ui_text.cpp
  ${COMMON}/ui_widget.cpp
  ${COMMON}/ui_language.cpp
  ${COMMON}/utility.cpp
  ${COMMON}/wm8731.cpp
  ${COMMON}/i2cdevmanager.cpp
  ${I2CDEV_SOURCES}
  ${COMMON}/battery.cpp
  ${COMMON}/performance_counter.cpp
  ${COMMON}/bmpfile.cpp
  app_settings.cpp
  audio.cpp
  baseband_api.cpp
  capture_thread.cpp
  clock_manager.cpp
  core_control.cpp
  database.cpp
  gradient.cpp
  rfm69.cpp
  event_m0.cpp
  file_reader.cpp
  file.cpp
  file_path.cpp
  freqman_db.cpp
  freqman.cpp
  io_convert.cpp
  io_file.cpp
  io_wave.cpp
  iq_trim.cpp
  irq_controls.cpp
  irq_lcd_frame.cpp
  irq_rtc.cpp
  log_file.cpp
  metadata_file.cpp
  flipper_subfile.cpp
  portapack.cpp
  # --- USB files removed:
  # usb_serial_shell.cpp
  # usb_serial_shell_filesystem.cpp
  # usb_serial_event.cpp
  # usb_serial_thread.cpp
  # usb_serial.cpp
  # usb_serial_host_to_device.cpp
  # usb_serial_asyncmsg.cpp
  qrcodegen.cpp
  radio.cpp
  receiver_model.cpp
  recent_entries.cpp
  replay_thread.cpp
  rf_path.cpp
  rtc_time.cpp
  sd_card.cpp
  serializer.cpp
  spectrum_color_lut.cpp
  string_format.cpp
  temperature_logger.cpp
  theme.cpp
  touch.cpp
  tone_key.cpp
  transmitter_model.cpp
  tuning.cpp
  hw/debounce.cpp
  hw/encoder.cpp
  hw/max2837.cpp
  hw/max2839.cpp
  hw/max5864.cpp
  hw/rffc507x.cpp
  hw/rffc507x_spi.cpp
  hw/si5351.cpp
  hw/spi_pp.cpp
  hw/touch_adc.cpp
  ook_file.cpp
  ui_baseband_stats_view.cpp
  ui_navigation.cpp
  ui_record_view.cpp
  ui_sd_card_status_view.cpp
  ui/ui_alphanum.cpp
  ui/ui_audio.cpp
  ui/ui_channel.cpp
  ui/ui_font_fixed_5x8.cpp
  ui/ui_font_fixed_8x16.cpp
  ui/ui_geomap.cpp
  ui/ui_qrcode.cpp
  ui/ui_menu.cpp
  ui/ui_btngrid.cpp
  ui/ui_receiver.cpp
  ui/ui_rssi.cpp
  ui/ui_freqlist.cpp
  ui/ui_tv.cpp
  ui/ui_spectrum.cpp
  ui/ui_tabview.cpp
  ui/ui_textentry.cpp
  ui/ui_tone_key.cpp
  ui/ui_transmitter.cpp
  ui/ui_bmpview.cpp
  apps/ais_app.cpp
  apps/analog_audio_app.cpp
  apps/ble_rx_app.cpp
  apps/ble_tx_app.cpp
  apps/capture_app.cpp
  apps/pocsag_app.cpp
  apps/ui_about_simple.cpp
  apps/ui_adsb_rx.cpp
  apps/ui_aprs_rx.cpp
  apps/ui_aprs_tx.cpp
  apps/ui_battinfo.cpp
  apps/ui_bht_tx.cpp
  apps/ui_bmp_file_viewer.cpp
  apps/ui_btle_rx.cpp
  apps/ui_debug.cpp
  apps/ui_debug_max17055.cpp
  # apps/ui_dfu_menu.cpp      # (DFU wyłączone – stub zapewnia symbole)
  apps/ui_encoders.cpp
  apps/ui_external_module_view.cpp
  apps/ui_fileman.cpp
  apps/ui_flash_utility.cpp
  apps/ui_freqman.cpp
  apps/ui_iq_trim.cpp
  apps/ui_looking_glass_app.cpp
  apps/ui_mictx.cpp
  apps/ui_modemsetup.cpp
  apps/ui_playlist.cpp
  apps/ui_pocsag_tx.cpp
  apps/ui_rds.cpp
  apps/ui_recon_settings.cpp
  apps/ui_recon.cpp
  # apps/ui_sd_over_usb.cpp   # (Mass-storage po USB wyłączone – stub zapewnia symbole)
  apps/ui_search.cpp
  apps/ui_settings.cpp
  apps/ui_siggen.cpp
  apps/ui_sonde.cpp
  apps/ui_ss_viewer.cpp
  apps/ui_standalone_view.cpp
  apps/ui_text_editor.cpp
  apps/ui_touch_calibration.cpp
  apps/ui_touchtunes.cpp
  apps/ui_weatherstation.cpp
  protocols/aprs.cpp
  protocols/ax25.cpp
  protocols/bht.cpp
  protocols/dcs.cpp
  protocols/encoders.cpp
  protocols/modems.cpp
  protocols/rds.cpp
  ui_sd_card_debug.cpp
  config_mode.cpp
  ${CPLD_20150901_DATA_CPP}
  ${CPLD_20170522_DATA_CPP}
  ${HACKRF_CPLD_DATA_CPP}
  ui_external_items_menu_loader.cpp
  view_factory_base.cpp

  # --- NOWE: stuby wyłączające USB/DFU (koniecznie dodaj ten plik do repo)
  usb_disabled_stubs.cpp
)

# Dodaj LTO do wewnętrznych (nie-external) źródeł
set_source_files_properties(${CPPSRC} PROPERTIES COMPILE_FLAGS -flto)
# Dołóż moduły external po ustawieniu flag
list(APPEND CPPSRC ${EXTCPPSRC})

# ASM
set(ASMSRC ${PORTASM} lz4.S)

# ---- include dirs (bez libopencm3) ------------------------------------
set(INCDIR
  ${CMAKE_CURRENT_BINARY_DIR}
  ${COMMON}
  ${PORTINC} ${KERNINC} ${TESTINC}
  ${HALINC} ${PLATFORMINC} ${BOARDINC}
  ${FATFSINC}
  ${CHIBIOS}/os/various
  ui
  hw
  apps
  protocols
  bmps
  # Usunięto:
  # ${HACKRF_PATH}/firmware/libopencm3/include
  # ${HACKRF_PATH}/firmware/common
  # ${HACKRF_PATH}/firmware
)

# ---- compiler settings ------------------------------------------------
set(MCU cortex-m0)
set(AOPT)
set(TOPT "-mthumb -DTHUMB")
set(CWARN "-Wall -Wextra -Wstrict-prototypes")
set(CPPWARN "-Wall -Wextra -Wno-psabi")

# ---- defines ----------------------------------------------------------
set(DDEFS "-DLPC43XX -DLPC43XX_M0 -D__NEWLIB__ -DHACKRF_ONE -DTOOLCHAIN_GCC -DTOOLCHAIN_GCC_ARM -D_RANDOM_TCC=0 -D'VERSION_STRING=\"${VERSION}\"' -DVERSION_MD5=${VERSION_MD5} -D'GCC_VERSION_MISMATCH=${GCC_VERSION_MISMATCH}'")
set(DADEFS)
set(DINCDIR)
set(DLIBDIR)
set(DLIBS)
set(UDEFS)
set(UADEFS)
set(UINCDIR)
set(ULIBDIR)
set(ULIBS)

# ---- rules & generators -----------------------------------------------
set(RULESPATH ${CHIBIOS}/os/ports/GCC/ARMCMx)
include(${RULESPATH}/rules.cmake)

add_custom_command(
  OUTPUT ${CPLD_20150901_DATA_CPP}
  COMMAND ${EXTRACT_CPLD_DATA} ${CPLD_20150901_SVF_PATH} rev_20150901 >${CPLD_20150901_DATA_CPP}
  DEPENDS ${EXTRACT_CPLD_DATA} ${CPLD_20150901_SVF_PATH}
)

add_custom_command(
  OUTPUT ${CPLD_20170522_DATA_CPP}
  COMMAND ${EXTRACT_CPLD_DATA} ${CPLD_20170522_SVF_PATH} rev_20170522 >${CPLD_20170522_DATA_CPP}
  DEPENDS ${EXTRACT_CPLD_DATA} ${CPLD_20170522_SVF_PATH}
)

add_custom_command(
  OUTPUT ${HACKRF_CPLD_DATA_CPP}
  COMMAND ${HACKRF_CPLD_TOOL} --xsvf ${HACKRF_CPLD_XSVF_PATH} --portapack-data ${HACKRF_CPLD_DATA_CPP}
  DEPENDS ${HACKRF_CPLD_TOOL} ${HACKRF_CPLD_XSVF_PATH}
)

# ---- targets ----------------------------------------------------------
add_executable(${PROJECT_NAME}.elf ${CSRC} ${CPPSRC} ${ASMSRC})
set_target_properties(${PROJECT_NAME}.elf PROPERTIES LINK_DEPENDS ${LDSCRIPT})
add_definitions(${DEFS})
include_directories(. ${INCDIR})
link_directories(${LLIBDIR})
target_link_libraries(${PROJECT_NAME}.elf ${LIBS} "-L${CMAKE_CURRENT_LIST_DIR}/external")
target_link_libraries(${PROJECT_NAME}.elf -Wl,-Map=${PROJECT_NAME}.map)
target_link_libraries(${PROJECT_NAME}.elf -Wl,--print-memory-usage)

add_custom_command(
  OUTPUT ${PROJECT_NAME}.bin
  COMMAND ${CMAKE_OBJCOPY} -v -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin --remove-section=.external_app_*
  COMMAND ${EXPORT_EXTERNAL_APP_IMAGES} ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_OBJCOPY} ${EXTAPPLIST}
  DEPENDS ${PROJECT_NAME}.elf
)

add_custom_target(${PROJECT_NAME} DEPENDS ${PROJECT_NAME}.bin)
