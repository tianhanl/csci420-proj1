/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields
  C++ starter code


  Student username: Tianhang Liu
*/

/*
  Notice:
  Users may change how the height map was rendered as follows:
  p : points mode
  l : lines mode
  t : triangles mode

  The way to change the vao is correct, but there is problem in reading the points for lines.
>>>>>>> hw1-finished
*/

#include <iostream>
#include <cstring>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <vector>
#include "openGLHeader.h"
#include "imageIO.h"
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
vector<float> pos;
vector<float> uvs;
// This constant will determine how much segements there will be between two points
float USTEP = 0.001;
int currentFrameNumber = 0;
bool needAnimate = false;
// variabels for hw2
// represents one control point along the spline
struct Point
{
  double x;
  double y;
  double z;
};
// spline struct
// contains how many control points the spline has, and an array of control points
struct Spline
{
  int numControlPoints;
  Point *points;
};
// the spline array
Spline *splines;
// total number of splines
int numSplines;
float s = 0.5;
float basis[4][4] = {{-s, 2 - s, s - 2, s}, {2 * s, s - 3, 3 - 2 * s, -s}, {-s, 0, s, 0}, {0, 1, 0, 0}};
// Variables for textures
GLuint groundTextureHandle;
// Variables for ground
vector<float> groundPos;
vector<float> groundUVs;
GLuint groundBuffer;
GLuint groundVao;

GLuint program;

int loadSplines(char *argv)
{
  char *cName = (char *)malloc(128 * sizeof(char));
  FILE *fileList;
  FILE *fileSpline;
  int iType, i = 0, j, iLength;
  uvs.push_back(1.0);

  // load the track file
  fileList = fopen(argv, "r");
  if (fileList == NULL)
  {
    printf("can't open file\n");
    exit(1);
  }

  // stores the number of splines in a global variable
  fscanf(fileList, "%d", &numSplines);

  splines = (Spline *)malloc(numSplines * sizeof(Spline));

  // reads through the spline files
  for (j = 0; j < numSplines; j++)
  {
    i = 0;
    fscanf(fileList, "%s", cName);
    fileSpline = fopen(cName, "r");

    if (fileSpline == NULL)
    {
      printf("can't open file\n");
      exit(1);
    }

    // gets length for spline file
    fscanf(fileSpline, "%d %d", &iLength, &iType);

    // allocate memory for all the points
    splines[j].points = (Point *)malloc(iLength * sizeof(Point));
    splines[j].numControlPoints = iLength;

    // saves the data to the struct
    while (fscanf(fileSpline, "%lf %lf %lf",
                  &splines[j].points[i].x,
                  &splines[j].points[i].y,
                  &splines[j].points[i].z) != EOF)
    {
      i++;
    }
  }

  free(cName);

  return 0;
}

Point calculateSpline(float u, Point controlPoint1, Point controlPoint2, Point controlPoint3, Point controlPoint4)
{
  Point output;
  float middleMatrix[4][3];
  for (int i = 0; i < 4; i++)
  {
    middleMatrix[i][0] = basis[i][0] * controlPoint1.x + basis[i][1] * controlPoint2.x + basis[i][2] * controlPoint3.x + basis[i][3] * controlPoint4.x;
    middleMatrix[i][1] = basis[i][0] * controlPoint1.y + basis[i][1] * controlPoint2.y + basis[i][2] * controlPoint3.y + basis[i][3] * controlPoint4.y;
    middleMatrix[i][2] = basis[i][0] * controlPoint1.z + basis[i][1] * controlPoint2.z + basis[i][2] * controlPoint3.z + basis[i][3] * controlPoint4.z;
  }
  float indexes[4] = {pow(u, 3.0), pow(u, 2.0), pow(u, 1.0), 1};
  output.x = indexes[0] * middleMatrix[0][0] + indexes[1] * middleMatrix[1][0] + indexes[2] * middleMatrix[2][0] + indexes[3] * middleMatrix[3][0];
  output.y = indexes[0] * middleMatrix[0][1] + indexes[1] * middleMatrix[1][1] + indexes[2] * middleMatrix[2][1] + indexes[3] * middleMatrix[3][1];
  output.z = indexes[0] * middleMatrix[0][2] + indexes[1] * middleMatrix[1][2] + indexes[2] * middleMatrix[2][2] + indexes[3] * middleMatrix[3][2];
  return output;
}

int initTexture(const char *imageFilename, GLuint textureHandle)
{
  // read the texture image
  ImageIO img;
  ImageIO::fileFormatType imgFormat;
  ImageIO::errorType err = img.load(imageFilename, &imgFormat);

  if (err != ImageIO::OK)
  {
    printf("Loading texture from %s failed.\n", imageFilename);
    return -1;
  }

  // check that the number of bytes is a multiple of 4
  if (img.getWidth() * img.getBytesPerPixel() % 4)
  {
    printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
    return -1;
  }

  // allocate space for an array of pixels
  int width = img.getWidth();
  int height = img.getHeight();
  unsigned char *pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

  // fill the pixelsRGBA array with the image pixels
  memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
  for (int h = 0; h < height; h++)
    for (int w = 0; w < width; w++)
    {
      // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
      pixelsRGBA[4 * (h * width + w) + 0] = 0;   // red
      pixelsRGBA[4 * (h * width + w) + 1] = 0;   // green
      pixelsRGBA[4 * (h * width + w) + 2] = 0;   // blue
      pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

      // set the RGBA channels, based on the loaded image
      int numChannels = img.getBytesPerPixel();
      for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
        pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
    }

  // bind the texture
  glBindTexture(GL_TEXTURE_2D, textureHandle);

  // initialize the texture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

  // generate the mipmaps for this texture
  glGenerateMipmap(GL_TEXTURE_2D);

  // set the texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // query support for anisotropic texture filtering
  GLfloat fLargest;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
  printf("Max available anisotropic samples: %f\n", fLargest);
  // set anisotropic texture filtering
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

  // query for any errors
  GLenum errCode = glGetError();
  if (errCode != 0)
  {
    printf("Texture initialization error. Error code: %d.\n", errCode);
    return -1;
  }

  // de-allocate the pixel array -- it is no longer needed
  delete[] pixelsRGBA;

  return 0;
}

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

  // sent viewmodel matrix
  float m[16];
  matrix->GetMatrix(m);
  glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);

  matrix->SetMatrixMode(OpenGLMatrix::Projection);

  // sent projection matrix
  float p[16]; // column-major
  matrix->GetMatrix(p);
  glUniformMatrix4fv(h_projectionMatrix, 1, isRowMajor, p);

  pipelineProgram->Bind();
}

void renderLines()
{
  GLint first = 0;
  GLsizei numberOfVertices = pos.size() / 3;
  glDrawArrays(GL_LINE_STRIP, first, numberOfVertices);
  glBindVertexArray(0);
}
void setTextureUnit(GLint unit)
{
  glActiveTexture(unit); // select the active texture unit
  // get a handle to the “textureImage” shader variable
  GLint h_textureImage = glGetUniformLocation(program, "textureImage");
  // deem the shader variable “textureImage” to read from texture unit “unit”
  glUniform1i(h_textureImage, unit - GL_TEXTURE0);
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
  matrix->LookAt(0, 0, 3.690276557, 0, 0, -20, 0, 1, 0);
  matrix->Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
  matrix->Rotate(landRotate[0], 1.0, 0.0, 0.0);
  matrix->Rotate(landRotate[1], 0.0, 1.0, 0.0);
  matrix->Rotate(landRotate[2], 0.0, 0.0, 1.0);
  matrix->Scale(landScale[0], landScale[1], landScale[2]);
  bindProgram();
  // change how the program render the vertices

  setTextureUnit(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, groundTextureHandle);
  glBindVertexArray(vao);
  renderLines();
  glBindVertexArray(groundVao);
  GLint first = 0;
  GLsizei numberOfVertices = groundPos.size() / 3;
  glDrawArrays(GL_TRIANGLES, first, numberOfVertices);
  glBindVertexArray(0);

  glutSwapBuffers();
}

void animate()
{
  int step = 1;
  landRotate[0] += step * 3;
  // landRotate[1] += step;
  // landRotate[2] += step * 5;
  // landScale[0] *= 1.0f + step * 0.01f;
  landScale[1] *= 1.0f - step * 0.01f;
  // landScale[2] *= 1.0f - step * 0.01f;
  // landTranslate[0] += step * 0.01f;
  // landTranslate[1] -= step * 0.01f;
  if (currentFrameNumber == 100)
  {
    currMode = lineMode;
  }
  else if (currentFrameNumber == 200)
  {
    currMode = triangleMode;
  }
  stringstream filename;
  filename << "frame_" << currentFrameNumber << ".jpg";
  saveScreenshot(filename.str().c_str());
  currentFrameNumber++;
  if (currentFrameNumber > 300)
  {
    needAnimate = false;
  }
}

void idleFunc()
{
  // do some stuff...

  // for example, here, you can save the screenshots to disk (to make the animation)

  // make the screen update
  if (needAnimate)
  {
    animate();
  }
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

  matrix->Perspective(45, (1.0 * 1280) / (1.0 * 720), 0.01, 1500.0);
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
  case 'a':
    needAnimate = true;
    break;
  }
}

// initialization for the vertex buffer object
void initVBO()
{

  // vbo for points
  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, (pos.size() + uvs.size()) * sizeof(float), NULL, GL_STATIC_DRAW);
  // upload position data
  glBufferSubData(GL_ARRAY_BUFFER, 0, pos.size() * sizeof(float), pos.data());
  // upload color data
  glBufferSubData(GL_ARRAY_BUFFER, pos.size() * sizeof(float), uvs.size() * sizeof(float), uvs.data());

  //vbo for grounds
  glGenBuffers(1, &groundBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, groundBuffer);
  glBufferData(GL_ARRAY_BUFFER, (groundPos.size() + groundUVs.size()) * sizeof(float), NULL, GL_STATIC_DRAW);
  // upload position data
  glBufferSubData(GL_ARRAY_BUFFER, 0, groundPos.size() * sizeof(float), groundPos.data());
  // upload color data
  glBufferSubData(GL_ARRAY_BUFFER, groundPos.size() * sizeof(float), groundUVs.size() * sizeof(float), groundUVs.data());
}

// initialization for the pipeline (shaders, etc.)
void initPipelineProgram()
{
  // initialize shader pipeline program
  pipelineProgram = new BasicPipelineProgram();
  pipelineProgram->Init("./shaders");
  pipelineProgram->Bind();
}

void initpointVAO()
{

  program = pipelineProgram->GetProgramHandle();
  GLsizei stride = 0;
  GLboolean normalized = GL_FALSE;

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  // points
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  GLuint loc = glGetAttribLocation(program, "position");
  glEnableVertexAttribArray(loc);
  const void *offset = (const void *)0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
  GLuint loc2 = glGetAttribLocation(program, "texCoord");
  glEnableVertexAttribArray(loc2);
  offset = (const void *)(pos.size() * sizeof(float));
  glVertexAttribPointer(loc2, 2, GL_FLOAT, normalized, stride, offset);
  glBindVertexArray(0);

  glGenVertexArrays(1, &groundVao);
  glBindVertexArray(groundVao);
  glBindBuffer(GL_ARRAY_BUFFER, groundBuffer);
  loc = glGetAttribLocation(program, "position");
  glEnableVertexAttribArray(loc);
  offset = (const void *)0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
  loc2 = glGetAttribLocation(program, "texCoord");
  glEnableVertexAttribArray(loc2);
  offset = (const void *)(groundPos.size() * sizeof(float));
  glVertexAttribPointer(loc2, 2, GL_FLOAT, normalized, stride, offset);
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

void initTextures()
{
  glGenTextures(1, &groundTextureHandle);
  int code = initTexture("ground_texture_1024.jpg", groundTextureHandle);
  if (code != 0)
  {
    printf("Error loading ground texture");
    exit(EXIT_FAILURE);
  }
}

void initScene(int argc, char *argv[])
{
  // load the image from a jpeg disk file to main memory
  // heightmapImage = new ImageIO();
  // if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  // {
  //   cout << "Error reading image " << argv[1] << "." << endl;
  //   exit(EXIT_FAILURE);
  // }
  initTextures();
  float minX = 9999999;
  float maxX = -9999999;
  float minY = 9999999;
  float maxY = -9999999;
  float minZ = 9999999;
  float maxZ = -9999999;

  for (int i = 0; i < numSplines; i++)
  {
    int currPointLength = splines[i].numControlPoints;
    for (int j = 0; j <= currPointLength - 4; j++)
    {
      for (float u = 0; u <= 1.0; u += USTEP)
      {
        Point currPoint = calculateSpline(u, splines[i].points[j], splines[i].points[j + 1], splines[i].points[j + 2], splines[i].points[j + 3]);
        pos.push_back(currPoint.x);
        pos.push_back(currPoint.y);
        pos.push_back(currPoint.z);
        uvs.push_back(1.0);
        uvs.push_back(0.0);
        if (currPoint.x > maxX)
          maxX = currPoint.x;
        if (currPoint.x < minX)
          minX = currPoint.x;
        if (currPoint.y > maxY)
          maxY = currPoint.y;
        if (currPoint.y < minY)
          minY = currPoint.y;
        if (currPoint.z > maxZ)
          maxZ = currPoint.z;
        if (currPoint.z < minZ)
          minZ = currPoint.z;
      }
    }
  }
  // initialize ground
  cout << minX << minY << minZ << endl;
  cout << maxX << maxY << maxZ << endl;
  groundPos.push_back(0);
  groundPos.push_back(0);
  groundPos.push_back(0);

  groundPos.push_back(0);
  groundPos.push_back(0);
  groundPos.push_back(100);

  groundPos.push_back(100);
  groundPos.push_back(0);
  groundPos.push_back(0);

  groundUVs.push_back(0.0);
  groundUVs.push_back(0.0);
  groundUVs.push_back(0.2);
  groundUVs.push_back(0.2);
  groundUVs.push_back(0.2);
  groundUVs.push_back(0.0);

  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  // do additional initialization here...
  glEnable(GL_DEPTH_TEST);
  matrix = new OpenGLMatrix();
  initVBO();
  initPipelineProgram();
  initpointVAO();
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    printf("usage: %s <trackfile>\n", argv[0]);
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
  loadSplines(argv[1]);
  printf("Loaded %d spline(s).\n", numSplines);
  for (int i = 0; i < numSplines; i++)
    printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}
