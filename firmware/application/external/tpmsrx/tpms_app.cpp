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
// ==== Renault TPMS helpers (0435R) =====================================

static inline uint8_t crc8_poly07(const uint8_t* d, size_t n) {
    uint8_t c = 0x00;
    for (size_t i = 0; i < n; i++) {
        c ^= d[i];
        for (int b = 0; b < 8; b++) c = (c & 0x80) ? uint8_t((c<<1) ^ 0x07) : uint8_t(c<<1);
    }
    return c;
}

// Jeśli masz bity Manchester – zamień je na bajty.
// Jeśli z packetu masz już gotowe bajty, tej funkcji nie używaj.
static std::vector<uint8_t> manchester_to_bytes(const std::vector<uint8_t>& bits) {
    std::vector<uint8_t> data_bits;
    data_bits.reserve(bits.size() / 2);
    for (size_t i = 0; i + 1 < bits.size(); i += 2) {
        const uint8_t a = bits[i], b = bits[i + 1];
        if (a == 1 && b == 0) data_bits.push_back(1);   // 10 -> 1
        else if (a == 0 && b == 1) data_bits.push_back(0); // 01 -> 0
    }
    std::vector<uint8_t> out((data_bits.size() + 7) / 8, 0);
    for (size_t i = 0; i < data_bits.size(); ++i)
        out[i >> 3] |= (data_bits[i] & 1) << (7 - (i & 7));
    return out;
}

struct RenaultFrame { uint32_t id; float pressure_kpa; int temp_c; };

static bool decode_renault_payload(const std::vector<uint8_t>& d, RenaultFrame& out) {
    if (d.size() != 10 && d.size() != 9) return false;      // 9 danych + CRC
    if (d.size() == 10 && crc8_poly07(d.data(), 9) != d[9]) return false;

    // Mapowanie wg rtl_433: P=10 bitów (0.75 kPa/LSB), T=byte-30, ID=3 bajty
    const uint16_t p_raw = (uint16_t(d[1] & 0x03) << 8) | d[0];
    const float pressure_kpa = p_raw * 0.75f;
    const int   temp_c = int(d[2]) - 30;
    const uint32_t id = (uint32_t(d[4]) << 16) | (uint32_t(d[5]) << 8) | uint32_t(d[6]);

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

    const auto reading_opt = packet.reading();
    if (reading_opt.is_valid()) {
        const auto reading = reading_opt.value();
        auto& entry = ::on_packet(recent, TPMSRecentEntry::Key{reading.type(), reading.id()});
        entry.update(reading);
        recent_entries_view.set_dirty();
    }

    if (pmem::beep_on_packets()) {
        baseband::request_audio_beep(1000, 24000, 60);
    }
}

void TPMSAppView::on_show_list() {
    recent_entries_view.hidden(false);
    recent_entries_view.focus();
}

}  // namespace ui::external_app::tpmsrx

namespace ui {

template <>
void RecentEntriesTable<ui::external_app::tpmsrx::TPMSRecentEntries>::draw(
    const Entry& entry,
    const Rect& target_rect,
    Painter& painter,
    const Style& style) {
    std::string line = ui::external_app::tpmsrx::format::type(entry.type) + " " + ui::external_app::tpmsrx::format::id(entry.id);

    if (entry.last_pressure.is_valid()) {
        line += "  " + ui::external_app::tpmsrx::format::pressure(entry.last_pressure.value());
    } else {
        line +=
            "  "
            "   ";
    }

    if (entry.last_temperature.is_valid()) {
        line += "  " + ui::external_app::tpmsrx::format::temperature(entry.last_temperature.value());
    } else {
        line +=
            "  "
            "   ";
    }

    if (entry.received_count > 999) {
        line += " +++";
    } else {
        line += " " + to_string_dec_uint(entry.received_count, 3);
    }

    if (entry.last_flags.is_valid()) {
        line += " " + ui::external_app::tpmsrx::format::flags(entry.last_flags.value());
    } else {
        line +=
            " "
            "  ";
    }

    line.resize(target_rect.width() / 8, ' ');
    painter.draw_string(target_rect.location(), style, line);
}

}  // namespace ui
