#include <QStylePlugin>
#include "winxpstyle.h"

class WinXPStylePlugin : public QStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QStyleFactoryInterface" FILE "winxp.json")
public:
    QStringList keys() const;
    QStyle *create(const QString &key);
};

QStringList WinXPStylePlugin::keys() const
{
    return QStringList()
        << QStringLiteral("winxp")         // Classic (palette-based)
        << QStringLiteral("winxp-blue")    // Windows XP Luna Blue
        << QStringLiteral("winxp-silver")  // Windows XP Luna Silver
        << QStringLiteral("winxp-olive");  // Windows XP Luna Olive
}

QStyle *WinXPStylePlugin::create(const QString &key)
{
    const QString lower = key.toLower();
    if (lower == QLatin1String("winxp"))
        return new WinXPStyle(WinXPStyle::Classic);
    if (lower == QLatin1String("winxp-blue"))
        return new WinXPStyle(WinXPStyle::Blue);
    if (lower == QLatin1String("winxp-silver"))
        return new WinXPStyle(WinXPStyle::Silver);
    if (lower == QLatin1String("winxp-olive"))
        return new WinXPStyle(WinXPStyle::Olive);
    return 0;
}

#include "plugin.moc"
