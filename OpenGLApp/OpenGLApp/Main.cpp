#include "stb_image.h"
#include "shader_s.h"
#include "camera.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <random>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <json.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <map>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <irrKlang.h>
#include <ft2build.h>
#include FT_FREETYPE_H

using namespace std;
using json = nlohmann::json;
using namespace irrklang;

ISoundEngine* soundEngine = createIrrKlangDevice();

// Collision handling
// AABB (Axis-Aligned Bounding Box) structure
struct AABB {
	glm::vec3 min;
	glm::vec3 max;
};

// Letters
struct Character {
	unsigned int TextureID; // ID handle of the glyph texture
	glm::ivec2 Size; // Size of glyph
	glm::ivec2 Bearing; // Offset from baseline to left/top of glyph
	unsigned int Advance; // Horizontal offset to advance to next glyph
};

// Handle objects
struct Food {
	glm::vec3 position;
	int type;
};
std::vector<Food> foods;

// Handle powerup objects
struct FlyingObject {
	glm::vec3 position;
	float speedY;
	float size;
	int type;
	bool collided = false;
};
std::vector<FlyingObject> flyingObjects;

// Struct for Vertex
struct Vertex {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
};

// Struct for Texture
struct Texture {
	unsigned int id = 0;
	std::string type = "";
	std::string path = "";
};

// build and compile shader
Shader* ourShader = nullptr;

std::map<char, Character> Characters;
unsigned int txtVAO, txtVBO;

// settings
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;
int windowWidth = SCR_WIDTH;
int windowHeight = SCR_HEIGHT;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

enum class GameState { MainMenu, Game, PauseMenu, GameOverMenu, GuideMenu };
GameState currentState = GameState::MainMenu;

enum class DifficultyLevel {
	Easy = 1,
	Medium = 2,
	Hard = 3
};
DifficultyLevel currentDifficulty = DifficultyLevel::Easy;

int numberOfCollisions = 0;
int lives = 3;
int numberOfObject = 1;

float pastTime = 0.0f;
float increaseDifficulty = 6.0f;
float pastDifficulty = 0.0f;
int level = 1;
float delay = 2.0f;
float cubeSpeed = 0.8f;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

float randomX = 0;
float randomY = 0;

const float vibrationDuration = 0.9f;
const float vibrationIntensity = 0.2f;
const float powerupDuration = 10.0f;
float powerupStartTime = 0.0f;
int collectedPowerupId = -1;
int activePowerupId = -1;

bool powerupActive = false;
bool showGuide = false;
bool isVibrating = false;

// lighting
glm::vec3 lightPos(0.2f, 0.10f, 0.01f);

float plateVerteces[] = {
	// first triangle
	0.15f, 0.10f, 0.01f,    1.0f, 1.0f,  // top right
	0.15f, -0.10f, 0.01f,   1.0f, 0.0f,  // bottom right
	-0.15f, 0.10f, 0.01f,   0.0f, 1.0f,  // top left

	// second triangle 
	0.15f, -0.10f, 0.01f,   1.0f, 0.0f,  // bottom right
	-0.15f, -0.10f, 0.01f,  0.0f, 0.0f,  // bottom left
	-0.15f, 0.10f, 0.01f,   0.0f, 1.0f   // top left
};
glm::vec3 platePosition = { 0.0f, -1.10f, 0.2f };

// Vertici per la barra verde (3 vite)
float greenBarVertices[] = {
	// Posizioni         // Colori (verde)
	0.0f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,  // Vertice inferiore sinistro
	1.0f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,  // Vertice inferiore destro
	1.0f,  0.1f, 0.0f,  0.0f, 1.0f, 0.0f,  // Vertice superiore destro
	0.0f,  0.1f, 0.0f,  0.0f, 1.0f, 0.0f   // Vertice superiore sinistro
};
// Vertici per la barra gialla (2 vite)
float yellowBarVertices[] = {
	// Posizioni         // Colori (giallo)
	0.0f,  0.0f, 0.0f,  1.0f, 1.0f, 0.0f,  // Vertice inferiore sinistro
	0.66f, 0.0f, 0.0f,  1.0f, 1.0f, 0.0f,  // Vertice inferiore destro
	0.66f, 0.1f, 0.0f,  1.0f, 1.0f, 0.0f,  // Vertice superiore destro
	0.0f,  0.1f, 0.0f,  1.0f, 1.0f, 0.0f   // Vertice superiore sinistro
};
// Vertici per la barra rossa (1 vita)
float redBarVertices[] = {
	// Posizioni         // Colori (rosso)
	0.0f,  0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  // Vertice inferiore sinistro
	0.33f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  // Vertice inferiore destro
	0.33f, 0.1f, 0.0f,  1.0f, 0.0f, 0.0f,  // Vertice superiore destro
	0.0f,  0.1f, 0.0f,  1.0f, 0.0f, 0.0f   // Vertice superiore sinistro
};
unsigned int barIndices[] = {
	0, 1, 2,  // Primo triangolo
	2, 3, 0   // Secondo triangolo
};

float laserVertices[] = {
	// Posizioni         // Texture Coords (opzionale)
	-0.03f,  0.0f, 0.0f,  0.0f, 0.0f,  // Vertice inferiore sinistro
	 0.03f,  0.0f, 0.0f,  1.0f, 0.0f,  // Vertice inferiore destro
	 0.03f,  0.2f, 0.0f,  1.0f, 1.0f,  // Vertice superiore destro
	-0.03f,  0.2f, 0.0f,  0.0f, 1.0f   // Vertice superiore sinistro
};
unsigned int laserIndices[] = {
	0, 1, 2,  // Primo triangolo
	2, 3, 0   // Secondo triangolo
};

// Bad alien structures
glm::mat4 modelAlienStructure = glm::mat4(1.0f);
glm::vec3 alienPosition = { 0.0f, 1.10f, 0.f };

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window, int caller);
void processMenusKeys(GLFWwindow* window, int caller);
glm::vec3 generateRandomPosition(Food food);
AABB createAABB(const glm::vec3& position);
AABB createPlateAABB(const glm::vec3& position);
bool checkCollision(const AABB& a, const AABB& b);
void renderText(Shader& s, std::string text, float x, float y, float scale, glm::vec3 color);
unsigned int TextureFromFile(const char* path, const std::string& directory);
int generateRandomObject();
void renderBoundingBox(float left, float right, float top, float bottom, glm::vec3 color, Shader& shader);
void startGame();
int getRandomNumberX();
int getRandomNumberY();
void createFlyingObject();
AABB createRocketAABB(const glm::vec3& position, float size);
void saveScore(int& collected, int& dropped, float& timePlayed);
bool loadScores(int& collected, int& dropped, float& timePlayed, int& bestCollected, int& bestDropped, float& bestTimePlayed);
void renderGuidePage(Shader& shader, GLFWwindow* window);
void LoadTexture(const char* filename, GLuint& textureID, int textureToCompare);
bool fileExists(const std::string& filename);

// Mesh class
class Mesh {
public:
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;
	unsigned int VAO;
	glm::vec4 diffuseColor;

	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures, glm::vec4 diffuseColor = glm::vec4(1.0f))
		: vertices(vertices), indices(indices), textures(textures), diffuseColor(diffuseColor) {
		setupMesh();
	}

	void Draw(Shader& shader) {
		unsigned int diffuseNr = 1;
		unsigned int specularNr = 1;
		shader.setVec4("diffuseColor", diffuseColor);
		shader.setBool("useTexture", !textures.empty());

		for (unsigned int i = 0; i < textures.size(); i++) {
			glActiveTexture(GL_TEXTURE0 + i);
			std::string number;
			std::string name = textures[i].type;
			if (name == "texture_diffuse")
				number = std::to_string(diffuseNr++);
			else if (name == "texture_specular")
				number = std::to_string(specularNr++);
			shader.setInt(("material." + name + number).c_str(), i);
			glBindTexture(GL_TEXTURE_2D, textures[i].id);
		}
		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

private:
	unsigned int VBO, EBO;

	void setupMesh() {
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

		glBindVertexArray(0);
	}
};

// Model class
class Model {
public:
	Model(const std::string& path) : isLoaded(false) {
		loadModel(path);
	}

	bool IsLoaded() const {
		return isLoaded;
	}

	void Draw(Shader& shader) {
		if (!isLoaded) {
			std::cerr << "ERROR::MODEL:: Model not loaded, cannot draw." << std::endl;
			return;
		}
		for (unsigned int i = 0; i < meshes.size(); i++)
			meshes[i].Draw(shader);
	}

private:
	std::vector<Mesh> meshes;
	std::string directory;
	bool isLoaded;

	void loadModel(const std::string& path) {
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

		// Check if the scene was loaded successfully
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
			isLoaded = false;
			return;
		}

		directory = path.substr(0, path.find_last_of('/'));
		processNode(scene->mRootNode, scene);
		isLoaded = true;
	}

	void processNode(aiNode* node, const aiScene* scene) {
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(processMesh(mesh, scene));
		}
		for (unsigned int i = 0; i < node->mNumChildren; i++) {
			processNode(node->mChildren[i], scene);
		}
	}

	Mesh processMesh(aiMesh* mesh, const aiScene* scene) {
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::vector<Texture> textures;

		// Load vertices
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			Vertex vertex;
			glm::vec3 vector;
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;

			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.Normal = vector;

			if (mesh->mTextureCoords[0]) {
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;
			}
			else {
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);
			}
			vertices.push_back(vertex);
		}

		// Load indices
		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		glm::vec4 diffuseColor(1.0f);

		// Load material
		if (mesh->mMaterialIndex >= 0) {
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

			std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

			std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

			aiColor4D color;
			if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
				diffuseColor = glm::vec4(color.r, color.g, color.b, color.a);
			}
		}

		return Mesh(vertices, indices, textures, diffuseColor);
	}

	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName) const {
		std::vector<Texture> textures;
		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
			aiString str;
			mat->GetTexture(type, i, &str);
			Texture texture;
			texture.id = TextureFromFile(str.C_Str(), directory);
			texture.type = typeName;
			texture.path = str.C_Str();
			textures.push_back(texture);
		}
		return textures;
	}
};

Model LoadModelWithFallback(const std::string& primaryPath, const std::string& secondaryPath);

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Collect it!", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// Capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// Create the shader object based on their paths
	if (fileExists("shaders/shader.vs")) {
		ourShader = new Shader("shaders/shader.vs", "shaders/shader.frag");
		cout << "File trovato\n";
	}
	else {
		ourShader = new Shader("../../OpenGLApp/shader.vs", "../../OpenGLApp/shader.frag");
	}

	Shader shader("", "");
	if (fileExists("shaders/text.vs")) {
		shader = Shader("shaders/text.vs", "shaders/text.frag");
	}
	else {
		shader = Shader("../../OpenGLApp/text.vs", "../../OpenGLApp/text.frag");
	}
	
	// Define objects models
	// -----------------------------
	Model croissantModel = LoadModelWithFallback(
		"objects/croissant.obj",
		"../../OpenGLApp/objects/croissant.obj"
	);
	Model plateModel = LoadModelWithFallback(
		"objects/sgorbio.obj",
		"../../OpenGLApp/objects/sgorbio.obj"
	);
	Model cupModel = LoadModelWithFallback(
		"objects/togocup.obj",
		"../../OpenGLApp/objects/togocup.obj"
	);
	Model gusModel = LoadModelWithFallback(
		"objects/gus2.obj",
		"../../OpenGLApp/objects/gus2.obj"
	);
	Model muffinModel = LoadModelWithFallback(
		"objects/muffin2.obj",
		"../../OpenGLApp/objects/muffin2.obj"
	);
	Model alienModel = LoadModelWithFallback(
		"objects/ufo.obj",
		"../../OpenGLApp/objects/ufo.obj"
	);
	Model laserModel = LoadModelWithFallback(
		"objects/laser.obj",
		"../../OpenGLApp/OpenGLApp/objects/laser.obj"
	);
	Model devilModel = LoadModelWithFallback(
		"objects/devil.obj",
		"../../OpenGLApp/OpenGLApp/objects/devil.obj"
	);
	Model carrotModel = LoadModelWithFallback(
		"objects/carrot.obj",
		"../../OpenGLApp/OpenGLApp/objects/carrot.obj"
	);
	Model wineModel = LoadModelWithFallback(
		"objects/wine.obj",
		"../../OpenGLApp/OpenGLApp/objects/wine.obj"
	);
	Model auraPowerupModel = LoadModelWithFallback(
		"objects/auraPowerup.obj",
		"../../OpenGLApp/OpenGLApp/objects/auraPowerup.obj"
	);
	Model rocketModel = LoadModelWithFallback(
		"objects/rocket.obj",
		"../../OpenGLApp/OpenGLApp/objects/rocket.obj"
	);

	// Compile and setup the shader
	// ----------------------------
	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
	shader.use();
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	shader.use();
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
		return -1;
	}
	std::string font_name = "../../OpenGLApp/resources/fonts/Antonio/static/Antonio-Bold.ttf";
	FT_Face face;
	if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
		cout << "Not fouded first\n";
		std::string font_name = "resources/fonts/Antonio/static/Antonio-Bold.ttf";
		if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
			std::string font_name = "resources/fonts/Antonio/static/Antonio-Bold.ttf";
			std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
			return -1;
		}
		else {
			cout << "Founded Antonio fonts\n";
		}
	}

	if (!soundEngine) {
		std::cerr << "Could not initialize irrKlang sound engine" << std::endl;
		return -1;
	}

	FT_Set_Pixel_Sizes(face, 0, 48);

	if (FT_Load_Char(face, 'X', FT_LOAD_RENDER))
	{
		std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
		return -1;
	}

	// disable byte-alignment restriction
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Load characters
	for (unsigned char c = 0; c < 128; c++)
	{
		// load character glyph 
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
			continue;
		}
		// generate texture
		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);
		// set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// now store character for later use
		Character character = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x
		};
		Characters.insert(std::pair<char, Character>(c, character));
	}

	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// configure VAO/VBO for texture quads
	// -----------------------------------
	glGenVertexArrays(1, &txtVAO);
	glGenBuffers(1, &txtVBO);
	glBindVertexArray(txtVAO);
	glBindBuffer(GL_ARRAY_BUFFER, txtVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	//----------- END text handling

	float conveyorBeltVertices[] = {
		// first triangle
		0.60f, 1.20f, -0.01f,    1.0f, 1.0f,  // top right
		0.60f, -1.20f, -0.01f,   1.0f, 0.0f,  // bottom right
		-0.60f, 1.20f, -0.01f,   0.0f, 1.0f,  // top left

		// second triangle 
		0.60f, -1.20f, -0.01f,   1.0f, 0.0f,  // bottom right
		-0.60f, -1.20f, -0.01f,  0.0f, 0.0f,  // bottom left
		-0.60f, 1.20f, -0.01f,   0.0f, 1.0f   // top left
	};
	glm::vec3 conveyorBeltPositions[] = {
	glm::vec3(0.0f, 0.0f, 0.0f),
	glm::vec3(0.0f, 2.4f, 0.0f)
	};

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float objectsVertices[] = {
		// first (upper) triangle
		0.10f, 0.10f, 0.0f,    1.0f, 1.0f,  // top right
		0.10f, -0.10f, 0.0f,   1.0f, 0.0f,  // bottom right
		-0.10f, 0.10f, 0.0f,   0.0f, 1.0f,  // top left

		// second triangle 
		0.10f, -0.10f, 0.0f,   1.0f, 0.0f,  // bottom right
		-0.10f, -0.10f, 0.0f,  0.0f, 0.0f,  // bottom left
		-0.10f, 0.10f, 0.0f,   0.0f, 1.0f   // top left
	};
	// world space positions of the objects

	std::vector<glm::vec3> objectsPositions;
	Food food;
	food.type = -1; // required inizialization
	objectsPositions.push_back(generateRandomPosition(food));

	// Declare VAO and VBO for the conveyor belt
	unsigned int conveyorVAO, conveyorVBO;
	glGenVertexArrays(1, &conveyorVAO);
	glGenBuffers(1, &conveyorVBO);

	// Bind and set the conveyor belt vertices
	glBindVertexArray(conveyorVAO);

	glBindBuffer(GL_ARRAY_BUFFER, conveyorVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(conveyorBeltVertices), conveyorBeltVertices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Texture coordinate attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);


	// PLATE
	// --------------------------------------------------------------------------------------------------
	// Declare VAO and VBO for the conveyor belt
	unsigned int plateVAO, plateVBO;
	glGenVertexArrays(1, &plateVAO);
	glGenBuffers(1, &plateVBO);

	glBindVertexArray(plateVAO);

	glBindBuffer(GL_ARRAY_BUFFER, plateVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(plateVerteces), plateVerteces, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Texture coordinate attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);


	// CUBES
	// --------------------------------------------------------------------------------------------------
	unsigned int VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(objectsVertices), objectsVertices, GL_STATIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);


	// LASER
	// --------------------------------------------------------------------------------------------------
	unsigned int laserVAO, laserVBO, laserEBO;
	glGenVertexArrays(1, &laserVAO);
	glGenBuffers(1, &laserVBO);
	glGenBuffers(1, &laserEBO);

	glBindVertexArray(laserVAO);

	glBindBuffer(GL_ARRAY_BUFFER, laserVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(laserVertices), laserVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, laserEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(laserIndices), laserIndices, GL_STATIC_DRAW);

	// Posizioni
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Coordinate di texture (opzionale)
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// LIFE BAR
	// --------------------------------------------------------------------------------------------------
	unsigned int greenBarVAO, greenBarVBO;
	unsigned int yellowBarVAO, yellowBarVBO;
	unsigned int redBarVAO, redBarVBO;
	unsigned int barEBO;

	// Configura la barra verde
	glGenVertexArrays(1, &greenBarVAO);
	glGenBuffers(1, &greenBarVBO);
	glGenBuffers(1, &barEBO);

	glBindVertexArray(greenBarVAO);
	glBindBuffer(GL_ARRAY_BUFFER, greenBarVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(greenBarVertices), greenBarVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, barEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(barIndices), barIndices, GL_STATIC_DRAW);

	// Posizioni
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Colori
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Configura la barra gialla (stesso EBO)
	glGenVertexArrays(1, &yellowBarVAO);
	glGenBuffers(1, &yellowBarVBO);

	glBindVertexArray(yellowBarVAO);
	glBindBuffer(GL_ARRAY_BUFFER, yellowBarVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(yellowBarVertices), yellowBarVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, barEBO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Configura la barra rossa (stesso EBO)
	glGenVertexArrays(1, &redBarVAO);
	glGenBuffers(1, &redBarVBO);

	glBindVertexArray(redBarVAO);
	glBindBuffer(GL_ARRAY_BUFFER, redBarVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(redBarVertices), redBarVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, barEBO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	// Load and create textures
	// -------------------------
	GLuint texture0, texture1, texture2, texture3, texture4, texture5, texture6, texture7, texture8, texture9, texture10;

	LoadTexture("resources/container.jpg", texture0, 0);
	LoadTexture("resources/carrot.jpg", texture1, 0);
	LoadTexture("resources/cb4.jpg", texture2, 0);
	LoadTexture("resources/croissant.jpg", texture3, 0);
	LoadTexture("resources/cup.jpg", texture4, 0);
	LoadTexture("resources/gus.jpg", texture5, 0);
	LoadTexture("resources/muffin.jpg", texture6, 1);
	LoadTexture("resources/alien.jpg", texture7, 0);
	LoadTexture("resources/wine.jpg", texture8, 0);
	LoadTexture("resources/ufo.jpg", texture9, 0);
	LoadTexture("resources/rocket.jpg", texture10, 0);

	// Textures for the objects
	// -------------------------------------------------------------------------------------------
	ourShader->use();
	ourShader->setInt("texture0", 0);
	ourShader->setInt("texture1", 1);
	ourShader->setInt("texture2", 2);
	ourShader->setInt("texture3", 3);
	ourShader->setInt("texture4", 4);
	ourShader->setInt("texture5", 5);
	ourShader->setInt("texture6", 6);
	ourShader->setInt("texture7", 7);
	ourShader->setInt("texture8", 8);
	ourShader->setInt("texture9", 9);
	ourShader->setInt("texture10", 10);

	// Lightning definitions
	Shader lightingShader("", "");
	if (fileExists("shader/shader_light.vs")) {
		lightingShader = Shader("shaders/shader_light.vs", "shaders/shader_light.frag");
	}
	else {
		lightingShader = Shader("../../OpenGLApp/shader_light.vs", "../../OpenGLApp/shader_light.frag");
	}

	float lightVertices[] = {
	-0.5f, -0.5f, -0.5f,  // Front-bottom-left
	 0.5f, -0.5f, -0.5f,  // Front-bottom-right
	 0.5f,  0.5f, -0.5f,  // Front-top-right
	 0.5f,  0.5f, -0.5f,  // Front-top-right
	-0.5f,  0.5f, -0.5f,  // Front-top-left
	-0.5f, -0.5f, -0.5f,  // Front-bottom-left

	-0.5f, -0.5f,  0.5f,  // Back-bottom-left
	 0.5f, -0.5f,  0.5f,  // Back-bottom-right
	 0.5f,  0.5f,  0.5f,  // Back-top-right
	 0.5f,  0.5f,  0.5f,  // Back-top-right
	-0.5f,  0.5f,  0.5f,  // Back-top-left
	-0.5f, -0.5f,  0.5f,  // Back-bottom-left

	-0.5f,  0.5f,  0.5f,  // Top-back-left
	-0.5f,  0.5f, -0.5f,  // Top-front-left
	 0.5f,  0.5f, -0.5f,  // Top-front-right
	 0.5f,  0.5f, -0.5f,  // Top-front-right
	 0.5f,  0.5f,  0.5f,  // Top-back-right
	-0.5f,  0.5f,  0.5f,  // Top-back-left

	-0.5f, -0.5f, -0.5f,  // Bottom-front-left
	-0.5f, -0.5f,  0.5f,  // Bottom-back-left
	 0.5f, -0.5f,  0.5f,  // Bottom-back-right
	 0.5f, -0.5f,  0.5f,  // Bottom-back-right
	 0.5f, -0.5f, -0.5f,  // Bottom-front-right
	-0.5f, -0.5f, -0.5f   // Bottom-front-left
	};

	unsigned int lightVAO, lightVBO;

	// Create the VAO and VBO
	glGenVertexArrays(1, &lightVAO);
	glGenBuffers(1, &lightVBO);

	// Bind the VAO
	glBindVertexArray(lightVAO);

	// Bind the VBO and load data
	glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(lightVertices), lightVertices, GL_STATIC_DRAW);

	// Define the vertex position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Unbind the VAO and VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Set light properties
	lightingShader.use();
	lightingShader.setVec3("light.position", lightPos);
	lightingShader.setVec3("light.color", glm::vec3(1.0f, 1.0f, 1.0f));
	lightingShader.setVec3("viewPos", camera.Position);

	// Model transformation matrix
	glm::mat4 lightModel = glm::mat4(1.0f);
	lightingShader.setMat4("model", lightModel);
	lightingShader.setMat4("view", camera.GetViewMatrix());
	lightingShader.setMat4("projection", projection);
	// -----------------------------
	// END lightning definitions

	float quitLeft = (SCR_WIDTH / 2.0f - 50) + 15;
	float quitRight = quitLeft + 65;
	float quitTop = (SCR_HEIGHT / 2.0f - 100 - 30) - 20;
	float quitBottom = SCR_HEIGHT / 2.0f - 100;

	std::string collisionMessage = "Object collected: " + std::to_string(numberOfCollisions);
	std::string objectMessage = "Object dropped: " + std::to_string(numberOfObject);
	std::string livesCounter = "Lives: " + std::to_string(lives);
	std::string powerupMessage = "Powerup: " + std::to_string(collectedPowerupId); // Ultimo powerup raccolto

	// Load scores
	int lastCollected = 0, lastDropped = 0; float lastTimePlayed = 0.0f; DifficultyLevel usedDifficulty = DifficultyLevel::Easy;
	int bestCollected = 0, bestDropped = 0; float bestTimePlayed = 0.0f; DifficultyLevel bestUsedDifficulty = DifficultyLevel::Easy;

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		float vibrationTimer = 0.0f;

		const float conveyorSpeed = 0.2f * deltaTime;

		switch (currentState) {
		case GameState::MainMenu: {
			// Timer to track when the pause menu was entered
			static float pauseMenuEnterTime = 0.0f;
			static bool firstEnter = true;
			static bool escKeyProcessed = false;
			if (firstEnter) {
				pauseMenuEnterTime = static_cast<float>(glfwGetTime());
				firstEnter = false;
				escKeyProcessed = false;
			}

			processInput(window, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

			if (showGuide) {
				currentState = GameState::GuideMenu;
			}

			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);

			// Convert mouse coordinates
			float mouseX = xpos * (static_cast<float>(SCR_WIDTH) / windowWidth);
			float mouseY = (windowHeight - ypos) * (static_cast<float>(SCR_HEIGHT) / windowHeight);

			// Define bounding box
			float easyLeft = SCR_WIDTH / 2.0f - 180;
			float easyRight = easyLeft + 50;
			float easyTop = (SCR_HEIGHT / 2.0f) + 30;
			float easyBottom = (SCR_HEIGHT / 2.0f);

			float mediumLeft = (SCR_WIDTH / 2.0f - 60);
			float mediumRight = mediumLeft + 80;
			float mediumTop = (SCR_HEIGHT / 2.0f) + 30;
			float mediumBottom = (SCR_HEIGHT / 2.0f);

			float hardLeft = (SCR_WIDTH / 2.0f + 80);
			float hardRight = hardLeft + 60;
			float hardTop = (SCR_HEIGHT / 2.0f) + 30;
			float hardBottom = (SCR_HEIGHT / 2.0f);

			float startLeft = (SCR_WIDTH / 2.0f - 60);
			float startRight = startLeft + 163;
			float startTop = (SCR_HEIGHT / 2.0f - 60 - 30) + 10;
			float startBottom = (SCR_HEIGHT / 2.0f - 60) + 15;

			float guideLeft = (SCR_WIDTH / 2.0f - 60) - 320;
			float guideRight = guideLeft + 165;
			float guideTop = (SCR_HEIGHT / 2.0f - 60 - 30) - 180;
			float guideBottom = (SCR_HEIGHT / 2.0f - 60) - 175;

			quitLeft = (SCR_WIDTH / 2.0f - 50) + 15 - 20;
			quitRight = quitLeft + 62;
			quitTop = (SCR_HEIGHT / 2.0f - 100 - 30) - 5;
			quitBottom = SCR_HEIGHT / 2.0f - 100;

			loadScores(lastCollected, lastDropped, lastTimePlayed, bestCollected, bestDropped, bestTimePlayed);

			std::string lastRunCollected = "Ultima run - Raccolti: " + std::to_string(lastCollected);
			std::string lastRunDropped = "Oggetti caduti: " + std::to_string(lastDropped);
			std::string lastRunTime = "Tempo giocato: " + std::to_string(static_cast<int>(std::round(lastTimePlayed)));

			std::string bestRunCollected = "Miglior run - Raccolti: " + std::to_string(bestCollected);
			std::string bestRunDropped = "Oggetti caduti: " + std::to_string(bestDropped);
			std::string bestRunTime = "Tempo giocato: " + std::to_string(static_cast<int>(std::round(bestTimePlayed)));


			// Print on screen
			float lineSpacing = 30.0f;
			renderText(shader, lastRunCollected, SCR_WIDTH / 2.0f - 370, SCR_HEIGHT / 2.0f + 250, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));
			renderText(shader, lastRunDropped, SCR_WIDTH / 2.0f - 370, SCR_HEIGHT / 2.0f + 250 - lineSpacing, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));
			renderText(shader, lastRunTime, SCR_WIDTH / 2.0f - 370, SCR_HEIGHT / 2.0f + 250 - 2 * lineSpacing, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));

			renderText(shader, bestRunCollected, SCR_WIDTH / 2.0f + 150, SCR_HEIGHT / 2.0f + 370 - 4 * lineSpacing, 0.5f, glm::vec3(1.0f, 1.0f, 0.0f));
			renderText(shader, bestRunDropped, SCR_WIDTH / 2.0f + 150, SCR_HEIGHT / 2.0f + 370 - 5 * lineSpacing, 0.5f, glm::vec3(1.0f, 1.0f, 0.0f));
			renderText(shader, bestRunTime, SCR_WIDTH / 2.0f + 150, SCR_HEIGHT / 2.0f + 370 - 6 * lineSpacing, 0.5f, glm::vec3(1.0f, 1.0f, 0.0f));

			renderText(shader, "Welcome!", SCR_WIDTH / 2.0f - 100, SCR_HEIGHT / 2.0f + 130, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
			renderText(shader, "Choose difficulty:", SCR_WIDTH / 2.0f - 130, SCR_HEIGHT / 2.0f + 50, 0.7f, glm::vec3(1.0f, 1.0f, 1.0f));

			float textOffsetY = -20.0f;
			renderText(shader, "Easy", easyLeft, easyTop + textOffsetY, 0.6f, glm::vec3(1.0f, 1.0f, 1.0f));
			renderText(shader, "Medium", mediumLeft, mediumTop + textOffsetY, 0.6f, glm::vec3(1.0f, 1.0f, 1.0f));
			renderText(shader, "Hard", hardLeft, hardTop + textOffsetY, 0.6f, glm::vec3(1.0f, 1.0f, 1.0f));

			renderText(shader, "Start Game", startLeft, startTop, 0.8f, glm::vec3(0.0f, 1.0f, 0.0f));
			renderText(shader, "Quit", quitLeft, quitTop, 0.8f, glm::vec3(1.0f, 0.0f, 0.0f));
			renderText(shader, "Guide Page", guideLeft, guideTop, 0.8f, glm::vec3(1.0f, 1.0f, 0.0f));

			//renderBoundingBox(startLeft, startRight, startTop, startBottom, glm::vec3(0.0f, 1.0f, 0.0f), *ourShader);
			//renderBoundingBox(quitLeft, quitRight, quitTop, quitBottom, glm::vec3(0.0f, 1.0f, 0.0f), *ourShader);
			//renderBoundingBox(guideLeft, guideRight, guideTop, guideBottom, glm::vec3(0.0f, 1.0f, 0.0f), *ourShader);

			//renderBoundingBox(easyLeft, easyRight, easyTop, easyBottom, glm::vec3(0.0f, 1.0f, 0.0f), *ourShader);
			//renderBoundingBox(mediumLeft, mediumRight, mediumTop, mediumBottom, glm::vec3(0.0f, 1.0f, 0.0f), *ourShader);
			//renderBoundingBox(hardLeft, hardRight, hardTop, hardBottom, glm::vec3(0.0f, 1.0f, 0.0f), *ourShader);

			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
				if (mouseX >= startLeft && mouseX <= startRight && mouseY >= startTop && mouseY <= startBottom) {
					startGame();
					//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
					pastTime = static_cast<float>(glfwGetTime());
					currentState = GameState::Game;
				}
				if (mouseX >= quitLeft && mouseX <= quitRight && mouseY >= quitTop && mouseY <= quitBottom) {
					glfwSetWindowShouldClose(window, true);
				}

				if (mouseX >= easyLeft && mouseX <= easyRight && mouseY <= easyTop && mouseY >= easyBottom) {
					std::cout << "current diff " << static_cast<int>(currentDifficulty) << std::endl;
					currentDifficulty = DifficultyLevel::Easy;
				}
				if (mouseX >= mediumLeft && mouseX <= mediumRight && mouseY <= mediumTop && mouseY >= mediumBottom) {
					std::cout << "current diff " << static_cast<int>(currentDifficulty) << std::endl;
					currentDifficulty = DifficultyLevel::Medium;
				}
				if (mouseX >= hardLeft && mouseX <= hardRight && mouseY <= hardTop && mouseY >= hardBottom) {
					std::cout << "current diff " << static_cast<int>(currentDifficulty) << std::endl;
					currentDifficulty = DifficultyLevel::Hard;
				}
				if (mouseX >= guideLeft && mouseX <= guideRight && mouseY >= guideTop && mouseY <= guideBottom) {
					std::cout << "premuto/n";
					currentState = GameState::GuideMenu;
				}
			}

			// Block ESC input for the first 0.5 seconds to prevent flickering
			float currentTime = glfwGetTime();
			ourShader->setFloat("time", currentTime);

			if (currentTime - pauseMenuEnterTime > 0.5f) {
				if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
					if (!escKeyProcessed) {
						//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
						pastTime = static_cast<float>(glfwGetTime());
						currentState = GameState::MainMenu;
						firstEnter = true;
						escKeyProcessed = true;
					}
				}
				else {
					escKeyProcessed = false;
				}
			}
			break;
		}

		case GameState::Game: {
			if (lives > 0) {
				// Timer to track when the pause menu was entered
				static float pauseMenuEnterTime = 0.0f;
				static bool firstEnter = true;
				static bool escKeyProcessed = false;

				float alienLeft = 0;
				float alienRight = 0;
				float alienTop = 0;
				float alienBottom = 0;
				float width = 30.0f;
				float height = 30.0f;

				if (firstEnter) {
					pauseMenuEnterTime = static_cast<float>(glfwGetTime());
					firstEnter = false;
					escKeyProcessed = false;
				}

				processInput(window, 1);

				// render
				// ------
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

				double xpos, ypos;
				glfwGetCursorPos(window, &xpos, &ypos);

				// Convert mouse coordinates
				float mouseX = xpos * (static_cast<float>(SCR_WIDTH) / windowWidth);
				float mouseY = (windowHeight - ypos) * (static_cast<float>(SCR_HEIGHT) / windowHeight);

				// Handle continuous cube appearance
				float currentTime = static_cast<float>(glfwGetTime());

				if (currentTime >= pastDifficulty + increaseDifficulty) {
					if (level > 1) cubeSpeed += 0.003f / level;
					delay -= 0.5f / level;
					pastDifficulty = currentTime;
					level++;
				}

				if (currentTime >= pastTime + delay) {
					// keep adding cubes
					Food food;
					food.type = -1; // required inizialization
					food.type = generateRandomObject();
					food.position = generateRandomPosition(food);
					foods.push_back(food);

					std::cout << "Spawned at " << currentTime << " with speed " << cubeSpeed << " with delay " << delay << std::endl;
					cout << "Game time: " << pastTime << "/n";
					if (food.type == 5) {
						randomY = getRandomNumberY();
						randomX = getRandomNumberX();
					}
					if (food.type != 4) numberOfObject++;
					pastTime = currentTime;

					objectMessage = "Object dropped: " + std::to_string(numberOfObject);
				}
				
				if (powerupActive && activePowerupId == 0) {
					float currentTime = static_cast<float>(glfwGetTime());
					float powerupElapsedTime = currentTime - powerupStartTime;

					if (powerupElapsedTime >= powerupDuration) {
						collectedPowerupId = -1;
						activePowerupId = -1;
						powerupActive = false;
						std::cout << "Power-up scaduto!" << std::endl;
						powerupMessage = "None";
					}
				}
				else if (powerupActive && activePowerupId == 1) {
					float currentTime = static_cast<float>(glfwGetTime());
					float powerupElapsedTime = currentTime - powerupStartTime;

					if (powerupElapsedTime >= powerupDuration) {
						collectedPowerupId = -1;
						activePowerupId = -1;
						powerupActive = false;
						std::cout << "Power-up scaduto!" << std::endl;
						powerupMessage = "None";
					}
				}

				// RENDER OBJECTS
				ourShader->use();
				for (unsigned int i = 0; i < foods.size(); i++) {
					foods[i].position.z = 0.2f;
					if (foods[i].position.y <= -1.10f) {
						continue;
					}

					// Update position
					foods[i].position.y -= cubeSpeed * deltaTime;

					// Create AABB for the current object after position update and plate
					AABB objectAABB = createAABB(foods[i].position);
					AABB plateAABB = createPlateAABB(platePosition);

					// Check for collision
					if (checkCollision(objectAABB, plateAABB)) {
						if (foods[i].type == 4 || foods[i].type == 5) {
							if (activePowerupId == 0) {
								foods[i].position.y = -10.0f;

								livesCounter = "Lives: " + std::to_string(lives);
							}
							else {
								lives--;
								foods[i].position.y = -10.0f;
								livesCounter = "Lives: " + std::to_string(lives);
								if (fileExists("../../OpenGLApp/sounds/laser2.wav")) {
									soundEngine->play2D("../../OpenGLApp/sounds/laser2.wav", false);
								}
								else if (fileExists("sounds/laser2.wav")) {
									soundEngine->play2D("sounds/laser2.wav", false);
								}
								isVibrating = true;
								vibrationTimer = glfwGetTime();
							}
						}
						else {
							if (foods[i].type == 6) {
								collectedPowerupId = 0;
								powerupMessage = "Carrot!";
							}
							else if (foods[i].type == 7) {
								collectedPowerupId = 1;
								powerupMessage = "Wine!";
							}
							numberOfCollisions++;
							foods[i].position.y = -10.0f;

							collisionMessage = "Object collected: " + std::to_string(numberOfCollisions);
							if (fileExists("../../OpenGLApp/sounds/pickup_sound.wav")) {
								soundEngine->play2D("../../OpenGLApp/sounds/pickup_sound.wav", false);
							}
							else if (fileExists("sounds/pickup_sound.wav")) {
								soundEngine->play2D("sounds/pickup_sound.wav", false);
							}
						}
					}

					// Render cube
					glm::mat4 objModel = glm::mat4(1.0f);
					if (foods[i].type == 0) {
						float angle = glfwGetTime();
						objModel = glm::translate(objModel, foods[i].position);
						objModel = glm::scale(objModel, glm::vec3(0.2f, 0.2f, 0.2f));
						objModel = glm::rotate(objModel, angle, glm::vec3(0.0f, 1.0f, 0.0f));

						ourShader->setInt("textureID", 3);
						ourShader->setMat4("model", objModel);
						croissantModel.Draw(*ourShader);
					}
					else if (foods[i].type == 1) {
						float angle = glfwGetTime();
						objModel = glm::translate(objModel, foods[i].position);
						objModel = glm::scale(objModel, glm::vec3(0.045f, 0.045f, 0.045f));
						objModel = glm::rotate(objModel, angle, glm::vec3(0.0f, 1.0f, 0.0f));

						ourShader->setInt("textureID", 4);
						ourShader->setMat4("model", objModel);
						cupModel.Draw(*ourShader);
					}
					else if (foods[i].type == 2) {
						float angle = glfwGetTime();
						objModel = glm::translate(objModel, foods[i].position);
						objModel = glm::scale(objModel, glm::vec3(0.03f, 0.03f, 0.03f));
						objModel = glm::rotate(objModel, angle, glm::vec3(0.0f, 1.0f, 0.0f));

						ourShader->setInt("textureID", 5);
						ourShader->setMat4("model", objModel);
						gusModel.Draw(*ourShader);
					}
					else if (foods[i].type == 3) {
						float angle = glfwGetTime();
						objModel = glm::translate(objModel, foods[i].position);
						objModel = glm::scale(objModel, glm::vec3(0.04f, 0.04f, 0.04f));
						objModel = glm::rotate(objModel, angle, glm::vec3(0.0f, 1.0f, 0.0f));

						ourShader->setInt("textureID", 6);
						ourShader->setMat4("model", objModel);
						muffinModel.Draw(*ourShader);
					}
					else if (foods[i].type == 4) {
						alienPosition.x = foods[i].position.x;
						ourShader->setBool("isLaser", true);
						ourShader->setVec3("laserColor", glm::vec3(1.0f, 0.0f, 0.0f)); // Red

						// Render il laser
						glm::mat4 laserModelStructure = glm::mat4(1.0f);
						laserModelStructure = glm::translate(laserModelStructure, foods[i].position);

						ourShader->setMat4("model", laserModelStructure);
						glBindVertexArray(laserVAO);
						glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
						glBindVertexArray(0);

						ourShader->setBool("isLaser", false);
					}
					else if (foods[i].type == 5) {
						float angle = glfwGetTime();
						glm::mat4 devilModelStructure = glm::mat4(1.0f);
						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, texture0);
						glBindVertexArray(plateVAO);
						devilModelStructure = glm::translate(devilModelStructure, foods[i].position);
						devilModelStructure = glm::scale(devilModelStructure, glm::vec3(0.08f, 0.08f, 0.08f));

						ourShader->setMat4("model", devilModelStructure);
						ourShader->setInt("textureID", 7);
						devilModel.Draw(*ourShader);
						glDrawArrays(GL_TRIANGLES, 0, 6);

						// Update bounding box coordinates based on object position
						alienLeft = (foods[i].position.x - width) + randomX;
						alienRight = (foods[i].position.x + width) + randomX;
						alienTop = (foods[i].position.y + height) + randomY;
						alienBottom = (foods[i].position.y - height) + randomY;
						renderBoundingBox(alienLeft, alienRight, alienTop, alienBottom, glm::vec3(1.0f, 0.0f, 0.0f), *ourShader);

						if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
							if (mouseX >= alienLeft && mouseX <= alienRight && mouseY <= alienTop && mouseY >= alienBottom)
							{
								cout << "Preso!" << "/n";
								numberOfCollisions++;
								foods[i].position.y = -10.0f;
								collisionMessage = "Object collected: " + std::to_string(numberOfCollisions);

								if (fileExists("../../OpenGLApp/sounds/pickup_sound.wav")) {
									soundEngine->play2D("../../OpenGLApp/sounds/pickup_sound.wav", false);
								}
								else if (fileExists("sounds/pickup_sound.wav")) {
									soundEngine->play2D("sounds/pickup_sound.wav", false);
								}
							}
						}
					}
					else if (foods[i].type == 6) {
						float angle = glfwGetTime();
						objModel = glm::translate(objModel, foods[i].position);
						objModel = glm::scale(objModel, glm::vec3(0.1f, 0.1f, 0.1f));
						objModel = glm::rotate(objModel, angle, glm::vec3(0.0f, 1.0f, 0.0f));

						ourShader->setInt("textureID", 1);
						ourShader->setMat4("model", objModel);
						carrotModel.Draw(*ourShader);
					}
					else if (foods[i].type == 7) {
						float angle = glfwGetTime();
						objModel = glm::translate(objModel, foods[i].position);
						objModel = glm::scale(objModel, glm::vec3(0.04f, 0.04f, 0.04f));
						objModel = glm::rotate(objModel, angle, glm::vec3(0.0f, 1.0f, 0.0f));

						ourShader->setInt("textureID", 8);
						ourShader->setMat4("model", objModel);
						wineModel.Draw(*ourShader);
					}
				}

				for (unsigned int i = 0; i < flyingObjects.size(); i++) {
					// Update rocket position
					flyingObjects[i].position.y += flyingObjects[i].speedY * deltaTime;
					flyingObjects[i].position.z = 0.2f;

					// Create rocket AABB
					AABB rocketAABB = createRocketAABB(flyingObjects[i].position, flyingObjects[i].size);

					for (unsigned int j = 0; j < foods.size(); j++) {
						if (foods[j].type == 4) { // Se è il laser

							// Crea laser AABB
							AABB laserAABB = createAABB(foods[j].position);

							if (checkCollision(rocketAABB, laserAABB) && !flyingObjects[i].collided) {
								std::cout << "Collisione tra razzo e laser!" << std::endl;

								foods[j].type = -1; // Deactivate il laser
								flyingObjects[i].collided = true;
								flyingObjects[i].position = glm::vec3{ -10, -10, -10 };
								break;
							}
						}
					}

					// Render rocket
					if (!flyingObjects[i].collided) {
						glm::mat4 objModel = glm::mat4(1.0f);
						float angle = glfwGetTime();

						objModel = glm::translate(objModel, flyingObjects[i].position);
						objModel = glm::scale(objModel, glm::vec3(0.045f, 0.045f, 0.045f));
						objModel = glm::rotate(objModel, angle, glm::vec3(0.0f, 1.0f, 0.0f));

						ourShader->setInt("textureID", 10);
						ourShader->setMat4("model", objModel);
						rocketModel.Draw(*ourShader);
					}
				}

				ourShader->use();
				// Bind textures
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, texture0);
				ourShader->setInt("texture0", 0);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, texture1);
				ourShader->setInt("texture1", 1);

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, texture2);
				ourShader->setInt("texture2", 2);

				glActiveTexture(GL_TEXTURE3);
				glBindTexture(GL_TEXTURE_2D, texture3);
				ourShader->setInt("texture3", 3);

				glActiveTexture(GL_TEXTURE4);
				glBindTexture(GL_TEXTURE_2D, texture4);
				ourShader->setInt("texture4", 4);
				
				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_2D, texture5);
				ourShader->setInt("texture5", 5);

				glActiveTexture(GL_TEXTURE6);
				glBindTexture(GL_TEXTURE_2D, texture6);
				ourShader->setInt("texture6", 6);

				glActiveTexture(GL_TEXTURE7);
				glBindTexture(GL_TEXTURE_2D, texture7);
				ourShader->setInt("texture7", 7);

				glActiveTexture(GL_TEXTURE8);
				glBindTexture(GL_TEXTURE_2D, texture8);
				ourShader->setInt("texture8", 8);

				glActiveTexture(GL_TEXTURE9);
				glBindTexture(GL_TEXTURE_2D, texture9);
				ourShader->setInt("texture9", 9);

				glActiveTexture(GL_TEXTURE10);
				glBindTexture(GL_TEXTURE_2D, texture10);
				ourShader->setInt("texture10", 10);


				// Set projection and view matrices
				glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
				ourShader->setMat4("projection", projection);
				glm::mat4 view = camera.GetViewMatrix();
				ourShader->setMat4("view", view);

				// Render conveyor belt
				ourShader->use();
				glBindVertexArray(conveyorVAO);
				glm::mat4 model = glm::mat4(1.0f);
				for (int i = 0; i < 2; i++) {
					conveyorBeltPositions[i].y -= conveyorSpeed;
					if (conveyorBeltPositions[i].y <= -2.4f) {
						// Reset position when off-screen
						conveyorBeltPositions[i].y = 2.4f;
					}

					// Render the conveyor belt
					glm::mat4 conveyorModel = glm::mat4(1.0f);
					conveyorModel = glm::translate(conveyorModel, conveyorBeltPositions[i]);
					ourShader->setMat4("model", conveyorModel);
					ourShader->setInt("textureID", 2);
					glBindVertexArray(conveyorVAO);
					glDrawArrays(GL_TRIANGLES, 0, 6);
				}

				// light properties
				glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

				glm::vec3 diffuseColor = lightColor * glm::vec3(1.0f);
				glm::vec3 ambientColor = diffuseColor * glm::vec3(1.0f);

				ourShader->setVec3("light.color", lightColor);
				ourShader->setVec3("light.ambient", ambientColor);
				ourShader->setVec3("light.diffuse", diffuseColor);
				ourShader->setVec3("light.specular", glm::vec3(1.0f, 1.0f, 1.0f));

				// material properties
				ourShader->setVec3("material.ambient", 1.0f, 1.0f, 1.0f);
				ourShader->setVec3("material.diffuse", 1.0f, 1.0f, 1.0f);
				ourShader->setVec3("material.specular", 0.0f, 0.0f, 0.0f);
				ourShader->setFloat("material.shininess", 2.0f);

				// Render plate
				if (isVibrating) {
					float elapsedTime = glfwGetTime() - vibrationTimer;
					float offsetVib = sin(elapsedTime * 50.0f) * vibrationIntensity;

					if (elapsedTime < vibrationDuration) {
						platePosition.x += offsetVib;
					}
					else {
						platePosition.x += offsetVib / 2;
						isVibrating = false;
					}
				}
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, texture0);
				glBindVertexArray(plateVAO);
				model = glm::translate(glm::mat4(1.0f), platePosition);
				model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
				ourShader->setMat4("model", model);
				ourShader->setInt("textureID", 0);
				plateModel.Draw(*ourShader);

				if (powerupActive == true) {
					glm::mat4 objModel = glm::mat4(1.0f);
					float angle = 80.0f;

					objModel = glm::translate(objModel, platePosition);
					objModel = glm::scale(objModel, glm::vec3(0.13f, 0.13f, 0.13f));
					objModel = glm::rotate(objModel, angle, glm::vec3(0.0f, 1.0f, 0.0f));

					ourShader->setInt("textureID", -1);
					ourShader->setMat4("model", objModel);
					auraPowerupModel.Draw(*ourShader);
				}
				glDrawArrays(GL_TRIANGLES, 0, 6);

				// Render alien
				float angle = glfwGetTime();
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, texture0);
				glBindVertexArray(plateVAO);
				modelAlienStructure = glm::translate(glm::mat4(1.0f), alienPosition);
				modelAlienStructure = glm::scale(modelAlienStructure, glm::vec3(0.15f, 0.15f, 0.15f));
				modelAlienStructure = glm::rotate(modelAlienStructure, angle, glm::vec3(0.0f, 1.0f, 0.0f));
				ourShader->setMat4("model", modelAlienStructure);
				ourShader->setInt("textureID", 9);
				alienModel.Draw(*ourShader);
				glDrawArrays(GL_TRIANGLES, 0, 6);

				// Render text
				glUseProgram(shader.ID); 
				renderText(shader, objectMessage, 10.0f, 550.0f, 0.6f, glm::vec3(1.0f, 1.0f, 1.0f));
				renderText(shader, collisionMessage, 10.0f, 480.0f, 0.6f, glm::vec3(1.0f, 1.0f, 1.0f));
				renderText(shader, livesCounter, 10.0f, 410.0f, 0.6f, glm::vec3(1.0f, 1.0f, 1.0f));
				renderText(shader, powerupMessage, 10.0f, 340.0f, 0.6f, glm::vec3(1.0f, 1.0f, 1.0f));

				// Restore OpenGL state for 3D rendering
				glBindVertexArray(0);
				glBindTexture(GL_TEXTURE_2D, 0);
				glDisable(GL_BLEND);

				// Block ESC input for the first 0.5 seconds to prevent flickering
				float currentEscTime = static_cast<float>(glfwGetTime());
				if (currentEscTime - pauseMenuEnterTime > 0.5f) {
					if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
						if (!escKeyProcessed) {
							pastTime = static_cast<float>(glfwGetTime());
							currentState = GameState::PauseMenu;
							firstEnter = true;
							escKeyProcessed = true;
						}
					}
					else {
						escKeyProcessed = false;
					}
				}

			}
			else currentState = GameState::GameOverMenu;
			break;
		}

		case GameState::GameOverMenu: {
			processInput(window, 2);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);

			// Convert mouse coordinates
			float mouseX = xpos * (static_cast<float>(SCR_WIDTH) / windowWidth);
			float mouseY = (windowHeight - ypos) * (static_cast<float>(SCR_HEIGHT) / windowHeight);

			// Define bounding box
			float restartLeft = SCR_WIDTH / 2.0f - 60;
			float restartRight = restartLeft + 110;
			float restartTop = SCR_HEIGHT / 2.0f - 60 - 30;
			float restartBottom = SCR_HEIGHT / 2.0f - 60;

			// Render "Game Over" screen
			int totalObjDropped = numberOfObject - 2;
			std::string correctObjDropped = "Total object dropped: " + std::to_string(totalObjDropped);
			renderText(shader, "Game Over", SCR_WIDTH / 2.0f - 100, SCR_HEIGHT / 2.0f + 100, 1.0f, glm::vec3(1.0f, 0.0f, 0.0f));
			renderText(shader, correctObjDropped, (SCR_WIDTH / 2.0f - 150) + 10, SCR_HEIGHT / 2.0f - 10, 0.8f, glm::vec3(1.0f, 1.0f, 1.0f));
			renderText(shader, collisionMessage, (SCR_WIDTH / 2.0f - 150) + 10, SCR_HEIGHT / 2.0f + 40, 0.8f, glm::vec3(1.0f, 1.0f, 1.0f));
			renderText(shader, "Restart", restartLeft, restartTop, 0.8f, glm::vec3(0.0f, 1.0f, 0.0f));
			renderText(shader, "Quit", quitLeft, quitTop, 0.8f, glm::vec3(1.0f, 0.0f, 0.0f));
			
			saveScore(numberOfCollisions, totalObjDropped, pastTime);

			// Handle mouse input for "Restart" and "Quit"
			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
				if (mouseX >= restartLeft && mouseX <= restartRight && mouseY >= restartTop && mouseY <= restartBottom) {
					startGame();
					pastTime = static_cast<float>(glfwGetTime());
					currentState = GameState::Game;
				}

				if (mouseX >= quitLeft && mouseX <= quitRight && mouseY >= quitTop && mouseY <= quitBottom) {
					glfwSetWindowShouldClose(window, true);
				}
			}
			break;
		}

		case GameState::PauseMenu: {
			// Timer to track when the pause menu was entered
			static float pauseMenuEnterTime = 0.0f;
			static bool firstEnter = true;
			static bool escKeyProcessed = false;
			if (firstEnter) {
				pauseMenuEnterTime = static_cast<float>(glfwGetTime());
				firstEnter = false;
				escKeyProcessed = false;
			}

			// Clear buffer from previous elements
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);

			// Convert mouse coordinates
			float mouseX = xpos * (static_cast<float>(SCR_WIDTH) / windowWidth);
			float mouseY = (windowHeight - ypos) * (static_cast<float>(SCR_HEIGHT) / windowHeight);

			// Define the bounding boxes with consistent spacing
			float buttonWidth = 200.0f;
			float buttonHeight = 35.0f;
			float buttonSpacing = 20.0f;

			// Calculate the top-left corner of the first button (Resume)
			float startX = SCR_WIDTH / 2.0f - buttonWidth / 2.0f;
			float startY = SCR_HEIGHT / 2.0f - (buttonHeight * 1.5f + buttonSpacing * 2.0f);

			// Quit Game button
			float quitLeft = startX;
			float quitRight = quitLeft + buttonWidth - 140;
			float quitTop = startY;
			float quitBottom = quitTop + buttonHeight;

			// Restart Game button
			float restartLeft = startX;
			float restartRight = restartLeft + buttonWidth;
			float restartTop = quitBottom + buttonSpacing;
			float restartBottom = restartTop + buttonHeight;

			// Resume button
			float resumeLeft = startX;
			float resumeRight = resumeLeft + buttonWidth + 10;
			float resumeTop = restartBottom + buttonSpacing;
			float resumeBottom = resumeTop + buttonHeight;

			// Render text and bounding boxes
			renderText(shader, "Pause", SCR_WIDTH / 2.0f - 100, SCR_HEIGHT / 2.0f + 100, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
			renderText(shader, "Resume Game", resumeLeft, resumeTop, 0.8f, glm::vec3(0.0f, 1.0f, 0.0f));
			renderText(shader, "Restart Game", restartLeft, restartTop, 0.8f, glm::vec3(0.0f, 1.0f, 0.0f));
			renderText(shader, "Quit", quitLeft, quitTop, 0.8f, glm::vec3(1.0f, 0.0f, 0.0f));

			//renderBoundingBox(resumeLeft, resumeRight, resumeTop, resumeBottom, glm::vec3(0.0f, 1.0f, 0.0f), *ourShader); // Green
			//renderBoundingBox(restartLeft, restartRight, restartTop, restartBottom, glm::vec3(0.0f, 1.0f, 0.0f), *ourShader); // Green
			//renderBoundingBox(quitLeft, quitRight, quitTop, quitBottom, glm::vec3(1.0f, 0.0f, 0.0f), *ourShader); // Red

			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
				// Check if the mouse is inside the "Resume Game" bounding box
				if (mouseX >= resumeLeft && mouseX <= resumeRight && mouseY >= resumeTop && mouseY <= resumeBottom) {
					pastTime = static_cast<float>(glfwGetTime());
					currentState = GameState::Game;
					firstEnter = true;
				}

				// Check if the mouse is inside the "Restart Game" bounding box
				if (mouseX >= restartLeft && mouseX <= restartRight && mouseY >= restartTop && mouseY <= restartBottom) {
					startGame();
					pastTime = static_cast<float>(glfwGetTime());
					currentState = GameState::Game;
					firstEnter = true;
				}

				// Check if the mouse is inside the "Quit" bounding box
				if (mouseX >= quitLeft && mouseX <= quitRight && mouseY >= quitTop && mouseY <= quitBottom) {
					int totObjCorrect = numberOfObject - 2;
					saveScore(numberOfCollisions, totObjCorrect, pastTime);

					glfwSetWindowShouldClose(window, true);
				}
			}

			// Block ESC input for the first 0.5 seconds to prevent flickering
			float currentTime = static_cast<float>(glfwGetTime());
			if (currentTime - pauseMenuEnterTime > 0.5f) {
				if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
					if (!escKeyProcessed) {
						pastTime = static_cast<float>(glfwGetTime());
						currentState = GameState::Game;
						firstEnter = true;
						escKeyProcessed = true;
					}
				}
				else {
					escKeyProcessed = false;
				}
			}

			break;
		}

		case GameState::GuideMenu: {
			// Timer to track when the pause menu was entered
			static float pauseMenuEnterTime = 0.0f;
			static bool firstEnter = true;
			static bool escKeyProcessed = false;
			if (firstEnter) {
				pauseMenuEnterTime = static_cast<float>(glfwGetTime());
				firstEnter = false;
				escKeyProcessed = false;
			}

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			processInput(window, 4);
			renderGuidePage(shader, window);

			// Block ESC input for the first 0.5 seconds to prevent flickering
			float currentTime = static_cast<float>(glfwGetTime());
			if (currentTime - pauseMenuEnterTime > 0.5f) {
				if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
					if (!escKeyProcessed) {
						pastTime = static_cast<float>(glfwGetTime());
						currentState = GameState::MainMenu;
						firstEnter = true;
						escKeyProcessed = true;
					}
				}
				else {
					escKeyProcessed = false;
				}
			}
			break;
		}
		}

		// Swap buffers and poll events
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	std::cout << "Oggetti: " << numberOfObject << std::endl;
	std::cout << "Collisioni: " << numberOfCollisions << std::endl;


	// De-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &conveyorVAO);
	glDeleteBuffers(1, &conveyorVBO);

	glDeleteVertexArrays(1, &plateVAO);
	glDeleteBuffers(1, &plateVBO);

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	soundEngine->drop();

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------

	// Clean up
	delete ourShader;
	glfwTerminate();
	return 0;
}

// Helper function for loading textures
unsigned int TextureFromFile(const char* path, const std::string& directory) {
	std::string filename = std::string(path);
	filename = directory + '/' + filename;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data) {
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else {
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

// Declare the random number generator
std::random_device rd;
std::mt19937 gen(rd());
glm::vec3 generateRandomPosition(Food food) {
	// Fixed x positions
	static float xPositions[3] = { -0.35f, 0.0f, 0.35f };
	static int currentIndex = 0;

	// Shuffle the array if we've used all positions
	if (currentIndex == 0) {
		std::shuffle(std::begin(xPositions), std::end(xPositions), gen);
	}

	// Get the current x-coordinate and move to the next index
	float randomX = xPositions[currentIndex];
	currentIndex = (currentIndex + 1) % 3;

	// offset position for more random position
	static std::random_device rd_offset;
	static std::mt19937 gen(rd_offset());
	static std::uniform_real_distribution<double> dist(-0.15, 0.15);
	float offset = dist(gen);

	if (food.type == 4) {
		return glm::vec3(randomX + offset, 0.88f, 0.2f);
	}

	return glm::vec3(randomX + offset, 1.20f, 0.2f);
}

int generateRandomObject() {
	static int count = 0;
	static std::mt19937 rng(std::random_device{}());
	static std::uniform_int_distribution<int> dist(0, 6);

	count++;
	// Every 3 numbers generate a laser
	if (count % 3 == 0) {
		return 4;
	}
	else {
		int num = dist(rng);
		if (num >= 4) num++;
		return num;
	}
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window, int caller)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		processMenusKeys(window, caller);
	}

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		if (platePosition.x <= 0.45f)
			platePosition.x += 1.0f * deltaTime;
		else {
			platePosition.x = 0.45f;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		if (platePosition.x >= -0.45f)
			platePosition.x -= 1.0f * deltaTime;
		else {
			platePosition.x = -0.45f;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		if (collectedPowerupId == 0 && !powerupActive) {    // Invincibility
			activePowerupId = 0;
			powerupStartTime = static_cast<float>(glfwGetTime());
			powerupActive = true;
			std::cout << "Power-up" << collectedPowerupId << "attivato!" << std::endl;
		}
		else if (collectedPowerupId == 1 && !powerupActive) {	// Rocket launcher
			activePowerupId = 1;
			powerupStartTime = static_cast<float>(glfwGetTime());
			powerupActive = true;
			std::cout << "Power-up" << collectedPowerupId << "attivato!" << std::endl;
		}
		else if (collectedPowerupId == 1 && powerupActive) {
			createFlyingObject();
		}
	}

	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
		showGuide = true;
	}
}

// process keys inpunts for all the menus
// ------------------------------------------
void processMenusKeys(GLFWwindow* window, int caller)
{
	if (caller == 0 || caller == 2) {
		glfwSetWindowShouldClose(window, true);
	}
	else if (caller == 1) {
		//currentState = GameState::PauseMenu;
	}
	else if (caller == 3) {
		currentState = GameState::Game;
	}
	else if (caller == 4) {
		showGuide = false;
		currentState = GameState::MainMenu;
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	windowWidth = width;
	windowHeight = height;
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// Function to create an AABB from position
AABB createAABB(const glm::vec3& position) {
	float halfWidth = 0.075f;
	float halfHeight = 0.075f;

	return AABB{
		glm::vec3(position.x - halfWidth, position.y - halfHeight, position.z - 0.01f),  // min
		glm::vec3(position.x + halfWidth, position.y + halfHeight, position.z + 0.01f)   // max
	};
}

// Function to create an AABB from position
AABB createPlateAABB(const glm::vec3& position) {
	float halfWidth = 0.075f;
	float halfHeight = 0.075f;

	return AABB{
		glm::vec3(position.x - halfWidth, position.y - halfHeight, position.z - 0.01f),  // min
		glm::vec3(position.x + halfWidth, position.y + halfHeight + 0.05, position.z + 0.01f)   // max
	};
}

AABB createRocketAABB(const glm::vec3& position, float size) {
	return {
		glm::vec3(position.x - size / 2, position.y - size / 2, position.z - 0.01f),
		glm::vec3(position.x + size / 2, position.y + size / 2, position.z + 0.01f)
	};
}

// Function to check for collision between two AABBs
bool checkCollision(const AABB& a, const AABB& b) {
	return (a.max.x >= b.min.x && a.min.x <= b.max.x) &&
		(a.max.y >= b.min.y && a.min.y <= b.max.y) &&
		(a.max.z >= b.min.z && a.min.z <= b.max.z);
}

void renderText(Shader& s, std::string text, float x, float y, float scale, glm::vec3 color) {
	// Activate corresponding render state	
	s.use();
	glUniform3f(glGetUniformLocation(s.ID, "textColor"), color.x, color.y, color.z);

	// Enable blending to handle glyph transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(txtVAO);

	// iterate through all characters
	std::string::const_iterator c;
	for (c = text.begin(); c != text.end(); c++)
	{
		Character ch = Characters[*c];

		float xpos = x + ch.Bearing.x * scale;
		float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

		float w = ch.Size.x * scale;
		float h = ch.Size.y * scale;
		// update VBO for each character
		float vertices[6][4] = {
			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos,     ypos,       0.0f, 1.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },

			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos + w, ypos + h,   1.0f, 0.0f }
		};
		// render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, ch.TextureID);
		// update content of VBO memory
		glBindBuffer(GL_ARRAY_BUFFER, txtVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		x += (ch.Advance >> 6) * scale;
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void renderBoundingBox(float left, float right, float top, float bottom, glm::vec3 color, Shader& shader) {
	shader.use();
	shader.setVec3("color", color);

	// Matrici identità per disegno diretto
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT);

	shader.setMat4("model", model);
	shader.setMat4("view", view);
	shader.setMat4("projection", projection);

	// Vertici del bounding box
	float vertices[] = {
		left, top, 0.0f,
		right, top, 0.0f,
		right, bottom, 0.0f,
		left, bottom, 0.0f
	};

	unsigned int VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glDrawArrays(GL_LINE_LOOP, 0, 4);

	glBindVertexArray(0);

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
}

// Game reset/begin
void startGame() {
	numberOfCollisions = 0;
	lives = 3;
	numberOfObject = 1;

	std::cout << "current diff " << static_cast<int>(currentDifficulty) << std::endl;
	if (currentDifficulty == DifficultyLevel::Easy) {
	}
	else if (currentDifficulty == DifficultyLevel::Medium) {
		cubeSpeed *= 1.5f; // Speed up by 50%
		delay *= 0.75f; // Reduce delay by 25%
	}
	else if (currentDifficulty == DifficultyLevel::Hard) {
		cubeSpeed *= 2.0f; // Sped up x2
		delay *= 0.5f; // Half the delay
	}

	pastTime = 0.0f;
	level = 1;
	pastDifficulty = 0.0f;
	platePosition = { 0.0f, -1.10f, 0.2f };

	increaseDifficulty = 6.0f;
	delay = 2.0f;
	cubeSpeed = 0.8f;
	collectedPowerupId = -1;
	activePowerupId = -1;
	powerupStartTime = 0.0f;
	powerupActive = false;


	foods.clear();
	Food food;
	food.type = generateRandomObject();
	food.position = generateRandomPosition(food);
	randomX = getRandomNumberX();
	randomY = getRandomNumberY();
	foods.push_back(food);
	
	return;
}

int getRandomNumberX() {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(570, 770);
	return dist(gen);
}

int getRandomNumberY() {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(30, 570);
	return dist(gen);
}

void createFlyingObject() {
	FlyingObject obj;
	obj.position = platePosition;
	obj.speedY = cubeSpeed;
	obj.size = 0.1f;
	obj.type = 8;
	flyingObjects.push_back(obj);
}

void saveScore(int& collected, int& dropped, float& timePlayed) {
	json j;

	// Load the JSON
	std::ifstream file("score.json");
	if (file.is_open()) {
		file >> j;
		file.close();
	}

	j["last_run"]["collected"] = collected;
	j["last_run"]["dropped"] = dropped;
	j["last_run"]["time_played"] = timePlayed;

	// Check if best run
	if (!j.contains("best_run") || collected > j["best_run"]["collected"]) {
		j["best_run"]["collected"] = collected;
		j["best_run"]["dropped"] = dropped;
		j["best_run"]["time_played"] = timePlayed;
	}

	std::ofstream outFile("score.json");
	outFile << j.dump(4);
	outFile.close();
}

bool loadScores(int& collected, int& dropped, float& timePlayed, int& bestCollected, int& bestDropped, float& bestTimePlayed) {
	std::ifstream file("score.json");
	if (!file.is_open()) {
		return false;
	}

	json j;
	file >> j;
	file.close();

	collected = j["last_run"]["collected"];
	dropped = j["last_run"]["dropped"];
	timePlayed = j["last_run"]["time_played"];
	
	bestCollected = j["best_run"]["collected"];
	bestDropped = j["best_run"]["dropped"];
	bestTimePlayed = j["best_run"]["time_played"];

	return true;
}

void renderGuidePage(Shader& shader, GLFWwindow* window) {
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// Convert mouse coordinates
	float mouseX = xpos * (static_cast<float>(SCR_WIDTH) / windowWidth);
	float mouseY = (windowHeight - ypos) * (static_cast<float>(SCR_HEIGHT) / windowHeight);

	float startX = SCR_WIDTH / 2.0f - 300;
	float startY = SCR_HEIGHT / 2.0f + 250;
	float lineSpacing = 30.0f;

	float quitLeft = (SCR_WIDTH / 2.0f - 60) - 0;
	float quitRight = quitLeft + 60;
	float quitTop = (SCR_HEIGHT / 2.0f - 60 - 30) + 10 - 190;
	float quitBottom = (SCR_HEIGHT / 2.0f - 60) + 15 - 190;

	glm::vec3 titleColor = glm::vec3(1.0f, 1.0f, 0.0f);
	glm::vec3 textColor = glm::vec3(1.0f, 1.0f, 1.0f);

	renderText(shader, "GUIDA", startX, startY, 0.7f, titleColor);

	startY -= lineSpacing * 2;
	renderText(shader, "Powerup:", startX, startY, 0.6f, titleColor);
	renderText(shader, "- Carota: invincibile per 10 secondi", startX + 20, startY - lineSpacing, 0.5f, textColor);
	renderText(shader, "- Vino: spara razzi che distruggono i laser per 10 secondi", startX + 20, startY - 2 * lineSpacing, 0.5f, textColor);

	startY -= 4 * lineSpacing;
	renderText(shader, "Comandi:", startX, startY, 0.6f, titleColor);
	renderText(shader, "- A/Freccia sx: spostarsi a sinistra", startX + 20, startY - lineSpacing, 0.5f, textColor);
	renderText(shader, "- D/Freccia dx: spostarsi a destra", startX + 20, startY - 2 * lineSpacing, 0.5f, textColor);
	renderText(shader, "- SPACEBAR: attivazione powerup raccolti", startX + 20, startY - 3 * lineSpacing, 0.5f, textColor);
	renderText(shader, "- Tasto sx mouse: catturare il demone", startX + 20, startY - 4 * lineSpacing, 0.5f, textColor);

	startY -= 6 * lineSpacing;
	renderText(shader, "Oggetti:", startX, startY, 0.6f, titleColor);
	renderText(shader, "- Laser: sparato dall’alieno. Toglie una vita se colpito", startX + 20, startY - lineSpacing, 0.5f, textColor);
	renderText(shader, "- Demone: toglie una vita se colpito. Evitabile cliccando il box", startX + 20, startY - 2 * lineSpacing, 0.5f, textColor);
	renderText(shader, "- Carota/Vino: danno poteri speciali", startX + 20, startY - 3 * lineSpacing, 0.5f, textColor);
	renderText(shader, "- Altri oggetti: collezionabili", startX + 20, startY - 4 * lineSpacing, 0.5f, textColor);

	renderText(shader, "Quit", quitLeft, quitTop, 0.8f, glm::vec3(1.0f, 0.0f, 0.0f));

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (mouseX >= quitLeft && mouseX <= quitRight && mouseY >= quitTop && mouseY <= quitBottom) {
			showGuide = false;
			currentState = GameState::MainMenu;
		}
	}

	return;
}

void LoadTexture(const char* filename, GLuint& textureID, int textureToCompare) {
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	if (textureToCompare == 1) {
		// Disable  wrapping (GL_REPEAT) and use GL_CLAMP_TO_EDGE
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// Use filtering GL_NEAREST to avoid interpolations
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		stbi_set_flip_vertically_on_load(GL_TRUE);
	}

	// Load imange
	int width, height, nrChannels;
	unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);

	if (data) {
		GLenum format = (nrChannels == 1) ? GL_RED :
			(nrChannels == 3) ? GL_RGB :
			(nrChannels == 4) ? GL_RGBA : GL_RGB;

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		std::cout << "Failed to load texture: " << filename << std::endl;
	}

	stbi_image_free(data);
}

Model LoadModelWithFallback(const std::string& primaryPath, const std::string& secondaryPath) {
	Model model(primaryPath);
	if (model.IsLoaded()) {
		//std::cout << "Loaded model from primary path: " << primaryPath << std::endl;
		return model;
	}
	else {
		//std::cerr << "Failed to load model from primary path: " << primaryPath << ". Trying secondary path: " << secondaryPath << std::endl;
		Model fallbackModel(secondaryPath);
		if (fallbackModel.IsLoaded()) {
			//std::cout << "Loaded model from secondary path: " << secondaryPath << std::endl;
			return fallbackModel;
		}
		else {
			//std::cerr << "Failed to load model from secondary path: " << secondaryPath << std::endl;
			throw std::runtime_error("Failed to load model from both paths");
		}
	}
}

bool fileExists(const std::string& filename) {
	std::ifstream file(filename);
	return file.good();
}
