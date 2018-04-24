#include <GL/glew.h>
#include <debuggl.h>
#include <iostream>
#include "texture_to_render.h"

TextureToRender::TextureToRender()
{
}

TextureToRender::~TextureToRender()
{
	if (fb_ < 0)
		return ;
	unbind();
	glDeleteFramebuffers(1, &fb_);
	glDeleteTextures(1, &tex_);
	glDeleteRenderbuffers(1, &dep_);
}

void TextureToRender::create(int width, int height)
{
	w_ = width;
	h_ = height;
	// FIXME: Create the framebuffer object backed by a texture

	//create the frame buffer
	glGenFramebuffers(1, &fb_);
	glBindFramebuffer(GL_FRAMEBUFFER, fb_);

	//the texture we're going to render to
	glGenTextures(1, &tex_);

	//bind the texture
	glBindTexture(GL_TEXTURE_2D, tex_);

	//give an empty image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w_, h_, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	//poor filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	//the depth buffer
	glGenRenderbuffers(1, &dep_);
	glBindRenderbuffer(GL_RENDERBUFFER, dep_);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w_, h_);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, dep_);
	
	//Set "renderedTexture" as our colour attachement #0
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex_, 0);

	// Set the list of draw buffers.
	GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers




	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "Failed to create framebuffer object as render target" << std::endl;
	} else {
		std::cerr << "Framebuffer ready" << std::endl;
	}
	unbind();
}

void TextureToRender::bind()
{
	// FIXME: Unbind the framebuffer object to GL_FRAMEBUFFER
	// Render to our framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, fb_);
	glViewport(0, 0, w_, h_); // Render on the whole framebuffer, complete from the lower left corner to the upper right
}

void TextureToRender::unbind()
{
	// FIXME: Unbind current framebuffer object from the render target
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glViewport(0,0,w_,h_);
}
