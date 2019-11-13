// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "TTauri/Foundation/Path.hpp"
#include "TTauri/Foundation/PathString.hpp"
#include "TTauri/Foundation/gstring.hpp"
#include "TTauri/Foundation/ResourceView.hpp"
#include "TTauri/Foundation/exceptions.hpp"
#include "TTauri/Foundation/required.hpp"
#include "TTauri/Foundation/URL.hpp"
#include "TTauri/Foundation/GlyphMetrics.hpp"
#include <vector>
#include <map>
#include <gsl/gsl>

namespace TTauri {



class Font {
public:
    Font() = default;
    virtual ~Font() = default;
    Font(Font const &) = delete;
    Font &operator=(Font const &) = delete;
    Font(Font &&) = delete;
    Font &operator=(Font &&) = delete;

    /*! Find a glyph in the font based on an Unicode code-point.
    * This is separated from loading a glyph so that graphemes and ligatures can be found.
    *
    * \param c Unicode code point to look up.
    * \return a glyph-index if a glyph has been found. 0 means "not found", -1 means "parse error".
    */
    virtual int searchCharacterMap(char32_t c) const noexcept = 0;

    /*! Load a glyph into a path.
    * The glyph is directly loaded from the font file.
    * 
    * \param glyphIndex the index of a glyph inside the font.
    * \param path The path constructed by the loader.
    * \return true on success, false on error.
    */
    virtual bool loadGlyph(int glyphIndex, Path &path) const noexcept = 0;

    /*! Load a glyph into a path.
    * The glyph is directly loaded from the font file.
    * 
    * \param glyphIndex the index of a glyph inside the font.
    * \param metrics The metrics constructed by the loader.
    * \return true on success, false on error.
    */
    virtual bool loadGlyphMetrics(int glyphIndex, GlyphMetrics &metrics) const noexcept = 0;

    PathString getGlyphs(gstring const &graphemes) const noexcept {
        PathString r;

        for (int graphemeIndex = 0; graphemeIndex < graphemes.size(); graphemeIndex++) {
            let &grapheme = graphemes.at(graphemeIndex);

            // XXX Try and find ligatures in font.

            // First try composed normalization
            std::vector<Path> graphemeGlyphs;
            for (let codePoint: grapheme.NFC()) {
                let glyphIndex = searchCharacterMap(codePoint);
                if (glyphIndex <= 0) {
                    // The codePoint was not found in the font, or a parse error occurred.
                    graphemeGlyphs.clear();
                    break;
                }

                Path glyph;
                if (!loadGlyph(glyphIndex, glyph)) {
                    // Some kind of parsing error, causes the glyph not to be loaded.
                    graphemeGlyphs.clear();
                    break;
                }

                graphemeGlyphs.push_back(std::move(glyph));
            }

            if (graphemeGlyphs.size() == 0) {
                // Try again with decomposed normalization.
                for (let codePoint: grapheme.NFD()) {
                    let glyphIndex = searchCharacterMap(codePoint);
                    if (glyphIndex <= 0) {
                        graphemeGlyphs.clear();
                        break;
                    }

                    Path glyph;
                    if (!loadGlyph(glyphIndex, glyph)) {
                        // Some kind of parsing error, causes the glyph not to be loaded.
                        graphemeGlyphs.clear();
                        break;
                    }

                    graphemeGlyphs.push_back(std::move(glyph));
                }
            }

            // XXX Try fall back fonts.

            if (graphemeGlyphs.size() == 0) {
                // Replace with not-found-glyph at index 0.
                Path glyph;
                if (!loadGlyph(0, glyph)) {
                    // Some kind of parsing error, causes the glyph not to be loaded.
                    LOG_FATAL("Could not load glyph 0 from font file.");
                }

                graphemeGlyphs.push_back(std::move(glyph));
            }

            // Add graphemeGlyphs.
            for (let glyph: graphemeGlyphs) {
                r.add(std::move(glyph));
            }
        }

        return r;
    }

    PathString getGlyphs(std::string const &s) const noexcept {
        return getGlyphs(translateString<gstring>(s));
    }
};

}

namespace TTauri {

template<>
std::unique_ptr<Font> parseResource(URL const &location);

}