/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/EnumBits.h>
#include <AK/Noncopyable.h>
#include <AK/Vector.h>
#include <LibVT/Color.h>
#include <LibVT/XtermColors.h>

namespace VT {

struct Attribute {
    static constexpr Color default_foreground_color = Color::named(Color::ANSIColor::DefaultForeground);
    static constexpr Color default_background_color = Color::named(Color::ANSIColor::DefaultBackground);

    void reset_style()
    {
        foreground_color = default_foreground_color;
        background_color = default_background_color;
        flags = Flags::NoAttributes;
    }

    Color foreground_color { default_foreground_color };
    Color background_color { default_background_color };

    // Index into Terminal::m_hyperlinks (0 = no hyperlink). Keeping the strings
    // out of every cell is what makes Cell small: a 1024-line, 160-column
    // scrollback no longer pays for two empty ByteStrings per cell.
    u32 hyperlink { 0 };

    void reset_hyperlink() { hyperlink = 0; }
    bool has_hyperlink() const { return hyperlink != 0; }

    enum class Flags : u8 {
        NoAttributes = 0x00,
        Bold = 0x01,
        Italic = 0x02,
        Underline = 0x04,
        Negative = 0x08,
        Blink = 0x10,
        Touched = 0x20,
        Concealed = 0x40,
    };
    AK_ENUM_BITWISE_FRIEND_OPERATORS(Flags);

    constexpr Color effective_background_color() const { return has_flag(flags, Flags::Negative) ? foreground_color : background_color; }
    constexpr Color effective_foreground_color() const { return has_flag(flags, Flags::Negative) ? background_color : foreground_color; }

    constexpr bool is_untouched() const { return !has_flag(flags, Flags::Touched); }

    Flags flags { Flags::NoAttributes };

    constexpr bool operator==(Attribute const& other) const
    {
        return foreground_color == other.foreground_color
            && background_color == other.background_color
            && flags == other.flags
            && hyperlink == other.hyperlink;
    }
};

}
