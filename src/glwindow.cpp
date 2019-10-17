#include <iostream>
#include <stdio.h>
#include <ctime>

#include "SDL.h"
#include <GL/glew.h>

#include "glwindow.h"
#include "geometry.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

const char* glGetErrorString(GLenum error)
{
	switch (error)
	{
	case GL_NO_ERROR:
		return "GL_NO_ERROR";
	case GL_INVALID_ENUM:
		return "GL_INVALID_ENUM";
	case GL_INVALID_VALUE:
		return "GL_INVALID_VALUE";
	case GL_INVALID_OPERATION:
		return "GL_INVALID_OPERATION";
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		return "GL_INVALID_FRAMEBUFFER_OPERATION";
	case GL_OUT_OF_MEMORY:
		return "GL_OUT_OF_MEMORY";
	default:
		return "UNRECOGNIZED";
	}
}

void glPrintError(const char* label = "Unlabelled Error Checkpoint", bool alwaysPrint = false)
{
	GLenum error = glGetError();
	if (alwaysPrint || (error != GL_NO_ERROR))
	{
		printf("%s: OpenGL error flag is %s\n", label, glGetErrorString(error));
	}
}

GLuint loadShader(const char* shaderFilename, GLenum shaderType)
{
	/*FILE* shaderFile;// = fopen(shaderFilename, "r"); - deprecated fopen method
	error_t err = fopen_s(&shaderFile, shaderFilename, "r");
	if (err != 0)
		return 0;*/
	FILE* shaderFile = fopen(shaderFilename, "r");
	if(!shaderFile)
	{
			return 0;
	}

	fseek(shaderFile, 0, SEEK_END);
	long shaderSize = ftell(shaderFile);
	fseek(shaderFile, 0, SEEK_SET);

	char* shaderText = new char[shaderSize + 1];
	size_t readCount = fread(shaderText, 1, shaderSize, shaderFile);
	shaderText[readCount] = '\0';
	fclose(shaderFile);

	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, (const char**)&shaderText, NULL);
	glCompileShader(shader);

	delete[] shaderText;

	return shader;
}

GLuint loadShaderProgram(const char* vertShaderFilename,
	const char* fragShaderFilename)
{
	//create shaders
	GLuint vertShader = loadShader(vertShaderFilename, GL_VERTEX_SHADER);
	GLuint fragShader = loadShader(fragShaderFilename, GL_FRAGMENT_SHADER);

	// link the program
	GLuint program = glCreateProgram();
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);
	glLinkProgram(program);
	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	GLint linkStatus;
	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
	if (linkStatus != GL_TRUE)
	{
		GLsizei logLength = 0;
		GLchar message[1024];
		glGetProgramInfoLog(program, 1024, &logLength, message);
		cout << "Shader load error: " << message << endl;
		return 0;
	}

	return program;
}

OpenGLWindow::OpenGLWindow()
{
}


void OpenGLWindow::initGL()
{
	objmode = NONE;
	tplane = XY;
	rotaxis = ZAXIS;
	translateSpeed = 0.001f;
	NOW = SDL_GetPerformanceCounter();
	LAST = 0;
	deltaTime = 0;
	NoLoadedModels = 1;
	ModelNo = 0;

	// We need to first specify what type of OpenGL context we need before we can create the window
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	sdlWin = SDL_CreateWindow("OpenGL Prac 1",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		960, 720, SDL_WINDOW_OPENGL);
	if (!sdlWin)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Error", "Unable to create window", 0);
	}
	SDL_GLContext glc = SDL_GL_CreateContext(sdlWin);
	SDL_GL_MakeCurrent(sdlWin, glc);
	SDL_GL_SetSwapInterval(1);

	// Initialize GLEW
	glewExperimental = true;
	GLenum glewInitResult = glewInit();
	glGetError(); // Consume the error erroneously set by glewInit()
	if (glewInitResult != GLEW_OK)
	{
		const GLubyte* errorString = glewGetErrorString(glewInitResult);
		cout << "Unable to initialize glew: " << errorString;
	}

	int glMajorVersion;
	int glMinorVersion;
	glGetIntegerv(GL_MAJOR_VERSION, &glMajorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &glMinorVersion);
	cout << "Loaded OpenGL " << glMajorVersion << "." << glMinorVersion << " with:" << endl;
	cout << "\tVendor: " << glGetString(GL_VENDOR) << endl;
	cout << "\tRenderer: " << glGetString(GL_RENDERER) << endl;
	cout << "\tVersion: " << glGetString(GL_VERSION) << endl;
	cout << "\tGLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClearColor(1.0f, 0.56f, 0.06f, 1.0f); //use orange to clear the screen
	//glClearColor(0.0f, 0.0f, 0.4f, 0.0f); // use dark blue to clear the screen
	glGenVertexArrays(2, vaos);
	glBindVertexArray(vaos[ModelNo]);

	// Note that this path is relative to your working directory
	// when running the program (IE if you run from within build
	// then you need to place these files in build as well)
	shader = loadShaderProgram("simple.vert", "simple.frag");
	glUseProgram(shader);

	//int colorLoc = glGetUniformLocation(shader, "objectColor");
	//glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);

	// Get a handle for our "MVP" uniform
	MatrixID = glGetUniformLocation(shader, "MVP");

	// Projection matrix : 45 degrees Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units - clipping
	Projection = glm::perspective(glm::radians(45.0f),4.0f/3.0f,0.1f, 100.0f);

	// Camera matrix
	View = glm::lookAt(
		glm::vec3(0, 0, 20), // Camera is at (0,0,20), in World Space - on the z-axis - so far away to accomodate for dragon object
		glm::vec3(0, 0, 0), // and looks at the origin
		glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
	);

	// Model matrix : an identity matrix - no transformation yet
	Model[0] = glm::mat4(1.0f);
	Model[1] = glm::mat4(1.0f);

	// Load the model that we want to use and buffer the vertex attributes
	geometry[ModelNo].loadFromOBJFile("../objects/cube.obj");
	//vector<float>* vertices = static_cast<float*>(geometry.vertexData());
	int vertexLoc = glGetAttribLocation(shader, "position");
	//float vertices[9] = { 0.0f,  0.5f, 0.0f,
	//	-0.5f, -0.5f, 0.0f,
	//	0.5f, -0.5f, 0.0f };
	//------------------------------------------------
	//adding color to model 1
	const int colorSize1 = geometry[ModelNo].vertexCount() * 3 * 3;
	float *colors1 = new float[colorSize1];

	// Initialize the random seed from the system time
	srand(time(NULL));
  	// Fill colors with random numbers from 0 to 1, use continuous polynomials for r,g,b:
  	int k = 0;
 	for(int i = 0; i < colorSize1/3; ++i) {
 		float t = (float)rand()/(float)RAND_MAX;
 		colors1[k] = 9*(1-t)*t*t*t;
 		k++;
		colors1[k] = 15*(1-t)*(1-t)*t*t;
 		k++;
 		colors1[k] = 8.5*(1-t)*(1-t)*(1-t)*t;
 		k++;
 	}

	glGenBuffers(2, colorID);
	glBindBuffer(GL_ARRAY_BUFFER, colorID[ModelNo]);
	glBufferData(GL_ARRAY_BUFFER, colorSize1 * sizeof(float), &colors1[0], GL_STATIC_DRAW);
	// Color attribute
	GLint color_attribute = glGetAttribLocation(shader, "color");
	glVertexAttribPointer(color_attribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(color_attribute);
	delete[] colors1;

	//------------------------------------------------

	glGenBuffers(2, vertexBuffers);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[ModelNo]);
	//glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), vertices, GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, geometry[ModelNo].vertexCount()*3 * sizeof(float), (float*)geometry[ModelNo].vertexData(), GL_STATIC_DRAW);
	glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(vertexLoc);
	//model centred on the origin in world space - translate
	//get centre of model and move to origin
	Model[0] = glm::translate(Model[0], glm::vec3(geometry[ModelNo].getCentreX(), geometry[ModelNo].getCentreY(), geometry[ModelNo].getCentreZ()));
	++ModelNo;

	geometry[ModelNo].loadFromOBJFile("../objects/teapot.obj");
	glBindVertexArray(vaos[ModelNo]);
	//------------------------------------------------
	//adding color to model 2
	const int colorSize2 = geometry[ModelNo].vertexCount() * 3 * 3;
	float *colors2 = new float[colorSize2];

	// Initialize the random seed from the system time
	srand(time(NULL));
	// Fill colors with random numbers from 0 to 1, use continuous polynomials for r,g,b:
	int j = 0;
	for (int s = 0; s < colorSize2 / 3; ++s) {
		float t = (float)rand() / (float)RAND_MAX;
		colors2[j] = 9 * (1 - t)*t*t*t;
		j++;
		colors2[j] = 15 * (1 - t)*(1 - t)*t*t;
		j++;
		colors2[j] = 8.5*(1 - t)*(1 - t)*(1 - t)*t;
		j++;
	}

	glBindBuffer(GL_ARRAY_BUFFER, colorID[ModelNo]);
	glBufferData(GL_ARRAY_BUFFER, colorSize2 * sizeof(float), &colors2[0], GL_STATIC_DRAW);
	glVertexAttribPointer(color_attribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(color_attribute);
	delete[] colors2;

	//------------------------------------------------
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[ModelNo]);
	glBufferData(GL_ARRAY_BUFFER, geometry[ModelNo].vertexCount() * 3 * sizeof(float), (float*)geometry[ModelNo].vertexData(), GL_STATIC_DRAW);
	glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, false, 0, 0);
	glEnableVertexAttribArray(vertexLoc);
	++ModelNo;

	int windowWidth, windowHeight;
	SDL_GetWindowSize(sdlWin, &windowWidth, &windowHeight);
	SDL_WarpMouseInWindow(NULL,windowWidth/2, windowHeight/2);

	glPrintError("Setup complete", true);
}

void OpenGLWindow::render()
{
	LAST = NOW;
	NOW = SDL_GetPerformanceCounter();
	deltaTime = (NOW - LAST) * 1000 / (double)SDL_GetPerformanceFrequency();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	Model[1] = glm::translate(Model[0], glm::vec3(-geometry[0].getCentreX() + geometry[1].getCentreX(), -geometry[0].getCentreY()+ geometry[1].getCentreY(), -geometry[0].getCentreZ() + geometry[1].getCentreZ()+((geometry[0].getMaxZ() - geometry[0].getMinZ()) / 2)+((geometry[1].getMaxZ()-geometry[1].getMinZ())/2)));
	for (int i = 0;i < NoLoadedModels;++i) {
		// Our ModelViewProjection : multiplication of our 3 matrices
		glm::mat4 mvp = Projection * View * Model[i];

		// Send our transformation to the currently bound shader, in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);
		glBindVertexArray(vaos[i]);
		glDrawArrays(GL_TRIANGLES, 0, geometry[i].vertexCount());
	}

	//glDrawArrays(GL_POINTS, 0, geometry.vertexCount());
	// Swap the front and back buffers on the window, effectively putting what we just "drew"
	// onto the screen (whereas previously it only existed in memory)
	SDL_GL_SwapWindow(sdlWin);
}

// The program will exit if this function returns false
bool OpenGLWindow::handleEvent(SDL_Event e)
{
	// handles mouse and keyboard inputs
	switch (e.type) {

		//key pressed: to switch between different modes
		case SDL_KEYDOWN:
			switch (e.key.keysym.sym) {
				//'ESC' to quit
				case SDLK_ESCAPE:
					return false;

				//'T' to switch between planes
				case SDLK_t:
					cout << "Translate Mode" << endl;
					objmode = TRANSLATE;
					//set first call to relative mouse state to mark start position
					SDL_GetRelativeMouseState(NULL, NULL);
					switch (tplane) {
						case XY:
							tplane = XZ;
							cout << "Translating on XZ-plane..." << endl;
							break;
						case XZ:
							tplane = XY;
							cout << "Translating on XY-plane..." << endl;
							break;
						default:
							break;
					}
					break;

				//'R' for to switch between rotation axes
				case SDLK_r:
					cout << "Rotate Mode" << endl;
					objmode = ROTATE;
					switch (rotaxis) {
						case XAXIS:
							rotaxis = YAXIS;
							cout << "Rotating around Y-axis..." << endl;
							break;
						case YAXIS:
							rotaxis = ZAXIS;
							cout << "Rotating around Z-axis..." << endl;
							break;
						case ZAXIS:
							rotaxis = XAXIS;
							cout << "Rotating around X-axis..." << endl;
							break;
						default:
							break;
					}
					break;

				//'S' for scale
				case SDLK_s:
					cout << "Scale Mode" << endl;
					objmode = SCALE;
					break;

				case SDLK_SPACE:
					cout << "No Mode" << endl;
					objmode = NONE;
					break;

				case SDLK_l:
					cout << "Resetting all transformations of first model..." << endl;
					Model[0] = glm::translate(glm::mat4(1.0f), glm::vec3(geometry[0].getCentreX(), geometry[0].getCentreY(), geometry[0].getCentreZ()));
					cout << "Loading second model..." << endl;
					++NoLoadedModels;
					break;

				case SDLK_c:
					cout << "Changning colors..." << endl;
					//changing colors of both models
					for (int k = 0;k < NoLoadedModels;++k) {
						const int colorSize = geometry[k].vertexCount() * 3 * 3;
						float *colors = new float[colorSize];

						// Initialize the random seed from the system time
						srand(time(NULL));
						// Fill colors with random numbers from 0 to 1, use continuous polynomials for r,g,b:
						int l = 0;
						for (int i = 0; i < colorSize / 3; ++i) {
							float t = (float)rand() / (float)RAND_MAX;
							colors[l] = 9 * (1 - t)*t*t*t;
							l++;
							colors[l] = 15 * (1 - t)*(1 - t)*t*t;
							l++;
							colors[l] = 8.5*(1 - t)*(1 - t)*(1 - t)*t;
							l++;
						}
						glBindBuffer(GL_ARRAY_BUFFER, colorID[k]);
						glBufferData(GL_ARRAY_BUFFER, colorSize * sizeof(float), &colors[0], GL_STATIC_DRAW);
						// Color attribute
						//GLint color_attribute = glGetAttribLocation(shader, "color");
						//glVertexAttribPointer(color_attribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
						//glEnableVertexAttribArray(color_attribute);
						delete[] colors;
					}


				default:
					break;
			}
			break;

		//mouse moved: move movement moves model on plane for translation
		case SDL_MOUSEMOTION:
			if (objmode == TRANSLATE) {
				//get difference in mouse positions of mouse movement
				int mouseX, mouseY;
				SDL_GetRelativeMouseState(&mouseX, &mouseY);
				float translateX = translateSpeed *deltaTime*mouseX;
				float translateY = translateSpeed *deltaTime*mouseY;
				switch (tplane) {
				case XY:
					Model[0] = glm::translate(Model[0], glm::vec3(translateX, -translateY, 0.0f));
					break;
				case XZ:
					Model[0] = glm::translate(Model[0], glm::vec3(translateX, 0.0f, translateY));
					break;
				default:
					break;
				}

			}
			break;

		//mouse button pressed: right click rotate right, left click rotate left for rotation
		case SDL_MOUSEBUTTONDOWN:
			if (objmode == ROTATE) {
				float fRotate = 0.0f;
				if (e.button.button == SDL_BUTTON_LEFT) {
					fRotate = -10.0f;
				}
				else if (e.button.button == SDL_BUTTON_RIGHT) {
					fRotate = 10.0f;
				}
				switch (rotaxis) {
				case XAXIS:
					Model[0] = glm::rotate(Model[0], glm::radians(fRotate), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case YAXIS:
					Model[0] = glm::rotate(Model[0], glm::radians(fRotate), glm::vec3(0.0f, 1.0f, 0.0f));
					break;
				case ZAXIS:
					Model[0] = glm::rotate(Model[0], glm::radians(fRotate), glm::vec3(0.0f, 0.0f, 1.0f));
					break;
				default:
					break;
				}
			}
			break;

		//mouse wheel motion: mouse wheel scroll up scale up, scroll down scale down for scale
		case SDL_MOUSEWHEEL:
			if (objmode == SCALE) {
				//scroll up
				if (e.wheel.y > 0) {
					Model[0] = glm::scale(Model[0], glm::vec3(1.2f,1.2f,1.2f));
				}
				//scroll down
				else if (e.wheel.y < 0) {
					Model[0] = glm::scale(Model[0], glm::vec3(0.8f, 0.8f, 0.8f));
				}

			}
			break;

		default:
			break;
	}

	return true;
}

void OpenGLWindow::cleanup()
{
	glDeleteBuffers(2, vertexBuffers);
	glDeleteVertexArrays(2, vaos);
	SDL_DestroyWindow(sdlWin);
}
