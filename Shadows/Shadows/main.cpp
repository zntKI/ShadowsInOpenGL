#include "Utils/Shader.hpp"
#include "Utils/Camera.hpp"
#include "Utils/Model.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stb_image.h>

#include <GLM/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

void scrollCallback (GLFWwindow* window, double xpos, double ypos);
void mouseCallback (GLFWwindow* window, double xposIn, double yposIn);
void processInput (GLFWwindow* window);
unsigned int loadTexture (const char* path);

const unsigned int SCREEN_WIDTH = 1920, SCREEN_HEIGHT = 1080;

float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

Camera camera (glm::vec3 (0.f, 0.f, 3.f));
bool firstMouse = true;
float lastX = SCREEN_WIDTH / 2.f, lastY = SCREEN_HEIGHT / 2.f;

void renderDepthSceneComplex (Shader& shader);
void renderDepthSceneSimple (Shader& shader);

void renderFloorShadow (Shader& shader);
void renderBackpackShadow (Shader& shader);
void renderCubesShadow (Shader& shader);

void calculateNormalMat (glm::mat4& modelMat, Shader& shader);

// TODO: maybe make a class that represents code-defined simple objects to reduce code clutter in main
void renderFloor ();
void renderCube ();
Model* backpack;

void renderQuad ();

int main ()
{
#pragma region Setup

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

#pragma endregion

#pragma region Main

	stbi_set_flip_vertically_on_load (true);

	backpack = new Model ("Assets/Models/Backpack/backpack.obj");

	unsigned int depthMapFBO;
	glGenFramebuffers (1, &depthMapFBO);

	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

	unsigned int depthMap;
	glGenTextures (1, &depthMap);
	glBindTexture (GL_TEXTURE_2D, depthMap);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

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

	Shader shadowShaderComplex ("Shaders/shadowShader.vts", "Shaders/shadowShaderComplex.frs");
	shadowShaderComplex.setInt ("u_ShadowMap", 2);

#pragma endregion

#pragma region RenderLoop

	glEnable (GL_DEPTH_TEST);

	// input
	glfwSetInputMode (window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback (window, mouseCallback);
	glfwSetScrollCallback (window, scrollCallback);

	glm::vec3 lightPos (-2.f, 4.f, -1.f);

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose (window)) {
		/* Render here */

		// Keep track of time
		float currentFrame = glfwGetTime ();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Input
		processInput (window);

#pragma region FirstPass

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

		renderDepthSceneComplex (depthShader);

		/* If you want cubes instead of backpack
		renderDepthSceneSimple (depthShader);
		*/

#pragma endregion


#pragma region SecondPass

		glBindFramebuffer (GL_FRAMEBUFFER, 0);
		glViewport (0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Configure shader and matrices

		// View matrix
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

		/* If you want cubes instead of backpack
		renderCubesShadow (shadowShaderSimple);
		*/

		
		shadowShaderComplex.use ();

		glActiveTexture (GL_TEXTURE2);
		glBindTexture (GL_TEXTURE_2D, depthMap);

		// TODO: Use UBO's

		shadowShaderComplex.setMatrix4 ("u_LightSpaceMatrix", lightSpaceMatrix);
		shadowShaderComplex.setVec3 ("u_LightPos", lightPos);

		shadowShaderComplex.setMatrix4 ("u_Proj", projection);
		shadowShaderComplex.setMatrix4 ("u_View", view);
		shadowShaderComplex.setVec3 ("u_ViewPos", camera.Position);

		renderBackpackShadow (shadowShaderComplex);

#pragma endregion


		/* Swap front and back buffers */
		glfwSwapBuffers (window);
		/* Poll for and process events */
		glfwPollEvents ();
	}

#pragma endregion

	delete backpack;

	glfwTerminate ();
	return 0;
}

void renderDepthSceneComplex (Shader& shader)
{
	// floor
	glm::mat4 model = glm::mat4 (1.f);
	shader.setMatrix4 ("u_Model", model);
	renderFloor ();

	// backpack(s)
	model = glm::mat4 (1.f);
	model = glm::translate (model, glm::vec3 (0.0f, 1.f, 0.0f));
	model = glm::scale (model, glm::vec3 (0.5f));
	shader.setMatrix4 ("u_Model", model);
	backpack->Draw (shader);


	//// cubes
	//model = glm::mat4 (1.0f);
	//model = glm::translate (model, glm::vec3 (0.0f, 1.5f, 0.0));
	//model = glm::scale (model, glm::vec3 (0.5f));
	//shader.setMatrix4 ("u_Model", model);
	//renderCube ();
	//model = glm::mat4 (1.0f);
	//model = glm::translate (model, glm::vec3 (2.0f, 0.0f, 1.0));
	//model = glm::scale (model, glm::vec3 (0.5f));
	//shader.setMatrix4 ("u_Model", model);
	//renderCube ();
	//model = glm::mat4 (1.0f);
	//model = glm::translate (model, glm::vec3 (-1.0f, 0.0f, 2.0));
	//model = glm::rotate (model, glm::radians (60.0f), glm::normalize (glm::vec3 (1.0, 0.0, 1.0)));
	//model = glm::scale (model, glm::vec3 (0.25));
	//shader.setMatrix4 ("u_Model", model);
	//renderCube ();
}

void renderDepthSceneSimple (Shader& shader)
{
	// floor
	glm::mat4 model = glm::mat4 (1.f);
	shader.setMatrix4 ("u_Model", model);
	renderFloor ();

	// cubes
	model = glm::mat4 (1.0f);
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

void renderFloorShadow (Shader& shader)
{
	glm::mat4 model = glm::mat4 (1.f);
	shader.setMatrix4 ("u_Model", model);

	calculateNormalMat (model, shader);

	renderFloor ();
}

void renderBackpackShadow (Shader& shader)
{
	glm::mat4 model = glm::mat4 (1.f);
	model = glm::translate (model, glm::vec3 (0.0f, 1.f, 0.0f));
	model = glm::scale (model, glm::vec3 (0.5f));
	shader.setMatrix4 ("u_Model", model);

	calculateNormalMat (model, shader);

	backpack->Draw (shader);
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
			 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
			-25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
			-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

			 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
			-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
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

unsigned int quadVAO = 0, quadVBO = 0;
void renderQuad ()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		glGenVertexArrays (1, &quadVAO);
		glBindVertexArray (quadVAO);

		glGenBuffers (1, &quadVBO);
		glBindBuffer (GL_ARRAY_BUFFER, quadVBO);

		glBufferData (GL_ARRAY_BUFFER, sizeof (quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray (0);
		glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof (float), (void*)0);
		glEnableVertexAttribArray (1);
		glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof (float), (void*)(3 * sizeof (float)));

		glBindBuffer (GL_ARRAY_BUFFER, 0);
		glBindVertexArray (0);
	}

	glBindVertexArray (quadVAO);
	glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
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

void processInput (GLFWwindow* window)
{
	if (glfwGetKey (window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose (window, true);
	}

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