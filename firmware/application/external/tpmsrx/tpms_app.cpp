/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Mark Thompson
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "tpms_app.hpp"

#include "baseband_api.hpp"
#include "audio.hpp"
#include "portapack.hpp"
using namespace portapack;

#include "string_format.hpp"

#include "utility.hpp"
#include "file_path.hpp"

#include <cctype>
#include <string>
#include <vector>

// ==== Renault TPMS helpers (0435R) =====================================

static inline uint8_t crc8_poly07(const uint8_t* d, size_t n) {
    uint8_t c = 0x00;
    for (size_t i = 0; i < n; i++) {
        c ^= d[i];
        for (int b = 0; b < 8; b++) c = (c & 0x80) ? uint8_t((c<<1) ^ 0x07) : uint8_t(c<<1);
    }
    return c;
}

// Prosty parser hex -> bajty (ignoruje spacje/znaki nie-hex)
static std::vector<uint8_t> bytes_from_hex(const std::string& s) {
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        c = (char)std::tolower((unsigned char)c);
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        return -1;
    };
    std::vector<uint8_t> out;
    int hi = -1;
    for (char c : s) {
        const int n = nibble(c);
        if (n < 0) continue;
        if (hi < 0) hi = n;
        else { out.push_back(uint8_t((hi << 4) | n)); hi = -1; }
    }
    return out;
}

struct RenaultFrame { uint32_t id; float pressure_kpa; int temp_c; };

static bool decode_renault_payload(const std::vector<uint8_t>& d, RenaultFrame& out) {
    // Oczekujemy 9 bajtów danych + CRC (10 bajtów łącznie) lub samych 9 danych.
    if (d.size() != 10 && d.size() != 9) return false;
    if (d.size() == 10 && crc8_poly07(d.data(), 9) != d[9]) return false;

    // Mapowanie wg rtl_433: P=10 bitów (0.75 kPa/LSB), T=byte-30, ID=3 bajty
    const uint16_t p_raw = (uint16_t(d[1] & 0x03) << 8) | d[0];
    const float pressure_kpa = p_raw * 0.75f;
    const int   temp_c = int(d[2]) - 30;
    const uint32_t id = (uint32_t(d[4]) << 16) | (uint32_t(d[5]) << 8) | uint32_t(d[6]);

    // Sanity-check
    if (pressure_kpa < 50.0f || pressure_kpa > 500.0f) return false; // 0.5–5.0 bar
    if (temp_c < -40 || temp_c > 125) return false;

    out = RenaultFrame{ id, pressure_kpa, temp_c };
    return true;
}

// ==== /Renault helpers =================================================


namespace pmem = portapack::persistent_memory;

namespace ui::external_app::tpmsrx {

namespace format {

std::string type(tpms::Reading::Type type) {
    return to_string_dec_uint(toUType(type), 2);
}

std::string id(tpms::TransponderID id) {
    return to_string_hex(id.value(), 8);
}

std::string pressure(Pressure pressure) {
    return to_string_dec_int(units_psi ? pressure.psi() : pressure.kilopascal(), 3);
}

std::string temperature(Temperature temperature) {
    return to_string_dec_int(units_fahr ? temperature.fahrenheit() : temperature.celsius(), 3);
}

std::string flags(tpms::Flags flags) {
    return to_string_hex(flags, 2);
}

static std::string signal_type(tpms::SignalType signal_type) {
    switch (signal_type) {
        case tpms::SignalType::FSK_19k2_Schrader:
            return "FSK 38400 19200 Schrader";
        case tpms::SignalType::OOK_8k192_Schrader:
            return "OOK - 8192 Schrader";
        case tpms::SignalType::OOK_8k4_Schrader:
            return "OOK - 8400 Schrader";
        default:
            return "- - - -";
    }
}

} /* namespace format */

void TPMSLogger::on_packet(const tpms::Packet& packet, const uint32_t target_frequency) {
    const auto hex_formatted = packet.symbols_formatted();

    // TODO: function doesn't take uint64_t, so when >= 1<<32, weirdness will ensue!
    const auto target_frequency_str = to_string_dec_uint(target_frequency, 10);

    std::string entry = target_frequency_str + " " + ui::external_app::tpmsrx::format::signal_type(packet.signal_type()) + " " + hex_formatted.data + "/" + hex_formatted.errors;
    log_file.write_entry(packet.received_at(), entry);
}

const TPMSRecentEntry::Key TPMSRecentEntry::invalid_key = {tpms::Reading::Type::None, 0};

void TPMSRecentEntry::update(const tpms::Reading& reading) {
    received_count++;

    if (reading.pressure().is_valid()) {
        last_pressure = reading.pressure();
    }
    if (reading.temperature().is_valid()) {
        last_temperature = reading.temperature();
    }
    if (reading.flags().is_valid()) {
        last_flags = reading.flags();
    }
}

TPMSAppView::TPMSAppView(NavigationView&) {
    // baseband::run_image(portapack::spi_flash::image_tag_tpms);
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&rssi,
                  &field_volume,
                  &channel,
                  &options_band,
                  &options_pressure,
                  &options_temperature,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &recent_entries_view});

    receiver_model.enable();

    options_band.on_change = [this](size_t, OptionsField::value_t v) {
        receiver_model.set_target_frequency(v);
    };
    options_band.set_by_value(receiver_model.target_frequency());

    options_pressure.on_change = [this](size_t, int32_t i) {
        format::units_psi = (bool)i;
        update_view();
    };
    options_pressure.set_selected_index(format::units_psi, true);

    options_temperature.on_change = [this](size_t, int32_t i) {
        format::units_fahr = (bool)i;
        update_view();
    };
    options_temperature.set_selected_index(format::units_fahr, true);

    logger = std::make_unique<TPMSLogger>();
    if (logger) {
        logger->append(logs_dir / u"TPMS.TXT");
    }

    if (pmem::beep_on_packets()) {
        audio::set_rate(audio::Rate::Hz_24000);
        audio::output::start();
    }
}

TPMSAppView::~TPMSAppView() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

void TPMSAppView::focus() {
    options_band.focus();
}

void TPMSAppView::update_view() {
    recent_entries_view.set_parent_rect(view_normal_rect);
}

void TPMSAppView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);

    view_normal_rect = {0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height};

    update_view();
}

void TPMSAppView::on_packet(const tpms::Packet& packet) {
    if (logger) {
        logger->on_packet(packet, receiver_model.target_frequency());
    }

    // --- [Renault] spróbuj dekodować ramkę Renault 0435R z surowych symboli ---
    // Pobierz symbole w heksie (jak zapisuje logger) i przerób na bajty
    const auto hex_formatted = packet.symbols_formatted(); // ma .data i .errors
    std::vector<uint8_t> ren_bytes = bytes_from_hex(hex_formatted.data);

    RenaultFrame rf;
    if (!ren_bytes.empty() && decode_renault_payload(ren_bytes, rf)) {
        // Dodaj/zaktualizuj wpis Recent z kluczem (typ=None, ID=rf.id)
        auto& entry = ::on_packet(recent, TPMSRecentEntry::Key{tpms::Reading::Type::None, rf.id});

        // Oznacz jako Renault (dopisek [R] w tabeli)
        entry.renault = true;
        entry.received_count++;

        // (opcjonalnie można by ustawić last_pressure/last_temperature,
        //  ale typy Pressure/Temperature są specyficzne — zostawiamy minimalnie wymagane)

        recent_entries_view.set_dirty();

        if (pmem::beep_on_packets())
