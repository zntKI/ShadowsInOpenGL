#include "Utils/Shader.hpp"
#include "Utils/Camera.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <GLM/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

void scrollCallback( GLFWwindow* window, double xpos, double ypos );
void mouseCallback( GLFWwindow* window, double xposIn, double yposIn );
void processInput( GLFWwindow* window );

const unsigned int SCREEN_WIDTH = 1920, SCREEN_HEIGHT = 1080;

float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

Camera camera( glm::vec3( 0.f, 0.f, 3.f ) );
bool firstMouse = true;
float lastX = SCREEN_WIDTH / 2.f, lastY = SCREEN_HEIGHT / 2.f;

int main()
{
#pragma region Setup

	GLFWwindow* window;

	/* Initialize the library */
	if ( !glfwInit() )
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow( SCREEN_WIDTH, SCREEN_HEIGHT, "Hello World", NULL, NULL );
	if ( !window ) {
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent( window );

	if ( glewInit() != GLEW_OK ) {
		std::cout << "Glew failed to init!" << std::endl;
		return -1;
	}

#pragma endregion

#pragma region Main

	unsigned int depthMapFBO;
	glGenFramebuffers( 1, &depthMapFBO );

	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

	unsigned int depthMap;
	glGenTextures( 1, &depthMap );
	glBindTexture( GL_TEXTURE_2D, depthMap );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

	glBindFramebuffer( GL_FRAMEBUFFER, depthMapFBO );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0 );
	glDrawBuffer( GL_NONE );
	glReadBuffer( GL_NONE );
	if ( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE ) {
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";
	}
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );


	Shader depthShader( "Shaders/simpleDepthShader.vts", "Shaders/simpleDepthShader.frs" );

#pragma endregion

#pragma region RenderLoop

	// input
	glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
	glfwSetCursorPosCallback( window, mouseCallback );
	glfwSetScrollCallback( window, scrollCallback );

	/* Loop until the user closes the window */
	while ( !glfwWindowShouldClose( window ) ) {
		/* Render here */
		
		// Keep track of time
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Input
		processInput( window );

#pragma region FirstPass

		glBindFramebuffer( GL_FRAMEBUFFER, depthMapFBO );
		glViewport( 0, 0, SHADOW_WIDTH, SHADOW_HEIGHT );
		glClear( GL_DEPTH_BUFFER_BIT );

		// Configure shaders and matrices
		depthShader.use();

		float near_plane = 1.0f, far_plane = 7.5f;
		glm::mat4 lightOrthoProjection = glm::ortho( -10.f, 10.f, -10.f, 10.f, near_plane, far_plane );

		glm::mat4 lightView = glm::lookAt( glm::vec3( -2.f, 4.f, -1.f ),
			glm::vec3( 0.f ),
			glm::vec3( 0.f, 1.f, 0.f ) );

		glm::mat4 lightSpaceMatrix = lightOrthoProjection * lightView;

		depthShader.setMatrix4( "u_LightSpaceMatrix", lightSpaceMatrix );

		// Render scene

#pragma endregion


#pragma region SecondPass

		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		glViewport( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		// Configure shader and matrices
		glBindTexture( GL_TEXTURE_2D, depthMap );
		// Render scene

#pragma endregion


		/* Swap front and back buffers */
		glfwSwapBuffers( window );
		/* Poll for and process events */
		glfwPollEvents();
	}

#pragma endregion

	glfwTerminate();
	return 0;
}

void scrollCallback( GLFWwindow* window, double xpos, double ypos )
{
	camera.ProcessMouseScroll( static_cast< float >( ypos ) );
}

void mouseCallback( GLFWwindow* window, double xposIn, double yposIn )
{
	float xpos = static_cast< float >( xposIn );
	float ypos = static_cast< float >( yposIn );

	if ( firstMouse ) // initially set to true
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed
	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement( xoffset, yoffset );
}

void processInput( GLFWwindow* window )
{
	if ( glfwGetKey( window, GLFW_KEY_ESCAPE ) == GLFW_PRESS ) {
		glfwSetWindowShouldClose( window, true );
	}

	if ( glfwGetKey( window, GLFW_KEY_LEFT_SHIFT ) == GLFW_PRESS )
		camera.ProcessKeyboard( MOVE_FAST, deltaTime );
	else
		camera.ProcessKeyboard( SLOW_DOWN, deltaTime );

	if ( glfwGetKey( window, GLFW_KEY_W ) == GLFW_PRESS )
		camera.ProcessKeyboard( FORWARD, deltaTime );
	if ( glfwGetKey( window, GLFW_KEY_S ) == GLFW_PRESS )
		camera.ProcessKeyboard( BACKWARD, deltaTime );
	if ( glfwGetKey( window, GLFW_KEY_A ) == GLFW_PRESS )
		camera.ProcessKeyboard( LEFT, deltaTime );
	if ( glfwGetKey( window, GLFW_KEY_D ) == GLFW_PRESS )
		camera.ProcessKeyboard( RIGHT, deltaTime );
	if ( glfwGetKey( window, GLFW_KEY_E ) == GLFW_PRESS )
		camera.ProcessKeyboard( UP, deltaTime );
	if ( glfwGetKey( window, GLFW_KEY_Q ) == GLFW_PRESS )
		camera.ProcessKeyboard( DOWN, deltaTime );
}