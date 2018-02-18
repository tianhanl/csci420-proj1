/*
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

  The way to change the vao is correct, but there is problem in reading the points for lines.
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
GLuint pointBuffer;
GLuint pointVAO;

GLuint lineBuffer;
GLuint lineVAO;

GLuint triangleBuffer;
GLuint triangleVAO;

const int pointMode = 0;
const int lineMode = 1;
const int triangleMode = 2;

// set default mode as point mode
int currMode = pointMode;

OpenGLMatrix *matrix;

GLfloat theta[3] = {0.0, 0.0, 0.0};

BasicPipelineProgram *pipelineProgram;

// vertexes for points
float *pointPositions;
int pointPositionsSize;
float *pointColors;
int pointColorsSize;

// vertexes for lines
float *linePositions;
int linePositionsSize;
float *lineColors;
int lineColorsSize;

// vertexes for triangles
float *trianglePositions;
int trianglePositionsSize;
float *triangleColors;
int triangleColorsSize;

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
  // bind pointBuffer, so that glVertexAttribPointer refers to the corerect pointBuffer
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
}

void renderPoints()
{
  GLint first = 0;
  GLsizei numberOfVertices = pointPositionsSize / 3;
  glDrawArrays(GL_POINTS, first, numberOfVertices);
  glBindVertexArray(0);
}

void renderLines()
{
  GLint first = 0;
  GLsizei numberOfVertices = linePositionsSize / 3;
  glDrawArrays(GL_LINES, first, numberOfVertices);
  glBindVertexArray(0);
}

void renderTriangles()
{
  GLint first = 0;
  GLsizei numberOfVertices = trianglePositionsSize / 3;
  glDrawArrays(GL_TRIANGLES, first, numberOfVertices);
  glBindVertexArray(0);
}

void displayFunc()
{
  // render some stuff...
  // the steps to display is to
  // clear -> action -> draw -> swap (pointBuffer)
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
    glBindVertexArray(pointVAO);
    renderPoints();
    break;
  case lineMode:
    glBindVertexArray(lineVAO);
    renderLines();
    break;
  case triangleMode:
    glBindVertexArray(triangleVAO);
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
    delete[] pointPositions;
    delete[] pointColors;
    delete[] linePositions;
    delete[] lineColors;
    delete[] triangleColors;
    delete[] trianglePositions;
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

// initialization for the vertex pointBuffer object
void initVBO()
{
  // vbo for points
  glGenBuffers(1, &pointBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
  glBufferData(GL_ARRAY_BUFFER, pointPositionsSize * sizeof(float) + pointColorsSize * sizeof(float), NULL, GL_STATIC_DRAW);
  // upload position data
  glBufferSubData(GL_ARRAY_BUFFER, 0, pointPositionsSize * sizeof(float), pointPositions);
  // upload color data
  glBufferSubData(GL_ARRAY_BUFFER, pointPositionsSize * sizeof(float), pointColorsSize * sizeof(float), pointColors);

  // vbo for lines
  glGenBuffers(1, &lineBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, lineBuffer);
  glBufferData(GL_ARRAY_BUFFER, linePositionsSize * sizeof(float) + lineColorsSize * sizeof(float), NULL, GL_STATIC_DRAW);
  // upload position data
  glBufferSubData(GL_ARRAY_BUFFER, 0, linePositionsSize * sizeof(float), linePositions);
  // upload color data
  glBufferSubData(GL_ARRAY_BUFFER, linePositionsSize * sizeof(float), lineColorsSize * sizeof(float), lineColors);

  // vbo for triangles
  glGenBuffers(1, &triangleBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, triangleBuffer);
  glBufferData(GL_ARRAY_BUFFER, trianglePositionsSize * sizeof(float) + triangleColorsSize * sizeof(float), NULL, GL_STATIC_DRAW);
  // upload position data
  glBufferSubData(GL_ARRAY_BUFFER, 0, trianglePositionsSize * sizeof(float), trianglePositions);
  // upload color data
  glBufferSubData(GL_ARRAY_BUFFER, trianglePositionsSize * sizeof(float), triangleColorsSize * sizeof(float), triangleColors);
  cout << "triangle finished" << endl;
}

// initialization for the pipeline (shaders, etc.)
void initPipelineProgram()
{
  // initialize shader pipeline program
  pipelineProgram = new BasicPipelineProgram();
  pipelineProgram->Init("../openGLHelper-starterCode");
  pipelineProgram->Bind();
}

void initpointVAO()
{

  GLuint program = pipelineProgram->GetProgramHandle();
  GLsizei stride = 0;
  GLboolean normalized = GL_FALSE;

  glGenVertexArrays(1, &pointVAO);
  glBindVertexArray(pointVAO);
  // points
  glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
  GLuint loc = glGetAttribLocation(program, "position");
  glEnableVertexAttribArray(loc);
  const void *offset = (const void *)0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
  GLuint loc2 = glGetAttribLocation(program, "color");
  glEnableVertexAttribArray(loc2);
  offset = (const void *)(pointPositionsSize * sizeof(float));
  glVertexAttribPointer(loc2, 4, GL_FLOAT, normalized, stride, offset);
  glBindVertexArray(0);
  // lines
  glGenVertexArrays(1, &lineVAO);
  glBindVertexArray(lineVAO);
  glBindBuffer(GL_ARRAY_BUFFER, lineBuffer);
  glEnableVertexAttribArray(loc);
  offset = (const void *)0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
  glEnableVertexAttribArray(loc2);
  offset = (const void *)(linePositionsSize * sizeof(float));
  glVertexAttribPointer(loc2, 4, GL_FLOAT, normalized, stride, offset);
  glBindVertexArray(0);

  // triangles
  glGenVertexArrays(1, &triangleVAO);
  glBindVertexArray(triangleVAO);
  glBindBuffer(GL_ARRAY_BUFFER, triangleBuffer);
  glEnableVertexAttribArray(loc);
  offset = (const void *)0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
  glEnableVertexAttribArray(loc2);
  offset = (const void *)(trianglePositionsSize * sizeof(float));
  glVertexAttribPointer(loc2, 4, GL_FLOAT, normalized, stride, offset);
  glBindVertexArray(0);
}

int pointIsAt(int x, int y, int height, int span)
{
  return x * height * span + y * span;
}

float calculateGradient(float fromVal, float toVal, float fraction)
{
  return fromVal + (toVal - fromVal) * fraction;
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

  pointPositionsSize = height * width * 3;
  pointColorsSize = height * width * 4;

  // init image
  pointPositions = new float[pointPositionsSize];
  pointColors = new float[pointColorsSize];

  // get all the points
  float scale = 0.2;
  for (int i = 0; i < width; i++)
  {
    int offset = i * height * 3;
    for (int j = 0; j < height; j++)
    {
      pointPositions[offset + j * 3] = (float)i;
      pointPositions[offset + j * 3 + 1] = (float)(scale * heightmapImage->getPixel(i, j, 0));
      pointPositions[offset + j * 3 + 2] = (float)-j;
    }
  }

  for (int i = 0; i < width; i++)
  {
    int offset = i * height * 4;
    for (int j = 0; j < height; j++)
    {
      // 51 8 103
      float fraction = heightmapImage->getPixel(i, j, 0) / 255.0;
      pointColors[offset + j * 4] = calculateGradient(67, 24, fraction) / 255.0;
      pointColors[offset + j * 4 + 1] = calculateGradient(206, 90, fraction) / 255.0;
      pointColors[offset + j * 4 + 2] = calculateGradient(162, 157, fraction) / 255.0;
      pointColors[offset + j * 4 + 3] = fraction;
    }
  }

  // craete lines vertices from poitns
  cout << "Begin get lines" << endl;
  linePositionsSize = ((height - 1) * width + height * (width - 1)) * 2 * 3;
  lineColorsSize = ((height - 1) * width + height * (width - 1)) * 2 * 4;
  linePositions = new float[linePositionsSize];
  lineColors = new float[lineColorsSize];

  // set up positions
  int currLineAt = 0;
  // vertical lines
  for (int i = 0; i < width; i++)
  {
    for (int j = 0; j < height - 1; j++)
    {
      int pointALoc = pointIsAt(i, j, height, 3);
      linePositions[currLineAt] = pointPositions[pointALoc];
      linePositions[currLineAt + 1] = pointPositions[pointALoc + 1];
      linePositions[currLineAt + 2] = pointPositions[pointALoc + 2];
      linePositions[currLineAt + 3] = pointPositions[pointALoc + 3];
      linePositions[currLineAt + 4] = pointPositions[pointALoc + 4];
      linePositions[currLineAt + 5] = pointPositions[pointALoc + 5];
      currLineAt += 6;
    }
  }
  cout << "Finish vertical lines" << endl;
  // horizontal lines
  for (int i = 0; i < width - 1; i++)
  {
    for (int j = 0; j < height; j++)
    {
      int pointALoc = pointIsAt(i, j, height, 3);
      linePositions[currLineAt] = pointPositions[pointALoc];
      linePositions[currLineAt + 1] = pointPositions[pointALoc + 1];
      linePositions[currLineAt + 2] = pointPositions[pointALoc + 2];
      int pointBLoc = pointIsAt(i + 1, j, height, 3);
      linePositions[currLineAt + 3] = pointPositions[pointBLoc];
      linePositions[currLineAt + 4] = pointPositions[pointBLoc + 1];
      linePositions[currLineAt + 5] = pointPositions[pointBLoc + 2];
      currLineAt += 6;
    }
  }
  cout << "Finish get positions" << endl;
  // set up colors
  currLineAt = 0;
  for (int i = 0; i < width; i++)
  {
    for (int j = 0; j < height - 1; j++)
    {
      int pointALoc = pointIsAt(i, j, height, 4);
      lineColors[currLineAt] = pointColors[pointALoc];
      lineColors[currLineAt + 1] = pointColors[pointALoc + 1];
      lineColors[currLineAt + 2] = pointColors[pointALoc + 2];
      lineColors[currLineAt + 3] = pointColors[pointALoc + 3];
      lineColors[currLineAt + 4] = pointColors[pointALoc + 4];
      lineColors[currLineAt + 5] = pointColors[pointALoc + 5];
      lineColors[currLineAt + 6] = pointColors[pointALoc + 6];
      lineColors[currLineAt + 7] = pointColors[pointALoc + 7];
      currLineAt += 8;
    }
  }
  for (int i = 0; i < width - 1; i++)
  {
    for (int j = 0; j < height; j++)
    {
      int pointALoc = pointIsAt(i, j, height, 4);
      lineColors[currLineAt] = pointColors[pointALoc];
      lineColors[currLineAt + 1] = pointColors[pointALoc + 1];
      lineColors[currLineAt + 2] = pointColors[pointALoc + 2];
      lineColors[currLineAt + 3] = pointColors[pointALoc + 3];
      int pointBLoc = pointIsAt(i + 1, j, height, 4);
      lineColors[currLineAt + 4] = pointColors[pointBLoc];
      lineColors[currLineAt + 5] = pointColors[pointBLoc + 1];
      lineColors[currLineAt + 6] = pointColors[pointBLoc + 2];
      lineColors[currLineAt + 7] = pointColors[pointBLoc + 3];
      currLineAt += 8;
    }
  }

  cout << "Begin get triangles" << endl;
  trianglePositionsSize = (width - 1) * (height - 1) * 6 * 3;
  triangleColorsSize = (width - 1) * (height - 1) * 6 * 4;
  trianglePositions = new float[trianglePositionsSize];
  triangleColors = new float[triangleColorsSize];

  currLineAt = 0;
  for (int i = 0; i < width - 1; i++)
  {
    for (int j = 0; j < height - 1; j++)
    {
      int locPointBottomLeft = pointIsAt(i, j, height, 3);
      int locPointBottomRight = pointIsAt(i + 1, j, height, 3);
      int locPointTopLeft = pointIsAt(i, j + 1, height, 3);
      int locPointTopRight = pointIsAt(i + 1, j + 1, height, 3);

      // add bottom left triangle
      trianglePositions[currLineAt] = pointPositions[locPointBottomLeft];
      trianglePositions[currLineAt + 1] = pointPositions[locPointBottomLeft + 1];
      trianglePositions[currLineAt + 2] = pointPositions[locPointBottomLeft + 2];
      currLineAt += 3;
      trianglePositions[currLineAt] = pointPositions[locPointBottomRight];
      trianglePositions[currLineAt + 1] = pointPositions[locPointBottomRight + 1];
      trianglePositions[currLineAt + 2] = pointPositions[locPointBottomRight + 2];
      currLineAt += 3;
      trianglePositions[currLineAt] = pointPositions[locPointTopLeft];
      trianglePositions[currLineAt + 1] = pointPositions[locPointTopLeft + 1];
      trianglePositions[currLineAt + 2] = pointPositions[locPointTopLeft + 2];
      currLineAt += 3;

      // add top right triangle
      trianglePositions[currLineAt] = pointPositions[locPointTopLeft];
      trianglePositions[currLineAt + 1] = pointPositions[locPointTopLeft + 1];
      trianglePositions[currLineAt + 2] = pointPositions[locPointTopLeft + 2];
      currLineAt += 3;
      trianglePositions[currLineAt] = pointPositions[locPointTopRight];
      trianglePositions[currLineAt + 1] = pointPositions[locPointTopRight + 1];
      trianglePositions[currLineAt + 2] = pointPositions[locPointTopRight + 2];
      currLineAt += 3;
      trianglePositions[currLineAt] = pointPositions[locPointBottomRight];
      trianglePositions[currLineAt + 1] = pointPositions[locPointBottomRight + 1];
      trianglePositions[currLineAt + 2] = pointPositions[locPointBottomRight + 2];
      currLineAt += 3;
    }
  }

  currLineAt = 0;
  for (int i = 0; i < width - 1; i++)
  {
    for (int j = 0; j < height - 1; j++)
    {
      int locPointBottomLeft = pointIsAt(i, j, height, 4);
      int locPointBottomRight = pointIsAt(i + 1, j, height, 4);
      int locPointTopLeft = pointIsAt(i, j + 1, height, 4);
      int locPointTopRight = pointIsAt(i + 1, j + 1, height, 4);

      // add bottom left triangle
      triangleColors[currLineAt] = pointColors[locPointBottomLeft];
      triangleColors[currLineAt + 1] = pointColors[locPointBottomLeft + 1];
      triangleColors[currLineAt + 2] = pointColors[locPointBottomLeft + 2];
      triangleColors[currLineAt + 3] = pointColors[locPointBottomLeft + 3];
      currLineAt += 4;
      triangleColors[currLineAt] = pointColors[locPointBottomRight];
      triangleColors[currLineAt + 1] = pointColors[locPointBottomRight + 1];
      triangleColors[currLineAt + 2] = pointColors[locPointBottomRight + 2];
      triangleColors[currLineAt + 3] = pointColors[locPointBottomRight + 3];
      currLineAt += 4;
      triangleColors[currLineAt] = pointColors[locPointTopLeft];
      triangleColors[currLineAt + 1] = pointColors[locPointTopLeft + 1];
      triangleColors[currLineAt + 2] = pointColors[locPointTopLeft + 2];
      triangleColors[currLineAt + 3] = pointColors[locPointTopLeft + 3];
      currLineAt += 4;

      // add top right triangle
      triangleColors[currLineAt] = pointColors[locPointTopLeft];
      triangleColors[currLineAt + 1] = pointColors[locPointTopLeft + 1];
      triangleColors[currLineAt + 2] = pointColors[locPointTopLeft + 2];
      triangleColors[currLineAt + 3] = pointColors[locPointTopLeft + 3];
      currLineAt += 4;
      triangleColors[currLineAt] = pointColors[locPointTopRight];
      triangleColors[currLineAt + 1] = pointColors[locPointTopRight + 1];
      triangleColors[currLineAt + 2] = pointColors[locPointTopRight + 2];
      triangleColors[currLineAt + 3] = pointColors[locPointTopRight + 3];
      currLineAt += 4;
      triangleColors[currLineAt] = pointColors[locPointBottomRight];
      triangleColors[currLineAt + 1] = pointColors[locPointBottomRight + 1];
      triangleColors[currLineAt + 2] = pointColors[locPointBottomRight + 2];
      triangleColors[currLineAt + 3] = pointColors[locPointBottomRight + 3];
      currLineAt += 4;
    }
  }
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  cout << "inital color cleared" << endl;
  // do additional initialization here...
  glEnable(GL_DEPTH_TEST);
  matrix = new OpenGLMatrix();
  cout << "matrix created" << endl;
  initVBO();
  initPipelineProgram();
  initpointVAO();
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
