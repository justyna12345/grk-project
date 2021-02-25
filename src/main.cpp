#include "glew.h"
#include "freeglut.h"
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <ctime>

#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Camera.h"


#include "Box.cpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#define M_PI 3.14159265358979323846
#include "stb_image.h"

GLuint teslaTexture;
GLuint earthTexture;
GLuint sunTexture;
GLuint marsTexture;
GLuint cometTexture;
GLuint jupiterTexture;
GLuint erisTexture;


GLuint program;
GLuint programShip;
GLuint programSun;
GLuint programTexture;
GLuint programSkybox;
GLuint programParticles;
GLuint programParticlesTex;

GLuint billboard_vertex_buffer;
GLuint particles_position_buffer;
GLuint particles_color_buffer;

double lastTime;

const int MaxParticles = 10000;

GLuint programBloom;
GLuint programBlur;

unsigned int hdrFBO;
unsigned int colorBuffers[2];
unsigned int pingpongFBO[2];
unsigned int pingpongColorbuffers[2];
int bloom = 0;
float exposure = 1.0f;
bool earthDead = false;

void startBloom();
void drawBloom();
void renderQuad();
void createBuffers();

Core::Shader_Loader shaderLoader;

GLuint skyboxTexture;

glm::vec2 mousePoint = glm::vec2(0);
static const float sensitivity = 0.00000001;


float frustumScale = 1.0f;
int w, h;

std::vector<std::string> faces
{
		"skybox/right.jpg",
		"skybox/left.jpg",
		"skybox/top.jpg",
		"skybox/bottom.jpg",
		"skybox/front.jpg",
		"skybox/back.jpg"
};


struct Bullet {
	glm::mat4 bulletModelMatrix;
	glm::vec3 position;
	float velocityDiv;
	Core::RenderContext* context;
	GLuint textureID;
	glm::vec3 scaleVector;
	glm::vec3 dir;
	float age;

};
std::vector<std::shared_ptr<Bullet>> bullets;
static GLfloat* g_particule_position_size_data = new GLfloat[MaxParticles * 4];
static GLubyte* g_particule_color_data = new GLubyte[MaxParticles * 4];
glm::vec3 jetParticlesDir = glm::vec3(0, 0, -1);


float cameraAngle = 0;
glm::vec3 cameraPos = glm::vec3(-70, 0, 0);
glm::vec3 cameraDir;

obj::Model sphere;
obj::Model ship;
Core::RenderContext shipContext;
glm::mat4 shipModelMatrix = glm::mat4(1.0f);
Core::RenderContext sphereContext;

obj::Model cube;
Core::RenderContext cubeContext;

Core::RenderContext particleContext;

glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

struct Light {
	glm::vec3 position;
	glm::vec3 color;
};

std::vector<Light> lightCollector;

glm::mat4 cameraMatrix, perspectiveMatrix;

float timer = 0.0f;
float delta = 0.0f;

float changeCameraAngle = 0; 
float step = 0.1f; // wartosc hamowania obrotu
float angleSpeed = 0.3f; //wartosc przyspieszenia obrotu
float speedlimit = 1.5; // max predkosc obrotu
bool breaking = false; //czy hamowanie ma byc aktywne
bool activeBloom = false;
//struct Renderable {
//	Core::RenderContext* context;
//	glm::mat4 modelMatrix;
//	glm::mat4 scale;
//	GLuint texttureID;
//};
//
//std::vector<Renderable*> redelables;
//int rendelableSize = 0;

void keyboard(unsigned char key, int x, int y)
{
		float moveSpeed = 0.7f;

	switch (key)
	{
	/*case 'z':
		if (changeCameraAngle > -speedlimit) {
			changeCameraAngle -= angleSpeed;
		}
		break;
	case 'x': 
		if (changeCameraAngle < speedlimit) {
		changeCameraAngle += angleSpeed;
		}
		break;*/
	case 'w': cameraPos += cameraDir * moveSpeed; break;
	case 's': cameraPos -= cameraDir * moveSpeed; break;
	case 'd': cameraPos += glm::cross(cameraDir, glm::vec3(0, 1, 0)) * moveSpeed; break;
	case 'a': cameraPos -= glm::cross(cameraDir, glm::vec3(0, 1, 0)) * moveSpeed; break;
	/*case 'e': cameraPos += glm::cross(cameraDir, glm::vec3(1, 0, 0)) * moveSpeed; break;
	case 'q': cameraPos -= glm::cross(cameraDir, glm::vec3(1, 0, 0)) * moveSpeed; break;*/
	case 'b': bloom = !activeBloom; activeBloom = !activeBloom; break;
	case 'o':
		if (exposure > 0.0f)
			exposure -= 0.01f;
		else
			exposure = 0.0f;
		break;
	case 'p': exposure += 0.01f; break;
	}
}


void mouseMovement(int x, int y) {
	float moveSpeed = 0.7f;
	if(x > 1100) {
		breaking = false;
		if (changeCameraAngle < speedlimit ) {
			changeCameraAngle += angleSpeed;
		}
	}
	if(x < 220) {
		breaking = false;
		if (changeCameraAngle > -speedlimit) {
			changeCameraAngle -= angleSpeed;
		}
	}
	if (x >= 520 && x <= 720) {
		breaking = true;
	}
	if (y <= 320) {
		cameraPos -= glm::cross(cameraDir, glm::vec3(1, 0, 0)) * moveSpeed;
	}
	if (y >= 600) {
		cameraPos += glm::cross(cameraDir, glm::vec3(1, 0, 0)) * moveSpeed;
	}
	
}


void createBullet() {
	auto bullet = std::make_shared<Bullet>();
	bullet->bulletModelMatrix = glm::translate(cameraPos + cameraDir * 0.5f + glm::vec3(0, -0.15f, 0));
	bullet->dir = cameraDir/100;
	bullet->velocityDiv = 1.0f;
	bullet->context = &sphereContext;
	bullet->textureID = marsTexture;
	bullet->scaleVector = glm::vec3(0.05f);
	bullet->age = 5;

	bullets.emplace_back(bullet);
}


void mouseClick(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		createBullet();
	}
}

glm::mat4 createCameraMatrix()
{
	// Obliczanie kierunku patrzenia kamery (w plaszczyznie x-z) przy uzyciu zmiennej cameraAngle kontrolowanej przez klawisze.
	cameraDir = glm::vec3(cosf(cameraAngle), 0.0f, sinf(cameraAngle));

	glm::quat x = glm::angleAxis(mousePoint.x, glm::vec3(0, 1, 0));
	glm::quat y = glm::angleAxis(mousePoint.y, glm::vec3(1, 0, 0));

	mousePoint = glm::vec2(0);

	glm::vec3 up = glm::vec3(0, 1, 0);

	return Core::createViewMatrix(cameraPos, cameraDir, up);
}
struct Particle {
	glm::vec3 pos, speed;
	unsigned char r, g, b, a; // Color
	float size, angle, weight;
	float life; // Remaining life of the particle. if <0 : dead and unused.
	float cameradistance; // *Squared* distance to the camera. if dead : -1.0f

	bool operator<(const Particle& that) const {
		// Sort in reverse order : far particles drawn first.
		return this->cameradistance > that.cameradistance;
	}
};

Particle ParticlesContainer[MaxParticles];
int LastUsedParticle = 0;

int FindUnusedParticle() {

	for (int i = LastUsedParticle; i < MaxParticles; i++) {
		if (ParticlesContainer[i].life < 0) {
			LastUsedParticle = i;
			return i;
		}
	}

	for (int i = 0; i < LastUsedParticle; i++) {
		if (ParticlesContainer[i].life < 0) {
			LastUsedParticle = i;
			return i;
		}
	}

	return 0; // All particles are taken, override the first one
}

void SortParticles() {
	std::sort(&ParticlesContainer[0], &ParticlesContainer[MaxParticles]);
}

void drawObject(GLuint program, Core::RenderContext context, glm::mat4 modelMatrix, glm::vec3 color)
{
	glUniform3f(glGetUniformLocation(program, "objectColor"), color.x, color.y, color.z);

	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;


	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);

	Core::DrawContext(context);
}

unsigned int loadCubemap(std::vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}


void drawObjectTexture(GLuint program, Core::RenderContext context, glm::mat4 modelMatrix, glm::vec3 color, GLuint textureID)
{
	glUseProgram(program);

	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;


	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);

	Core::SetActiveTexture(textureID, "textureSampler", program, 0);

	Core::DrawContext(context);
}

void updateParticlesBuffers(int ParticlesCount, glm::mat4 ViewProjectionMatrix)
{
	// Update the buffers that OpenGL uses for rendering.
	// There are much more sophisticated means to stream data from the CPU to the GPU,
	// but this is outside the scope of this tutorial.
	// http://www.opengl.org/wiki/Buffer_Object_Streaming


	glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
	glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
	glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLfloat) * 4, g_particule_position_size_data); // prypisanie nowych wartosci w vertex array

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(programParticles);

	glUniformMatrix4fv(glGetUniformLocation(programParticles, "VP"), 1, GL_FALSE, (float*)&ViewProjectionMatrix);

	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
	glVertexAttribPointer(
		0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	// 2nd attribute buffer : positions of particles' centers
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
	glVertexAttribPointer(
		1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		4,                                // size : x + y + z + size => 4
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
	);

	// These functions are specific to glDrawArrays*Instanced*.
	// The first parameter is the attribute buffer we're talking about.
	// The second parameter is the "rate at which generic vertex attributes advance when rendering multiple instances"
	// http://www.opengl.org/sdk/docs/man/xhtml/glVertexAttribDivisor.xml
	glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices -> 0
	glVertexAttribDivisor(1, 1); // positions : one per quad (its center)                 -> 1                               -> 1

	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ParticlesCount);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

void drawParticles(glm::mat4 shipModelMatrix)
{
	// Clear the screen
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	double currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	double deltaP = currentTime - lastTime;
	lastTime = currentTime;

	glm::mat4 ViewProjectionMatrix = perspectiveMatrix * cameraMatrix;// *shipModelMatrix;

	// Generate 10 new particule each millisecond,
	// but limit this to 16 ms (60 fps), or if you have 1 long frame (1sec),
	// newparticles will be huge and the next frame even longer.
	int newparticles = (int)(deltaP * 10000.0);
	if (newparticles > (int)(0.016f * 10000.0))
		newparticles = (int)(0.016f * 10000.0);

	for (int i = 0; i < newparticles; i++) {
		int particleIndex = FindUnusedParticle();
		ParticlesContainer[particleIndex].life = 1.0f; // This particle will live 5 seconds.
		ParticlesContainer[particleIndex].pos = glm::vec3((shipModelMatrix * glm::vec4(0, 0, -0.5, 1)));

		float spread = 1.5f;
		glm::vec3 maindir = -cameraDir;
		// Very bad way to generate a random direction;
		// See for instance http://stackoverflow.com/questions/5408276/python-uniform-spherical-distribution instead,
		// combined with some user-controlled parameters (main direction, spread, etc)
		glm::vec3 randomdir = glm::vec3(
			(rand() % 2000 - 1000.0f) / 1000.0f,
			(rand() % 2000 - 1000.0f) / 1000.0f,
			(rand() % 2000 - 1000.0f) / 1000.0f
		);

		ParticlesContainer[particleIndex].speed = maindir + randomdir * spread;

		ParticlesContainer[particleIndex].size = ((rand() % 1000) / 2000.0f + 0.1f) * 0.005f;

	}



	// Simulate all particles
	int ParticlesCount = 0;
	for (int i = 0; i < MaxParticles; i++) {

		Particle& p = ParticlesContainer[i]; // shortcut

		if (p.life > 0.0f) {

			// Decrease life
			p.life -= deltaP;
			if (p.life > 0.0f) {

				// Simulate simple physics : gravity only, no collisions
				p.speed += glm::vec3(0.0f, 0.f, -10.0f) * (float)deltaP * 0.5f;
				p.pos += p.speed * (float)deltaP;
				p.cameradistance = glm::length2(p.pos - cameraPos);
				ParticlesContainer[i].pos += glm::vec3(0.0f, 0.0f, 0.0f) * (float)deltaP;

				// Fill the GPU buffer
				// vertex array update
				g_particule_position_size_data[4 * ParticlesCount + 0] = p.pos.x;
				g_particule_position_size_data[4 * ParticlesCount + 1] = p.pos.y;
				g_particule_position_size_data[4 * ParticlesCount + 2] = p.pos.z;

				g_particule_position_size_data[4 * ParticlesCount + 3] = p.size;

			}
			else {
				// Particles that just died will be put at the end of the buffer in SortParticles();
				p.cameradistance = -1.0f;
			}

			ParticlesCount++;

		}

	}

	SortParticles();

	//updateParticlesBuffers(ParticlesCount, ViewProjectionMatrix);


	glUseProgram(programParticles);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glm::mat4 modelMatrix = glm::mat4(1);
	glm::vec4 t = glm::vec4(0);
	glUniform3f(glGetUniformLocation(programParticles, "particleColor"), 0.0f, 0.0f, 0.0f);
	glUniformMatrix4fv(glGetUniformLocation(programParticles, "VP"), 1, GL_FALSE, (float*)&ViewProjectionMatrix);

	//Core::DrawContextInstanced(particleContext, ParticlesCount, MaxParticles, g_particule_position_size_data);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ParticlesCount);

	glUseProgram(0);
}

void drawSkybox(GLuint program, Core::RenderContext context, GLuint textureID) {
	glUseProgram(program);

	glDepthFunc(GL_LEQUAL);

	glm::mat4 transformation = perspectiveMatrix * glm::mat4(glm::mat3(cameraMatrix));

	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);

	glUniform1i(glGetUniformLocation(program, "skybox"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	Core::DrawContext(context);

	glDepthFunc(GL_LESS);
	glUseProgram(0);
}


void renderScene()
{

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.3f, 0.3f, 1.0f);

	//tutaj ustawiamy buffer na ktorym bedziemy wszystko rysowac
	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	cameraMatrix = createCameraMatrix();
	perspectiveMatrix = Core::createPerspectiveMatrix(0.01f, 1000.0f, frustumScale);

	delta = glutGet(GLUT_ELAPSED_TIME) / 1000.f - timer;
	timer = glutGet(GLUT_ELAPSED_TIME) / 1000.f;

	cameraAngle = (cameraAngle + changeCameraAngle) * delta + cameraAngle * (1 - delta); // plynniejszy obrot z i x

	if (breaking == true) {
		if (changeCameraAngle > 0) { // plynniejsze hamowanie
			changeCameraAngle -= step;
		}
		else {
			changeCameraAngle += step;
		}

		if (abs(changeCameraAngle - step) < step) {
			changeCameraAngle = 0; //pelne zatrzymanie obrotu
		}
	}

	//earth
	glm::mat4 sphereModelMatrix = glm::mat4(1.0f);
	sphereModelMatrix = glm::rotate(timer / 3, glm::vec3(-0.2f, 1.0f, 0.0f)) *
		glm::translate(glm::vec3(0.0f, 0.0f, 0.0f)) * glm::rotate(timer / 2, glm::vec3(0.0f, 0.8f, 0.0f)) *
		glm::scale(glm::vec3(0.5f));

	//ship
	shipModelMatrix = glm::translate(cameraPos + cameraDir * 0.5f + glm::vec3(0, -0.15f, 0)) *
		glm::rotate(-cameraAngle + glm::radians(0.0f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.03f));

	//sun
	glm::mat4 sunModelMatrix = glm::mat4(1.0f);
	sunModelMatrix = glm::rotate(timer / 8, glm::vec3(-0.2f, 1.0f, 0.0f)) *
		glm::translate(glm::vec3(0.0f, 0.0f, 40.0f)) * glm::rotate(timer / 5, glm::vec3(0.0f, -0.1f, 0.0f)) *
		glm::scale(glm::vec3(5.0f));

	//comet
	glm::mat4 sunModelMatrix2 = glm::mat4(1.0f);
	sunModelMatrix2 = glm::rotate(timer / 3, glm::vec3(-0.8f, 0.4f, 0.0f)) *
		glm::translate(glm::vec3(0.0f, 0.0f, 70.0f)) * glm::scale(glm::vec3(0.7f));

	glm::mat4 sunLight = sunModelMatrix;
	glm::mat4 sunLight2 = sunModelMatrix2;


	lightCollector[0].position = glm::vec3(sunLight[3][0], sunLight[3][1], sunLight[3][2]);
	lightCollector[1].position = glm::vec3(sunLight2[3][0], sunLight2[3][1], sunLight2[3][2]);

	lightCollector[0].color = glm::vec3(1.0f, 1.0f, 0.9f);
	lightCollector[1].color = glm::vec3(0.3f, 0.3f, 0.9f);


	glUseProgram(programTexture);

	for (int i = 0; i < lightCollector.size(); i++) {
		std::string pos = "lights[" + std::to_string(i) + "].Pos";
		std::string color = "lights[" + std::to_string(i) + "].Color";
		glUniform3f(glGetUniformLocation(programTexture, pos.c_str()), lightCollector[i].position.x, lightCollector[i].position.y,
			lightCollector[i].position.z);
		glUniform3f(glGetUniformLocation(programTexture, color.c_str()), lightCollector[i].color.x, lightCollector[i].color.y,
			lightCollector[i].color.z);
	}

	glUniform3f(glGetUniformLocation(programTexture, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);


	//draw Ship
	drawObject(programTexture, shipContext, shipModelMatrix, glm::vec3(0.1f));
	//glUniform3f(glGetUniformLocation(programTexture1, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
	//glm::mat4 shipModelMatrix1 = glm::mat4(1.0f);
	//float angle = 90.0 * (M_PI / 180.0);
	//shipModelMatrix1 = shipModelMatrix * glm::rotate(angle, glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.2f));
	//drawObjectTexture(programTexture1, shipContext, shipModelMatrix1, glm::vec3(0.6f), teslaTexture);

	//draw Earth
	if (earthDead == false)
	{
		drawObjectTexture(programTexture, sphereContext, sphereModelMatrix, glm::vec3(1.0f, 0.3f, 0.3f), earthTexture);
	}

	//creating planets matrixs
	glm::mat4 sphereModelMatrix1 = glm::mat4(1.0f);
	sphereModelMatrix1 = glm::rotate(timer / 3, glm::vec3(-0.2f, 1.0f, 0.0f)) *
		glm::translate(glm::vec3(0.0f, 0.0f, 10.0f)) * glm::rotate(timer / 2, glm::vec3(0.0f, 0.8f, 0.0f)) *
		glm::scale(glm::vec3(0.5f));

	glm::mat4 sphereModelMatrix2 = glm::mat4(1.0f);
	sphereModelMatrix2 = glm::rotate(timer / 6, glm::vec3(-0.2f, 1.0f, 0.0f)) *
		glm::translate(glm::vec3(0.0f, 0.0f, 24.0f)) * glm::rotate(timer / 2, glm::vec3(0.0f, -0.2f, 0.0f)) *
		glm::scale(glm::vec3(0.7f));

	//draw moon
	glm::mat4 sphereModelMatrix3 = glm::mat4(1.0f);
	sphereModelMatrix3 = glm::rotate(timer / 6, glm::vec3(-0.2f, 1.0f, 0.0f)) * glm::translate(glm::vec3(0.0f, 0.0f, 24.0f)) *
		glm::rotate(timer, glm::vec3(-0.5f, 0.0f, 0.0f)) * glm::translate(glm::vec3(0.0f, 0.0f, 2.0f)) *
		glm::rotate(timer / 2, glm::vec3(-0.5f, 0.0f, 0.0f)) * glm::scale(glm::vec3(0.3f));

	//glUniform3f(glGetUniformLocation(programTexture, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	drawObjectTexture(programTexture, sphereContext, sphereModelMatrix1, glm::vec3(1.0f, 0.3f, 0.3f), marsTexture);
	drawObjectTexture(programTexture, sphereContext, sphereModelMatrix2, glm::vec3(1.0f, 0.3f, 0.3f), jupiterTexture);
	drawObjectTexture(programTexture, sphereContext, sphereModelMatrix3, glm::vec3(1.0f, 0.3f, 0.3f), erisTexture);

	//draw bullet
	for (auto bulletit = bullets.begin(); bulletit != bullets.end();)
	{
		auto bullet = bulletit->get();
		bullet->age -= delta;
		if (glm::distance(bullet->position, glm::vec3(0.0f, 0.0f, 0.0f)) < 0.05f + 0.5f)
		{

			bulletit = bullets.erase(bulletit);
			earthDead = true;
		}
		else if (bullet->age > 0.0f)
		{
			bullet->bulletModelMatrix *= glm::translate(bullet->dir / bullet->velocityDiv);
			bullet->position += bullet->dir;
			drawObjectTexture(programTexture, *bullet->context, bullet->bulletModelMatrix * glm::scale(bullet->scaleVector), glm::vec3(1.0f, 0.3f, 0.3f), bullet->textureID);
			bulletit++;
		}
		else
		{
			bulletit = bullets.erase(bulletit);
		}
	}


	glUseProgram(programSun);

	//draw sun and comet
	glUniform3f(glGetUniformLocation(programSun, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	drawObjectTexture(programSun, sphereContext, sunModelMatrix, lightColor, sunTexture);
	drawObjectTexture(programSun, sphereContext, sunModelMatrix2, lightColor, cometTexture);

	drawSkybox(programSkybox, cubeContext, skyboxTexture);

	drawBloom();

	drawParticles(shipModelMatrix);

	glUseProgram(0);
	glutSwapBuffers();
}

void init()
{
	glEnable(GL_DEPTH_TEST);
	//programShip = shaderLoader.CreateProgram("shaders/shader_4_1.vert", "shaders/shader_4_1.frag");
	//program = shaderLoader.CreateProgram("shaders/shader_4_1.vert", "shaders/shader_4_1.frag");
	programSun = shaderLoader.CreateProgram("shaders/shader_4_sun.vert", "shaders/shader_4_sun.frag");
	programTexture = shaderLoader.CreateProgram("shaders/shader_tex.vert", "shaders/shader_tex.frag");
	programSkybox = shaderLoader.CreateProgram("shaders/shader_skybox.vert", "shaders/shader_skybox.frag");
	programParticles = shaderLoader.CreateProgram("shaders/shader_particles.vert", "shaders/shader_particles.frag");

	startBloom();

	ship = obj::loadModelFromFile("models/spaceship.obj");
	shipContext.initFromOBJ(ship);

	sphere = obj::loadModelFromFile("models/sphere.obj");
	sphereContext.initFromOBJ(sphere);

	cube = obj::loadModelFromFile("models/cube.obj");
	cubeContext.initFromOBJ(cube);

	earthTexture = Core::LoadTexture("textures/earth.png");
	marsTexture = Core::LoadTexture("textures/mars.png");
	jupiterTexture = Core::LoadTexture("textures/jupiter.png");
	erisTexture = Core::LoadTexture("textures/eris.png");
	cometTexture = Core::LoadTexture("textures/neptune.png");
	sunTexture = Core::LoadTexture("textures/sun.png");

	skyboxTexture = loadCubemap(faces);

	Light l1;
	l1.position = glm::vec3(0.0f);
	l1.color = glm::vec3(1.0f, 0.0f, 0.0f);
	lightCollector.push_back(l1);

	Light l2;
	l2.position = glm::vec3(0.0f);
	l2.color = glm::vec3(1.0f, 0.0f, 0.0f);
	lightCollector.push_back(l2);

	Light l3;
	l3.position = glm::vec3(0.0f);
	l3.color = glm::vec3(1.0f, 0.0f, 0.0f);
	lightCollector.push_back(l3);


	lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

	for (int i = 0; i < MaxParticles; i++) {
		ParticlesContainer[i].life = -1.0f;
		ParticlesContainer[i].cameradistance = -1.0f;
	}

	static const GLfloat g_vertex_buffer_data[] =
	{
			 -0.5f, -0.5f, 0.0f,
			  0.5f, -0.5f, 0.0f,
			 -0.5f,  0.5f, 0.0f,
			  0.5f,  0.5f, 0.0f,
	};
	/*
	printf("%i", sphereContextSize);*/

	glGenBuffers(1, &billboard_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glGenBuffers(1, &particles_position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
	glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
	}

void shutdown()
{
	shaderLoader.DeleteProgram(programSun);
}

void idle()
{
	glutPostRedisplay();
}

void onReshape(int width, int height) {
	frustumScale = (float)width / (float)height;
	glViewport(0, 0, width, height);
	w = width;
	h = height;
	//createBuffers();
}

int main(int argc, char** argv)
{
	w = 1240;
	h = 720;
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(200, 300);
	glutInitWindowSize(w, h);
	glutCreateWindow("Space Flight");
	glewInit();

	init();
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouseClick);
	glutPassiveMotionFunc(mouseMovement);
	glutDisplayFunc(renderScene);
	glutIdleFunc(idle);

	glutReshapeFunc(onReshape);
	glutMainLoop();

	shutdown();

	return 0;
}

void startBloom()
{
	programBlur = shaderLoader.CreateProgram("shaders/shader_blur.vert", "shaders/shader_blur.frag");
	programBloom = shaderLoader.CreateProgram("shaders/shader_bloom.vert", "shaders/shader_bloom.frag");

	//hdr to taki normalny framebuffer, na ktorm bedziem rysowac wszystko
	// configure (floating point) framebuffers
// ---------------------------------------
	glGenFramebuffers(1, &hdrFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
	// create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)

	createBuffers();

	glUseProgram(programBlur);
	glUniform1i(glGetUniformLocation(programBlur, "image"), 0);
	glUseProgram(programBloom);
	glUniform1i(glGetUniformLocation(programBloom, "scene"), 0);
	glUniform1i(glGetUniformLocation(programBloom, "bloomBlur"), 1);
}
void createBuffers()
{
	glGenTextures(2, colorBuffers);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// attach texture to framebuffer
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
	}
	// create and attach depth buffer (renderbuffer) potrzebujemy go zeby moc obslugiwac glebokosc na bufferze
	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 

	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);
	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// ping-pong-framebuffer for blurring
	glGenFramebuffers(2, pingpongFBO);
	glGenTextures(2, pingpongColorbuffers);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
		glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
		// also check if framebuffers are complete (no need for depth buffer)
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;
	}
}
void drawBloom()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 2. blur bright fragments with two-pass Gaussian Blur 
	// --------------------------------------------------
	bool horizontal = true, first_iteration = true;
	unsigned int amount = 10;
	glUseProgram(programBlur);
	for (unsigned int i = 0; i < amount; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
		glUniform1i(glGetUniformLocation(programBlur, "horizontal"), horizontal);
		glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
		renderQuad();
		horizontal = !horizontal;
		if (first_iteration)
			first_iteration = false;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 3. now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
	// --------------------------------------------------------------------------------------------------------------------------
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(programBloom);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
	//tu mozna zmienic potem
	glUniform1i(glGetUniformLocation(programBloom, "bloom"), bloom);
	glUniform1f(glGetUniformLocation(programBloom, "exposure"), exposure);
	renderQuad();

	//std::cout << "bloom: " << (bloom ? "on" : "off") << "| exposure: " << exposure << std::endl;

	// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
	// -------------------------------------------------------------------------------
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
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
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}
