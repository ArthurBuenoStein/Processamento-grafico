
#define _CRT_SECURE_NO_WARNINGS
//#define STB_IMAGE_IMPLEMENTATION
#include "gl_utils.cpp"
#include "stb_image.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string.h>
#include <time.h>
#define GL_LOG_FILE "gl.log"
#include <iostream>
#include <vector>
#include "TileMap.h"
#include "DiamondView.h"
#include "SlideView.h"
#include "ltMath.h"
#include <fstream>

using namespace std;

int g_gl_width = 800;
int g_gl_height = 800;
float xi = -1.0f;
float xf = 1.0f;
float yi = -1.0f;
float yf = 1.0f;
float w = xf - xi;
float h = yf - yi;
float tw, th, tw2, th2;
int tileSetCols = 9, tileSetRows = 9;
float tileW, tileW2;
float tileH, tileH2;
int cx = 0, cy = 0;
bool jogoFinalizado = false;

TilemapView* tview = new DiamondView();
TileMap* tmap = NULL;
TileMap* collideMap = NULL;

GLFWwindow* g_window = NULL;

TileMap* readMap(char* filename) {
	ifstream arq(filename);
	int w, h;
	arq >> w >> h;
	TileMap* tmap = new TileMap(w, h, 0);
	for (int r = 0; r < h; r++) {
		for (int c = 0; c < w; c++) {
			int tid;
			arq >> tid;
		//	cout << tid << " ";
			tmap->setTile(c, h - r - 1, tid);
		}
	//	cout << endl;
	}
	arq.close();
	return tmap;
}

int loadTexture(unsigned int& texture, char* filename)
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	GLfloat max_aniso = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
	// set the maximum!
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);

	int width, height, nrChannels;

	unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
	if (data)
	{
		if (nrChannels == 4)
		{
			cout << "Alpha channel" << endl;
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		else
		{
			cout << "Without Alpha channel" << endl;
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
	return 1;
}

void SRD2SRU(double& mx, double& my, float& x, float& y) {
	x = xi + (mx / g_gl_width) * w;
	y = yi + (1 - (my / g_gl_height)) * h;
}

void moveObject(int c, int r, const int direction) {
	tview->computeTileWalking(c, r, direction);

	if ((c < 0) || (c >= collideMap->getWidth()) || (r < 0) || (r >= collideMap->getHeight())) {
		cout << "Fora do mapa: " << c << ", " << r << endl;
		return; // posi��o inv�lida!
	}

	if (c == 9) {
		cout << "Muito bem! Sully esta muito feliz por ter chegado ao outro lado do mapa! Pressione espa�o se quiser reiniciar a partida" << endl;
		jogoFinalizado = true;
		cx = -1;
		cy = -1;
		return;
	}

	unsigned char t_id = collideMap->getTile(c, r);
	if(t_id == 0) cout << "Terra" << endl;
	else if (t_id == 1) {
		cout << "Ops! Voce deixou o Sully cair na agua... Pressione espaco para reiniciar a partida" << endl;
		jogoFinalizado = true;
		cx = -1;
		cy = -1;
		return;
	};

	cout << "Posicao c=" << c << "," << r << endl;
	cx = c; cy = r;
}

void restart() {
	cx = 0;
	cy = 0;
	jogoFinalizado = 0;
}

int main()
{

#pragma region inicializacao do OpenGL
	restart_gl_log();
	// all the GLFW and GLEW start-up code is moved to here in gl_utils.cpp
	start_gl();
	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS);
#pragma endregion

#pragma region carregamento do tmap e collideMap
	cout << "Tentando criar tmap" << endl;
	tmap = readMap("terrain1.tmap");
	collideMap = readMap("collide.tmap");
	tw = w / (float)tmap->getWidth();
	th = tw / 2.0f;
	tw2 = th;
	th2 = th / 2.0f;
	tileW = 1.0f / (float)tileSetCols;
	tileW2 = tileW / 2.0f;
	tileH = 1.0f / (float)tileSetRows;
	tileH2 = tileH / 2.0f;

	//cout << "tw=" << tw << " th=" << th << " tw2=" << tw2 << " th2=" << th2
	//	<< " tileW=" << tileW << " tileH=" << tileH
	//	<< " tileW2=" << tileW2 << " tileH2=" << tileH2
	//	<< endl;
#pragma endregion

#pragma region carregamento de texturas e associacao com tmap
	// cenario
	GLuint tid;
	loadTexture(tid, "images/terrain.png");


	tmap->setTid(tid);
	cout << "Tmap inicializado" << endl;

	unsigned int texturaObjeto1;
	glGenTextures(1, &texturaObjeto1);
	glBindTexture(GL_TEXTURE_2D, texturaObjeto1);

	unsigned int texturaObjeto2;
	glGenTextures(1, &texturaObjeto2);
	glBindTexture(GL_TEXTURE_2D, texturaObjeto2);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	GLfloat max_aniso = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
	// set the maximum!
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);

	int width, height, nrChannels;

	// unsigned char *data = stbi_load("spritesheet-muybridge.jpg", &width, &height, &nrChannels, 0);
	//unsigned char* data = stbi_load("spritesheet-muybridge.png", &width, &height, &nrChannels, 0);
	// MAPEAMENTO PARA SULLY
	unsigned char* data = stbi_load("images/sully.png", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		// MAPEAMENTO PARA SULLY
		// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);


	// MAPEAMENTO PARA KEY
	// MEXEMOS AQUI
	unsigned char* data1 = stbi_load("images/key.png", &width, &height, &nrChannels, 0);
	if (data1)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data1);
		// MAPEAMENTO PARA SULLY
		// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data1);

		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data1);
	
	


#pragma endregion

#pragma region vertices
	float verticesCenario[] = {
		// positions   // texture coords
		xi    , yi + th2, 0.0f, tileH2,   // left
		xi + tw2, yi    , tileW2, 0.0f,   // bottom
		xi + tw , yi + th2, tileW, tileH2,  // right
		xi + tw2, yi + th , tileW2, tileH,  // top
	};
	unsigned int indices[] = {
		0, 1, 3, // first triangle
		3, 1, 2  // second triangle
	};

	float verticesObjeto[] = {
		 -2.6f, -0.8f, 0.25f, 0.25f, // top right
		 -2.6f, -1.0f, 0.25f, 0.0f, // bottom right
		 -2.8f, -1.0f, 0.0f, 0.0f, // bottom left
		 -2.8f, -0.8f, 0.0f, 0.25f, // top left
	};

	//MEXEMOS AQUI
	float verticesKey[] = {
	 -2.6f, -0.8f, 0.25f, 0.25f, // top right
	 -2.6f, -1.0f, 0.25f, 0.0f,  // bottom right
	 -2.8f, -1.0f, 0.0f, 0.0f,   // bottom left
	 -2.8f, -0.8f, 0.0f, 0.25f,  // top left
	};
#pragma endregion


#pragma region passagem dados para GPU
	unsigned int VBOCenario, VBOObjeto, VAO, EBO, VBOKey;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBOCenario);
	glGenBuffers(1, &VBOObjeto);
	glGenBuffers(1, &VBOKey);//MEXEMOS AQUI
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	// cen�rio
	glBindBuffer(GL_ARRAY_BUFFER, VBOCenario);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCenario), verticesCenario, GL_STATIC_DRAW);
	// position attribute
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// objeto
	glBindBuffer(GL_ARRAY_BUFFER, VBOObjeto);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesObjeto), verticesObjeto, GL_STATIC_DRAW);
	// position attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(2);
	// texture coord attribute
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(3);


	// MEXEMOS AQUI
	// key
	glBindBuffer(GL_ARRAY_BUFFER, VBOKey);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesKey), verticesKey, GL_STATIC_DRAW);
	// position attribute
	glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(4);
	// texture coord attribute
	glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(5);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
#pragma endregion

#pragma region shaders
	char vertex_shader[1024 * 256];
	char fragment_shader[1024 * 256];
	parse_file_into_str("_geral_vs.glsl", vertex_shader, 1024 * 256);
	parse_file_into_str("_geral_fs.glsl", fragment_shader, 1024 * 256);

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	const GLchar* p = (const GLchar*)vertex_shader;
	glShaderSource(vs, 1, &p, NULL);
	glCompileShader(vs);
#pragma endregion

#pragma region  check for compile errors
	// check for compile errors
	int params = -1;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: GL shader index %i did not compile\n", vs);
		print_shader_info_log(vs);
		return 1; // or exit or something
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	p = (const GLchar*)fragment_shader;
	glShaderSource(fs, 1, &p, NULL);
	glCompileShader(fs);

	// check for compile errors
	glGetShaderiv(fs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: GL shader index %i did not compile\n", fs);
		print_shader_info_log(fs);
		return 1; // or exit or something
	}
#pragma endregion

#pragma region cria programa e linka os shaders com o shader program
	GLuint shader_programme = glCreateProgram();
	glAttachShader(shader_programme, fs);
	glAttachShader(shader_programme, vs);
	glLinkProgram(shader_programme);

	glGetProgramiv(shader_programme, GL_LINK_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: could not link shader programme GL index %i\n",
			shader_programme);
		// 		print_programme_info_log( shader_programme );
		return false;
	}
#pragma endregion
	for (int r = 0; r < tmap->getHeight(); r++) {
		for (int c = 0; c < tmap->getWidth(); c++) {
			unsigned char t_id = tmap->getTile(c, r);
	//		cout << ((int)t_id) << " ";
		}
	//	cout << endl;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// glEnable(GL_DEPTH_TEST);


#pragma region instrucoes
	cout << "Jogo iniciado! Sully tem a missao de atravessar o mapa sem se molhar, ajude-o nesta jornada. Boa sorte!" << endl;
#pragma endregion

#pragma region inicializacao das variaveis
	float fw = 0.25f;
	float fh = 0.25f;
	float offsetx = 0, offsety = 0;
	int frameAtual = 0;
	int acao = 3;
	int sign = 1;
	float previous = glfwGetTime();
#pragma endregion

	bool rightPressed = false;
	bool leftPressed = false;
	bool upPressed = false;
	bool downPressed = false;
	bool spacePressed = false;

	while (!glfwWindowShouldClose(g_window))
	{
		_update_fps_counter(g_window);
		double current_seconds = glfwGetTime();

#pragma region limpa a tela
		// wipe the drawing surface clear
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// glClear(GL_COLOR_BUFFER_BIT);
#pragma endregion

		glViewport(0, 0, g_gl_width, g_gl_height);

#pragma region define programa
		glUseProgram(shader_programme);
#pragma endregion

#pragma region bind da geometria
		glBindVertexArray(VAO);
#pragma endregion

#pragma region passagem de informacoes aos shaders e desenho do cenario
		glBindBuffer(GL_ARRAY_BUFFER, VBOCenario);
		glUniform1f(glGetUniformLocation(shader_programme, "isObject"), false);

		//para cada linha e columa da matriz � calculada a posi��o de desenho chamando computeDrawPosition
		float x, y;
		int r = 0, c = 0;
		for (int r = 0; r < tmap->getHeight(); r++) {
			for (int c = 0; c < tmap->getWidth(); c++) {
				//t_id: n� do tile na matriz usado para calcular a por��o da textura que deve ser apresentada nesse tile
				int t_id = (int)tmap->getTile(c, r);
				int u = t_id % tileSetCols;
				int v = t_id / tileSetCols;

				tview->computeDrawPosition(c, r, tw, th, x, y);

				glUniform1f(glGetUniformLocation(shader_programme, "offsetx"), u * tileW);
				glUniform1f(glGetUniformLocation(shader_programme, "offsety"), v * tileH);
				glUniform1f(glGetUniformLocation(shader_programme, "tx"), x);
				glUniform1f(glGetUniformLocation(shader_programme, "ty"), y + 1.0);
				glUniform1f(glGetUniformLocation(shader_programme, "layer_z"), 0.50);
		
				//adiciona cor ao tile selecionado
				glUniform1f(glGetUniformLocation(shader_programme, "weight"), (c == cx) && (r == cy) ? 0.5 : 0.0);

				// bind Texture
				// glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, tmap->getTileSet());
				glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
		}
#pragma endregion

#pragma region Objeto
		glBindBuffer(GL_ARRAY_BUFFER, VBOObjeto);
		glUniform1f(glGetUniformLocation(shader_programme, "isObject"), true);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texturaObjeto1);
		glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);

		// MEXEMOS AQUI
		glBindBuffer(GL_ARRAY_BUFFER, VBOKey);
		glUniform1f(glGetUniformLocation(shader_programme, "isObject"), true);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texturaObjeto2);
		glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);


		glUseProgram(shader_programme);

		float tx, ty;
		tview->computeDrawPosition(cx, cy, tw, th, tx, ty);
		glUniform1f(glGetUniformLocation(shader_programme, "tx"), 1.8 +  tx);
		glUniform1f(glGetUniformLocation(shader_programme, "ty"), 1.0 + ty);
		glUniform1f(glGetUniformLocation(shader_programme, "offsetx"), offsetx);
		glUniform1f(glGetUniformLocation(shader_programme, "offsety"), offsety);
		glUniform1f(glGetUniformLocation(shader_programme, "layer_z"), 0.10);

		if(!jogoFinalizado)glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
#pragma endregion

#pragma region Eventos
		glfwPollEvents();

		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_ESCAPE))
		{
			glfwSetWindowShouldClose(g_window, 1);
		}

		const int rightState = glfwGetKey(g_window, GLFW_KEY_RIGHT);
		if (GLFW_PRESS == rightState) {
			rightPressed = true;
		}
		if (GLFW_RELEASE == rightState && rightPressed) {
			moveObject(cx, cy, DIRECTION_EAST);
			acao = (4 + (1)) % 4;
			frameAtual = (frameAtual + 1) % 4;
			offsetx = fw * (float)frameAtual;
			offsety = fh * (float)acao;
			rightPressed = false;
		}

		const int leftState = glfwGetKey(g_window, GLFW_KEY_LEFT);
		if (GLFW_PRESS == leftState) {
			leftPressed = true;
		}
		if (GLFW_RELEASE == leftState && leftPressed) {
			moveObject(cx, cy, DIRECTION_WEST);
			acao = (4 + (2)) % 4;
			frameAtual = (frameAtual + 1) % 4;
			offsetx = fw * (float)frameAtual;
			offsety = fh * (float)acao;
			leftPressed = false;
		}

		const int upState = glfwGetKey(g_window, GLFW_KEY_UP);
		if (GLFW_PRESS == upState) {
			upPressed = true;
		}
		if (GLFW_RELEASE == upState && upPressed) {
			moveObject(cx, cy, DIRECTION_NORTH);
			acao = (4 + (0)) % 4;
			frameAtual = (frameAtual + 1) % 4;
			offsetx = fw * (float)frameAtual;
			offsety = fh * (float)acao;
			upPressed = false;
		}

		const int downState = glfwGetKey(g_window, GLFW_KEY_DOWN);
		if (GLFW_PRESS == downState) {
			downPressed = true;
		}
		if (GLFW_RELEASE == downState && downPressed) {
			moveObject(cx, cy, DIRECTION_SOUTH);
			acao = (4 + (3)) % 4;
			frameAtual = (frameAtual + 1) % 4;
			offsetx = fw * (float)frameAtual;
			offsety = fh * (float)acao;
			downPressed = false;
		}

		const int spaceState = glfwGetKey(g_window, GLFW_KEY_SPACE);
		if (GLFW_PRESS == spaceState) {
			spacePressed = true;
		}
		if (GLFW_RELEASE == spaceState && spacePressed) {
			restart();
			spacePressed = false;
		}
		
#pragma endregion

		// put the stuff we've been drawing onto the display
		glfwSwapBuffers(g_window);
	}

	// close GL context and any other GLFW resources
	glfwTerminate();
	delete tmap;
	delete collideMap;
	return 0;
}

