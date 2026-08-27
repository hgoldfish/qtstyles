#include <QStylePlugin>
#include "phasestyle.h"

class PhaseStylePlugin : public QStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QStyleFactoryInterface" FILE "phase.json")
public:
    QStyle *create(const QString &key);
};

QStyle *PhaseStylePlugin::create(const QString &key)
{
    if (key.toLower() == "phase")
        return new PhaseStyle;
    if (key.toLower() == "phase-classic")
        return new PhaseStyle(true);
    return 0;
}

#include "plugin.moc"
