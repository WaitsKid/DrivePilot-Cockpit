#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QtWebEngineQuick::initialize();

    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("WaitsKid"));
    QGuiApplication::setApplicationName(QStringLiteral("DrivePilotCockpit"));
    app.setWindowIcon(QIcon(QStringLiteral(":/Images/Home/vehicle.png")));

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("DrivePilot", "Main");

    return QCoreApplication::exec();
}
