// Copyright 2019, 2020 Pokitec
// All rights reserved.

#pragma once

#include "TTauri/Foundation/URL.hpp"
#include "TTauri/Foundation/grapheme.hpp"
#include "TTauri/Foundation/text.hpp"
#include "TTauri/Foundation/attributes.hpp"
#include "TTauri/Foundation/FontDescription.hpp"
#include <limits>
#include <array>
#include <new>


namespace TTauri {





class FontBook {
    /** Table of FontFamilyIDs index using the family-name.
     */
    std::unordered_map<std::string,FontFamilyID> family_name_table;

    /** Same as family_name_table, but will also have resolved font families from the fallback_chain.
     */
    mutable std::unordered_map<std::string,FontFamilyID> family_name_cache;

    /**
     */
    std::unordered_map<std::string,std::string> family_name_fallback_chain;


    /** FontID indexed using FontFamilyID + font_variant.
     */
    std::vector<std::array<FontID,FontVariant::max()>> font_variant_table;

    /** Descriptions of fonts, indexed using FontID.
     */
    std::vector<FontDescription> font_descriptions;

    /** If no matching font is found, lookup glyphs in one of the following fonts.
     */
    std::vector<FontID> last_resort_font_table;

public:
    FontBook(std::vector<URL> const &font_directories);

    /** Register a font.
     * Duplicate registrations will be ignored.
     * 
     * When a font file is registered the file will be temporarily opened to read and cache a set of properties:
     *  - The English Font Family from the 'name' table.
     *  - The weight, width, slant & design-size from the 'fdsc' table.
     *  - The character map 'cmap' table.
     */
    FontID register_font(URL font_file);

    /** Find font family id.
     * This function will always return a valid FontFamilyID by walking the fallback-chain.
     */
    [[nodiscard]] FontFamilyID find_family(std::string_view family_name) const noexcept;

    /** Register font family id.
     * If the family already exists the existing family_id is returned.
     */
    [[nodiscard]] FontFamilyID register_family(std::string_view family_name) noexcept;

    /** Find a font closest to the variant.
     * This function will always return a valid FontID.
     *
     * @param family_id a valid family id.
     * @param variant The variant of the font to select.
     * @return a valid font id.
     */
    [[nodiscard]] FontID find_font(FontFamilyID family_id, FontVariant variant) const noexcept;

    /** Find a font closest to the variant.
    * This function will always return a valid FontID.
    *
    * @param family_id a valid family id.
    * @param weight The weight of the font to select.
    * @param italic If the font to select should be italic or not.
    * @return a valid font id.
    */
    [[nodiscard]] FontID find_font(FontFamilyID family_id, font_weight weight, bool italic) const noexcept;

    /** Find a font closest to the variant.
     * This function will always return a valid FontID.
     *
     * @param family_id a valid family id.
     * @param weight The weight of the font to select.
     * @param italic If the font to select should be italic or not.
     * @return a valid font id.
     */
    [[nodiscard]] FontID find_font(std::string_view family_name, font_weight weight, bool italic) const noexcept;

    [[nodiscard]] Glyph find_glyphs(FontID font_id, char32_t code_point) const noexcept;

    /** Find a glyph using the given code-point.
     * This function will find a glyph for a font that looks closest to the given style.
     *
     * @param style The font style to use for finding glyphs.
     * @param codePoint the Unicode code-point to find in the font.
     * @return The font/glyph containing the code point, or boolean false if not found.
     */
    [[nodiscard]] GlyphArray find_glyphs(FontID font_id, grapheme grapheme) const noexcept;

private:

    /** Load the font.
     * We have already read the font once; if opening the font fails now we can't handle the errors.
     */
    void load_font(int id) const noexcept;

    /** Morph the set of glyphs using the font's morph tables.
     */
    void morph_glyphs(glyph_array &glyphs) const noexcept;

    /** Lookup the glyphs in the atlas and optionally render.
    */
    void atlas_lookup(glyph &glyph) noexcept;

    /** Lookup the glyphs in the atlas and optionally render.
     */
    void atlas_lookup(glyph_array &glyphs) noexcept;

    void kern_glyphs(glyph_array &glyphs) const noexcept;

    /** Find a fallback font family name
    * Repeated calls will follow the chain.
    */
    [[nodiscard]] std::string const &FontBook::find_fallback_family_name(std::string const &name) const noexcept;

    void create_family_name_fallback_chain() noexcept;

};


}