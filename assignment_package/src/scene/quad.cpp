#include "quad.h"
#include <vector>

Quad::Quad(OpenGLContext *context)
    : Drawable(context)
{}

Quad::~Quad()
{}

void Quad::createVBOdata() {
    std::vector<glm::vec4> glPos { glm::vec4(-1,-1,1, 1),
                                 glm::vec4( 1,-1,1, 1),
                                 glm::vec4( 1, 1,1, 1),
                                 glm::vec4(-1, 1,1, 1) };

    std::vector<glm::vec2> glUV { glm::vec2(0,0),
                                glm::vec2(1,0),
                                glm::vec2(1,1),
                                glm::vec2(0,1) };

    std::vector<GLuint> glIndex {0,1,2,0,2,3};
    indexCounts[INDEX] = 6;

    generateBuffer(POSITION);
    bindBuffer(POSITION);
    mp_context->glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * glPos.size(), glPos.data(), GL_STATIC_DRAW);

    generateBuffer(UV);
    bindBuffer(UV);
    mp_context->glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * glUV.size(), glUV.data(), GL_STATIC_DRAW);

    generateBuffer(INDEX);
    bindBuffer(INDEX);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * glIndex.size(), glIndex.data(), GL_STATIC_DRAW);

}
