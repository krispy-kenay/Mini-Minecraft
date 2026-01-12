#pragma once
#include <openglcontext.h>

class Texture
{
private:
    OpenGLContext* glContext;
    GLuint m_textureHandle;

public:
    Texture(OpenGLContext* context);
    ~Texture();

    void create(const char *texturePath,
                GLenum internalFormat = GL_RGBA,
                GLenum format = GL_BGRA);
    void bufferPixelData(unsigned int width, unsigned int height,
                         GLenum internalFormat, GLenum format, GLvoid *pixels);
    void bind(int texSlot);
    void destroy();

    GLuint getHandle() const;

};
