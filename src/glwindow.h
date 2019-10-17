#ifndef GL_WINDOW_H
#define GL_WINDOW_H

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "geometry.h"

enum OBJMODE {
	NONE,
	TRANSLATE,
	ROTATE,
	SCALE,
};

enum PLANE {
	XY,
	XZ
};

enum ROTAXIS {
	XAXIS,
	YAXIS,
	ZAXIS
};

class OpenGLWindow
{
public:
	OpenGLWindow();

	void initGL();
	void render();
	bool handleEvent(SDL_Event e);
	void cleanup();

private:
	SDL_Window * sdlWin;

	GLuint vaos[2];
	GLuint shader;
	GLuint vertexBuffers[2];

	GeometryData geometry[2];
	GLuint MatrixID;
	GLuint colorID[2];
	glm::mat4 Projection;
	glm::mat4 View;
	glm::mat4 Model[2];
	Uint64 NOW;
	Uint64 LAST;
	double deltaTime;

	OBJMODE objmode;
	PLANE tplane;
	ROTAXIS rotaxis;
	float translateSpeed;
	int NoLoadedModels;
	int ModelNo;
};

#endif
