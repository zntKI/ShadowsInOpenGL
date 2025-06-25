#include "Utils/Shader.hpp"
#include "Utils/Camera.hpp"
#include "Utils/Model.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stb_image.h>

#define GLM_ENABLE_EXPERIMENTAL // For vector rotation

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp> // For vector rotation

#include <iostream>
#include <fstream>

// Input methods
void scrollCallback (GLFWwindow* window, double xpos, double ypos);
void mouseCallback (GLFWwindow* window, double xposIn, double yposIn);
void processInput (GLFWwindow* window);
void processLightInput (GLFWwindow* window, glm::vec3& lightPos, float deltaTime);
bool processNrSamplesInput (GLFWwindow* window);
unsigned int loadTexture (const char* path);

const unsigned int SCREEN_WIDTH = 1920, SCREEN_HEIGHT = 1080;

// For FPS
float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

// Camera options
Camera camera (glm::vec3 (0.f, 0.f, 3.f));
bool firstMouse = true;
float lastX = SCREEN_WIDTH / 2.f, lastY = SCREEN_HEIGHT / 2.f;

// Rendering methods
void renderDepthSceneSimple (Shader& shader);

void calculateSamples (Shader& shader);

void renderLight (Shader& shader, glm::vec3& lightPos);
void renderFloorShadow (Shader& shader);
void renderCubesShadow (Shader& shader);

void calculateNormalMat (glm::mat4& modelMat, Shader& shader);

void renderFloor ();
void renderCube ();

// File IO
std::string filePath = "fpsCollector.txt";
void addFpsToFile (float fpsToAdd);
float getAverageFpsFromFile ();

// Samples vars
int nrSamples = 0;
float sampleRadius = 0.f;

/*

=========================
		How to Use
=========================

Movement Controls:
- W/A/S/D        Move forward/left/back/right
- Q/E            Move down/up
- SHIFT          Move faster
- Mouse Move     Look around
- Scroll Wheel   Zoom in/out (adjust FOV)

Light Movement:
- Arrow Keys                 Move the light source in X/Y directions
- Arrow Key Combos:
	- UP + LEFT / RIGHT      Move light forward-left / forward-right
	- DOWN + LEFT / RIGHT    Move light backward-left / backward-right

Shadow Sampling (PCF) Controls:
- Keypad +        Increase number of shadow samples (max: 32)
- Keypad -        Decrease number of shadow samples (min: 3)
- Keypad *        Increase sample radius
- Keypad /        Decrease sample radius (min: 0)

Shadow Settings:
- Shadow resolution: Change SHADOW_WIDTH / SHADOW_HEIGHT in code
- Shadow map uses front-face culling for better accuracy

FPS Logging:
- Outputs FPS once per second to "fpsCollector.txt"
- Prints average FPS after closing the app
- To time a test for a fixed duration, uncomment the relevant code in the main loop

Notes:
- VSync is disabled for raw performance measurements (enable with glfwSwapInterval(1) if needed)
- Default texture: wall.jpg (replace with your own in loadTexture)

*/

int main ()
{
#pragma region WindowSetup

	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit ())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow (SCREEN_WIDTH, SCREEN_HEIGHT, "Hello World", NULL, NULL);
	if (!window) {
		glfwTerminate ();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent (window);

	if (glewInit () != GLEW_OK) {
		std::cout << "Glew failed to init!" << std::endl;
		return -1;
	}

	// Uncomment if you want to enable VSync (not recommended for performance testing)
	//glfwSwapInterval (1);

#pragma endregion

#pragma region ShadowMappingSetup

	unsigned int depthMapFBO;
	glGenFramebuffers (1, &depthMapFBO);

	// Change here if you want to change the resolution of the shadow map
	const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

	unsigned int depthMap;
	glGenTextures (1, &depthMap);
	glBindTexture (GL_TEXTURE_2D, depthMap);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv (GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer (GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer (GL_NONE);
	glReadBuffer (GL_NONE);
	if (glCheckFramebufferStatus (GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";
	}
	glBindFramebuffer (GL_FRAMEBUFFER, 0);


	Shader depthShader ("Shaders/simpleDepthShader.vts", "Shaders/simpleDepthShader.frs");

	Shader shadowShaderSimple ("Shaders/shadowShader.vts", "Shaders/shadowShaderSimple.frs");
	shadowShaderSimple.setInt ("u_ShadowMap", 1);

	unsigned int floorTexture = loadTexture ("Assets/Textures/wall.jpg");
	shadowShaderSimple.setInt ("u_TexDiffuse", 0);

	Shader lightShader ("Shaders/lightShader.vts", "Shaders/lightShader.frs");

#pragma endregion

#pragma region PreRenderSetup

	glEnable (GL_DEPTH_TEST);
	glEnable (GL_CULL_FACE);

	// input
	glfwSetInputMode (window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback (window, mouseCallback);
	glfwSetScrollCallback (window, scrollCallback);

	// Change here if you want to change the start position of the light
	glm::vec3 lightPos (-2, 4, -1);

	// Frame calculation setup
	float previousTime = glfwGetTime ();

	// Clear the log file before each execution of the program in order to store only new fps data
	std::fstream fpsFile (filePath, std::fstream::out | std::fstream::trunc);
	fpsFile.close ();

	// Change here if you would like to start with a different number of samples
	nrSamples = 12;
	// Change here if you would like to start with a different sample radius
	sampleRadius = 1 / (float)SHADOW_WIDTH;

	// Initially calculate samples to feed the shader with any data for the first render
	calculateSamples (shadowShaderSimple);

	// Counter for timing performance tests
	int secondsCounter = 0;

	std::cout << "Output during rendering:\n\n";

#pragma endregion

#pragma region RenderLoop

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose (window)) {
		/* Render here */

		// Keep track of time
		float currentFrame = glfwGetTime ();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		if (currentFrame - previousTime >= 1.0f)
		{
			float fps = 1.0f / deltaTime;
			addFpsToFile (fps);

			previousTime = currentFrame;

			// Uncomment this if you would like to time performance test for a given time period
			// Done for the purpose of more reliable test results
			/**
			secondsCounter++;
			if (secondsCounter == 10)
			{
				glfwSetWindowShouldClose (window, true);
			}
			/**/
		}

		// Input
		if (glfwGetKey (window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose (window, true);
		}

		processInput (window);

		processLightInput (window, lightPos, deltaTime);

		if (processNrSamplesInput (window)) // Recalculate samples only if any parameter has changed
		{
			std::cout << nrSamples << " samples and " << sampleRadius << " radius\n";

			calculateSamples (shadowShaderSimple);
		}

#pragma region FirstPass_WritingToDepthTexture

		glBindFramebuffer (GL_FRAMEBUFFER, depthMapFBO);
		glViewport (0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Configure shaders and matrices
		depthShader.use ();

		float near_plane = 1.0f, far_plane = 7.5f;
		glm::mat4 lightOrthoProjection = glm::ortho (-10.f, 10.f, -10.f, 10.f, near_plane, far_plane);

		glm::mat4 lightView = glm::lookAt (lightPos,
			glm::vec3 (0.f),
			glm::vec3 (0.f, 1.f, 0.f));

		glm::mat4 lightSpaceMatrix = lightOrthoProjection * lightView;

		depthShader.setMatrix4 ("u_LightSpaceMatrix", lightSpaceMatrix);

		glClear (GL_DEPTH_BUFFER_BIT);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, floorTexture);

		// Cull the front faces and use the back faces for calculating the shadows for more accurate results
		glCullFace (GL_FRONT);
		renderDepthSceneSimple (depthShader);
		glCullFace (GL_BACK);

#pragma endregion


#pragma region SecondPass_RenderingTheFinalScene

		glBindFramebuffer (GL_FRAMEBUFFER, 0);
		glViewport (0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Configure shader and matrices

		// Uncomment this if you want a static position for the camera
		/**
		camera.Position = glm::vec3 (2.82838, 0.360989, 2.26124);
		camera.Front = glm::vec3 (-0.00256063, -0.679442, -0.733724);
		camera.Up = glm::vec3 (-0.00237118, 0.733729, -0.679438);
		/**/

		glm::mat4 view = camera.GetViewMatrix ();
		// Projection matrix
		glm::mat4 projection = glm::perspective (glm::radians (camera.Zoom), SCREEN_WIDTH / (float)SCREEN_HEIGHT, .1f, 100.f);

		shadowShaderSimple.use ();

		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, floorTexture);
		glActiveTexture (GL_TEXTURE1);
		glBindTexture (GL_TEXTURE_2D, depthMap);

		shadowShaderSimple.setMatrix4 ("u_LightSpaceMatrix", lightSpaceMatrix);
		shadowShaderSimple.setVec3 ("u_LightPos", lightPos);

		shadowShaderSimple.setMatrix4 ("u_Proj", projection);
		shadowShaderSimple.setMatrix4 ("u_View", view);
		shadowShaderSimple.setVec3 ("u_ViewPos", camera.Position);

		renderFloorShadow (shadowShaderSimple);
		renderCubesShadow (shadowShaderSimple);

		lightShader.use ();

		lightShader.setMatrix4 ("u_Proj", projection);
		lightShader.setMatrix4 ("u_View", view);

		renderLight (lightShader, lightPos);

#pragma endregion

		/* Swap front and back buffers */
		glfwSwapBuffers (window);
		/* Poll for and process events */
		glfwPollEvents ();
	}

#pragma endregion

	std::cout << "Output after end of rendering:\n\n";

	std::cout << getAverageFpsFromFile () << " average fps\n";

	glfwTerminate ();
	return 0;
}

void renderDepthSceneSimple (Shader& shader)
{
	// cubes
	glm::mat4 model = glm::mat4 (1.0f);
	model = glm::translate (model, glm::vec3 (0.0f, 1.5f, 0.0));
	model = glm::scale (model, glm::vec3 (0.5f));
	shader.setMatrix4 ("u_Model", model);
	renderCube ();
	model = glm::mat4 (1.0f);
	model = glm::translate (model, glm::vec3 (2.0f, 0.0f, 1.0));
	model = glm::scale (model, glm::vec3 (0.5f));
	shader.setMatrix4 ("u_Model", model);
	renderCube ();
	model = glm::mat4 (1.0f);
	model = glm::translate (model, glm::vec3 (-1.0f, 0.0f, 2.0));
	model = glm::rotate (model, glm::radians (60.0f), glm::normalize (glm::vec3 (1.0, 0.0, 1.0)));
	model = glm::scale (model, glm::vec3 (0.25));
	shader.setMatrix4 ("u_Model", model);
	renderCube ();
}

/// <summary>
/// Calculate sample offset vectors in a circular fashion away from the the fragment position
/// </summary>
/// <param name="shader">Shader to set parameters to</param>
void calculateSamples (Shader& shader)
{
	float angleBetweenEachDir = glm::two_pi<float> () / nrSamples;
	glm::vec2 sampleDir = glm::vec2 (1.0f, 0.0f);

	shader.use ();

	shader.setFloat ("u_SampleRadius", sampleRadius);
	shader.setInt ("u_SamplesAmount", nrSamples);

	for (int i = 0; i < nrSamples; i++)
	{
		glm::vec2 sample = glm::rotate (sampleDir, angleBetweenEachDir * i) * sampleRadius;

		shader.setVec2 ("u_Samples[" + std::to_string (i) + "]", sample);
	}
}

void renderLight (Shader& shader, glm::vec3& lightPos)
{
	glm::mat4 model = glm::mat4 (1.0f);
	model = glm::translate (model, lightPos);
	model = glm::scale (model, glm::vec3 (.1f));
	shader.setMatrix4 ("u_Model", model);

	renderCube ();
}

void renderFloorShadow (Shader& shader)
{
	glm::mat4 model = glm::mat4 (1.f);
	shader.setMatrix4 ("u_Model", model);

	calculateNormalMat (model, shader);

	renderFloor ();
}

void renderCubesShadow (Shader& shader)
{
	glm::mat4 model = glm::mat4 (1.0f);
	model = glm::translate (model, glm::vec3 (0.0f, 1.5f, 0.0));
	model = glm::scale (model, glm::vec3 (0.5f));
	shader.setMatrix4 ("u_Model", model);
	calculateNormalMat (model, shader);
	renderCube ();

	model = glm::mat4 (1.0f);
	model = glm::translate (model, glm::vec3 (2.0f, 0.0f, 1.0));
	model = glm::scale (model, glm::vec3 (0.5f));
	shader.setMatrix4 ("u_Model", model);
	calculateNormalMat (model, shader);
	renderCube ();

	model = glm::mat4 (1.0f);
	model = glm::translate (model, glm::vec3 (-1.0f, 0.0f, 2.0));
	model = glm::rotate (model, glm::radians (60.0f), glm::normalize (glm::vec3 (1.0, 0.0, 1.0)));
	model = glm::scale (model, glm::vec3 (0.25));
	shader.setMatrix4 ("u_Model", model);
	calculateNormalMat (model, shader);
	renderCube ();
}

void calculateNormalMat (glm::mat4& modelMat, Shader& shader)
{
	// Calculate normal matrix once in cpp code, instead of doing it for every vertex for performance reasons
	glm::mat3 normalMatrix = glm::mat3 (modelMat);
	normalMatrix = glm::transpose (normalMatrix);
	normalMatrix = glm::inverse (normalMatrix);

	shader.setMatrix3 ("u_NormalMat", normalMatrix);
}

unsigned int floorVAO = 0, floorVBO = 0;
void renderFloor ()
{
	if (floorVAO == 0)
	{
		float vertices[] = {

			// positions            // normals         // texcoords
			-25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
			 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
			-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

			-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
			 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
			 25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f

		};

		glGenVertexArrays (1, &floorVAO);
		glBindVertexArray (floorVAO);

		glGenBuffers (1, &floorVBO);
		glBindBuffer (GL_ARRAY_BUFFER, floorVBO);

		glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);

		glEnableVertexAttribArray (0);
		glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof (float), (void*)0);
		glEnableVertexAttribArray (1);
		glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof (float), (void*)(3 * sizeof (float)));
		glEnableVertexAttribArray (2);
		glVertexAttribPointer (2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof (float), (void*)(6 * sizeof (float)));

		glBindBuffer (GL_ARRAY_BUFFER, 0);
		glBindVertexArray (0);
	}

	glBindVertexArray (floorVAO);
	glDrawArrays (GL_TRIANGLES, 0, 6);
	glBindVertexArray (0);
}

unsigned int cubeVAO = 0, cubeVBO = 0;
void renderCube ()
{
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			 // bottom face
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			  1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 // top face
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			  1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			  1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			  1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};

		glGenVertexArrays (1, &cubeVAO);
		glGenBuffers (1, &cubeVBO);

		// fill buffer
		glBindBuffer (GL_ARRAY_BUFFER, cubeVBO);
		glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);

		// link vertex attributes
		glBindVertexArray (cubeVAO);
		glEnableVertexAttribArray (0);
		glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof (float), (void*)0);
		glEnableVertexAttribArray (1);
		glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof (float), (void*)(3 * sizeof (float)));
		glEnableVertexAttribArray (2);
		glVertexAttribPointer (2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof (float), (void*)(6 * sizeof (float)));

		glBindBuffer (GL_ARRAY_BUFFER, 0);
		glBindVertexArray (0);
	}

	// render Cube
	glBindVertexArray (cubeVAO);
	glDrawArrays (GL_TRIANGLES, 0, 36);
	glBindVertexArray (0);
}


void scrollCallback (GLFWwindow* window, double xpos, double ypos)
{
	camera.ProcessMouseScroll (static_cast<float>(ypos));
}

void mouseCallback (GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse) // initially set to true
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed
	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement (xoffset, yoffset);
}

/// <summary>
/// Process inputs regarding camera movement/properties
/// </summary>
void processInput (GLFWwindow* window)
{
	if (glfwGetKey (window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		camera.ProcessKeyboard (MOVE_FAST, deltaTime);
	else
		camera.ProcessKeyboard (SLOW_DOWN, deltaTime);

	if (glfwGetKey (window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard (FORWARD, deltaTime);
	if (glfwGetKey (window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard (BACKWARD, deltaTime);
	if (glfwGetKey (window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard (LEFT, deltaTime);
	if (glfwGetKey (window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard (RIGHT, deltaTime);
	if (glfwGetKey (window, GLFW_KEY_E) == GLFW_PRESS)
		camera.ProcessKeyboard (UP, deltaTime);
	if (glfwGetKey (window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboard (DOWN, deltaTime);
}

/// <summary>
/// Process input for camera position changes
/// </summary>
/// <param name="lightPos">The light pos to change</param>
void processLightInput (GLFWwindow* window, glm::vec3& lightPos, float deltaTime)
{
	float amountToMove = 1.f * deltaTime;

	if (glfwGetKey (window, GLFW_KEY_UP) == GLFW_PRESS
		&& glfwGetKey (window, GLFW_KEY_LEFT) == GLFW_PRESS)
	{
		lightPos.z -= amountToMove;
		lightPos.x -= amountToMove;
	}
	else if (glfwGetKey (window, GLFW_KEY_UP) == GLFW_PRESS
		&& glfwGetKey (window, GLFW_KEY_RIGHT) == GLFW_PRESS)
	{
		lightPos.z -= amountToMove;
		lightPos.x += amountToMove;
	}
	else if (glfwGetKey (window, GLFW_KEY_DOWN) == GLFW_PRESS
		&& glfwGetKey (window, GLFW_KEY_LEFT) == GLFW_PRESS)
	{
		lightPos.z += amountToMove;
		lightPos.x -= amountToMove;
	}
	else if (glfwGetKey (window, GLFW_KEY_DOWN) == GLFW_PRESS
		&& glfwGetKey (window, GLFW_KEY_RIGHT) == GLFW_PRESS)
	{
		lightPos.z += amountToMove;
		lightPos.x += amountToMove;
	}
	else if (glfwGetKey (window, GLFW_KEY_UP) == GLFW_PRESS)
		lightPos.y += amountToMove;
	else if (glfwGetKey (window, GLFW_KEY_DOWN) == GLFW_PRESS)
		lightPos.y -= amountToMove;
	else if (glfwGetKey (window, GLFW_KEY_LEFT) == GLFW_PRESS)
		lightPos.x -= amountToMove;
	else if (glfwGetKey (window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		lightPos.x += amountToMove;
}

bool processNrSamplesInput (GLFWwindow* window)
{
	static bool addPressed = false, subtrPressed = false, multPressed = false, dividePressed = false;
	if (glfwGetKey (window, GLFW_KEY_KP_ADD) == GLFW_PRESS)
		addPressed = true;
	else if (glfwGetKey (window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS)
		subtrPressed = true;
	else if (glfwGetKey (window, GLFW_KEY_KP_MULTIPLY) == GLFW_PRESS)
		multPressed = true;
	else if (glfwGetKey (window, GLFW_KEY_KP_DIVIDE) == GLFW_PRESS)
		dividePressed = true;

	if (addPressed && glfwGetKey (window, GLFW_KEY_KP_ADD) == GLFW_RELEASE)
	{
		addPressed = false;

		nrSamples++;
		if (nrSamples > 32)
		{
			nrSamples = 32;
			return false;
		}

		return true;
	}
	else if (subtrPressed && glfwGetKey (window, GLFW_KEY_KP_SUBTRACT) == GLFW_RELEASE)
	{
		subtrPressed = false;

		nrSamples--;
		if (nrSamples < 3)
		{
			nrSamples = 3;
			return false;
		}

		return true;
	}
	else if (multPressed && glfwGetKey (window, GLFW_KEY_KP_MULTIPLY) == GLFW_RELEASE)
	{
		sampleRadius += 0.0001f;

		multPressed = false;
		return true;
	}
	else if (dividePressed && glfwGetKey (window, GLFW_KEY_KP_DIVIDE) == GLFW_RELEASE)
	{
		dividePressed = false;

		sampleRadius -= 0.0001f;
		if (sampleRadius < 0.f)
		{
			sampleRadius = 0.f;
			return false;
		}

		return true;
	}

	return false;
}

unsigned int loadTexture (char const* path)
{
	unsigned int textureID;
	glGenTextures (1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load (path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture (GL_TEXTURE_2D, textureID);
		glTexImage2D (GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap (GL_TEXTURE_2D);

		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat 
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free (data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free (data);
	}

	return textureID;
}

void addFpsToFile (float fpsToAdd)
{
	// Append an fps to the end of the file

	std::ofstream fpsFile (filePath, std::ios::app);

	if (!fpsFile.fail ())
	{
		fpsFile << fpsToAdd << "\n";
		fpsFile.close ();
	}
	else
	{
		std::cerr << "File to write failed to open\n";
	}

}

float getAverageFpsFromFile ()
{
	// Read all fps values from the file at the end of the program and calculate the average to be outputted to the console
	// Append the average at the end of the file for backupt reasons

	std::fstream fpsFile (filePath, std::fstream::in); // Open for reading and calculating the average fps
	if (fpsFile.fail ())
	{
		std::cerr << "File to read failed to open\n";
		return -1.f;
	}

	int fpsCount = 0;
	float fpsSum = 0.f;

	std::string line;
	while (std::getline(fpsFile, line))
	{
		fpsSum += std::stof (line);
		fpsCount++;
	}

	float average = fpsSum / fpsCount;

	fpsFile.close ();

	fpsFile.open (filePath, std::fstream::out | std::fstream::app); // Open for writing the average fps at the end of the file
	if (fpsFile.fail ())
	{
		std::cerr << "File to write failed to open\n";
		return -1.f;
	}

	fpsFile << "\nAverage fps: " << average << "\n";
	fpsFile.close ();

	return average;

}