#include "EqualizerMainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("AI Equalizer");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("AudioTools");
    
    EqualizerMainWindow window;
    window.show();
    
    return app.exec();
}
