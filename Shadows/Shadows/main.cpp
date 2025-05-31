#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

const unsigned int SCREEN_WIDTH = 1920, SCREEN_HEIGHT = 1080;

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

#pragma endregion

#pragma region RenderLoop

	/* Loop until the user closes the window */
	while ( !glfwWindowShouldClose( window ) ) {
		/* Render here */
		

		glViewport( 0, 0, SHADOW_WIDTH, SHADOW_HEIGHT );
		glBindFramebuffer( GL_FRAMEBUFFER, depthMapFBO );
		glClear( GL_DEPTH_BUFFER_BIT );
		// Configure shaders and matrices
		// Render scene

		glViewport( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		// Configure shader and matrices
		glBindTexture( GL_TEXTURE_2D, depthMap );
		// Render scene


		/* Swap front and back buffers */
		glfwSwapBuffers( window );
		/* Poll for and process events */
		glfwPollEvents();
	}

#pragma endregion

	glfwTerminate();
	return 0;
}