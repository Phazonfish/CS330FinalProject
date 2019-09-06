// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <sstream>

// Include GLEW
#include <GL/glew.h>
#include <iostream>
#include <GL/freeglut.h>

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

#define WINDOW_TITLE "Modern OpenGL" // Window title Macro
#define LEG_X 0.067913f //Recurring Texture Coordinate

//Window Dimensions
GLint WindowWidth = 800, WindowHeight = 600;

//Buffer declarations
GLuint vertexbuffer, uvbuffer, normalbuffer;
GLuint VertexArrayID;

//Uniform Value ID's
GLuint programID, MatrixID, ModelMatrixID, ViewMatrixID, LightID, Texture, TextureID, ColorID, IntensityID;

//Input Function Values
GLfloat lastMouseX = 400, lastMouseY = 300;
GLfloat mouseXOffset, mouseYOffset, camYaw = 0.0f, camPitch = 0.0f;
GLfloat sensitivity = 0.010f;
GLchar currentKey;
bool mouseDetected = true;

//Global vector declarations
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f); // Initial camera position. Placed 5 units in Z
glm::vec3 CameraUpY = glm::vec3(0.0f, 1.0f, 0.0f); // Temporary y unit vector
glm::vec3 CameraForwardZ = glm::vec3(0.0f, 0.0f, -1.0f); // Temporary z unit vector
glm::vec3 front; // Temporary z unit vector for mouse

//Lighting Values
glm::vec3 lightPos = glm::vec3(4,4,4);
glm::vec3 lightColor = vec3(1,1,1);
GLfloat lightIntensity = 50.0f;

//Function Prototypes
void URenderGraphics(void);
void UResizeWindow(int w, int h);
void UCreateBuffers();
void UKeyboard(unsigned char key, GLint x, GLint y);
void UKeyReleased(unsigned char key, GLint x, GLint y);
void UMouseMove(int x, int y);
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path);
GLuint loadBMP_custom(const char * imagepath);

int main(int argc, char* argv[])
{
	// Open a window and create its OpenGL context
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WindowWidth, WindowHeight);
	glutCreateWindow(WINDOW_TITLE);

	glutReshapeFunc(UResizeWindow);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS); 

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "StandardShading.vertexshader", "StandardShading.fragmentshader" );

	// Get a handle for our "MVP" uniform
	MatrixID = glGetUniformLocation(programID, "MVP");
	ViewMatrixID = glGetUniformLocation(programID, "V");
	ModelMatrixID = glGetUniformLocation(programID, "M");

	// Load the texture
	Texture = loadBMP_custom("TableTexture.bmp");

	// Get a handle for our "myTextureSampler" uniform
	TextureID  = glGetUniformLocation(programID, "myTextureSampler");

	UCreateBuffers();

	// Get a handle for our lighting uniforms
	glUseProgram(programID);
	LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
	ColorID = glGetUniformLocation(programID, "LightColor");
	IntensityID = glGetUniformLocation(programID, "LightPower");

	glutKeyboardFunc(UKeyboard); //Detects keys pressed

	glutKeyboardUpFunc(UKeyReleased); //Detects keys released

	glutDisplayFunc(URenderGraphics);

	glutPassiveMotionFunc(UMouseMove); //Detects mouse movement

	glutMainLoop();

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteProgram(programID);
	glDeleteTextures(1, &Texture);
	glDeleteVertexArrays(1, &VertexArrayID);

	return 0;
}

void URenderGraphics(void){
	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Use our shader
	glUseProgram(programID);

	CameraForwardZ = front;

	//Determine projection based on whether or not Z is held
	glm::mat4 ProjectionMatrix;
		if (currentKey == 'z') {
			ProjectionMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
		}
		else {
			ProjectionMatrix = glm::perspective(glm::radians(45.0f), (GLfloat)WindowWidth / (GLfloat)WindowHeight, 0.1f, 100.0f);
		}
	glm::mat4 ViewMatrix = glm::lookAt(CameraForwardZ, cameraPosition, CameraUpY);
	glm::mat4 ModelMatrix = glm::mat4(1.0);
	glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

	// Send our transformation to the currently bound shader,
	// in the "MVP" uniform
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

	// Send lighting values to the shader
	glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);
	glUniform3f(ColorID, lightColor.r, lightColor.g, lightColor.b);
	glUniform1f(IntensityID, lightIntensity);

	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Texture);
	// Set our "myTextureSampler" sampler to use Texture Unit 0
	glUniform1i(TextureID, 0);

	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer(
		0,                  // attribute
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	// 2nd attribute buffer : UVs
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glVertexAttribPointer(
		1,                                // attribute
		2,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
	);

	// 3rd attribute buffer : normals
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glVertexAttribPointer(
		2,                                // attribute
		3,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
	);

	// Draw the triangles !
	glDrawArrays(GL_TRIANGLES, 0, 324 );

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	// Swap buffers
	glutSwapBuffers();
}

/* Resizes the window*/
void UResizeWindow(int w, int h)
{
	WindowWidth = w;
	WindowHeight = h;
	glViewport(0, 0, WindowWidth, WindowHeight);
}

void UCreateBuffers(){
	// Our vertices. Three consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
			GLfloat g_vertex_buffer_data[] = {

				//Front Left Leg

				-0.235f,-0.235f,-0.625f,	//front bottom left
				-0.235f,-0.165f,-0.625f,	//back bottom left		//Left Face
				-0.235f,-0.165f, 0.595f,	//back top left

				-0.165f,-0.235f,0.595f,		//front top right
				-0.235f,-0.235f,-0.625f,	//front bottom left		//Front Face
				-0.235f,-0.235f,0.595f,		//front top left

				-0.165f,-0.165f,-0.625f,	//back bottom right
				-0.235f,-0.235f,-0.625f,	//front bottom left		//Bottom Face
				-0.165f,-0.235f,-0.625f,	//front bottom right

				-0.165f,-0.235f,0.595f,		//front top right
				-0.165f,-0.235f,-0.625f,	//front bottom right	//Front Face
				-0.235f,-0.235f,-0.625f,	//front bottom left

				-0.235f,-0.235f,-0.625f,	//front bottom left
				-0.235f,-0.165f, 0.595f,	//back top left			//Left Face
				-0.235f,-0.235f,0.595f,		//front top left

				-0.165f,-0.165f,-0.625f,	//back bottom right
				-0.235f,-0.165f,-0.625f,	//back bottom left		//Bottom Face
				-0.235f,-0.235f,-0.625f,	//front bottom left

				-0.235f,-0.165f, 0.595f,	//back top left
				-0.235f,-0.165f,-0.625f,	//back bottom left		//Back Face
				-0.165f,-0.165f,-0.625f,	//back bottom right

				-0.165f,-0.165f,0.595f,		//back top right
				-0.165f,-0.235f,-0.625f,	//front bottom right	//Right Face
				-0.165f,-0.235f,0.595f,		//front top right

				-0.165f,-0.235f,-0.625f,	//front bottom right
				-0.165f,-0.165f,0.595f,		//back top right		//Right Face
				-0.165f,-0.165f,-0.625f,	//back bottom right

				-0.165f,-0.165f,0.595f,		//back top right
				-0.165f,-0.235f,0.595f,		//front top right		//Top Face
				-0.235f,-0.235f,0.595f,		//front top left

				-0.165f,-0.165f,0.595f,		//back top right
				-0.235f,-0.235f,0.595f,		//front top left		//Top Face
				-0.235f,-0.165f, 0.595f,	//back top left

				-0.165f,-0.165f,0.595f,		//back top right
				-0.235f,-0.165f, 0.595f,	//back top left			//Back Face
				-0.165f,-0.165f,-0.625f,	//back bottom right

				//Front Right Leg

				 0.165f,-0.235f,-0.625f,	//front bottom left
				 0.165f,-0.165f,-0.625f,	//back bottom left		//Left Face
				 0.165f,-0.165f, 0.595f,	//back top left

				 0.235f,-0.235f,0.595f,		//front top right
				 0.165f,-0.235f,-0.625f,	//front bottom left		//Front Face
				 0.165f,-0.235f,0.595f,		//front top left

				 0.235f,-0.165f,-0.625f,	//back bottom right
				 0.165f,-0.235f,-0.625f,	//front bottom left		//Bottom Face
				 0.235f,-0.235f,-0.625f,	//front bottom right

				 0.235f,-0.235f,0.595f,		//front top right
				 0.235f,-0.235f,-0.625f,	//front bottom right	//Front Face
				 0.165f,-0.235f,-0.625f,	//front bottom left

				 0.165f,-0.235f,-0.625f,	//front bottom left
				 0.165f,-0.165f, 0.595f,	//back top left			//Left Face
				 0.165f,-0.235f,0.595f,		//front top left

				 0.235f,-0.165f,-0.625f,	//back bottom right
				 0.165f,-0.165f,-0.625f,	//back bottom left		//Bottom Face
				 0.165f,-0.235f,-0.625f,	//front bottom left

				 0.165f,-0.165f, 0.595f,	//back top left
				 0.165f,-0.165f,-0.625f,	//back bottom left		//Back Face
				 0.235f,-0.165f,-0.625f,	//back bottom right

				 0.235f,-0.165f,0.595f,		//back top right
				 0.235f,-0.235f,-0.625f,	//front bottom right	//Right Face
				 0.235f,-0.235f,0.595f,		//front top right

				 0.235f,-0.235f,-0.625f,	//front bottom right
				 0.235f,-0.165f,0.595f,		//back top right		//Right Face
				 0.235f,-0.165f,-0.625f,	//back bottom right

				 0.235f,-0.165f,0.595f,		//back top right
				 0.235f,-0.235f,0.595f,		//front top right		//Top Face
				 0.165f,-0.235f,0.595f,		//front top left

				 0.235f,-0.165f,0.595f,		//back top right
				 0.165f,-0.235f,0.595f,		//front top left		//Top Face
				 0.165f,-0.165f, 0.595f,	//back top left

				 0.235f,-0.165f,0.595f,		//back top right
				 0.165f,-0.165f, 0.595f,	//back top left			//Back Face
				 0.235f,-0.165f,-0.625f,	//back bottom right

				 //Back Left Leg

				 -0.235f,0.165f,-0.625f,	//front bottom left
				 -0.235f,0.235f,-0.625f,	//back bottom left		//Left Face
				 -0.235f,0.235f, 0.595f,	//back top left

				 -0.165f,0.165f,0.595f,		//front top right
				 -0.235f,0.165f,-0.625f,	//front bottom left		//Front Face
				 -0.235f,0.165f,0.595f,		//front top left

				 -0.165f,0.235f,-0.625f,	//back bottom right
				 -0.235f,0.165f,-0.625f,	//front bottom left		//Bottom Face
				 -0.165f,0.165f,-0.625f,	//front bottom right

				 -0.165f,0.165f,0.595f,		//front top right
				 -0.165f,0.165f,-0.625f,	//front bottom right	//Front Face
				 -0.235f,0.165f,-0.625f,	//front bottom left

				 -0.235f,0.165f,-0.625f,	//front bottom left
				 -0.235f,0.235f, 0.595f,	//back top left			//Left Face
				 -0.235f,0.165f,0.595f,		//front top left

				 -0.165f,0.235f,-0.625f,	//back bottom right
				 -0.235f,0.235f,-0.625f,	//back bottom left		//Bottom Face
				 -0.235f,0.165f,-0.625f,	//front bottom left

				 -0.235f,0.235f, 0.595f,	//back top left
				 -0.235f,0.235f,-0.625f,	//back bottom left		//Back Face
				 -0.165f,0.235f,-0.625f,	//back bottom right

				 -0.165f,0.235f,0.595f,		//back top right
				 -0.165f,0.165f,-0.625f,	//front bottom right	//Right Face
				 -0.165f,0.165f,0.595f,		//front top right

				 -0.165f,0.165f,-0.625f,	//front bottom right
				 -0.165f,0.235f,0.595f,		//back top right		//Right Face
				 -0.165f,0.235f,-0.625f,	//back bottom right

				 -0.165f,0.235f,0.595f,		//back top right
				 -0.165f,0.165f,0.595f,		//front top right		//Top Face
				 -0.235f,0.165f,0.595f,		//front top left

				 -0.165f,0.235f,0.595f,		//back top right
				 -0.235f,0.165f,0.595f,		//front top left		//Top Face
				 -0.235f,0.235f, 0.595f,	//back top left

				 -0.165f,0.235f,0.595f,		//back top right
				 -0.235f,0.235f, 0.595f,	//back top left			//Back Face
				 -0.165f,0.235f,-0.625f,	//back bottom right

				 //Back Right Leg

				 0.165f,0.165f,-0.625f,		//front bottom left
				 0.165f,0.235f,-0.625f,		//back bottom left		//Left Face
				 0.165f,0.235f, 0.595f,		//back top left

				 0.235f,0.165f,0.595f,		//front top right
				 0.165f,0.165f,-0.625f,		//front bottom left		//Front Face
				 0.165f,0.165f,0.595f,		//front top left

				 0.235f,0.235f,-0.625f,		//back bottom right
				 0.165f,0.165f,-0.625f,		//front bottom left		//Bottom Face
				 0.235f,0.165f,-0.625f,		//front bottom right

				 0.235f,0.165f,0.595f,		//front top right
				 0.235f,0.165f,-0.625f,		//front bottom right	//Front Face
				 0.165f,0.165f,-0.625f,		//front bottom left

				 0.165f,0.165f,-0.625f,		//front bottom left
				 0.165f,0.235f, 0.595f,		//back top left			//Left Face
				 0.165f,0.165f,0.595f,		//front top left

				 0.235f,0.235f,-0.625f,		//back bottom right
				 0.165f,0.235f,-0.625f,		//back bottom left		//Bottom Face
				 0.165f,0.165f,-0.625f,		//front bottom left

				 0.165f,0.235f, 0.595f,		//back top left
				 0.165f,0.235f,-0.625f,		//back bottom left		//Back Face
				 0.235f,0.235f,-0.625f,		//back bottom right

				 0.235f,0.235f,0.595f,		//back top right
				 0.235f,0.165f,-0.625f,		//front bottom right	//Right Face
				 0.235f,0.165f,0.595f,		//front top right

				 0.235f,0.165f,-0.625f,		//front bottom right
				 0.235f,0.235f,0.595f,		//back top right		//Right Face
				 0.235f,0.235f,-0.625f,		//back bottom right

				 0.235f,0.235f,0.595f,		//back top right
				 0.235f,0.165f,0.595f,		//front top right		//Top Face
				 0.165f,0.165f,0.595f,		//front top left

				 0.235f,0.235f,0.595f,		//back top right
				 0.165f,0.165f,0.595f,		//front top left		//Top Face
				 0.165f,0.235f, 0.595f,		//back top left

				 0.235f,0.235f,0.595f,		//back top right
				 0.165f,0.235f, 0.595f,		//back top left			//Back Face
				 0.235f,0.235f,-0.625f,		//back bottom right

				 //Table Top

				 -0.255f,-0.255f,0.595f,	//front bottom left
				 -0.255f,0.255f,0.595f,		//back bottom left		//Left Face
				 -0.255f,0.255f,0.625f,		//back top left

				 0.255f,-0.255f,0.625f,		//front top right
				 -0.255f,-0.255f,0.595f,	//front bottom left		//Front Face
				 -0.255f,-0.255f,0.625f,	//front top left

				 0.255f,0.255f,0.595f,		//back bottom right
				 -0.255f,-0.255f,0.595f,	//front bottom left		//Bottom Face
				 0.255f,-0.255f,0.595f,		//front bottom right

				 0.255f,-0.255f,0.625f,		//front top right
				 0.255f,-0.255f,0.595f,		//front bottom right	//Front Face
				 -0.255f,-0.255f,0.595f,	//front bottom left

				 -0.255f,-0.255f,0.595f,	//front bottom left
				 -0.255f,0.255f,0.625f,		//back top left			//Left Face
				 -0.255f,-0.255f,0.625f,	//front top left

				 0.255f,0.255f,0.595f,		//back bottom right
				 -0.255f,0.255f,0.595f,		//back bottom left		//Bottom Face
				 -0.255f,-0.255f,0.595f,	//front bottom left

				 -0.255f,0.255f,0.625f,		//back top left
				 -0.255f,0.255f,0.595f,		//back bottom left		//Back Face
				 0.255f,0.255f,0.595f,		//back bottom right

				 0.255f,0.255f,0.625f,		//back top right
				 0.255f,-0.255f,0.595f,		//front bottom right	//Right Face
				 0.255f,-0.255f,0.625f,		//front top right

				 0.255f,-0.255f,0.595f,		//front bottom right
				 0.255f,0.255f,0.625f,		//back top right		//Right Face
				 0.255f,0.255f,0.595f,		//back bottom right

				 0.255f,0.255f,0.625f,		//back top right
				 0.255f,-0.255f,0.625f,		//front top right		//Top Face
				 -0.255f,-0.255f,0.625f,	//front top left

				 0.255f,0.255f,0.625f,		//back top right
				 -0.255f,-0.255f,0.625f,	//front top left		//Top Face
				 -0.255f,0.255f,0.625f,		//back top left

				 0.255f,0.255f,0.625f,		//back top right
				 -0.255f,0.255f,0.625f,		//back top left			//Back Face
				 0.255f,0.255f,0.595f,		//back bottom right

				 //Left Bridge

				 -0.235f,-0.165f,-0.325f,	//front bottom left
				 -0.235f,0.165f,-0.325f,	//back bottom left		//Left Face
				 -0.235f,0.165f,-0.255f,	//back top left

				 -0.165f,-0.165f,-0.255f,	//front top right
				 -0.235f,-0.165f,-0.325f,	//front bottom left		//Front Face
				 -0.235f,-0.165f,-0.255f,	//front top left

				 -0.165f,0.165f,-0.325f,	//back bottom right
				 -0.235f,-0.165f,-0.325f,	//front bottom left		//Bottom Face
				 -0.165f,-0.165f,-0.325f,	//front bottom right

				 -0.165f,-0.165f,-0.255f,	//front top right
				 -0.165f,-0.165f,-0.325f,	//front bottom right	//Front Face
				 -0.235f,-0.165f,-0.325f,	//front bottom left

				 -0.235f,-0.165f,-0.325f,	//front bottom left
				 -0.235f,0.165f,-0.255f,	//back top left			//Left Face
				 -0.235f,-0.165f,-0.255f,	//front top left

				 -0.165f,0.165f,-0.325f,	//back bottom right
				 -0.235f,0.165f,-0.325f,	//back bottom left		//Bottom Face
				 -0.235f,-0.165f,-0.325f,	//front bottom left

				 -0.235f,0.165f,-0.255f,	//back top left
				 -0.235f,0.165f,-0.325f,	//back bottom left		//Back Face
				 -0.165f,0.165f,-0.325f,	//back bottom right

				 -0.165f,0.165f,-0.255f,	//back top right
				 -0.165f,-0.165f,-0.325f,	//front bottom right	//Right Face
				 -0.165f,-0.165f,-0.255f,	//front top right

				 -0.165f,-0.165f,-0.325f,	//front bottom right
				 -0.165f,0.165f,-0.255f,	//back top right		//Right Face
				 -0.165f,0.165f,-0.325f,	//back bottom right

				 -0.165f,0.165f,-0.255f,	//back top right
				 -0.165f,-0.165f,-0.255f,	//front top right		//Top Face
				 -0.235f,-0.165f,-0.255f,	//front top left

				 -0.165f,0.165f,-0.255f,	//back top right
				 -0.235f,-0.165f,-0.255f,	//front top left		//Top Face
				 -0.235f,0.165f,-0.255f,	//back top left

				 -0.165f,0.165f,-0.255f,	//back top right
				 -0.235f,0.165f,-0.255f,	//back top left			//Back Face
				 -0.165f,0.165f,-0.325f,	//back bottom right

				 //Front Bridge

				 -0.165f,-0.235f,-0.325f,	//front bottom left
				 -0.165f,-0.165f,-0.325f,	//back bottom left		//Left Face
				 -0.165f,-0.165f,-0.255f,	//back top left

				 0.165f,-0.235f,-0.255f,	//front top right
				 -0.165f,-0.235f,-0.325f,	//front bottom left		//Front Face
				 -0.165f,-0.235f,-0.255f,	//front top left

				 0.165f,-0.165f,-0.325f,	//back bottom right
				 -0.165f,-0.235f,-0.325f,	//front bottom left		//Bottom Face
				 0.165f,-0.235f,-0.325f,	//front bottom right

				 0.165f,-0.235f,-0.255f,	//front top right
				 0.165f,-0.235f,-0.325f,	//front bottom right	//Front Face
				 -0.165f,-0.235f,-0.325f,	//front bottom left

				 -0.165f,-0.235f,-0.325f,	//front bottom left
				 -0.165f,-0.165f,-0.255f,	//back top left			//Left Face
				 -0.165f,-0.235f,-0.255f,	//front top left

				 0.165f,-0.165f,-0.325f,	//back bottom right
				 -0.165f,-0.165f,-0.325f,	//back bottom left		//Bottom Face
				 -0.165f,-0.235f,-0.325f,	//front bottom left

				 -0.165f,-0.165f,-0.255f,	//back top left
				 -0.165f,-0.165f,-0.325f,	//back bottom left		//Back Face
				 0.165f,-0.165f,-0.325f,	//back bottom right

				 0.165f,-0.165f,-0.255f,	//back top right
				 0.165f,-0.235f,-0.325f,	//front bottom right	//Right Face
				 0.165f,-0.235f,-0.255f,	//front top right

				 0.165f,-0.235f,-0.325f,	//front bottom right
				 0.165f,-0.165f,-0.255f,	//back top right		//Right Face
				 0.165f,-0.165f,-0.325f,	//back bottom right

				 0.165f,-0.165f,-0.255f,	//back top right
				 0.165f,-0.235f,-0.255f,	//front top right		//Top Face
				 -0.165f,-0.235f,-0.255f,	//front top left

				 0.165f,-0.165f,-0.255f,	//back top right
				 -0.165f,-0.235f,-0.255f,	//front top left		//Top Face
				 -0.165f,-0.165f,-0.255f,	//back top left

				 0.165f,-0.165f,-0.255f,	//back top right
				 -0.165f,-0.165f,-0.255f,	//back top left			//Back Face
				 0.165f,-0.165f,-0.325f,	//back bottom right

				 //Right Bridge

				 0.165f,-0.165f,-0.325f,	//front bottom left
				 0.165f,0.165f,-0.325f,		//back bottom left		//Left Face
				 0.165f,0.165f,-0.255f,		//back top left

				 0.235f,-0.165f,-0.255f,	//front top right
				 0.165f,-0.165f,-0.325f,	//front bottom left		//Front Face
				 0.165f,-0.165f,-0.255f,	//front top left

				 0.235f,0.165f,-0.325f,		//back bottom right
				 0.165f,-0.165f,-0.325f,	//front bottom left		//Bottom Face
				 0.235f,-0.165f,-0.325f,	//front bottom right

				 0.235f,-0.165f,-0.255f,	//front top right
				 0.235f,-0.165f,-0.325f,	//front bottom right	//Front Face
				 0.165f,-0.165f,-0.325f,	//front bottom left

				 0.165f,-0.165f,-0.325f,	//front bottom left
				 0.165f,0.165f,-0.255f,		//back top left			//Left Face
				 0.165f,-0.165f,-0.255f,	//front top left

				 0.235f,0.165f,-0.325f,		//back bottom right
				 0.165f,0.165f,-0.325f,		//back bottom left		//Bottom Face
				 0.165f,-0.165f,-0.325f,	//front bottom left

				 0.165f,0.165f,-0.255f,		//back top left
				 0.165f,0.165f,-0.325f,		//back bottom left		//Back Face
				 0.235f,0.165f,-0.325f,		//back bottom right

				 0.235f,0.165f,-0.255f,		//back top right
				 0.235f,-0.165f,-0.325f,	//front bottom right	//Right Face
				 0.235f,-0.165f,-0.255f,	//front top right

				 0.235f,-0.165f,-0.325f,	//front bottom right
				 0.235f,0.165f,-0.255f,		//back top right		//Right Face
				 0.235f,0.165f,-0.325f,		//back bottom right

				 0.235f,0.165f,-0.255f,		//back top right
				 0.235f,-0.165f,-0.255f,	//front top right		//Top Face
				 0.165f,-0.165f,-0.255f,	//front top left

				 0.235f,0.165f,-0.255f,		//back top right
				 0.165f,-0.165f,-0.255f,	//front top left		//Top Face
				 0.165f,0.165f,-0.255f,		//back top left

				 0.235f,0.165f,-0.255f,		//back top right
				 0.165f,0.165f,-0.255f,		//back top left			//Back Face
				 0.235f,0.165f,-0.325f,		//back bottom right

				 //Back Bridge

				 -0.165f,0.165f,-0.325f,	//front bottom left
				 -0.165f,0.235f,-0.325f,	//back bottom left		//Left Face
				 -0.165f,0.235f,-0.255f,	//back top left

				 0.165f,0.165f,-0.255f,		//front top right
				 -0.165f,0.165f,-0.325f,	//front bottom left		//Front Face
				 -0.165f,0.165f,-0.255f,	//front top left

				 0.165f,0.235f,-0.325f,		//back bottom right
				 -0.165f,0.165f,-0.325f,	//front bottom left		//Bottom Face
				 0.165f,0.165f,-0.325f,		//front bottom right

				 0.165f,0.165f,-0.255f,		//front top right
				 0.165f,0.165f,-0.325f,		//front bottom right	//Front Face
				 -0.165f,0.165f,-0.325f,	//front bottom left

				 -0.165f,0.165f,-0.325f,	//front bottom left
				 -0.165f,0.235f,-0.255f,	//back top left			//Left Face
				 -0.165f,0.165f,-0.255f,	//front top left

				 0.165f,0.235f,-0.325f,		//back bottom right
				 -0.165f,0.235f,-0.325f,	//back bottom left		//Bottom Face
				 -0.165f,0.165f,-0.325f,	//front bottom left

				 -0.165f,0.235f,-0.255f,	//back top left
				 -0.165f,0.235f,-0.325f,	//back bottom left		//Back Face
				 0.165f,0.235f,-0.325f,		//back bottom right

				 0.165f,0.235f,-0.255f,		//back top right
				 0.165f,0.165f,-0.325f,		//front bottom right	//Right Face
				 0.165f,0.165f,-0.255f,		//front top right

				 0.165f,0.165f,-0.325f,		//front bottom right
				 0.165f,0.235f,-0.255f,		//back top right		//Right Face
				 0.165f,0.235f,-0.325f,		//back bottom right

				 0.165f,0.235f,-0.255f,		//back top right
				 0.165f,0.165f,-0.255f,		//front top right		//Top Face
				 -0.165f,0.165f,-0.255f,	//front top left

				 0.165f,0.235f,-0.255f,		//back top right
				 -0.165f,0.165f,-0.255f,	//front top left		//Top Face
				 -0.165f,0.235f,-0.255f,	//back top left

				 0.165f,0.235f,-0.255f,		//back top right
				 -0.165f,0.235f,-0.255f,	//back top left			//Back Face
				 0.165f,0.235f,-0.325f		//back bottom right

			};

			// Two UV coordinatesfor each vertex. They were created with Blender.
			GLfloat g_uv_buffer_data[] = {
				//Front Left Leg
				LEG_X, 0.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				LEG_X, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 0.0f,
				0.0f, 1.0f,
				LEG_X, 1.0f,

				LEG_X, 1.0f,
				0.0f, 1.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				0.0f, 0.0f,
				LEG_X, 1.0f,
				LEG_X, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				0.0f, 1.0f,
				LEG_X, 1.0f,
				0.0f, 0.0f,

				//Front Right Leg

				LEG_X, 0.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				LEG_X, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 0.0f,
				0.0f, 1.0f,
				LEG_X, 1.0f,

				LEG_X, 1.0f,
				0.0f, 1.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				0.0f, 0.0f,
				LEG_X, 1.0f,
				LEG_X, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				0.0f, 1.0f,
				LEG_X, 1.0f,
				0.0f, 0.0f,

				//Back Left Leg

				LEG_X, 0.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				LEG_X, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 0.0f,
				0.0f, 1.0f,
				LEG_X, 1.0f,

				LEG_X, 1.0f,
				0.0f, 1.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				0.0f, 0.0f,
				LEG_X, 1.0f,
				LEG_X, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				0.0f, 1.0f,
				LEG_X, 1.0f,
				0.0f, 0.0f,

				//Back Right Leg

				LEG_X, 0.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				LEG_X, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 0.0f,
				0.0f, 1.0f,
				LEG_X, 1.0f,

				LEG_X, 1.0f,
				0.0f, 1.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				0.0f, 0.0f,
				LEG_X, 1.0f,
				LEG_X, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				0.0f, 1.0f,
				LEG_X, 1.0f,
				0.0f, 0.0f,

				//Table Top

				0.0f, 0.0f,
				0.0f, 1.0f,
				LEG_X, 1.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				LEG_X, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 0.0f,
				0.0f, 1.0f,
				LEG_X, 1.0f,

				LEG_X, 1.0f,
				0.0f, 1.0f,
				0.0f, 0.0f,

				0.0f, 1.0f,
				0.0f, 0.0f,
				LEG_X, 0.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				0.0f, 0.0f,
				LEG_X, 1.0f,
				LEG_X, 0.0f,

				1.0f, 1.0f,
				1.0f, 0.0f,
				0.25f, 0.0f,

				1.0f, 1.0f,
				0.25f, 0.0f,
				0.25f, 1.0f,

				LEG_X, 1.0f,
				0.0f, 1.0f,
				0.0f, 0.0f,

				//Front Bridge

				LEG_X, 0.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				LEG_X, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 0.0f,
				0.0f, 1.0f,
				LEG_X, 1.0f,

				LEG_X, 1.0f,
				0.0f, 1.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				0.0f, 0.0f,
				LEG_X, 1.0f,
				LEG_X, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				0.0f, 1.0f,
				LEG_X, 1.0f,
				0.0f, 0.0f,

				//Left Bridge

				LEG_X, 0.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				LEG_X, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 0.0f,
				0.0f, 1.0f,
				LEG_X, 1.0f,

				LEG_X, 1.0f,
				0.0f, 1.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				0.0f, 0.0f,
				LEG_X, 1.0f,
				LEG_X, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				0.0f, 1.0f,
				LEG_X, 1.0f,
				0.0f, 0.0f,

				//Right Bridge

				LEG_X, 0.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				LEG_X, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 0.0f,
				0.0f, 1.0f,
				LEG_X, 1.0f,

				LEG_X, 1.0f,
				0.0f, 1.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				0.0f, 0.0f,
				LEG_X, 1.0f,
				LEG_X, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				0.0f, 1.0f,
				LEG_X, 1.0f,
				0.0f, 0.0f,

				//Back Bridge

				LEG_X, 0.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				LEG_X, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 0.0f,
				0.0f, 1.0f,
				LEG_X, 1.0f,

				LEG_X, 1.0f,
				0.0f, 1.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				0.0f, 0.0f,
				LEG_X, 1.0f,
				LEG_X, 0.0f,

				LEG_X, 1.0f,
				LEG_X, 0.0f,
				0.0f, 0.0f,

				LEG_X, 1.0f,
				0.0f, 0.0f,
				0.0f, 1.0f,

				0.0f, 1.0f,
				LEG_X, 1.0f,
				0.0f, 0.0f
			};

			GLfloat normal_buffer_data[] = {

					//Front Left Leg

					-1.0f,0.0f,0.0f,			//front bottom left
					-1.0f,0.0f,0.0f,			//back bottom left		//Left Face
					-1.0f,0.0f,0.0f,			//back top left

					0.0f,0.0f,1.0f,				//front top right
					0.0f,0.0f,1.0f,				//front bottom left		//Front Face
					0.0f,0.0f,1.0f,				//front top left

					0.0f,-1.0f,0.0f,			//back bottom right
					0.0f,-1.0f,0.0f,			//front bottom left		//Bottom Face
					0.0f,-1.0f,0.0f,			//front bottom right

					0.0f,0.0f,1.0f,				//front top right
					0.0f,0.0f,1.0f,				//front bottom right	//Front Face
					0.0f,0.0f,1.0f,				//front bottom left

					-1.0f,0.0f,0.0f,			//front bottom left
					-1.0f,0.0f,0.0f,			//back top left			//Left Face
					-1.0f,0.0f,0.0f,			//front top left

					0.0f,-1.0f,0.0f,			//back bottom right
					0.0f,-1.0f,0.0f,			//back bottom left		//Bottom Face
					0.0f,-1.0f,0.0f,			//front bottom left

					0.0f,0.0f,-1.0f,			//back top left
					0.0f,0.0f,-1.0f,			//back bottom left		//Back Face
					0.0f,0.0f,-1.0f,			//back bottom right

					1.0f,0.0f,0.0f,				//back top right
					1.0f,0.0f,0.0f,				//front bottom right	//Right Face
					1.0f,0.0f,0.0f,				//front top right

					1.0f,0.0f,0.0f,				//front bottom right
					1.0f,0.0f,0.0f,				//back top right		//Right Face
					1.0f,0.0f,0.0f,				//back bottom right

					0.0f,1.0f,0.0f,				//back top right
					0.0f,1.0f,0.0f,				//front top right		//Top Face
					0.0f,1.0f,0.0f,				//front top left

					0.0f,1.0f,0.0f,				//back top right
					0.0f,1.0f,0.0f,				//front top left		//Top Face
					0.0f,1.0f,0.0f,				//back top left

					0.0f,0.0f,-1.0f,			//back top right
					0.0f,0.0f,-1.0f,			//back top left			//Back Face
					0.0f,0.0f,-1.0f,			//back bottom right

					//Front Right Leg

					-1.0f,0.0f,0.0f,			//front bottom left
					-1.0f,0.0f,0.0f,			//back bottom left		//Left Face
					-1.0f,0.0f,0.0f,			//back top left

					0.0f,0.0f,1.0f,				//front top right
					0.0f,0.0f,1.0f,				//front bottom left		//Front Face
					0.0f,0.0f,1.0f,				//front top left

					0.0f,-1.0f,0.0f,			//back bottom right
					0.0f,-1.0f,0.0f,			//front bottom left		//Bottom Face
					0.0f,-1.0f,0.0f,			//front bottom right

					0.0f,0.0f,1.0f,				//front top right
					0.0f,0.0f,1.0f,				//front bottom right	//Front Face
					0.0f,0.0f,1.0f,				//front bottom left

					-1.0f,0.0f,0.0f,			//front bottom left
					-1.0f,0.0f,0.0f,			//back top left			//Left Face
					-1.0f,0.0f,0.0f,			//front top left

					0.0f,-1.0f,0.0f,			//back bottom right
					0.0f,-1.0f,0.0f,			//back bottom left		//Bottom Face
					0.0f,-1.0f,0.0f,			//front bottom left

					0.0f,0.0f,-1.0f,			//back top left
					0.0f,0.0f,-1.0f,			//back bottom left		//Back Face
					0.0f,0.0f,-1.0f,			//back bottom right

					1.0f,0.0f,0.0f,				//back top right
					1.0f,0.0f,0.0f,				//front bottom right	//Right Face
					1.0f,0.0f,0.0f,				//front top right

					1.0f,0.0f,0.0f,				//front bottom right
					1.0f,0.0f,0.0f,				//back top right		//Right Face
					1.0f,0.0f,0.0f,				//back bottom right

					0.0f,1.0f,0.0f,				//back top right
					0.0f,1.0f,0.0f,				//front top right		//Top Face
					0.0f,1.0f,0.0f,				//front top left

					0.0f,1.0f,0.0f,				//back top right
					0.0f,1.0f,0.0f,				//front top left		//Top Face
					0.0f,1.0f,0.0f,				//back top left

					0.0f,0.0f,-1.0f,			//back top right
					0.0f,0.0f,-1.0f,			//back top left			//Back Face
					0.0f,0.0f,-1.0f,			//back bottom right

					//Back Left Leg

					-1.0f,0.0f,0.0f,			//front bottom left
					-1.0f,0.0f,0.0f,			//back bottom left		//Left Face
					-1.0f,0.0f,0.0f,			//back top left

					0.0f,0.0f,1.0f,				//front top right
					0.0f,0.0f,1.0f,				//front bottom left		//Front Face
					0.0f,0.0f,1.0f,				//front top left

					0.0f,-1.0f,0.0f,			//back bottom right
					0.0f,-1.0f,0.0f,			//front bottom left		//Bottom Face
					0.0f,-1.0f,0.0f,			//front bottom right

					0.0f,0.0f,1.0f,				//front top right
					0.0f,0.0f,1.0f,				//front bottom right	//Front Face
					0.0f,0.0f,1.0f,				//front bottom left

					-1.0f,0.0f,0.0f,			//front bottom left
					-1.0f,0.0f,0.0f,			//back top left			//Left Face
					-1.0f,0.0f,0.0f,			//front top left

					0.0f,-1.0f,0.0f,			//back bottom right
					0.0f,-1.0f,0.0f,			//back bottom left		//Bottom Face
					0.0f,-1.0f,0.0f,			//front bottom left

					0.0f,0.0f,-1.0f,			//back top left
					0.0f,0.0f,-1.0f,			//back bottom left		//Back Face
					0.0f,0.0f,-1.0f,			//back bottom right

					1.0f,0.0f,0.0f,				//back top right
					1.0f,0.0f,0.0f,				//front bottom right	//Right Face
					1.0f,0.0f,0.0f,				//front top right

					1.0f,0.0f,0.0f,				//front bottom right
					1.0f,0.0f,0.0f,				//back top right		//Right Face
					1.0f,0.0f,0.0f,				//back bottom right

					0.0f,1.0f,0.0f,				//back top right
					0.0f,1.0f,0.0f,				//front top right		//Top Face
					0.0f,1.0f,0.0f,				//front top left

					0.0f,1.0f,0.0f,				//back top right
					0.0f,1.0f,0.0f,				//front top left		//Top Face
					0.0f,1.0f,0.0f,				//back top left

					0.0f,0.0f,-1.0f,			//back top right
					0.0f,0.0f,-1.0f,			//back top left			//Back Face
					0.0f,0.0f,-1.0f,			//back bottom right

					//Back Right Leg

					-1.0f,0.0f,0.0f,			//front bottom left
					-1.0f,0.0f,0.0f,			//back bottom left		//Left Face
					-1.0f,0.0f,0.0f,			//back top left

					0.0f,0.0f,1.0f,				//front top right
					0.0f,0.0f,1.0f,				//front bottom left		//Front Face
					0.0f,0.0f,1.0f,				//front top left

					0.0f,-1.0f,0.0f,			//back bottom right
					0.0f,-1.0f,0.0f,			//front bottom left		//Bottom Face
					0.0f,-1.0f,0.0f,			//front bottom right

					0.0f,0.0f,1.0f,				//front top right
					0.0f,0.0f,1.0f,				//front bottom right	//Front Face
					0.0f,0.0f,1.0f,				//front bottom left

					-1.0f,0.0f,0.0f,			//front bottom left
					-1.0f,0.0f,0.0f,			//back top left			//Left Face
					-1.0f,0.0f,0.0f,			//front top left

					0.0f,-1.0f,0.0f,			//back bottom right
					0.0f,-1.0f,0.0f,			//back bottom left		//Bottom Face
					0.0f,-1.0f,0.0f,			//front bottom left

					0.0f,0.0f,-1.0f,			//back top left
					0.0f,0.0f,-1.0f,			//back bottom left		//Back Face
					0.0f,0.0f,-1.0f,			//back bottom right

					1.0f,0.0f,0.0f,				//back top right
					1.0f,0.0f,0.0f,				//front bottom right	//Right Face
					1.0f,0.0f,0.0f,				//front top right

					1.0f,0.0f,0.0f,				//front bottom right
					1.0f,0.0f,0.0f,				//back top right		//Right Face
					1.0f,0.0f,0.0f,				//back bottom right

					0.0f,1.0f,0.0f,				//back top right
					0.0f,1.0f,0.0f,				//front top right		//Top Face
					0.0f,1.0f,0.0f,				//front top left

					0.0f,1.0f,0.0f,				//back top right
					0.0f,1.0f,0.0f,				//front top left		//Top Face
					0.0f,1.0f,0.0f,				//back top left

					0.0f,0.0f,-1.0f,			//back top right
					0.0f,0.0f,-1.0f,			//back top left			//Back Face
					0.0f,0.0f,-1.0f,			//back bottom right

					//Table Top

					-1.0f,0.0f,0.0f,			//front bottom left
					-1.0f,0.0f,0.0f,			//back bottom left		//Left Face
					-1.0f,0.0f,0.0f,			//back top left

					0.0f,0.0f,1.0f,				//front top right
					0.0f,0.0f,1.0f,				//front bottom left		//Front Face
					0.0f,0.0f,1.0f,				//front top left

					0.0f,-1.0f,0.0f,			//back bottom right
					0.0f,-1.0f,0.0f,			//front bottom left		//Bottom Face
					0.0f,-1.0f,0.0f,			//front bottom right

					0.0f,0.0f,1.0f,				//front top right
					0.0f,0.0f,1.0f,				//front bottom right	//Front Face
					0.0f,0.0f,1.0f,				//front bottom left

					-1.0f,0.0f,0.0f,			//front bottom left
					-1.0f,0.0f,0.0f,			//back top left			//Left Face
					-1.0f,0.0f,0.0f,			//front top left

					0.0f,-1.0f,0.0f,			//back bottom right
					0.0f,-1.0f,0.0f,			//back bottom left		//Bottom Face
					0.0f,-1.0f,0.0f,			//front bottom left

					0.0f,0.0f,-1.0f,			//back top left
					0.0f,0.0f,-1.0f,			//back bottom left		//Back Face
					0.0f,0.0f,-1.0f,			//back bottom right

					1.0f,0.0f,0.0f,				//back top right
					1.0f,0.0f,0.0f,				//front bottom right	//Right Face
					1.0f,0.0f,0.0f,				//front top right

					1.0f,0.0f,0.0f,				//front bottom right
					1.0f,0.0f,0.0f,				//back top right		//Right Face
					1.0f,0.0f,0.0f,				//back bottom right

					0.0f,1.0f,0.0f,				//back top right
					0.0f,1.0f,0.0f,				//front top right		//Top Face
					0.0f,1.0f,0.0f,				//front top left

					0.0f,1.0f,0.0f,				//back top right
					0.0f,1.0f,0.0f,				//front top left		//Top Face
					0.0f,1.0f,0.0f,				//back top left

					0.0f,0.0f,-1.0f,			//back top right
					0.0f,0.0f,-1.0f,			//back top left			//Back Face
					0.0f,0.0f,-1.0f,			//back bottom right

					//Left Bridge

					-1.0f,0.0f,0.0f,			//front bottom left
					-1.0f,0.0f,0.0f,			//back bottom left		//Left Face
					-1.0f,0.0f,0.0f,			//back top left

					0.0f,0.0f,1.0f,				//front top right
					0.0f,0.0f,1.0f,				//front bottom left		//Front Face
					0.0f,0.0f,1.0f,				//front top left

					0.0f,-1.0f,0.0f,			//back bottom right
					0.0f,-1.0f,0.0f,			//front bottom left		//Bottom Face
					0.0f,-1.0f,0.0f,			//front bottom right

					0.0f,0.0f,1.0f,				//front top right
					0.0f,0.0f,1.0f,				//front bottom right	//Front Face
					0.0f,0.0f,1.0f,				//front bottom left

					-1.0f,0.0f,0.0f,			//front bottom left
					-1.0f,0.0f,0.0f,			//back top left			//Left Face
					-1.0f,0.0f,0.0f,			//front top left

					0.0f,-1.0f,0.0f,			//back bottom right
					0.0f,-1.0f,0.0f,			//back bottom left		//Bottom Face
					0.0f,-1.0f,0.0f,			//front bottom left

					0.0f,0.0f,-1.0f,			//back top left
					0.0f,0.0f,-1.0f,			//back bottom left		//Back Face
					0.0f,0.0f,-1.0f,			//back bottom right

					1.0f,0.0f,0.0f,				//back top right
					1.0f,0.0f,0.0f,				//front bottom right	//Right Face
					1.0f,0.0f,0.0f,				//front top right

					1.0f,0.0f,0.0f,				//front bottom right
					1.0f,0.0f,0.0f,				//back top right		//Right Face
					1.0f,0.0f,0.0f,				//back bottom right

					0.0f,1.0f,0.0f,				//back top right
					0.0f,1.0f,0.0f,				//front top right		//Top Face
					0.0f,1.0f,0.0f,				//front top left

					0.0f,1.0f,0.0f,				//back top right
					0.0f,1.0f,0.0f,				//front top left		//Top Face
					0.0f,1.0f,0.0f,				//back top left

					0.0f,0.0f,-1.0f,			//back top right
					0.0f,0.0f,-1.0f,			//back top left			//Back Face
					0.0f,0.0f,-1.0f,			//back bottom right

					//Front Bridge

					-1.0f,0.0f,0.0f,			//front bottom left
					-1.0f,0.0f,0.0f,			//back bottom left		//Left Face
					-1.0f,0.0f,0.0f,			//back top left

					0.0f,0.0f,1.0f,				//front top right
					0.0f,0.0f,1.0f,				//front bottom left		//Front Face
					0.0f,0.0f,1.0f,				//front top left

					0.0f,-1.0f,0.0f,			//back bottom right
					0.0f,-1.0f,0.0f,			//front bottom left		//Bottom Face
					0.0f,-1.0f,0.0f,			//front bottom right

					0.0f,0.0f,1.0f,				//front top right
					0.0f,0.0f,1.0f,				//front bottom right	//Front Face
					0.0f,0.0f,1.0f,				//front bottom left

					-1.0f,0.0f,0.0f,			//front bottom left
					-1.0f,0.0f,0.0f,			//back top left			//Left Face
					-1.0f,0.0f,0.0f,			//front top left

					0.0f,-1.0f,0.0f,			//back bottom right
					0.0f,-1.0f,0.0f,			//back bottom left		//Bottom Face
					0.0f,-1.0f,0.0f,			//front bottom left

					0.0f,0.0f,-1.0f,			//back top left
					0.0f,0.0f,-1.0f,			//back bottom left		//Back Face
					0.0f,0.0f,-1.0f,			//back bottom right

					1.0f,0.0f,0.0f,				//back top right
					1.0f,0.0f,0.0f,				//front bottom right	//Right Face
					1.0f,0.0f,0.0f,				//front top right

					1.0f,0.0f,0.0f,				//front bottom right
					1.0f,0.0f,0.0f,				//back top right		//Right Face
					1.0f,0.0f,0.0f,				//back bottom right

					0.0f,1.0f,0.0f,				//back top right
					0.0f,1.0f,0.0f,				//front top right		//Top Face
					0.0f,1.0f,0.0f,				//front top left

					0.0f,1.0f,0.0f,				//back top right
					0.0f,1.0f,0.0f,				//front top left		//Top Face
					0.0f,1.0f,0.0f,				//back top left

					0.0f,0.0f,-1.0f,			//back top right
					0.0f,0.0f,-1.0f,			//back top left			//Back Face
					0.0f,0.0f,-1.0f,			//back bottom right

					//Right Bridge

					-1.0f,0.0f,0.0f,			//front bottom left
					-1.0f,0.0f,0.0f,			//back bottom left		//Left Face
					-1.0f,0.0f,0.0f,			//back top left

					0.0f,0.0f,1.0f,				//front top right
					0.0f,0.0f,1.0f,				//front bottom left		//Front Face
					0.0f,0.0f,1.0f,				//front top left

					0.0f,-1.0f,0.0f,			//back bottom right
					0.0f,-1.0f,0.0f,			//front bottom left		//Bottom Face
					0.0f,-1.0f,0.0f,			//front bottom right

					0.0f,0.0f,1.0f,				//front top right
					0.0f,0.0f,1.0f,				//front bottom right	//Front Face
					0.0f,0.0f,1.0f,				//front bottom left

					-1.0f,0.0f,0.0f,			//front bottom left
					-1.0f,0.0f,0.0f,			//back top left			//Left Face
					-1.0f,0.0f,0.0f,			//front top left

					0.0f,-1.0f,0.0f,			//back bottom right
					0.0f,-1.0f,0.0f,			//back bottom left		//Bottom Face
					0.0f,-1.0f,0.0f,			//front bottom left

					0.0f,0.0f,-1.0f,			//back top left
					0.0f,0.0f,-1.0f,			//back bottom left		//Back Face
					0.0f,0.0f,-1.0f,			//back bottom right

					1.0f,0.0f,0.0f,				//back top right
					1.0f,0.0f,0.0f,				//front bottom right	//Right Face
					1.0f,0.0f,0.0f,				//front top right

					1.0f,0.0f,0.0f,				//front bottom right
					1.0f,0.0f,0.0f,				//back top right		//Right Face
					1.0f,0.0f,0.0f,				//back bottom right

					0.0f,1.0f,0.0f,				//back top right
					0.0f,1.0f,0.0f,				//front top right		//Top Face
					0.0f,1.0f,0.0f,				//front top left

					0.0f,1.0f,0.0f,				//back top right
					0.0f,1.0f,0.0f,				//front top left		//Top Face
					0.0f,1.0f,0.0f,				//back top left

					0.0f,0.0f,-1.0f,			//back top right
					0.0f,0.0f,-1.0f,			//back top left			//Back Face
					0.0f,0.0f,-1.0f,			//back bottom right

					//Back Bridge

					-1.0f,0.0f,0.0f,			//front bottom left
					-1.0f,0.0f,0.0f,			//back bottom left		//Left Face
					-1.0f,0.0f,0.0f,			//back top left

					0.0f,0.0f,1.0f,				//front top right
					0.0f,0.0f,1.0f,				//front bottom left		//Front Face
					0.0f,0.0f,1.0f,				//front top left

					0.0f,-1.0f,0.0f,			//back bottom right
					0.0f,-1.0f,0.0f,			//front bottom left		//Bottom Face
					0.0f,-1.0f,0.0f,			//front bottom right

					0.0f,0.0f,1.0f,				//front top right
					0.0f,0.0f,1.0f,				//front bottom right	//Front Face
					0.0f,0.0f,1.0f,				//front bottom left

					-1.0f,0.0f,0.0f,			//front bottom left
					-1.0f,0.0f,0.0f,			//back top left			//Left Face
					-1.0f,0.0f,0.0f,			//front top left

					0.0f,-1.0f,0.0f,			//back bottom right
					0.0f,-1.0f,0.0f,			//back bottom left		//Bottom Face
					0.0f,-1.0f,0.0f,			//front bottom left

					0.0f,0.0f,-1.0f,			//back top left
					0.0f,0.0f,-1.0f,			//back bottom left		//Back Face
					0.0f,0.0f,-1.0f,			//back bottom right

					1.0f,0.0f,0.0f,				//back top right
					1.0f,0.0f,0.0f,				//front bottom right	//Right Face
					1.0f,0.0f,0.0f,				//front top right

					1.0f,0.0f,0.0f,				//front bottom right
					1.0f,0.0f,0.0f,				//back top right		//Right Face
					1.0f,0.0f,0.0f,				//back bottom right

					0.0f,1.0f,0.0f,				//back top right
					0.0f,1.0f,0.0f,				//front top right		//Top Face
					0.0f,1.0f,0.0f,				//front top left

					0.0f,1.0f,0.0f,				//back top right
					0.0f,1.0f,0.0f,				//front top left		//Top Face
					0.0f,1.0f,0.0f,				//back top left

					0.0f,0.0f,-1.0f,			//back top right
					0.0f,0.0f,-1.0f,			//back top left			//Back Face
					0.0f,0.0f,-1.0f				//back bottom right
			};

			glGenVertexArrays(1, &VertexArrayID);
			glBindVertexArray(VertexArrayID);

			glGenBuffers(1, &vertexbuffer);
			glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

			glGenBuffers(1, &uvbuffer);
			glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(g_uv_buffer_data), g_uv_buffer_data, GL_STATIC_DRAW);

			glGenBuffers(1, &normalbuffer);
			glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(normal_buffer_data), normal_buffer_data, GL_STATIC_DRAW);
}
void UKeyboard(unsigned char key, GLint x, GLint y)
{
	//Takes input from the keyboard
	currentKey = key;
	switch (currentKey){
	case 'r':
		if(lightColor.r == 1.0f){
			lightColor.r = 0.0f;
		}
		else {
			lightColor.r = 1.0f;
		}
		break;
	case 'f':
		if(lightColor.g == 1.0f){
			lightColor.g = 0.0f;
		}
		else {
			lightColor.g = 1.0f;
		}
		break;
	case 'v':
		if(lightColor.b == 1.0f){
			lightColor.b = 0.0f;
		}
		else {
			lightColor.b = 1.0f;
		}
		break;
	case 'x':
		lightIntensity -= 2.0f;
		break;
	case 'c':
		lightIntensity += 2.0f;
		break;
	case 'a':
		lightPos.x--;
		break;
	case 'd':
		lightPos.x++;
		break;
	case 'w':
		lightPos.y++;
		break;
	case 's':
		lightPos.y--;
		break;
	case 'q':
		lightPos.z--;
		break;
	case 'e':
		lightPos.z++;
		break;
	default:
		break;
	}

	glutPostRedisplay();
}

void UKeyReleased(unsigned char key, GLint x, GLint y)
{
	//Takes note of when buttons are released
	currentKey = '0';

	glutPostRedisplay();
}

void UMouseMove(int x, int y)
{
	// Immediately replaces center locked coordinates with new mouse coordinates
	if(mouseDetected)
		{
			lastMouseX = x;
			lastMouseY = y;
			mouseDetected = false;
		}

	// Gets the direction the mouse was moved in x and y
	mouseXOffset = x - lastMouseX;
	mouseYOffset = lastMouseY - y; // Inverted Y

	// Updates with new mouse coordinates
	lastMouseX = x;
	lastMouseY = y;

	// Applies sensitivity to mouse direction
	mouseXOffset *= sensitivity;
	mouseYOffset *= sensitivity;

	// Accumulates the yaw and pitch variables
	if ((glutGetModifiers() == GLUT_ACTIVE_ALT)) {
		camYaw += mouseXOffset;
		if (camYaw > 1.57f)//Cannot rotate more than 90 degrees as per project requirements
			camYaw = 1.57f;
		if (camYaw < -1.57f)
			camYaw = -1.57f;
		camPitch += mouseYOffset;
		if (camPitch > 1.57f)//Cannot rotate more than 90 degrees as per project requirements
			camPitch = 1.57f;
		if (camPitch < -1.57f)
			camPitch = -1.57f;
	}
	// Orbits around the center
	front.x = 10.0f * cos(camYaw);
	front.y = 10.0f * sin(camPitch);
	front.z = sin(camYaw) * cos(camPitch) * 1.0f;

	glutPostRedisplay();
}
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path){

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open()){
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}else{
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;


	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}



	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}



	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> ProgramErrorMessage(InfoLogLength+1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}


	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

GLuint loadBMP_custom(const char * imagepath){

	printf("Reading image %s\n", imagepath);

	// Data read from the header of the BMP file
	unsigned char header[54];
	unsigned int dataPos;
	unsigned int imageSize;
	unsigned int width, height;
	// Actual RGB data
	unsigned char * data;

	// Open the file
	FILE * file = fopen(imagepath,"rb");
	if (!file){
		printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", imagepath);
		getchar();
		return 0;
	}

	// Read the header, i.e. the 54 first bytes

	// If less than 54 bytes are read, problem
	if ( fread(header, 1, 54, file)!=54 ){
		printf("Not a correct BMP file\n");
		fclose(file);
		return 0;
	}
	// A BMP files always begins with "BM"
	if ( header[0]!='B' || header[1]!='M' ){
		printf("Not a correct BMP file\n");
		fclose(file);
		return 0;
	}
	// Make sure this is a 24bpp file
	if ( *(int*)&(header[0x1E])!=0  )         {printf("Not a correct BMP file\n");    fclose(file); return 0;}
	if ( *(int*)&(header[0x1C])!=24 )         {printf("Not a correct BMP file\n");    fclose(file); return 0;}

	// Read the information about the image
	dataPos    = *(int*)&(header[0x0A]);
	imageSize  = *(int*)&(header[0x22]);
	width      = *(int*)&(header[0x12]);
	height     = *(int*)&(header[0x16]);

	// Some BMP files are misformatted, guess missing information
	if (imageSize==0)    imageSize=width*height*3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos==0)      dataPos=54; // The BMP header is done that way

	// Create a buffer
	data = new unsigned char [imageSize];

	// Read the actual data from the file into the buffer
	fread(data,1,imageSize,file);

	// Everything is in memory now, the file can be closed.
	fclose (file);

	// Create one OpenGL texture
	GLuint textureID;
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

	// OpenGL has now copied the data. Free our own version
	delete [] data;

	//trilinear filtering ...
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//...which requires mipmaps. Generate them automatically.
	glGenerateMipmap(GL_TEXTURE_2D);

	// Return the ID of the texture we just created
	return textureID;
}
