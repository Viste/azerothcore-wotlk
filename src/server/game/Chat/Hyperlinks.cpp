/*
* Copyright (C) 2016+     AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-GPL2
* Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
* Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
*/

#include "Hyperlinks.h"
#include "Common.h"
#include "DBCStores.h"
#include "Errors.h"
#include "SharedDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "QuestDef.h"
#include "World.h"

using namespace acore::Hyperlinks;

inline uint8 toHex(char c) { return (c >= '0' && c <= '9') ? c - '0' + 0x10 : (c >= 'a' && c <= 'f') ? c - 'a' + 0x1a : 0x00; }
// Validates a single hyperlink
HyperlinkInfo acore::Hyperlinks::ParseHyperlink(char const* pos)
{
    //color tag
    if (*(pos++) != '|' || *(pos++) != 'c')
        return nullptr;
    uint32 color = 0;
    for (uint8 i = 0; i < 8; ++i)
   {
        if (uint8 hex = toHex(*(pos++)))
            color = (color << 4) | (hex & 0xf);
        else
            return nullptr;
    }

    // link data start tag
    if (*(pos++) != '|' || *(pos++) != 'H')
        return nullptr;

    // link tag, find next : or |
    char const* tagStart = pos;
    size_t tagLength = 0;
    while (*pos && *pos != '|' && *(pos++) != ':') // we only advance pointer to one past if the last thing is : (not for |), this is intentional!
        ++tagLength;

    // ok, link data, skip to next |
    char const* dataStart = pos;
    size_t dataLength = 0;
    while (*pos && *(pos++) != '|')
        ++dataLength;

    // ok, next should be link data end tag...
    if (*(pos++) != 'h')
        return nullptr;

    // then visible link text, starts with [
    if (*(pos++) != '[')
        return nullptr;

    // skip until we hit the next ], abort on unexpected |
    char const* textStart = pos;
    size_t textLength = 0;
    while (*pos)
    {
        if (*pos == '|')
            return nullptr;
        if (*(pos++) == ']')
            break;
        ++textLength;
    }

    // link end tag
    if (*(pos++) != '|' || *(pos++) != 'h' || *(pos++) != '|' || *(pos++) != 'r')
        return nullptr;

    // ok, valid hyperlink, return info
    return { pos, color, tagStart, tagLength, dataStart, dataLength, textStart, textLength };
}

template <typename T>
struct LinkValidator
{
    static bool IsTextValid(typename T::value_type, char const*, size_t) { return true; }
    static bool IsColorValid(typename T::value_type, HyperlinkColor) { return true; }
};

// str1 is null-terminated, str2 is length-terminated, check if they are exactly equal
static bool equal_with_len(char const* str1, char const* str2, size_t len)
{
    if (!*str1)
        return false;
    while (len && *str1 && *(str1++) == *(str2++))
        --len;
    return !len && !*str1;
}

template <>
struct LinkValidator<LinkTags::achievement>
{
    static bool IsTextValid(AchievementLinkData const& data, char const* pos, size_t len)
    {
        if (!len)
            return false;
        for (uint8 i = 0; i < TOTAL_LOCALES; ++i)
            if (equal_with_len(data.Achievement->Title[i], pos, len))
                return true;
        return false;
    }

    static bool IsColorValid(AchievementLinkData const&, HyperlinkColor c)
    {
        return c == CHAT_LINK_COLOR_ACHIEVEMENT;
    }
};

template <>
struct LinkValidator<LinkTags::item>
{
    static bool IsTextValid(ItemLinkData const& data, char const* pos, size_t len)
    {
        ItemLocale const* locale = sObjectMgr->GetItemLocale(data.Item->ItemId);

        char const* const* randomSuffix = nullptr;
        if (data.RandomPropertyId < 0)
        {
            if (ItemRandomSuffixEntry const* suffixEntry = sItemRandomSuffixStore.LookupEntry(-data.RandomPropertyId))
                randomSuffix = suffixEntry->nameSuffix;
            else
                return false;
        }
        else if (data.RandomPropertyId > 0)
        {
            if (ItemRandomPropertiesEntry const* propEntry = sItemRandomPropertiesStore.LookupEntry(data.RandomPropertyId))
                randomSuffix = propEntry->nameSuffix;
            else
                return false;
        }

        for (uint8 i = 0; i < TOTAL_LOCALES; ++i)
        {
            if (!locale && i != DEFAULT_LOCALE)
                continue;
            std::string const& name = (i == DEFAULT_LOCALE) ? data.Item->Name1 : locale->Name[i];
            if (name.empty())
                continue;
            if (randomSuffix)
            {
                if (len > name.length() + 1 &&
                  (strncmp(name.c_str(), pos, name.length()) == 0) && 
                  (*(pos + name.length()) == ' ') &&
                  equal_with_len(randomSuffix[i], pos + name.length() + 1, len - name.length() - 1))
                    return true;
            }
            else if (equal_with_len(name.c_str(), pos, len))
                return true;
        }
        return false;
    }

    static bool IsColorValid(ItemLinkData const& data, HyperlinkColor c)
    {
        return c == ItemQualityColors[data.Item->Quality];
    }
};

template <>
struct LinkValidator<LinkTags::quest>
{
    static bool IsTextValid(QuestLinkData const& data, char const* pos, size_t len)
    {
        QuestLocale const* locale = sObjectMgr->GetQuestLocale(data.Quest->GetQuestId());
        if (!locale)
            return equal_with_len(data.Quest->GetTitle().c_str(), pos, len);

        for (uint8 i = 0; i < TOTAL_LOCALES; ++i)
        {
            std::string const& name = (i == DEFAULT_LOCALE) ? data.Quest->GetTitle() : locale->Title[i];
            if (name.empty())
                continue;
            if (equal_with_len(name.c_str(), pos, len))
                return true;
        }

        return false;
    }

    static bool IsColorValid(QuestLinkData const&, HyperlinkColor c)
    {
        for (uint8 i = 0; i < MAX_QUEST_DIFFICULTY; ++i)
            if (c == QuestDifficultyColors[i])
                return true;
        return false;
    }
};

template <>
struct LinkValidator<LinkTags::spell>
{
    static bool IsTextValid(SpellInfo const* info, char const* pos, size_t len)
    {
        for (uint8 i = 0; i < TOTAL_LOCALES; ++i)
            if (equal_with_len(info->SpellName[i], pos, len))
                return true;
        return false;
    }

    static bool IsColorValid(SpellInfo const*, HyperlinkColor c)
    {
        return c == CHAT_LINK_COLOR_SPELL;
    }
};

template <>
struct LinkValidator<LinkTags::enchant>
{
    static bool IsTextValid(SpellInfo const* info, char const* pos, size_t len)
    {
        SkillLineAbilityMapBounds bounds = sSpellMgr->GetSkillLineAbilityMapBounds(info->Id);
        if (bounds.first == bounds.second)
            return LinkValidator<LinkTags::spell>::IsTextValid(info, pos, len);

        SkillLineEntry const* skill = sSkillLineStore.LookupEntry(bounds.first->second->skillId);
        if (!skill)
            return false;

        for (uint8 i = 0; i < TOTAL_LOCALES; ++i)
        {
            char const* skillName = skill->name[i];
            size_t skillLen = strlen(skillName);
            if (len > skillLen + 2 &&                         // or of form [Skill Name: Spell Name]
                !strncmp(pos, skillName, skillLen) && !strncmp(pos + skillLen, ": ", 2) &&
                equal_with_len(info->SpellName[i], pos + (skillLen + 2), len - (skillLen + 2)))
                return true;
        }
        return false;
    }

    static bool IsColorValid(SpellInfo const*, HyperlinkColor c)
    {
        return c == CHAT_LINK_COLOR_ENCHANT;
    }
};

template <>
struct LinkValidator<LinkTags::glyph>
{
    static bool IsTextValid(GlyphLinkData const& data, char const* pos, size_t len)
    {
        if (SpellInfo const* info = sSpellMgr->GetSpellInfo(data.Glyph->SpellId))
            return LinkValidator<LinkTags::spell>::IsTextValid(info, pos, len);
        return false;
    }

    static bool IsColorValid(GlyphLinkData const&, HyperlinkColor c)
    {
        return c == CHAT_LINK_COLOR_GLYPH;
    }
};

template <>
struct LinkValidator<LinkTags::talent>
{
    static bool IsTextValid(TalentLinkData const& data, char const* pos, size_t len)
    {
        if (SpellInfo const* info = sSpellMgr->GetSpellInfo(data.Talent->RankID[data.Rank-1]))
            return LinkValidator<LinkTags::spell>::IsTextValid(info, pos, len);
        return false;
    }

    static bool IsColorValid(TalentLinkData const&, HyperlinkColor c)
    {
        return c == CHAT_LINK_COLOR_TALENT;
    }
};

template <>
struct LinkValidator<LinkTags::trade>
{
    static bool IsTextValid(TradeskillLinkData const& data, char const* pos, size_t len)
    {
        return LinkValidator<LinkTags::spell>::IsTextValid(data.Spell, pos, len);
    }

    static bool IsColorValid(TradeskillLinkData const&, HyperlinkColor c)
    {
        return c == CHAT_LINK_COLOR_TRADE;
    }
};

#define TryValidateAs(tagname)                                                                  \
{                                                                                               \
    using taginfo = typename LinkTags::tagname;                                                 \
    ASSERT(!strcmp(taginfo::tag(), #tagname));                                                  \
    if (info.tag.second == strlen(taginfo::tag()) &&                                            \
        !strncmp(info.tag.first, taginfo::tag(), strlen(taginfo::tag())))                       \
    {                                                                                           \
        advstd::remove_cvref_t<typename taginfo::value_type> t;                                 \
        if (!taginfo::StoreTo(t, info.data.first, info.data.second))                            \
            return false;                                                                       \
        if (!LinkValidator<taginfo>::IsColorValid(t, info.color))                               \
            return false;                                                                       \
        if (sWorld->getIntConfig(CONFIG_CHAT_STRICT_LINK_CHECKING_SEVERITY))                    \
            if (!LinkValidator<taginfo>::IsTextValid(t, info.text.first, info.text.second))     \
                return false;                                                                   \
        return true;                                                                            \
    }                                                                                           \
}

static bool ValidateLinkInfo(HyperlinkInfo const& info)
{
    TryValidateAs(achievement);
    TryValidateAs(area);
    TryValidateAs(areatrigger);
    TryValidateAs(creature);
    TryValidateAs(creature_entry);
    TryValidateAs(enchant);
    TryValidateAs(gameevent);
    TryValidateAs(gameobject);
    TryValidateAs(gameobject_entry);
    TryValidateAs(glyph);
    TryValidateAs(item);
    TryValidateAs(itemset);
    TryValidateAs(player);
    TryValidateAs(quest);
    TryValidateAs(skill);
    TryValidateAs(spell);
    TryValidateAs(talent);
    TryValidateAs(taxinode);
    TryValidateAs(tele);
    TryValidateAs(title);
    TryValidateAs(trade);
    return false;
}

// Validates all hyperlinks and control sequences contained in str
bool acore::Hyperlinks::ValidateLinks(std::string& str)
{
    bool allValid = true;
    std::string::size_type pos = std::string::npos;
    // Step 1: Strip all control sequences except ||, |H, |h, |c and |r
    do
    {
        if ((pos = str.rfind('|', pos)) == std::string::npos)
            break;
        if (pos && str[pos - 1] == '|')
        {
            --pos;
            continue;
        }
        char next = str[pos + 1];
        if (next == 'H' || next == 'h' || next == 'c' || next == 'r')
            continue;

        allValid = false;
        str.erase(pos, 2);
    } while (pos--);

    // Step 2: Parse all link sequences
    // They look like this: |c<color>|H<linktag>:<linkdata>|h[<linktext>]|h|r
    // - <color> is 8 hex characters AARRGGBB
    // - <linktag> is arbitrary length [a-z_]
    // - <linkdata> is arbitrary length, no | contained
    // - <linktext> is printable
    pos = 0;
    while (pos < str.size() && (pos = str.find('|', pos)) != std::string::npos)
    {
        if (str[pos + 1] == '|') // this is an escaped pipe character (||)
        {
            pos += 2;
            continue;
        }

        HyperlinkInfo info = ParseHyperlink(str.c_str() + pos);
        if (!info) 
        {   // cannot be parsed at all, so we'll need to cut it out entirely
            // find the next start of a link
            std::string::size_type next = str.find("|c", pos + 1);
            // then backtrack to the previous return control sequence
            std::string::size_type end = str.rfind("|r", next);
            if (end == std::string::npos || end <= pos) // there is no potential end tag, remove everything after pos (up to next, if available)
            {
                if (next == std::string::npos)
                    str.erase(pos);
                else
                    str.erase(pos, next - pos);
            }
            else
                str.erase(pos, end - pos + 2);

            allValid = false;
            continue;
        }

        // ok, link parsed successfully - now validate it based on the tag
        if (!ValidateLinkInfo(info))
        {
            // invalid link info, replace with just text
            str.replace(pos, (info.next - str.c_str()) - pos, str, info.text.first - str.c_str(), info.text.second);
            allValid = false;
            continue;
        }

        // tag is fine, find the next one
        pos = info.next - str.c_str();
    }

    // all tags validated
    return allValid;
}
