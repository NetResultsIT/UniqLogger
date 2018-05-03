#ifndef CONSOLECOLORSCHEME_H
#define CONSOLECOLORSCHEME_H

#include <QHash>
#include <unqlog_common.h>

namespace UNQL
{


/*!
  \enum ConsoleColorType
  \brief this enumeration defines the color that can be used on the console writer
  */
enum ConsoleColorType {
    NO_COLOR =   -999,
    BLACK    =   30,
    RED      =   31,
    GREEN    =   32,
    YELLOW   =   33,
    BLUE     =   34,
    MAGENTA  =   35,
    CYAN     =   36,
    GRAY     =   37,
    GREY     =   37,
    WHITE    =   37,

    no_color = NO_COLOR,
    black = BLACK,
    red = RED,
    green = GREEN,
    yellow = YELLOW,
    blue = BLUE,
    magenta = MAGENTA,
    cyan = CYAN,
    gray = GRAY,
    grey = GREY,
    white = WHITE
};


/*!
 * \brief This class defines a color scheme for every LogMessagePriorityType.
 *
 * You can use it with ConsoleWriter and ConsoleLogger to set a custom
 * log color scheme.
 */
class ULOG_LIB_API ConsoleColorScheme
{
    QHash<UNQL::LogMessagePriorityType, UNQL::ConsoleColorType> m_colorMap;
    UNQL::ConsoleColorType m_defaultColor;
    bool m_enabled;

public:
    /*!
     * \brief Create an empty color scheme.
     */
    ConsoleColorScheme();

    ConsoleColorScheme(const ConsoleColorScheme& c2);

    /*!
     * \brief Create a color scheme with a simple color mapping.
     *
     * DBG_ALL -> gray
     * DBG -> gray
     * INFO -> none
     * WARNING -> yellow
     * CRITICAL -> red
     * FATAL -> red
     * FORCED -> cyan
     *
     * \return A simple ConsoleColorScheme.
     */
    static ConsoleColorScheme defaultColorScheme();

    /*!
     * \brief Set the color for a specific log priority.
     */
    void setColorForLevel(UNQL::LogMessagePriorityType i_level, UNQL::ConsoleColorType i_color);

    /*!
     * \brief Get the color for a specific log priority or the default one if not set.
     */
    UNQL::ConsoleColorType getColorForLevel(UNQL::LogMessagePriorityType i_level);

    /*!
     * \brief Set the default color used when no specific log priority color was set.
     * \param i_color The default color of this scheme.
     */
    void setDefaultColor(UNQL::ConsoleColorType i_color);

    bool operator ==(const UNQL::ConsoleColorScheme &rhs) const;
};

}
#endif // CONSOLECOLORSCHEME_H
