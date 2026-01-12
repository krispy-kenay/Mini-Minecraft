#include "mygl.h"
#include "utils.h"
#include <glm_includes.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>
#include <QDateTime>


MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
      m_worldAxes(this),
    m_progLambert(this), m_progFlat(this), m_progInstanced(this), m_progBlinnPhong(this),
      m_progPostProcessNoOp(this), m_progPostProcessUnderWater(this), m_progPostProcessUnderLava(this),
      m_selectedPostProcessShader(&m_progPostProcessNoOp),
      m_quadDrawable(this),
      m_postProcessFrameBuffer(this, this->width(), this->height(), this->devicePixelRatio()),
      m_terrain(this), m_player(glm::vec3(48.f, 161.f, 48.f), m_terrain),
    currentMSecsSinceEpoch(0), inputBundle(), degY(0.f), m_texture(this), m_underwater(this), m_underlava(this)
{
    // Connect the timer to a function so that when the timer ticks the function is executed
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
    setFocusPolicy(Qt::ClickFocus);

    setMouseTracking(true); // MyGL will track the mouse's movements even if a mouse button is not pressed
    setCursor(Qt::BlankCursor); // Make the cursor invisible

    currentMSecsSinceEpoch = QDateTime::currentMSecsSinceEpoch(); // first time registration

    // Sound set up
    QString ariaMath = getCurrentPath() + "/sounds/aria_math.wav";
    m_ariaMath.setSource(QUrl::fromLocalFile(ariaMath));
    m_ariaMath.setLoopCount(QSoundEffect::Infinite);
    m_ariaMath.setVolume(0.25f);
    m_ariaMath.play();

    QString grassStepPath = getCurrentPath() + "/sounds/grass.wav";
    m_grassWalk.setSource(QUrl::fromLocalFile(grassStepPath));
    m_grassWalk.setLoopCount(QSoundEffect::Infinite);
    m_grassWalk.setVolume(0.4f);

    QString stoneStepPath = getCurrentPath() + "/sounds/stone.wav";
    m_stoneWalk.setSource(QUrl::fromLocalFile(stoneStepPath));
    m_stoneWalk.setLoopCount(QSoundEffect::Infinite);
    m_stoneWalk.setVolume(0.4f);

    QString snowStepPath = getCurrentPath() + "/sounds/snow.wav";
    m_snowWalk.setSource(QUrl::fromLocalFile(snowStepPath));
    m_snowWalk.setLoopCount(QSoundEffect::Infinite);
    m_snowWalk.setVolume(0.4f);

    QString swimPath = getCurrentPath() + "/sounds/swim.wav";
    m_swim.setSource(QUrl::fromLocalFile(swimPath));
    m_swim.setLoopCount(QSoundEffect::Infinite);
    m_swim.setVolume(0.2f);

    QString splashPath = getCurrentPath() + "/sounds/splash.wav";
    m_splash.setSource(QUrl::fromLocalFile(splashPath));
    m_splash.setLoopCount(1);
    m_splash.setVolume(0.6f);
}

MyGL::~MyGL() {
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
}


void MyGL::moveMouseToCenter() {
    QCursor::setPos(this->mapToGlobal(QPoint(width() / 2, height() / 2)));
}

void MyGL::initializeGL()
{
    // Create an OpenGL context using Qt's QOpenGLFunctions_3_2_Core class
    // If you were programming in a non-Qt context you might use GLEW (GL Extension Wrangler)instead
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();

    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // set for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.37f, 0.74f, 1.0f, 1);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);

    //Create the instance of the world axes
    m_worldAxes.createVBOdata();

    // create quad vbo data for post process
    m_quadDrawable.createVBOdata();

    m_postProcessFrameBuffer.create();

    // Create and set up the diffuse shader
    m_progLambert.create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl");
    // Create and set up the flat lighting shader
    m_progFlat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");
    m_progInstanced.create(":/glsl/instanced.vert.glsl", ":/glsl/lambert.frag.glsl");
    m_progBlinnPhong.create(":/glsl/water_wave.vert.glsl", ":/glsl/blinn_phong.frag.glsl");

    m_progPostProcessNoOp.create(":/glsl/passthrough.vert.glsl", ":/glsl/noOp.frag.glsl");
    m_progPostProcessUnderWater.create(":/glsl/passthrough.vert.glsl", ":/glsl/underwater.frag.glsl");
    m_progPostProcessUnderLava.create(":/glsl/passthrough.vert.glsl", ":/glsl/underlava.frag.glsl");

    // texture
    QString texturePath = getCurrentPath() + "/textures/minecraft_textures_all_copy.png";
    std::cout << texturePath.toStdString() << std::endl;
    m_texture.create(texturePath.toStdString().c_str());

    // QString underwaterPath = getCurrentPath() + "/textures/underwater.png";
    // std::cout << underwaterPath.toStdString() << std::endl;
    // m_underwater.create(underwaterPath.toStdString().c_str());

    // QString underlavaPath = getCurrentPath() + "/textures/underlava.png";
    // std::cout << underlavaPath.toStdString() << std::endl;
    // m_underlava.create(underlavaPath.toStdString().c_str());

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);

    // m_terrain.CreateTestScene();
}


void MyGL::resizeGL(int w, int h) {
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.

    m_player.setCameraWidthHeight(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)

    m_progLambert.setUnifMat4("u_ViewProj", viewproj);
    m_progFlat.setUnifMat4("u_ViewProj", viewproj);
    m_progInstanced.setUnifMat4("u_ViewProj", viewproj);
    m_progBlinnPhong.setUnifMat4("u_ViewProj", viewproj);

    m_postProcessFrameBuffer.resize(w * this->devicePixelRatio(), h * this->devicePixelRatio(), this->devicePixelRatio());
    m_postProcessFrameBuffer.destroy();
    m_postProcessFrameBuffer.create();

    printGLErrorLog();
}

void MyGL::startGame() {
    m_timer.start(16);
}

void MyGL::stopGame() {
    m_timer.stop();
}


// MyGL's constructor links tick() to a timer that fires 60 times per second.
// We're treating MyGL as our game engine class, so we're going to perform
// all per-frame actions here, such as performing physics updates on all
// entities in the scene.
void MyGL::tick() {
    update(); // Calls paintGL() as part of a larger QOpenGLWidget pipeline
    sendPlayerDataToGUI(); // Updates the info in the secondary window displaying player data

    int currentTime = QDateTime::currentMSecsSinceEpoch();
    float dT = (currentTime - currentMSecsSinceEpoch) * 0.001f;
    float currTimeFloat = currentTime * 0.001f;

    currentMSecsSinceEpoch = currentTime;

    m_progLambert.setUnifFloat("u_Time", currTimeFloat);
    m_progFlat.setUnifFloat("u_Time", currTimeFloat);
    m_progInstanced.setUnifFloat("u_Time", currTimeFloat);
    m_progPostProcessUnderWater.setUnifFloat("u_Time", currTimeFloat);
    m_progPostProcessUnderLava.setUnifFloat("u_Time", currTimeFloat);
    m_progBlinnPhong.setUnifFloat("u_Time", currTimeFloat);

    m_terrain.generate(m_player.mcr_position);

    m_player.tick(dT, inputBundle);

    if (m_player.inLiquid()) {
        if (jumpedIntoWater) {
            if (!m_splash.isPlaying()) m_splash.play();
            jumpedIntoWater = false;
        }
        stopStepSounds();
        if (!m_swim.isPlaying()) m_swim.play();
    } else if (m_player.isWalking && m_player.onFloor) {
        jumpedIntoWater = true;
        if (m_swim.isPlaying()) m_swim.stop();
        playStepSounds();
    } else {
        jumpedIntoWater = true;
        if (m_swim.isPlaying()) m_swim.stop();
        stopStepSounds();
    }
}

void MyGL::sendPlayerDataToGUI() const {
    emit sig_sendPlayerPos(m_player.posAsQString());
    emit sig_sendPlayerVel(m_player.velAsQString());
    emit sig_sendPlayerAcc(m_player.accAsQString());
    emit sig_sendPlayerLook(m_player.lookAsQString());
    glm::vec2 pPos(m_player.mcr_position.x, m_player.mcr_position.z);
    glm::ivec2 chunk(16 * glm::ivec2(glm::floor(pPos / 16.f)));
    glm::ivec2 zone(64 * glm::ivec2(glm::floor(pPos / 64.f)));
    emit sig_sendPlayerChunk(QString::fromStdString("( " + std::to_string(chunk.x) + ", " + std::to_string(chunk.y) + " )"));
    emit sig_sendPlayerTerrainZone(QString::fromStdString("( " + std::to_string(zone.x) + ", " + std::to_string(zone.y) + " )"));
}

// This function is called whenever update() is called.
// MyGL's constructor links update() to a timer that fires 60 times per second,`
// so paintGL() called at a rate of 60 frames per second.
void MyGL::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_postProcessFrameBuffer.bindFrameBuffer();
    glViewport(0, 0, this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());

    // Clear the screen so that we only see newly drawn images
    glClearColor(0.37f, 0.74f, 1.0f, 1); // sky color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();
    m_progLambert.setUnifMat4("u_ViewProj", viewproj);
    m_progFlat.setUnifMat4("u_ViewProj", viewproj);
    m_progInstanced.setUnifMat4("u_ViewProj", viewproj);
    m_progBlinnPhong.setUnifMat4("u_ViewProj", viewproj);

    // Render the terrain
    m_progLambert.setUnifMat4("u_Model", glm::mat4(1.f));
    m_progLambert.setUnifMat4("u_ModelInvTr", glm::inverse(glm::transpose(glm::mat4(1.f))));
    m_progBlinnPhong.setUnifMat4("u_Model", glm::mat4(1.f));
    m_progBlinnPhong.setUnifMat4("u_ModelInvTr", glm::inverse(glm::transpose(glm::mat4(1.f))));

    // SUN / light source
    m_progBlinnPhong.setUnifVec3("u_CamPos", glm::vec3(40.28f, 321.5f, 2.5f));
    m_progBlinnPhong.setUnifVec3("u_CamLook", glm::vec3(0.1f, -0.99f, -0.04f));

    m_texture.bind(0);
    m_terrain.draw(m_player.mcr_position, &m_progLambert, &m_progBlinnPhong, m_player.mcr_camera);

    glDisable(GL_DEPTH_TEST);
    m_progFlat.setUnifMat4("u_Model", glm::mat4());
    m_progFlat.draw(m_worldAxes);
    glEnable(GL_DEPTH_TEST);

    if (m_player.underLiquidBlock == WATER) {
        m_selectedPostProcessShader = &m_progPostProcessUnderWater;
    } else if (m_player.underLiquidBlock == LAVA) {
        m_selectedPostProcessShader = &m_progPostProcessUnderLava;
    } else {
        m_selectedPostProcessShader = &m_progPostProcessNoOp;
    }

    renderPostProcess();
}

void MyGL::keyPressEvent(QKeyEvent *e) {
    float amount = 2.0f;
    if(e->modifiers() & Qt::ShiftModifier){
        amount = 10.0f;
    }
    // http://doc.qt.io/qt-5/qt.html#Key-enum
    // This could all be much more efficient if a switch
    // statement were used, but I really dislike their
    // syntax so I chose to be lazy and use a long
    // chain of if statements instead

    if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
    } else if (e->key() == Qt::Key_Right) {
        m_player.rotateOnUpGlobal(-amount);
    } else if (e->key() == Qt::Key_Left) {
        m_player.rotateOnUpGlobal(amount);
    } else if (e->key() == Qt::Key_Up) {
        m_player.rotateOnRightLocal(-amount);
    } else if (e->key() == Qt::Key_Down) {
        m_player.rotateOnRightLocal(amount);
    } else if (e->key() == Qt::Key_W) {
        inputBundle.wPressed = true;
    } else if (e->key() == Qt::Key_S) {
        inputBundle.sPressed = true;
    } else if (e->key() == Qt::Key_D) {
        inputBundle.dPressed = true;
    } else if (e->key() == Qt::Key_A) {
        inputBundle.aPressed = true;
    } else if (e->key() == Qt::Key_Q) {
        inputBundle.qPressed = true;
    } else if (e->key() == Qt::Key_E) {
        inputBundle.ePressed = true;
    } else if (e->key() == Qt::Key_F) {
        inputBundle.fPressed = !inputBundle.fPressed;
    } else if (e->key() == Qt::Key_Space) {
        inputBundle.spacePressed = true;
    }
}

void MyGL::keyReleaseEvent(QKeyEvent *e) {
    float amount = 2.0f;
    if(e->modifiers() & Qt::ShiftModifier){
        amount = 10.0f;
    }
    // http://doc.qt.io/qt-5/qt.html#Key-enum
    // This could all be much more efficient if a switch
    // statement were used, but I really dislike their
    // syntax so I chose to be lazy and use a long
    // chain of if statements instead

    if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
    } else if (e->key() == Qt::Key_Right) {
        m_player.rotateOnUpGlobal(-amount);
    } else if (e->key() == Qt::Key_Left) {
        m_player.rotateOnUpGlobal(amount);
    } else if (e->key() == Qt::Key_Up) {
        m_player.rotateOnRightLocal(-amount);
    } else if (e->key() == Qt::Key_Down) {
        m_player.rotateOnRightLocal(amount);
    } else if (e->key() == Qt::Key_W) {
        inputBundle.wPressed = false;
    } else if (e->key() == Qt::Key_S) {
        inputBundle.sPressed = false;
    } else if (e->key() == Qt::Key_D) {
        inputBundle.dPressed = false;
    } else if (e->key() == Qt::Key_A) {
        inputBundle.aPressed = false;
    } else if (e->key() == Qt::Key_Q) {
        inputBundle.qPressed = false;
    } else if (e->key() == Qt::Key_E) {
        inputBundle.ePressed = false;
    } else if (e->key() == Qt::Key_Space) {
        inputBundle.spacePressed = false;
    }
}

void MyGL::mouseMoveEvent(QMouseEvent *e) {
    inputBundle.mouseX = width() / 2.f;
    inputBundle.mouseY = height() / 2.f;

    float x = e->pos().x();
    float y = e->pos().y();
    float sensitivity = .025f;

    float delta_x = x - inputBundle.mouseX;
    float delta_y = y - inputBundle.mouseY;

    float degreesX = -1.f * delta_x * sensitivity;
    float degreesY = -1.f * delta_y * sensitivity;

    if (degreesY > 0.f && degY < 88.f) {
        degY += degreesY;
        m_player.rotateOnRightLocal(degreesY);
    } else if (degreesY < 0.f && degY > -88.f) {
        degY += degreesY;
        m_player.rotateOnRightLocal(degreesY);
    }

    m_player.rotateOnUpGlobal(degreesX);
    moveMouseToCenter();
}

void MyGL::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        m_player.deleteBlock(m_terrain);
        update();
    } else if (e->button() == Qt::RightButton) {
        m_player.placeBlock(m_terrain);
        update();
    }
}

void MyGL::setFolderLocation(std::string folderLocation) {
    m_terrain.m_worldFolder = folderLocation;
}

void MyGL::triggerSave() {
    m_terrain.saveTerrain();
}
void MyGL::playStepSounds() {
    switch (m_player.onTopOf) {
    case GRASS:
        if (m_stoneWalk.isPlaying()) m_stoneWalk.stop();
        if (m_snowWalk.isPlaying()) m_snowWalk.stop();
        if (!m_grassWalk.isPlaying()) {
            m_grassWalk.play();
        }
        break;
    case STONE:
        if (m_grassWalk.isPlaying()) m_grassWalk.stop();
        if (m_snowWalk.isPlaying()) m_snowWalk.stop();
        if (!m_stoneWalk.isPlaying()) {
            m_stoneWalk.play();
        }
        break;
    case SNOW:
        if (m_stoneWalk.isPlaying()) m_stoneWalk.stop();
        if (m_grassWalk.isPlaying()) m_grassWalk.stop();
        if (!m_snowWalk.isPlaying()) {
            m_snowWalk.play();
        }
        break;
    default:
        break;
    }

    if (m_player.onTopOf == GRASS) {
        if (!m_grassWalk.isPlaying()) {
            m_grassWalk.play();
        }
    }
}

void MyGL::stopStepSounds() {
    if (m_grassWalk.isPlaying()) m_grassWalk.stop();
    if (m_stoneWalk.isPlaying()) m_stoneWalk.stop();
    if (m_snowWalk.isPlaying()) m_snowWalk.stop();
}

void MyGL::renderPostProcess() {
    // TODO: (Before you attempt the post-process shaders)
    // Use glBindFramebuffer to set the render output back
    // to your GL window. Normally, you would use 0 as the target
    // argument for glBindFramebuffer to do this, but when working
    // in a Qt environment you actually want to use
    // this->defaultFramebufferObject().
    // As in render3dScene, also set the glViewport.
    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
    glViewport(0, 0, this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());

    glClearColor(0.37f, 0.74f, 1.0f, 1); // sky color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    printGLErrorLog();
    // Place the texture that stores the image of the 3D render
    // into texture slot 0
    m_postProcessFrameBuffer.bindToTextureSlot(0);
    printGLErrorLog();

    // Set the sampler2D in the post-process shader to
    // read from the texture slot that we set the
    m_selectedPostProcessShader->draw(m_quadDrawable);
}
