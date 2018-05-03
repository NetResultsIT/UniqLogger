#include "ConsoleColorScheme.h"

UNQL::ConsoleColorScheme::ConsoleColorScheme()
    : m_defaultColor(UNQL::NO_COLOR)
{

}

UNQL::ConsoleColorScheme::ConsoleColorScheme(const UNQL::ConsoleColorScheme &c2)
{
    m_defaultColor = c2.m_defaultColor;
    m_colorMap     = c2.m_colorMap;
}

UNQL::ConsoleColorScheme UNQL::ConsoleColorScheme::defaultColorScheme()
{
    ConsoleColorScheme c;
    c.setColorForLevel(UNQL::LOG_DBG_ALL , UNQL::GRAY);
    c.setColorForLevel(UNQL::LOG_DBG     , UNQL::GRAY);
    c.setColorForLevel(UNQL::LOG_WARNING , UNQL::YELLOW);
    c.setColorForLevel(UNQL::LOG_CRITICAL, UNQL::RED);
    c.setColorForLevel(UNQL::LOG_FATAL   , UNQL::RED);
    c.setColorForLevel(UNQL::LOG_FORCED  , UNQL::CYAN);

    return c;
}

void UNQL::ConsoleColorScheme::setColorForLevel(UNQL::LogMessagePriorityType i_level, UNQL::ConsoleColorType i_color)
{
    m_colorMap.insert(i_level, i_color);
}

UNQL::ConsoleColorType UNQL::ConsoleColorScheme::getColorForLevel(UNQL::LogMessagePriorityType i_level)
{
    return m_colorMap.value(i_level, m_defaultColor);
}

void UNQL::ConsoleColorScheme::setDefaultColor(UNQL::ConsoleColorType i_color)
{
    m_defaultColor = i_color;
}

bool UNQL::ConsoleColorScheme::operator ==(const UNQL::ConsoleColorScheme &rhs) const
{
    return m_defaultColor == rhs.m_defaultColor && m_colorMap == rhs.m_colorMap;
}
