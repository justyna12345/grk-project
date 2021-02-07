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
//GLuint programTexture1;
GLuint programSun;
GLuint programTexture;
GLuint programSkybox;
Core::Shader_Loader shaderLoader;

GLuint skyboxTexture;


float frustumScale = 1.0f;

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
	//float age;

};
std::vector<Bullet*> bullets;


float cameraAngle = 0;
glm::vec3 cameraPos = glm::vec3(-70, 0, 0);
glm::vec3 cameraDir;

obj::Model sphere;
obj::Model shipModel;
Core::RenderContext shipContext;
glm::mat4 shipModelMatrix = glm::mat4(1.0f);
Core::RenderContext sphereContext;

obj::Model cube;
Core::RenderContext cubeContext;

//obj::Model ship;
//Core::RenderContext shipContext;

glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

struct Light {
	glm::vec3 position;
	glm::vec3 color;
};

std::vector<Light> lightCollector;

glm::mat4 cameraMatrix, perspectiveMatrix;


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
	float angleSpeed = 0.1f;
	float moveSpeed = 0.7f;
	switch (key)
	{
	case 'z': cameraAngle -= angleSpeed; break;
	case 'x': cameraAngle += angleSpeed; break;
	case 'w': cameraPos += cameraDir * moveSpeed; break;
	case 's': cameraPos -= cameraDir * moveSpeed; break;
	case 'd': cameraPos += glm::cross(cameraDir, glm::vec3(0, 1, 0)) * moveSpeed; break;
	case 'a': cameraPos -= glm::cross(cameraDir, glm::vec3(0, 1, 0)) * moveSpeed; break;
	case 'e': cameraPos += glm::cross(cameraDir, glm::vec3(1, 0, 0)) * moveSpeed; break;
	case 'q': cameraPos -= glm::cross(cameraDir, glm::vec3(1, 0, 0)) * moveSpeed; break;
	}
}



void createBullet() {
	Bullet* bullet = new Bullet();
	bullet->bulletModelMatrix = shipModelMatrix;
	bullet->velocityDiv = 4.0f;
	bullet->context = &sphereContext;
	bullet->textureID= marsTexture;
	bullet->scaleVector= glm::vec3(0.5f);

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
	glm::vec3 up = glm::vec3(0, 1, 0);

	return Core::createViewMatrix(cameraPos, cameraDir, up);
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

void drawSkybox(GLuint program, Core::RenderContext context, GLuint textureID) {
	glUseProgram(program);

	glm::mat4 transformation = perspectiveMatrix * glm::mat4(glm::mat3(cameraMatrix));

	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);

	glDepthMask(GL_FALSE);
	glUniform1i(glGetUniformLocation(program, "skybox"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	Core::DrawContext(context);
	glDepthMask(GL_TRUE);
	glUseProgram(0);
}

void renderScene()
{

	cameraMatrix = createCameraMatrix();
	perspectiveMatrix = Core::createPerspectiveMatrix(0.01f, 1000.0f, frustumScale);
	float time = glutGet(GLUT_ELAPSED_TIME) / 1000.f;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.3f, 0.3f, 1.0f);


	drawSkybox(programSkybox, cubeContext, skyboxTexture);

	glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 3.0f);

	
	glm::mat4 sphereModelMatrix = glm::mat4(1.0f);
	//sphereModelMatrix = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));

	glUseProgram(programTexture);

	//ship
	shipModelMatrix = glm::translate(cameraPos + cameraDir * 0.5f + glm::vec3(0.25, -0.15f, 0)) * 
		glm::rotate(-cameraAngle + glm::radians(0.0f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.10f));

	//sun
	glm::mat4 sunModelMatrix = glm::mat4(1.0f);
	sunModelMatrix = glm::rotate(time / 8, glm::vec3(-0.2f, 1.0f, 0.0f)) *
		glm::translate(glm::vec3(0.0f, 0.0f, 40.0f)) * glm::rotate(time / 5, glm::vec3(0.0f, -0.1f, 0.0f)) *
		glm::scale(glm::vec3(5.0f));

	//comet
	glm::mat4 sunModelMatrix2 = glm::mat4(1.0f);
	sunModelMatrix2 = glm::rotate(time / 3, glm::vec3(-0.8f, 0.4f, 0.0f)) *
		glm::translate(glm::vec3(0.0f, 0.0f, 70.0f)) * glm::scale(glm::vec3(0.7f));

	glm::mat4 sunLight = sunModelMatrix;
	glm::mat4 sunLight2 = sunModelMatrix2;


	lightCollector[0].position = glm::vec3(sunLight[3][0], sunLight[3][1], sunLight[3][2]);
	lightCollector[1].position = glm::vec3(sunLight2[3][0], sunLight2[3][1], sunLight2[3][2]);

	lightCollector[0].color = glm::vec3(1.0f, 1.0f, 0.9f);
	lightCollector[1].color = glm::vec3(0.3f, 0.3f, 0.9f);


	for (int i = 0; i < lightCollector.size(); i++) {
		std::string pos = "lights[" + std::to_string(i) + "].Pos";
		std::string color = "lights[" + std::to_string(i) + "].Color";
		glUniform3f(glGetUniformLocation(programTexture, pos.c_str()), lightCollector[i].position.x, lightCollector[i].position.y,
			lightCollector[i].position.z);
		glUniform3f(glGetUniformLocation(programTexture, color.c_str()), lightCollector[i].color.x, lightCollector[i].color.y,
			lightCollector[i].color.z);
	}

	glUniform3f(glGetUniformLocation(programShip, "light_dir"), 2, 1, 0);
	glUniform3f(glGetUniformLocation(programTexture, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);


	//draw Ship
	drawObject(programTexture, shipContext, shipModelMatrix, glm::vec3(0.1f));
	//glUniform3f(glGetUniformLocation(programTexture1, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
	//glm::mat4 shipModelMatrix1 = glm::mat4(1.0f);
	//float angle = 90.0 * (M_PI / 180.0);
	//shipModelMatrix1 = shipModelMatrix * glm::rotate(angle, glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.2f));
	//drawObjectTexture(programTexture1, shipContext, shipModelMatrix1, glm::vec3(0.6f), teslaTexture);

	//draw Earth
	drawObjectTexture(programTexture, sphereContext, sphereModelMatrix, glm::vec3(1.0f, 0.3f, 0.3f), earthTexture);

	//draw planets
	glm::mat4 sphereModelMatrix1 = glm::mat4(1.0f);
	sphereModelMatrix1 = glm::rotate(time / 3, glm::vec3(-0.2f, 1.0f, 0.0f)) *
		glm::translate(glm::vec3(0.0f, 0.0f, 10.0f)) * glm::rotate(time / 2, glm::vec3(0.0f, 0.8f, 0.0f)) *
		glm::scale(glm::vec3(0.5f));

	glm::mat4 sphereModelMatrix2 = glm::mat4(1.0f);
	sphereModelMatrix2 = glm::rotate(time / 6, glm::vec3(-0.2f, 1.0f, 0.0f)) *
		glm::translate(glm::vec3(0.0f, 0.0f, 24.0f)) * glm::rotate(time / 2, glm::vec3(0.0f, -0.2f, 0.0f)) *
		glm::scale(glm::vec3(0.7f));

	//draw moon
	glm::mat4 sphereModelMatrix3 = glm::mat4(1.0f);
	sphereModelMatrix3 = glm::rotate(time / 6, glm::vec3(-0.2f, 1.0f, 0.0f)) * glm::translate(glm::vec3(0.0f, 0.0f, 24.0f)) *
		glm::rotate(time, glm::vec3(-0.5f, 0.0f, 0.0f)) * glm::translate(glm::vec3(0.0f, 0.0f, 2.0f)) *
		glm::rotate(time / 2, glm::vec3(-0.5f, 0.0f, 0.0f)) * glm::scale(glm::vec3(0.3f));

	glUniform3f(glGetUniformLocation(programTexture, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	drawObjectTexture(programTexture, sphereContext, sphereModelMatrix1, glm::vec3(1.0f, 0.3f, 0.3f), marsTexture);
	drawObjectTexture(programTexture, sphereContext, sphereModelMatrix2, glm::vec3(1.0f, 0.3f, 0.3f), jupiterTexture);
	drawObjectTexture(programTexture, sphereContext, sphereModelMatrix3, glm::vec3(1.0f, 0.3f, 0.3f), erisTexture);

	glUseProgram(programSun);

	//draw sun and comet
	glUniform3f(glGetUniformLocation(programSun, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	drawObjectTexture(programSun, sphereContext, sunModelMatrix, lightColor, sunTexture);
	drawObjectTexture(programSun, sphereContext, sunModelMatrix2, lightColor, cometTexture);


	//draw bullet
	for (int i = 0; i < bullets.size(); i++) {
		glUniform3f(glGetUniformLocation(program, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
		bullets[i]->bulletModelMatrix*=glm::translate(cameraDir/bullets[i]->velocityDiv);
		drawObjectTexture(program, *bullets[i]->context, bullets[i]->bulletModelMatrix * glm::scale(bullets[i]->scaleVector), glm::vec3(1.0f, 0.3f, 0.3f), bullets[i]->textureID);

	}

	glUseProgram(0);
	glutSwapBuffers();
}

void init()
{
	glEnable(GL_DEPTH_TEST);
	programShip = shaderLoader.CreateProgram("shaders/shader_4_1.vert", "shaders/shader_4_1.frag");
	programSun = shaderLoader.CreateProgram("shaders/shader_4_sun.vert", "shaders/shader_4_sun.frag");
	program = shaderLoader.CreateProgram("shaders/shader_4_1.vert", "shaders/shader_4_1.frag");
	programTexture = shaderLoader.CreateProgram("shaders/shader_tex.vert", "shaders/shader_tex.frag");
	programSkybox = shaderLoader.CreateProgram("shaders/shader_skybox.vert", "shaders/shader_skybox.frag");

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
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(200, 300);
	glutInitWindowSize(1240, 720);
	glutCreateWindow("OpenGL Pierwszy Program");
	glewInit();

	init();
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouseClick);
	glutDisplayFunc(renderScene);
	glutIdleFunc(idle);

	glutReshapeFunc(onReshape);
	glutMainLoop();

	shutdown();

	return 0;
}