﻿/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields
  C++ starter code

  Student username: tianhanl
*/

/*
  Notice:
  Users may change how the height map was rendered as follows:
  p : points mode
  l : lines mode
  t : triangles mode
*/

#include <iostream>
#include <cstring>
#include "openGLHeader.h"
#include "glutHeader.h"

#include "imageIO.h"
#include "openGLMatrix.h"
#include "basicPipelineProgram.h"

#ifdef WIN32
#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#endif

#ifdef WIN32
char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0;   // 1 if pressed, 0 if not
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0;  // 1 if pressed, 0 if not

typedef enum { ROTATE,
               TRANSLATE,
               SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = {0.0f, 0.0f, 0.0f};
float landTranslate[3] = {0.0f, 0.0f, 0.0f};
float landScale[3] = {1.0f, 1.0f, 1.0f};

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

ImageIO *heightmapImage;

// properties for OpenGL
GLuint buffer;
GLuint vao;

const int pointMode = 0;
const int lineMode = 1;
const int triangleMode = 2;

// set default mode as point mode
int currMode = pointMode;

OpenGLMatrix *matrix;

GLfloat theta[3] = {0.0, 0.0, 0.0};

BasicPipelineProgram *pipelineProgram;

// temporary vertexes for a triangle
float *positions;
int sizePositions;

float *colors;
int sizeColors;

// write a screenshot to the specified filename
void saveScreenshot(const char *filename)
{
  unsigned char *screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else
    cout << "Failed to save file " << filename << '.' << endl;

  delete[] screenshotData;
}

void bindProgram()
{
  // bind buffer, so that glVertexAttribPointer refers to the corerect buffer
  // get a handle to the program
  GLuint program = pipelineProgram->GetProgramHandle();
  GLint h_modelViewMatrix = glGetUniformLocation(program, "modelViewMatrix");
  GLint h_projectionMatrix =
      glGetUniformLocation(program, "projectionMatrix");
  const GLboolean isRowMajor = false;

  float m[16];
  matrix->GetMatrix(m);
  glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);

  matrix->SetMatrixMode(OpenGLMatrix::Projection);
  float p[16]; // column-major
  matrix->GetMatrix(p);
  glUniformMatrix4fv(h_projectionMatrix, 1, isRowMajor, p);

  pipelineProgram->Bind();
  glBindVertexArray(vao);
}

void renderPoints()
{
  GLint first = 0;
  GLsizei numberOfVertices = sizePositions / 3;
  glDrawArrays(GL_POINTS, first, numberOfVertices);
  glBindVertexArray(0);
}

void renderLines()
{
  GLint first = 0;
  GLsizei numberOfVertices = sizePositions / 3;
  glDrawArrays(GL_LINES, first, numberOfVertices);
  glBindVertexArray(0);
}

void renderTriangles()
{
  GLint first = 0;
  GLsizei numberOfVertices = sizePositions / 3;
  glDrawArrays(GL_TRIANGLES, first, numberOfVertices);
  glBindVertexArray(0);
}

void displayFunc()
{
  // render some stuff...
  // the steps to display is to
  // clear -> action -> draw -> swap (buffer)
  glClear(GL_COLOR_BUFFER_BIT |
          GL_DEPTH_BUFFER_BIT);
  matrix->SetMatrixMode(OpenGLMatrix::ModelView);
  matrix->LoadIdentity();
  matrix->LookAt(125, 500, -120, 125, 0, -125, 0, 1, 0);
  matrix->Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
  matrix->Rotate(landRotate[0], 1.0, 0.0, 0.0);
  matrix->Rotate(landRotate[1], 0.0, 1.0, 0.0);
  matrix->Rotate(landRotate[2], 0.0, 0.0, 1.0);
  matrix->Scale(landScale[0], landScale[1], landScale[2]);
  bindProgram();
  // change how the program render the vertices
  switch (currMode)
  {
  case pointMode:
    renderPoints();
    break;
  case lineMode:
    renderLines();
    break;
  case triangleMode:
    renderTriangles();
    break;
  default:
    renderPoints();
    break;
  }
  glutSwapBuffers();
}

void idleFunc()
{
  // do some stuff...

  // for example, here, you can save the screenshots to disk (to make the animation)

  // make the screen update
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  // setup perspective matrix...
  GLfloat aspect = (GLfloat)w / (GLfloat)h;
  matrix->SetMatrixMode(OpenGLMatrix::Projection);
  matrix->LoadIdentity();
  // The camera must be pointing in the negative-z direction, and use
  // the perspective view: aspect ratio=1280:720, field of view = 45 degrees.
  matrix->Perspective(45, (1.0 * 1280) / (1.0 * 720), 0.01, 1000.0);
  matrix->SetMatrixMode(OpenGLMatrix::ModelView);
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = {x - mousePos[0], y - mousePos[1]};

  switch (controlState)
  {
  // translate the landscape
  case TRANSLATE:
    if (leftMouseButton)
    {
      // control x,y translation via the left mouse button
      landTranslate[0] += mousePosDelta[0] * 0.01f;
      landTranslate[1] -= mousePosDelta[1] * 0.01f;
    }
    if (middleMouseButton)
    {
      // control z translation via the middle mouse button
      landTranslate[2] += mousePosDelta[1] * 0.01f;
    }
    break;

  // rotate the landscape
  case ROTATE:
    if (leftMouseButton)
    {
      // control x,y rotation via the left mouse button
      landRotate[0] += mousePosDelta[1];
      landRotate[1] += mousePosDelta[0];
    }
    if (middleMouseButton)
    {
      // control z rotation via the middle mouse button
      landRotate[2] += mousePosDelta[1];
    }
    break;

  // scale the landscape
  case SCALE:
    if (leftMouseButton)
    {
      // control x,y scaling via the left mouse button
      landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
      landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
    }
    if (middleMouseButton)
    {
      // control z scaling via the middle mouse button
      landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
    }
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed

  // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
  switch (button)
  {
  case GLUT_LEFT_BUTTON:
    leftMouseButton = (state == GLUT_DOWN);
    break;

  case GLUT_MIDDLE_BUTTON:
    middleMouseButton = (state == GLUT_DOWN);
    break;

  case GLUT_RIGHT_BUTTON:
    rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // keep track of whether CTRL and SHIFT keys are pressed
  switch (glutGetModifiers())
  {
  case GLUT_ACTIVE_CTRL:
    controlState = TRANSLATE;
    break;

  case GLUT_ACTIVE_SHIFT:
    controlState = SCALE;
    break;

  // if CTRL and SHIFT are not pressed, we are in rotate mode
  default:
    controlState = ROTATE;
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
  case 27: // ESC key
    delete[] positions;
    delete[] colors;
    exit(0); // exit the program
    break;

  case ' ':
    cout << "You pressed the spacebar." << endl;
    break;

  case 'x':
    // take a screenshot
    saveScreenshot("screenshot.jpg");
    break;
  case 'p':
    currMode = pointMode;
    break;
  case 'l':
    currMode = lineMode;
    break;
  case 't':
    currMode = triangleMode;
    break;
  }
}

// initialization for the vertex buffer object
void initVBO()
{
  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, sizePositions * sizeof(float) + sizeColors * sizeof(float), NULL, GL_STATIC_DRAW);
  cout << "buffered binded" << endl;
  // upload position data
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizePositions * sizeof(float), positions);
  cout << "uploaded position data" << endl;
  // upload color data
  glBufferSubData(GL_ARRAY_BUFFER, sizePositions * sizeof(float), sizeColors * sizeof(float), colors);
  cout << "uploaded color data" << endl;
}

// initialization for the pipeline (shaders, etc.)
void initPipelineProgram()
{
  // initialize shader pipeline program
  pipelineProgram = new BasicPipelineProgram();
  pipelineProgram->Init("../openGLHelper-starterCode");
  pipelineProgram->Bind();
}

void initVAO()
{
  GLuint program = pipelineProgram->GetProgramHandle();
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  GLsizei stride = 0;
  GLboolean normalized = GL_FALSE;

  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  GLuint loc = glGetAttribLocation(program, "position");
  glEnableVertexAttribArray(loc);
  const void *offset = (const void *)0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

  GLuint loc2 = glGetAttribLocation(program, "color");
  glEnableVertexAttribArray(loc2);
  offset = (const void *)(sizePositions * sizeof(float));
  glVertexAttribPointer(loc2, 4, GL_FLOAT, normalized, stride, offset);

  // write projection and modelview matrix to shader
  glBindVertexArray(0);
}

void initScene(int argc, char *argv[])
{
  // load the image from a jpeg disk file to main memory
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }

  int height = heightmapImage->getHeight();
  int width = heightmapImage->getWidth();

  sizePositions = height * width * 3;
  sizeColors = height * width * 4;

  // init image
  positions = new float[sizePositions];
  colors = new float[sizeColors];
  // {{0.0, 0.0, -1.0}, {1.0, 0.0, -1.0}, {0.0, 1.0, -1.0}};
  // {{1.0, 0.0, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}

  float scale = 0.5;

  for (int i = 0; i < width; i++)
  {
    int offset = i * height * 3;
    for (int j = 0; j < height; j++)
    {
      positions[offset + j * 3] = (float)i;
      positions[offset + j * 3 + 1] = (float)(scale * heightmapImage->getPixel(i, j, 0));
      positions[offset + j * 3 + 2] = (float)-j;
    }
  }

  for (int i = 0; i < width; i++)
  {
    int offset = i * height * 4;
    for (int j = 0; j < height; j++)
    {
      colors[offset + j * 4] = 1.0;
      colors[offset + j * 4 + 1] = 1.0;
      colors[offset + j * 4 + 2] = 1.0;
      colors[offset + j * 4 + 3] = 1.0;
    }
  }
  cout << positions[sizePositions - 3] << endl;
  cout << positions[sizePositions - 2] << endl;
  cout << positions[sizePositions - 1] << endl;

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  cout << "inital color cleared" << endl;
  // do additional initialization here...
  glEnable(GL_DEPTH_TEST);
  matrix = new OpenGLMatrix();
  cout << "matrix created" << endl;
  initVBO();
  initPipelineProgram();
  initVAO();
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc, argv);

  cout << "Initializing OpenGL..." << endl;

#ifdef __APPLE__
  glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#else
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  // tells glut to use a particular display function to redraw
  glutDisplayFunc(displayFunc);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

// init glew
#ifdef __APPLE__
  // nothing is needed on Apple
#else
  // Windows, Linux
  GLint result = glewInit();
  if (result != GLEW_OK)
  {
    cout << "error: " << glewGetErrorString(result) << endl;
    exit(EXIT_FAILURE);
  }
#endif

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}
