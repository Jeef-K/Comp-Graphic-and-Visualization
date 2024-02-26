#include <iostream>
#include <cstdlib>
#include <vector>
#include <cmath>


#include<glad/glad.h>
//#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"      // Image loading Utility functions
#include "corecrt_math_defines.h"



//glm math header inclusions
#include<glm/glm.hpp>
#include<glm/gtx/transform.hpp>
#include<glm/gtc/type_ptr.hpp>

#include "camera.h" // Camera class


//shader program macro
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif // !GLSL



namespace {
	const char* const WINDOW_TITLE = "CS 330 assignment 2D Triangles";

	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;

	struct GLMesh
	{
		GLuint vao; //handle for vertex array object
		GLuint vbos[2]; //handle for vertex buffer object
		GLuint nIndices[7]; //number of indices of the mesh
	};


	//main GLFW window
	GLFWwindow* gWindow = nullptr;

	//triangle mesh data
	GLMesh gMesh;

	// Texture id
	GLuint gTextureId[9];
	glm::vec2 gUVScale(5.0f, 5.0f);
	GLint gTexWrapMode = GL_REPEAT;

	// Shader programs
	GLuint gCubeProgramId;
	GLuint gLampProgramId;


	// camera
	Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f));
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;
	bool isPerspective = true;

	// timing
	float gDeltaTime = 0.0f; // time between current frame and last frame
	float gLastFrame = 0.0f;



	// Subject position and scale
	glm::vec3 gCubePosition(0.0f, 0.0f, 0.0f);
	glm::vec3 gCubeScale(0.3f);

	// Cube and light color
	//m::vec3 gObjectColor(0.6f, 0.5f, 0.75f);
	glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
	glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);

	// Light position and scale
	glm::vec3 gLightPosition(10.0f, 10.0f, 10.0f);
	glm::vec3 gLightScale(0.3f);

	// Lamp animation
	bool gIsLampOrbiting = false;
}






bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);
void URender();

void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);

void generateCylinder(float radius, float height, int segments, std::vector<GLfloat>& vertices, std::vector<GLfloat>& texCoords);
void generateSphere(float radius, int sectors, int stacks, std::vector<GLfloat>& vertices, std::vector<GLfloat>& texCoords);




/* Cube Vertex Shader Source Code*/
const GLchar* cubeVertexShaderSource = GLSL(440,

	layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

	vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

	vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
	vertexTextureCoordinate = textureCoordinate;
}
);


/* Cube Fragment Shader Source Code*/
const GLchar* cubeFragmentShaderSource = GLSL(440,

	in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;

void main()
{
	/*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

	//Calculate Ambient lighting*/
	float ambientStrength = 0.1f; // Set ambient or global lighting strength
	vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

	//Calculate Diffuse lighting*/
	vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
	vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
	float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
	vec3 diffuse = impact * lightColor; // Generate diffuse light color

	//Calculate Specular lighting*/
	float specularIntensity = 0.8f; // Set specular light strength
	float highlightSize = 16.0f; // Set specular highlight size
	vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
	vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
	//Calculate specular component
	float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
	vec3 specular = specularIntensity * specularComponent * lightColor;

	// Texture holds the color to be used for all three components
	vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

	// Calculate phong result
	vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

	fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);



/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

	layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

		//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

	out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
	fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);



// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; ++j)
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;

		for (int i = width * channels; i > 0; --i)
		{
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			++index1;
			++index2;
		}
	}
}



int main(int argc, char* argv[]) {



	if (!UInitialize(argc, argv, &gWindow)) {
		return EXIT_FAILURE;
	}

	//create the mesh
	UCreateMesh(gMesh);

	// Create the shader programs
	if (!UCreateShaderProgram(cubeVertexShaderSource, cubeFragmentShaderSource, gCubeProgramId))
		return EXIT_FAILURE;

	if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
		return EXIT_FAILURE;


	// Load texture (relative to project's directory)
	const char* texFilename = "white_tile.jpg";
	if (!UCreateTexture(texFilename, gTextureId[0]))
	{
		std::cout << "Failed to load texture " << texFilename << std::endl;
		return EXIT_FAILURE;
	}

	// Load texture (relative to project's directory)
	const char* texFilename2 = "yellow_texture.jpg";
	if (!UCreateTexture(texFilename2, gTextureId[1]))
	{
		std::cout << "Failed to load texture " << texFilename2 << std::endl;
		return EXIT_FAILURE;
	}

	// Load texture (relative to project's directory)
	const char* texFilename3 = "cone.jpg";
	if (!UCreateTexture(texFilename3, gTextureId[2]))
	{
		std::cout << "Failed to load texture " << texFilename3 << std::endl;
		return EXIT_FAILURE;
	}

	// Load texture (relative to project's directory)
	const char* texFilename4 = "box_of_cards.jpg";
	if (!UCreateTexture(texFilename4, gTextureId[3]))
	{
		std::cout << "Failed to load texture " << texFilename4 << std::endl;
		return EXIT_FAILURE;
	}

	// Load texture (relative to project's directory)
	const char* texFilename5 = "grey_plastic.jpg";
	if (!UCreateTexture(texFilename5, gTextureId[4]))
	{
		std::cout << "Failed to load texture " << texFilename5 << std::endl;
		return EXIT_FAILURE;
	}

	// Load texture (relative to project's directory)
	const char* texFilename6 = "red_plastic.jpg";
	if (!UCreateTexture(texFilename6, gTextureId[5]))
	{
		std::cout << "Failed to load texture " << texFilename6 << std::endl;
		return EXIT_FAILURE;
	}

	// Load texture (relative to project's directory)
	const char* texFilename7 = "pink_texture.jpg";
	if (!UCreateTexture(texFilename7, gTextureId[6]))
	{
		std::cout << "Failed to load texture " << texFilename7 << std::endl;
		return EXIT_FAILURE;
	}

	// Load texture (relative to project's directory)
	const char* texFilename8 = "blue_texture.jpg";
	if (!UCreateTexture(texFilename8, gTextureId[7]))
	{
		std::cout << "Failed to load texture " << texFilename8 << std::endl;
		return EXIT_FAILURE;
	}
	// Load texture (relative to project's directory)
	const char* texFilename9 = "green_texture.jpg";
	if (!UCreateTexture(texFilename9, gTextureId[8]))
	{
		std::cout << "Failed to load texture " << texFilename9 << std::endl;
		return EXIT_FAILURE;
	}


	// tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	glUseProgram(gCubeProgramId);
	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gCubeProgramId, "uTexture"), 0);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);


	// render loop

	while (!glfwWindowShouldClose(gWindow)) {

		// per-frame timing
		// --------------------
		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;

		//input
		UProcessInput(gWindow);

		URender();

		glfwPollEvents();
	}

	UDestroyMesh(gMesh);

	// Release texture
	UDestroyTexture(gTextureId[0]);

	// Release shader programs
	UDestroyShaderProgram(gCubeProgramId);
	UDestroyShaderProgram(gLampProgramId);

	//terminates program successfully
	exit(EXIT_SUCCESS);
}

bool UInitialize(int argc, char* argv[], GLFWwindow** window) {

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif // __APPLE__

	//GLFW: window creation
	* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);

	if (*window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);
	glfwSetCursorPosCallback(*window, UMousePositionCallback);
	glfwSetScrollCallback(*window, UMouseScrollCallback);
	glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	//GLEW: intialize
	/*glewExperimental = GL_TRUE;
	GLenum GlewInitResult = glewInit();

	if (GLEW_OK != GlewInitResult) {
		std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
		return false;
	}*/

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	//display GPU OpenGL version
	std::cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

	return true;
}


void UProcessInput(GLFWwindow* window) {
	static const float cameraSpeed = 2.5f;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		gCamera.ProcessKeyboard(LEFT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		gCamera.ProcessKeyboard(UP, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		gCamera.ProcessKeyboard(DOWN, gDeltaTime);

	// toggle normal and orthographic projection when user presses "P"
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		isPerspective = !isPerspective;


	if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS)
	{
		gUVScale += 0.1f;
		std::cout << "Current scale (" << gUVScale[0] << ", " << gUVScale[1] << ")" << std::endl;
	}
	else if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS)
	{
		gUVScale -= 0.1f;
		std::cout << "Current scale (" << gUVScale[0] << ", " << gUVScale[1] << ")" << std::endl;
	}

	// Pause and resume lamp orbiting
	static bool isLKeyDown = false;
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !gIsLampOrbiting)
		gIsLampOrbiting = true;
	else if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && gIsLampOrbiting)
		gIsLampOrbiting = false;
}

void UResizeWindow(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}


void UCreateMesh(GLMesh& mesh) {

	//static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	//specifies normalized device coordinates (x,y,z) and color (r,g,b,a) for triangle verticies
	GLfloat verts[] =
	{
		//vertex Positions   //colors						//texture coords	//normals

		//Top cylinder
		0.0f, 0.0f, 5.0f,		0.8f, 0.5f, 1.0f, 1.0f,		1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom back middle vertex 0

		0.5f, 0.5f, 5.0f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom back right vertex 1
		0.7f, 0.0f, 5.0f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom middle right vertex 2
		0.5f, -0.5f, 5.0f,		0.0f, 1.0f, 1.0f, 1.0f,		1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom front right vertex 3

		0.0f, -0.7f, 5.0f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom back middle vertex 4

		-0.5f, -0.5f, 5.0f,		0.5f, 0.0f, 0.5f, 1.0f,		1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom front left vertex 5
		-0.7f, 0.0f, 5.0f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom middle left vertex 6
		-0.5f, 0.5f, 5.0f,		1.0f, 0.0f, 0.0f, 1.0f,		1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back left vertex 7

		0.0f, 0.7f, 5.0f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom front middle vertex 8








		//Bottom cylinder
		0.0f, 0.0f, -10.0f,		0.8f, 0.5f, 1.0f, 1.0f,		1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom back middle vertex 9

		0.5f, 0.5f, -10.0f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back right vertex 10
		0.7f, 0.0f, -10.0f,		0.0f, 1.0f, 0.0f, 1.0f,		0.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom middle right vertex 11
		0.5f, -0.5f, -10.0f,		0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom front right vertex 12

		0.0f, -0.7f, -10.0f,		1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back middle vertex 17

		-0.5f, -0.5f, -10.0f,		0.5f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom front left vertex 16
		-0.7f, 0.0f, -10.0f,		0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom middle left vertex 15
		-0.5f, 0.5f, -10.0f,		1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back left vertex 14

		0.0f, 0.7f, -10.0f,		1.0f, 1.0f, 0.0f, 1.0f,		0.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom front middle vertex 13









		//Sphere top of cylinder

		//top
		0.0f, 0.0f, 7.0f,		0.8f, 0.5f, 1.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//bottom back middle vertex 18

		//middle top
		0.7f, 0.7f, 6.8f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back right vertex 19
		0.9f, 0.0f, 6.8f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//middle right vertex 20
		0.7f, -0.7f, 6.8f,		0.0f, 1.0f, 1.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front right vertex 21

		0.0f, -0.9f, 6.8f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back middle vertex 22

		-0.7f, -0.7f, 6.8f,		0.5f, 0.0f, 0.5f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front left vertex 23
		-0.9f, 0.0f, 6.8f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//middle left vertex 24
		-0.7f, 0.7f, 6.8f,		1.0f, 0.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back left vertex 25

		0.0f, 0.9f, 6.8f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front middle vertex 26








		//middle upper
		1.0f, 1.0f, 6.2f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back right vertex 27
		1.2f, 0.0f, 6.2f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//middle right vertex 28
		1.0f, -1.0f, 6.2f,		0.0f, 1.0f, 1.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front right vertex 29

		0.0f, -1.2f, 6.2f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back middle vertex 30

		-1.0f, -1.0f, 6.2f,		0.5f, 0.0f, 0.5f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front left vertex 31
		-1.2f, 0.0f, 6.2f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//middle left vertex 32
		-1.0f, 1.0f, 6.2f,		1.0f, 0.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back left vertex 33

		0.0f, 1.2f, 6.2f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front middle vertex 34




		//middle lower
		1.0f, 1.0f, 5.3f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back right vertex 35
		1.2f, 0.0f, 5.3f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//middle right vertex 36
		1.0f, -1.0f, 5.3f,		0.0f, 1.0f, 1.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front right vertex 37

		0.0f, -1.2f, 5.3f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back middle vertex 38

		-1.0f, -1.0f, 5.3f,		0.5f, 0.0f, 0.5f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front left vertex 39
		-1.2f, 0.0f, 5.3f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//middle left vertex 40
		-1.0f, 1.0f, 5.3f,		1.0f, 0.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back left vertex 41

		0.0f, 1.2f, 5.3f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front middle vertex 42




		//middle bottom
		0.7f, 0.7f, 4.2f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back right vertex 43
		0.9f, 0.0f, 4.2f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//middle right vertex 44
		0.7f, -0.7f, 4.2f,		0.0f, 1.0f, 1.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front right vertex 45

		0.0f, -0.9f, 4.2f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back middle vertex 46

		-0.7f, -0.7f, 4.2f,		0.5f, 0.0f, 0.5f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front left vertex 47
		-0.9f, 0.0f, 4.2f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//middle left vertex 48
		-0.7f, 0.7f, 4.2f,		1.0f, 0.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back left vertex 49

		0.0f, 0.9f, 4.2f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front middle vertex 50



		//bottom
		0.0f, 0.0f, 5.0f,		0.8f, 0.5f, 1.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//bottom back middle vertex 51











		//sphere bottom of cylinder

		//top
		0.0f, 0.0f, -12.0f,		0.8f, 0.5f, 1.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//bottom back middle vertex 52

		//middle top
		0.7f, 0.7f, -11.8f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back right vertex 53
		0.9f, 0.0f, -11.8f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//middle right vertex 54
		0.7f, -0.7f, -11.8f,		0.0f, 1.0f, 1.0f, 1.0f,	1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front right vertex 55

		0.0f, -0.9f, -11.8f,		1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back middle vertex 56

		-0.7f, -0.7f, -11.8f,		0.5f, 0.0f, 0.5f, 1.0f, 1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front left vertex 57
		-0.9f, 0.0f, -11.8f,		0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//middle left vertex 58
		-0.7f, 0.7f, -11.8f,		1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back left vertex 59

		0.0f, 0.9f, -11.8f,		1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//front middle vertex 60




		//middle upper
		1.0f, 1.0f, -11.2f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back right vertex 61
		1.2f, 0.0f, -11.2f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//middle right vertex 62
		1.0f, -1.0f, -11.2f,		0.0f, 1.0f, 1.0f, 1.0f,	1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front right vertex 63

		0.0f, -1.2f, -11.2f,		1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back middle vertex 64

		-1.0f, -1.0f, -11.2f,		0.5f, 0.0f, 0.5f, 1.0f, 1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front left vertex 65
		-1.2f, 0.0f, -11.2f,		0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//middle left vertex 66
		-1.0f, 1.0f, -11.2f,		1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back left vertex 67

		0.0f, 1.2f, -11.2f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front middle vertex 68




		//middle lower
		1.0f, 1.0f, -10.3f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back right vertex 69
		1.2f, 0.0f, -10.3f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//middle right vertex 70
		1.0f, -1.0f, -10.3f,		0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front right vertex 71

		0.0f, -1.2f, -10.3f,		1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back middle vertex 72

		-1.0f, -1.0f, -10.3f,		0.5f, 0.0f, 0.5f, 1.0f, 1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front left vertex 73
		-1.2f, 0.0f, -10.3f,		0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//middle left vertex 74
		-1.0f, 1.0f, -10.3f,		1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back left vertex 75

		0.0f, 1.2f, -10.3f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front middle vertex 76




		//middle bottom
		0.7f, 0.7f, -9.2f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back right vertex 77
		0.9f, 0.0f, -9.2f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//middle right vertex 78
		0.7f, -0.7f, -9.2f,		0.0f, 1.0f, 1.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front right vertex 79

		0.0f, -0.9f, -9.2f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back middle vertex 80

		-0.7f, -0.7f, -9.2f,		0.5f, 0.0f, 0.5f, 1.0f, 1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front left vertex 81
		-0.9f, 0.0f, -9.2f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//middle left vertex 82
		-0.7f, 0.7f, -9.2f,		1.0f, 0.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//back left vertex 83

		0.0f, 0.9f, -9.2f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//front middle vertex 84


		//bottom
		0.0f, 0.0f, -10.0f,		0.8f, 0.5f, 1.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//bottom back middle vertex 85




		//plane

		-15.0f, -1.2f, -15.0f,	1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,		-1.0f, 0.0f, 0.0f,//vertex 86
		15.0f, -1.2f, -15.0f,   1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		1.0f, 0.0f, 0.0f,//vertex 87
		15.0f, -1.2f,  15.0f,    1.0f, 1.0f, 0.0f, 1.0f,	0.0f, 0.0f,		1.0f, 0.0f, 0.0f,//vertex 88
		-15.0f, -1.2f, 15.0f,   1.0f, 1.0f, 0.0f, 1.0f,		0.0f, 1.0f,		-1.0f, 0.0f, 0.0f,//vertex 89









		//Top cylinder
		0.0f, 0.0f, 5.0f,	 0.8f, 0.5f, 1.0f, 1.0f,	 1.0f, 1.0f,	0.0f, 0.0f, 0.0f,//bottom back middle vertex 90

		0.5f, 0.5f, 5.0f,	1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,		0.0f, 0.0f, 0.0f,//bottom back right vertex 91
		0.7f, 0.0f, 5.0f,	0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,		0.0f, 0.0f, 0.0f,//bottom middle right vertex 92
		0.5f, -0.5f, 5.0f,	0.0f, 1.0f, 1.0f, 1.0f,		1.0f, 1.0f,		0.0f, 0.0f, 0.0f,//bottom front right vertex 93

		0.0f, -0.7f, 5.0f,	1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,		0.0f, 0.0f, 0.0f,//bottom back middle vertex 94

		-0.5f, -0.5f, 5.0f,	0.5f, 0.0f, 0.5f, 1.0f,		1.0f, 1.0f,		0.0f, 0.0f, 0.0f,//bottom front left vertex 95
		-0.7f, 0.0f, 5.0f,	0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,		0.0f, 0.0f, 0.0f,//bottom middle left vertex 96
		-0.5f, 0.5f, 5.0f,	1.0f, 0.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//bottom back left vertex 97

		0.0f, 0.7f, 5.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,//bottom front middle vertex 98
			


		//Bottom cylinder
		0.5f, 0.5f, -10.0f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back right vertex 99
		0.7f, 0.0f, -10.0f,		0.0f, 1.0f, 0.0f, 1.0f,		0.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom middle right vertex 100
		0.5f, -0.5f, -10.0f,		0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom front right vertex 101

		0.0f, -0.7f, -10.0f,		1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back middle vertex 102

		-0.5f, -0.5f, -10.0f,		0.5f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom front left vertex 103
		-0.7f, 0.0f, -10.0f,		0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom middle left vertex 104
		-0.5f, 0.5f, -10.0f,		1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back left vertex 105

		0.0f, 0.7f, -10.0f,		1.0f, 1.0f, 0.0f, 1.0f,		0.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom front middle vertex 106






		//Bottom cylinder edge 1
		0.6f, 0.6f, -10.0f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back right vertex 107
		0.8f, 0.0f, -10.0f,		0.0f, 1.0f, 0.0f, 1.0f,		0.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom middle right vertex 108
		0.6f, -0.6f, -10.0f,		0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom front right vertex 109

		0.0f, -0.8f, -10.0f,		1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back middle vertex 110

		-0.6f, -0.6f, -10.0f,		0.5f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom front left vertex 111
		-0.8f, 0.0f, -10.0f,		0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom middle left vertex 112
		-0.6f, 0.6f, -10.0f,		1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back left vertex 113

		0.0f, 0.8f, -10.0f,		1.0f, 1.0f, 0.0f, 1.0f,		0.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom front middle vertex 114





		//Bottom cylinder edge 2
		0.6f, 0.6f, -9.6f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back right vertex 115
		0.8f, 0.0f, -9.6f,		0.0f, 1.0f, 0.0f, 1.0f,		0.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom middle right vertex 116
		0.6f, -0.6f, -9.6f,		0.0f, 1.0f, 1.0f, 1.0f,		1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom front right vertex 117

		0.0f, -0.8f, -9.6f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back middle vertex 118

		-0.6f, -0.6f, -9.6f,	0.5f, 0.0f, 0.5f, 1.0f,		0.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom front left vertex 119
		-0.8f, 0.0f, -9.6f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom middle left vertex 120
		-0.6f, 0.6f, -9.6f,		1.0f, 0.0f, 0.0f, 1.0f,		1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back left vertex 121

		0.0f, 0.8f, -9.6f,		1.0f, 1.0f, 0.0f, 1.0f,		0.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom front middle vertex 122




		//Bottom cylinder edge 3
		0.5f, 0.5f, -9.6f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back right vertex 123
		0.7f, 0.0f, -9.6f,		0.0f, 1.0f, 0.0f, 1.0f,		0.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom middle right vertex 124
		0.5f, -0.5f, -9.6f,		0.0f, 1.0f, 1.0f, 1.0f,		1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom front right vertex 125

		0.0f, -0.7f, -9.6f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back middle vertex 126

		-0.5f, -0.5f, -9.6f,	0.5f, 0.0f, 0.5f, 1.0f,		0.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom front left vertex 127
		-0.7f, 0.0f, -9.6f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom middle left vertex 128
		-0.5f, 0.5f, -9.6f,		1.0f, 0.0f, 0.0f, 1.0f,		1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back left vertex 129

		0.0f, 0.7f, -9.6f,		1.0f, 1.0f, 0.0f, 1.0f,		0.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom front middle vertex 130








		//box for cards
		
		
        //Back Face          
       -0.5f, -0.5f, -10.5f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,		0.0f,  0.0f, -1.0f, // back bottom left vertex 131
        0.5f, -0.5f, -10.5f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,		0.0f,  0.0f, -1.0f, // back bottom right vertex 132
        0.5f,  0.5f, -10.5f,		1.0f, 0.0f, 1.0f, 1.0f,		1.0f, 1.0f,		0.0f,  0.0f, -1.0f,	// back top right vertex 133
       -0.5f,  0.5f, -10.5f,		1.0f, 0.0f, 1.0f, 1.0f,		1.0f, 1.0f,		0.0f,  0.0f, -1.0f,	// back top left vertex 134

        //Front Face         
       -0.5f, -0.5f,  10.5f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f,  0.0f,  1.0f, // front bottom left vertex 135
        0.5f, -0.5f,  10.5f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f,  0.0f,  1.0f, // front bottom right vertex 136
        0.5f,  0.5f,  10.5f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f,  0.0f,  1.0f, // front top right vertex 137
       -0.5f,  0.5f,  10.5f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f,  0.0f,  1.0f, // front top left vertex 138










		//box for toy knife blade
		
        //Back Face          
       -0.1f, -0.5f, -10.5f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f,  0.0f, -1.0f, // back bottom left vertex 139
        0.1f, -0.3f, -10.5f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f,  0.0f, -1.0f, // back bottom right vertex 140
        0.5f,  0.3f, -10.1f,		1.0f, 0.0f, 1.0f, 1.0f,		0.0f,-0.5f,		0.0f,  0.0f, -1.0f,	// back top right vertex 141
       -0.5f,  0.3f, -10.1f,		1.0f, 0.0f, 1.0f, 1.0f,		0.0f, -0.5f,	0.0f,  0.0f, -1.0f,	// back top left vertex 142

        //Front Face         
       -0.1f, -0.4f,  10.5f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f,  0.0f,  1.0f, // front bottom left vertex 143
        0.1f, -0.3f,  10.5f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f,  0.0f,  1.0f, // front bottom right vertex 144
        0.5f,  0.5f,  10.5f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f,  0.0f,  1.0f, // front top right vertex 145
       -0.5f,  0.5f,  10.5f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f,  0.0f,  1.0f, // front top left vertex 146




		//cylander for toy knide handle

		//Top cylinder
		0.0f, 0.0f, 5.0f,		0.8f, 0.5f, 1.0f, 1.0f,		1.0f, 1.0f,		0.0f, 0.0f, 0.0f,//bottom back middle vertex 147

		0.5f, 0.5f, 5.0f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,		0.0f, 0.0f, 0.0f,//bottom back right vertex 148
		0.7f, 0.0f, 5.0f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,		0.0f, 0.0f, 0.0f,//bottom middle right vertex 149
		0.5f, -0.5f, 5.0f,		0.0f, 1.0f, 1.0f, 1.0f,		1.0f, 1.0f,		0.0f, 0.0f, 0.0f,//bottom front right vertex 150

		0.0f, -0.7f, 5.0f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,		0.0f, 0.0f, 0.0f,//bottom back middle vertex 151

		-0.5f, -0.5f, 5.0f,		0.5f, 0.0f, 0.5f, 1.0f,		1.0f, 1.0f,		0.0f, 0.0f, 0.0f,//bottom front left vertex 152
		-0.7f, 0.0f, 5.0f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,		0.0f, 0.0f, 0.0f,//bottom middle left vertex 153
		-0.5f, 0.5f, 5.0f,		1.0f, 0.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,//bottom back left vertex 154

		0.0f, 0.7f, 5.0f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,		0.0f, 0.0f, 0.0f,//bottom front middle vertex 155
			


		//Bottom cylinder
		0.5f, 0.5f, -10.0f,			1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back right vertex 156
		0.7f, 0.0f, -10.0f,			0.0f, 1.0f, 0.0f, 1.0f,		0.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom middle right vertex 157
		0.5f, -0.5f, -10.0f,		0.0f, 1.0f, 1.0f, 1.0f,		1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom front right vertex 158

		0.0f, -0.7f, -10.0f,		1.0f, 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back middle vertex 159

		-0.5f, -0.5f, -10.0f,		0.5f, 0.0f, 0.5f, 1.0f,		0.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom front left vertex 160
		-0.7f, 0.0f, -10.0f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom middle left vertex 161
		-0.5f, 0.5f, -10.0f,		1.0f, 0.0f, 0.0f, 1.0f,		1.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom back left vertex 162

		0.0f, 0.7f, -10.0f,			1.0f, 1.0f, 0.0f, 1.0f,		0.0f, 0.0f,			0.0f, 0.0f, 0.0f,//bottom front middle vertex 163

		0.0f, 0.0f, -10.0f,			0.8f, 0.5f, 1.0f, 1.0f,		1.0f, 1.0f,			0.0f, 0.0f, 0.0f,//bottom back middle vertex 164




	};

	//index data to share position data
	GLushort indices[] = {
		//cylinder top triangles
		0,1,2,
		0,2,3,
		0,3,4,
		0,4,5,
		0,5,6,
		0,6,7,
		0,7,8,
		0,8,1,

		//cylinder bottom triangles
		9,10,11,
		9,11,12,
		9,12,13,
		9,13,14,
		9,14,15,
		9,15,16,
		9,16,17,
		9,17,10,

		//cylinder side triangles
		1,10,11,
		11,1,2,
		2,11,12,
		12,2,3,
		3,12,13,
		13,3,4,
		4,13,14,
		14,4,5,
		5,14,15,
		15,5,6,
		6,15,16,
		16,6,7,
		7,16,17,
		17,7,8,
		8,17,10,
		10,8,1,




		//sphere top of cylinder

		//sphere top
		18,19,20,
		18,20,21,
		18,21,22,
		18,22,23,
		18,23,24,
		18,24,25,
		18,25,26,
		18,26,19,

		//sphere middle upper
		19,27,28,
		28,19,20,
		20,28,29,
		29,20,21,
		21,29,30,
		30,21,22,
		22,30,31,
		31,22,23,
		23,31,32,
		32,23,24,
		24,32,33,
		33,24,25,
		25,33,34,
		34,25,26,
		26,34,27,
		27,26,19,



		//sphere middle
		35,27,28,
		28,35,36,
		36,28,29,
		29,36,37,
		37,29,30,
		30,37,38,
		38,30,31,
		31,38,39,
		39,31,32,
		32,39,40,
		40,32,33,
		33,40,41,
		41,33,34,
		34,41,42,
		42,34,27,
		27,42,35,



		//sphere middle lower
		43,35,36,
		36,43,44,
		44,36,37,
		37,44,45,
		45,37,38,
		38,45,46,
		46,38,39,
		39,46,47,
		47,39,40,
		40,47,48,
		48,40,41,
		41,48,49,
		49,41,42,
		42,49,50,
		50,42,35,
		35,50,43,


		//sphere bottom
		51,43,44,
		51,44,45,
		51,45,46,
		51,46,47,
		51,47,48,
		51,48,49,
		51,49,50,
		51,50,43,



		//sphere bottom of cylinder

		//sphere top
		52,53,54,
		52,54,55,
		52,55,56,
		52,56,57,
		52,57,58,
		52,58,59,
		52,59,60,
		52,60,53,

		//sphere middle upper
		53,61,62,
		62,53,54,
		54,62,63,
		63,54,55,
		55,63,64,
		64,55,56,
		56,64,65,
		65,56,57,
		57,65,66,
		66,57,58,
		58,66,67,
		67,58,59,
		59,67,68,
		68,59,60,
		60,68,61,
		61,60,53,

		//sphere middle
		61,69,70,
		70,61,62,
		62,70,71,
		71,62,63,
		63,71,72,
		72,63,64,
		64,72,73,
		73,64,65,
		65,73,74,
		74,65,66,
		66,74,75,
		75,66,67,
		67,75,76,
		76,67,68,
		68,76,69,
		69,68,61,

		//sphere middle lower
		69,77,78,
		78,69,70,
		70,78,79,
		79,70,71,
		71,79,80,
		80,71,72,
		72,80,81,
		81,72,73,
		73,81,82,
		82,73,74,
		74,82,83,
		83,74,75,
		75,83,84,
		84,75,76,
		76,84,77,
		77,76,69,

		//sphere bottom
		85,77,78,
		85,78,79,
		85,79,80,
		85,80,81,
		85,81,82,
		85,82,83,
		85,83,84,
		85,84,77,



		//plane
		86,87,88,
		86,89,88,



		//cup

		//bottom
		90,91,92,
		90,92,93,
		90,93,94,
		90,94,95,
		90,95,96,
		90,96,97,
		90,97,98,
		90,98,91, //8

		//sides
		91,99,100,
		100,91,92,
		92,100,101,
		101,92,93,
		93,101,102,
		102,93,94,
		94,102,103,
		103,94,95,
		95,103,104,
		104,95,96,
		96,104,105,
		105,96,97,
		97,105,106,
		106,97,98,
		98,106,99,
		99,98,91, //16

		//edge 1
		99,107,108,
		108,99,100,
		100,108,109,
		109,100,101,
		101,109,110,
		110,101,102,
		102,110,111,
		111,102,103,
		103,111,112,
		112,103,104,
		104,112,113,
		113,104,105,
		105,113,114,
		114,105,106,
		106,114,107,
		107,106,99, //16

		//edge 2
		107,115,116,
		116,107,108,
		108,116,117,
		117,108,109,
		109,117,118,
		118,109,110,
		110,118,119,
		119,110,111,
		111,119,120,
		120,111,112,
		112,120,121,
		121,112,113,
		113,121,122,
		122,113,114,
		114,122,115,
		115,114,107, //16

		//edge 3
		115,123,124,
		124,115,116,
		116,124,125,
		125,116,117,
		117,125,126,
		126,117,118,
		118,126,127,
		127,118,119,
		119,127,128,
		128,119,120,
		120,128,129,
		129,120,121,
		121,129,130,
		130,121,122,
		122,130,123,
		123,122,115, //16


		//box of cards

		//bottom
		131,132,133,
		131,133,134, //2

		//top
		135,136,137,
		135,137,138, //2

		//sides
		131,135,136,
		136,131,132,
		132,136,137,
		137,132,133,
		133,137,138,
		138,133,134,
		134,138,135,
		135,134,131, //8



		// toy knife


		//blade
		//top
		139,140,141,
		139,141,142, //2

		//bottom
		143,144,145,
		143,145,146, //2

		//sides
		139,143,144,
		144,139,140,
		140,144,145,
		145,140,141,
		141,145,146,
		146,141,142,
		142,146,143,
		143,142,139, //8



		//handle
		//top
		147,148,149,
		147,149,150,
		147,150,151,
		147,151,152,
		147,152,153,
		147,153,154,
		147,154,155,
		147,155,148, //8

		//bottom
		164,156,157,
		164,157,158,
		164,158,159,
		164,159,160,
		164,160,161,
		164,161,162,
		164,162,163,
		164,163,156, //8

		//sides
		148,156,157,
		157,148,149,
		149,157,158,
		158,149,150,
		150,158,159,
		159,150,151,
		151,159,160,
		160,151,152,
		152,160,161,
		161,152,153,
		153,161,162,
		162,153,154,
		154,162,163,
		163,154,155,
		155,163,156,
		156,155,148, //16
	};


	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	glGenBuffers(2, mesh.vbos);
	glBindVertexArray(mesh.vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); //acctivates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); //sends vertex or coordinate data to the GPU


	/*mesh.nIndices[0] = sizeof(indices) / sizeof(indices[0]);*/
	mesh.nIndices[0] = 96;
	mesh.nIndices[1] = 384;
	mesh.nIndices[2] = 6;
	mesh.nIndices[3] = 216;
	mesh.nIndices[4] = 36;
	mesh.nIndices[5] = 36;
	mesh.nIndices[6] = 96;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//create the vertex atribute pointer
	const GLuint floatsPerVertex = 3; //number of coordinates per vertex
	const GLuint floatsPerColor = 4; //(r,g,b,a)
	const GLuint floatsPerUV = 2;
	const GLuint floatsPerNormal = 3;

	//strides between vertex coordinates is 6 (x,y,r,g,b,a)
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor + floatsPerUV + floatsPerNormal); // the number of floats before each

	//creates the vertex attribute pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0); //specifies the initial position of the coordinates in the buffer

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
	glEnableVertexAttribArray(2);
}

void UDestroyMesh(GLMesh& mesh) {
	glDeleteVertexArrays(1, &mesh.vao);
	glDeleteBuffers(2, mesh.vbos);
}
bool UCreateShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource, GLuint& programId) {
	//compilation and linkage error reporting
	int success = 0;
	char infoLog[512];

	//create a shader program object
	programId = glCreateProgram();

	//create the vertex and fragment shader objects
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	//retrieve the shader source
	glShaderSource(vertexShaderId, 1, &vertexShaderSource, NULL);
	glShaderSource(fragmentShaderId, 1, &fragmentShaderSource, NULL);

	//compile both shaders
	glCompileShader(vertexShaderId);
	//check for shder compile errors
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShaderId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}
	glCompileShader(fragmentShaderId);
	//check for shader compile errors
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	//attach compiled shaders to the shader program
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	glLinkProgram(programId);
	//check for linking errors
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glUseProgram(programId); //use the shader program

	return true;
}

void UDestroyShaderProgram(GLuint programId) {
	glDeleteProgram(programId);
}

void URender()
{
	// Lamp orbits around the origin
	const float angularVelocity = glm::radians(45.0f);
	if (gIsLampOrbiting)
	{
		glm::vec4 newPosition = glm::rotate(angularVelocity * gDeltaTime, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(gLightPosition, 1.0f);
		gLightPosition.x = newPosition.x;
		gLightPosition.y = newPosition.y;
		gLightPosition.z = newPosition.z;
	}


	//enable z depth
	glEnable(GL_DEPTH_TEST);

	//clear the background
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Activate the cube VAO (used by cube and lamp)
	glBindVertexArray(gMesh.vao);

	// CUBE: draw cube
	//----------------
	// Set the shader to be used
	glUseProgram(gCubeProgramId);




	// Model matrix: transformations are applied right-to-left order
	glm::mat4 model = glm::translate(gCubePosition) * glm::scale(gCubeScale);

	// camera/view transformation
	glm::mat4 view = gCamera.GetViewMatrix();

	// Creates a perspective projection
	//glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

	//creates ann orthographic projection
	//glm::mat4 projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
	// NOW check if 'isPerspective' and THEN set the perspective
	glm::mat4 projection;
	if (isPerspective) {
		projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}
	else {
		float scale = 90;
		projection = glm::ortho((800.0f / scale), -(800.0f / scale), -(600.0f / scale), (600.0f / scale), -2.5f, 6.5f);
	}


	//set the shader to be used
	glUseProgram(gCubeProgramId);

	//recieves and apsses matrices to the shader program
	GLuint modelLoc = glGetUniformLocation(gCubeProgramId, "model");
	GLuint viewLoc = glGetUniformLocation(gCubeProgramId, "view");
	GLuint projLoc = glGetUniformLocation(gCubeProgramId, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


	// Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
	GLint objectColorLoc = glGetUniformLocation(gCubeProgramId, "objectColor");
	GLint lightColorLoc = glGetUniformLocation(gCubeProgramId, "lightColor");
	GLint lightPositionLoc = glGetUniformLocation(gCubeProgramId, "lightPos");
	GLint viewPositionLoc = glGetUniformLocation(gCubeProgramId, "viewPosition");

	// Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
	glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
	glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
	glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
	const glm::vec3 cameraPosition = gCamera.Position;
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

	GLint UVScaleLoc = glGetUniformLocation(gCubeProgramId, "uvScale");
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));





	//activate the VBOs contained within the mesh's VAO
	glBindVertexArray(gMesh.vao);
	



	//STICK: draw stick
	//-----------------

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureId[1]);


	model = glm::translate(glm::vec3(2.2f, -0.62f, 4.0f)) * glm::rotate(90.0f, glm::vec3(1.1f, 1.1f, 1.0f)) * glm::scale(glm::vec3(0.3f, 0.3f, 0.5f));

	modelLoc = glGetUniformLocation(gCubeProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the triangle
	glDrawElements(GL_TRIANGLES, gMesh.nIndices[0], GL_UNSIGNED_SHORT, NULL);

	glDrawElements(GL_TRIANGLES, gMesh.nIndices[1], GL_UNSIGNED_SHORT, (GLvoid*)(sizeof(GLushort) * gMesh.nIndices[0]));





	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureId[8]);

	model = glm::translate(glm::vec3(-3.2f, -0.36f, 8.0f)) * glm::rotate(24.9f, glm::vec3(-5.0f, 1.0f, 5.0f)) * glm::scale(glm::vec3(0.3f, 0.3f, 0.5f));

	modelLoc = glGetUniformLocation(gCubeProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the triangle
	glDrawElements(GL_TRIANGLES, gMesh.nIndices[0], GL_UNSIGNED_SHORT, NULL);

	glDrawElements(GL_TRIANGLES, gMesh.nIndices[1], GL_UNSIGNED_SHORT, (GLvoid*)(sizeof(GLushort) * gMesh.nIndices[0]));




	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureId[6]);

	model = glm::translate(glm::vec3(-1.2f, 0.19f, 5.0f)) * glm::rotate(25.9f, glm::vec3(-6.3f, -15.0f, 5.0f)) * glm::scale(glm::vec3(0.3f, 0.3f, 0.5f));

	modelLoc = glGetUniformLocation(gCubeProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the triangle
	glDrawElements(GL_TRIANGLES, gMesh.nIndices[0], GL_UNSIGNED_SHORT, NULL);

	glDrawElements(GL_TRIANGLES, gMesh.nIndices[1], GL_UNSIGNED_SHORT, (GLvoid*)(sizeof(GLushort) * gMesh.nIndices[0]));




	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureId[7]);

	model = glm::translate(glm::vec3(0.5f, -0.26f, 8.0f)) * glm::rotate(25.8f, glm::vec3(2.6f, 2.5f, 5.0f)) * glm::scale(glm::vec3(0.3f, 0.3f, 0.5f));

	modelLoc = glGetUniformLocation(gCubeProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the triangle
	glDrawElements(GL_TRIANGLES, gMesh.nIndices[0], GL_UNSIGNED_SHORT, NULL);

	glDrawElements(GL_TRIANGLES, gMesh.nIndices[1], GL_UNSIGNED_SHORT, (GLvoid*)(sizeof(GLushort) * gMesh.nIndices[0]));




	//PLANE: draw floor
	//-----------------

	model = glm::translate(gCubePosition) * glm::scale(glm::vec3(1.0f));

	modelLoc = glGetUniformLocation(gCubeProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureId[0]);

	glDrawElements(GL_TRIANGLES, gMesh.nIndices[2], GL_UNSIGNED_SHORT, (GLvoid*)(sizeof(GLushort) * (gMesh.nIndices[1] + gMesh.nIndices[0])));



	//CUP: draw cup
	//-------------
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureId[2]);

	model = glm::translate(glm::vec3(-4.2f, 1.18f, -1.0f)) * glm::rotate(180.0f, glm::vec3(1.1f, 1.1f, 1.0f)) * glm::scale(glm::vec3(3.0f, 3.0f, 0.2f));


	modelLoc = glGetUniformLocation(gCubeProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));


	glDrawElements(GL_TRIANGLES, gMesh.nIndices[3], GL_UNSIGNED_SHORT, (GLvoid*)(sizeof(GLushort) * (gMesh.nIndices[1] + gMesh.nIndices[0] + gMesh.nIndices[2])));





	//BOX: draw box of cards
	//----------------------
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureId[3]);

	model = glm::translate(glm::vec3(-3.0f, -0.9f, 3.0f)) * glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f)) * glm::scale(glm::vec3(3.0f, 0.5f, 0.2f));


	modelLoc = glGetUniformLocation(gCubeProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));


	glDrawElements(GL_TRIANGLES, gMesh.nIndices[4], GL_UNSIGNED_SHORT, (GLvoid*)(sizeof(GLushort) * (gMesh.nIndices[1] + gMesh.nIndices[0] + gMesh.nIndices[2] + gMesh.nIndices[3])));






	//Toy: draw toy knife
	//-------------------

	//knife head
	//----------
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureId[4]);

	model = glm::translate(glm::vec3(-1.54f, -0.7f, 0.0f)) * glm::rotate(24.9f, glm::vec3(0.0f, -1.0f, -1.0f)) * glm::scale(glm::vec3(0.3f, 1.f, 0.15f));


	modelLoc = glGetUniformLocation(gCubeProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));


	glDrawElements(GL_TRIANGLES, gMesh.nIndices[5], GL_UNSIGNED_SHORT, (GLvoid*)(sizeof(GLushort)* (gMesh.nIndices[1] + gMesh.nIndices[0] + gMesh.nIndices[2] + gMesh.nIndices[3] + gMesh.nIndices[4])));




	//knife handle
	//------------
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureId[5]);

	model = glm::translate(glm::vec3(-1.08f, -0.4f, 3.0f)) * glm::rotate(24.9f, glm::vec3(0.0f, -1.0f, -1.0f)) * glm::scale(glm::vec3(0.3f, 0.3f, 0.15f));


	modelLoc = glGetUniformLocation(gCubeProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));


	glDrawElements(GL_TRIANGLES, gMesh.nIndices[6], GL_UNSIGNED_SHORT, (GLvoid*)(sizeof(GLushort)* (gMesh.nIndices[1] + gMesh.nIndices[0] + gMesh.nIndices[2] + gMesh.nIndices[3] + gMesh.nIndices[4] + gMesh.nIndices[5])));




	// LAMP: draw lamp
	//----------------
	glUseProgram(gLampProgramId);

	//Transform the smaller cube used as a visual que for the light source
	model = glm::translate(gLightPosition) * glm::scale(gLightScale);

	// Reference matrix uniforms from the Lamp Shader program
	modelLoc = glGetUniformLocation(gLampProgramId, "model");
	viewLoc = glGetUniformLocation(gLampProgramId, "view");
	projLoc = glGetUniformLocation(gLampProgramId, "projection");

	// Pass matrix data to the Lamp Shader program's matrix uniforms
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glDrawElements(GL_TRIANGLES, gMesh.nIndices[0], GL_UNSIGNED_SHORT, NULL);

	// Deactivate the Vertex Array Object and shader program
	glBindVertexArray(0);
	glUseProgram(0);

	// glfw: swap buffers and poll IO events
	glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (gFirstMouse)
	{
		gLastX = xpos;
		gLastY = ypos;
		gFirstMouse = false;
	}

	float xoffset = xpos - gLastX;
	float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

	gLastX = xpos;
	gLastY = ypos;

	gCamera.ProcessMouseMovement(xoffset, yoffset);
}

void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	gCamera.ProcessMouseScroll(yoffset);
}

// glfw: Handle mouse button events.
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
	{
		if (action == GLFW_PRESS)
			std::cout << "Left mouse button pressed" << std::endl;
		else
			std::cout << "Left mouse button released" << std::endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_MIDDLE:
	{
		if (action == GLFW_PRESS)
			std::cout << "Middle mouse button pressed" << std::endl;
		else
			std::cout << "Middle mouse button released" << std::endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_RIGHT:
	{
		if (action == GLFW_PRESS)
			std::cout << "Right mouse button pressed" << std::endl;
		else
			std::cout << "Right mouse button released" << std::endl;
	}
	break;

	default:
		std::cout << "Unhandled mouse button event" << std::endl;
		break;
	}
}








/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
	int width, height, channels;
	unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
	if (image)
	{
		flipImageVertically(image, width, height, channels);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << channels << " channels" << std::endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		return true;
	}

	// Error loading the image
	return false;
}


void UDestroyTexture(GLuint textureId)
{
	glGenTextures(1, &textureId);
}







// Function to generate the cylinder data
void generateCylinder(float radius, float height, int segments, std::vector<GLfloat>& vertices, std::vector<GLfloat>& texCoords) {
	float angle = 2.0f * 3.1415926f / segments;

	// Generate the tube
	for (int i = 0; i <= segments; ++i) {
		float x = radius * cos(i * angle);
		float y = radius * sin(i * angle);

		vertices.push_back(x);
		vertices.push_back(y);
		vertices.push_back(height);
		vertices.push_back((float)i / segments); // u
		vertices.push_back(1.0f); // v

		vertices.push_back(x);
		vertices.push_back(y);
		vertices.push_back(0.0f);
		vertices.push_back((float)i / segments); // u
		vertices.push_back(0.0f); // v
	}

	// Generate the top disk
	vertices.push_back(0.0f);
	vertices.push_back(0.0f);
	vertices.push_back(height);
	vertices.push_back(0.5f); // u
	vertices.push_back(0.5f); // v

	for (int i = 0; i <= segments; ++i) {
		float x = radius * cos(i * angle);
		float y = radius * sin(i * angle);

		vertices.push_back(x);
		vertices.push_back(y);
		vertices.push_back(height);
		vertices.push_back(0.5f + 0.5f * cos(i * angle)); // u
		vertices.push_back(0.5f + 0.5f * sin(i * angle)); // v
	}

	// Generate the bottom disk
	vertices.push_back(0.0f);
	vertices.push_back(0.0f);
	vertices.push_back(0.0f);
	vertices.push_back(0.5f); // u
	vertices.push_back(0.5f); // v

	for (int i = segments; i >= 0; --i) {
		float x = radius * cos(i * angle);
		float y = radius * sin(i * angle);

		vertices.push_back(x);
		vertices.push_back(y);
		vertices.push_back(0.0f);
		vertices.push_back(0.5f + 0.5f * cos(i * angle)); // u
		vertices.push_back(0.5f + 0.5f * sin(i * angle)); // v
	}
}

// Function to generate the sphere data
void generateSphere(float radius, int sectors, int stacks, std::vector<GLfloat>& vertices, std::vector<GLfloat>& texCoords) {
	float x, y, z, xy;
	float sectorStep = 2 * M_PI / sectors;
	float stackStep = M_PI / stacks;
	float sectorAngle, stackAngle;

	for (int i = 0; i <= stacks; ++i) {
		stackAngle = M_PI / 2 - i * stackStep;
		xy = radius * cosf(stackAngle);
		z = radius * sinf(stackAngle);

		for (int j = 0; j <= sectors; ++j) {
			sectorAngle = j * sectorStep;

			x = xy * cosf(sectorAngle);
			y = xy * sinf(sectorAngle);

			vertices.push_back(x);
			vertices.push_back(y);
			vertices.push_back(z);

			vertices.push_back((float)j / sectors);
			vertices.push_back((float)i / stacks);
		}
	}
}
