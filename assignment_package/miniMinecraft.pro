QT += core widgets multimedia
qtHaveModule(openglwidgets) {
    QT += openglwidgets
} else {
    QT += opengl
}
TARGET = MiniMinecraft
TEMPLATE = app
CONFIG += console
CONFIG += c++1z
win32 {
    LIBS += -lopengl32
#    LIBS += -lglut32
    LIBS += -lglu32
}
CONFIG += warn_on
CONFIG += debug

INCLUDEPATH += include

include(src/src.pri)

FORMS += forms/mainwindow.ui \
    forms/cameracontrolshelp.ui \
    forms/playerinfo.ui

RESOURCES += glsl.qrc

*-clang*|*-g++* {
    message("Enabling additional warnings")
    CONFIG -= warn_on
    QMAKE_CXXFLAGS += -Wall -Wextra -pedantic -Winit-self
    QMAKE_CXXFLAGS += -Wno-strict-aliasing
    QMAKE_CXXFLAGS += -fno-omit-frame-pointer
}
linux-clang*|linux-g++*|macx-clang*|macx-g++* {
    message("Enabling stack protector")
    QMAKE_CXXFLAGS += -fstack-protector-all
}

# FOR LINUX & MAC USERS INTERESTED IN ADDITIONAL BUILD TOOLS
# ----------------------------------------------------------
# This conditional exists to enable Address Sanitizer (ASAN) during
# the automated build. ASAN is a compiled-in tool which checks for
# memory errors (like Valgrind). You may enable it for yourself;
# check the hidden `.build.sh` file for info. But be aware: ASAN may
# trigger a lot of false-positive leak warnings for the Qt libraries.
# (See `.run.sh` for how to disable leak checking.)
address_sanitizer {
    message("Enabling Address Sanitizer")
    QMAKE_CXXFLAGS += -fsanitize=address
    QMAKE_LFLAGS += -fsanitize=address
}

HEADERS +=

SOURCES +=

DISTFILES += \
    glsl/blinn_phong.frag.glsl \
    glsl/flat.frag.glsl \
    glsl/flat.vert.glsl \
    glsl/instanced.vert.glsl \
    glsl/lambert.frag.glsl \
    glsl/lambert.vert.glsl \
    glsl/noOp.frag.glsl \
    glsl/passthrough.vert.glsl \
    glsl/underlava.frag.glsl \
    glsl/underwater.frag.glsl \
    glsl/water_wave.vert.glsl \
    glsl/waver_wave.vert.glsl
