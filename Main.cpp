#include <iostream>
#include <SDL.h>
#include <vector>
#include <algorithm>
#include <SDL_image.h>
#include <random>
#include <time.h>
#include <SDL_mixer.h>

#define WIDTH 1024
#define HEIGHT 768
#define FPS 60
#define BGS 2

using namespace std;

struct Sprite {
	SDL_Rect m_dst;
};

struct Background {
	SDL_Rect m_src, m_dst;
};
//Obstacles for the game, basically a bunch of rocks
class Obstacles : public Sprite
{
public:
	bool m_active = true;

public:
	Obstacles(SDL_Rect d = { 0,0,0,0 }) // Note the default parameters.
	{
		m_dst = d;
	}
	void update()
	{
		//speed is random 3 - 7
		int tempSpeed;
		srand((unsigned)time(NULL));
		tempSpeed = rand() % 5 + 3;
		m_dst.x -= tempSpeed;
		if (m_dst.x < 0 - m_dst.w) {// if obstacles get off the screen
			m_dst.x = 0 - m_dst.w;
			m_active = false;
		}
	}
};

//Obstacles for the game, basically a bunch of rocks
class Obstacle_small : public Sprite
{
public:
	bool m_active = true;

public:
	Obstacle_small(SDL_Rect d = { 0,0,0,0 }) // Note the default parameters.
	{
		m_dst = d;
	}
	void update()
	{
		//speed is random between 5 - 14
		int tempSpeed;
		srand((unsigned)time(NULL));
		tempSpeed = (rand() * 251) % 10 + 5; //makes this one differ from larger one
		m_dst.x -= tempSpeed;
		if (m_dst.x < 0 - m_dst.w) {// if obstacles get off the screen
			m_dst.x = 0 - m_dst.w;
			m_active = false;
		}
	}
};

class EmyBullet : public Sprite // Inherits m_dst from Sprite.
{
public:
	bool m_active = true;
	EmyBullet(SDL_Rect d)
	{
		m_dst = d;
	}
	void update()
	{
		m_dst.x -= 6; // Just a literal speed. You may want a variable.
		if (m_dst.x < 0 - m_dst.w) // Off-screen.
			m_active = false;
	}
};

class Enemy : public Sprite
{
public:
	int m_bulletTimer = 0, m_timerMax, m_channel;
	vector<EmyBullet*>* bulletVec; // A pointer to a vector. NOT a vector on its own.
	bool m_active = true;
	Mix_Chunk* m_pSound;
public:
	Enemy(SDL_Rect d = { 0,0,0,0 }, vector<EmyBullet*>* bVec = nullptr, Mix_Chunk* s = nullptr, int c = 0,  int t = 120) // Note the default parameters.
	{
		m_dst = d;
		bulletVec = bVec;
		m_pSound = s;
		m_channel = c;
		m_timerMax = t;
	}
	void update()
	{
		m_dst.x -= 1;
		if (m_bulletTimer++ == m_timerMax)
		{
			m_bulletTimer = 0;
			spawnBullet();
		}
		if (m_dst.x < 0 - m_dst.w) {// if enemy get off the screen
			m_dst.x = 0 - m_dst.w;
			m_active = false;
		}
	}
	void spawnBullet()
	{ // Note the -> because we have a POINTER to a vector.
		//set the size/location of the bullet
		bulletVec->push_back(new EmyBullet({ m_dst.x + 24, m_dst.y + 36, 55, 50 }));

		Mix_PlayChannel(m_channel, m_pSound, 0);
	}
};


class Bullet {
public: //members
	bool m_active = true; //unique
	SDL_Rect m_dst; //unique
	static const int speed = 4; //shared
public: //methods
	Bullet(int x, int y) {
		m_dst = { x - 2, y - 2, 4, 4 };
	}
	void update() {
		m_dst.x += speed;
		if (m_dst.x > WIDTH) {// if bullet get off the screen
			m_active = false;
		}
	}
};

// Global engine variables
bool g_bRunning = false; // Loop control flag.
int g_iSpeed = 5; // Speed of box.
const Uint8* g_iKeystates; // Keyboard state container.
Uint32 g_start, g_end, g_delta, g_fps; // Fixed timestep variables.
SDL_Window* g_pWindow; // This represents the SDL window.
SDL_Renderer* g_pRenderer; // This represents the buffer to draw to.
//SDL_Rect g_rBox; // Box representing our actor in the window. Don't need.

// New variables for sprite.
SDL_Rect g_src, g_dst;
SDL_Rect explosion_src, temp_dst;
SDL_Texture* g_pTexture;
SDL_Surface* g_pSurface;
SDL_Texture* g_pBackground;
SDL_Texture* g_pTextureEnemy;
SDL_Texture* g_pTextureObstacle;
SDL_Texture* g_pTextureObstacleSmall;
SDL_Texture* g_pTextureShadowBall;
SDL_Texture* g_explosion;

Mix_Music* g_pMusic;
Mix_Chunk* g_playerShooting;
Mix_Chunk* g_collsion;
Mix_Chunk* g_collsion2;
Mix_Chunk* g_stone;
Mix_Chunk* g_bump;

bool g_isCol = false;
Background bgArray[BGS];
bool player_active = true;
int delayCounter = 0;
// vector for bullet
vector<Bullet*> bulletVec; // hold pointers to dynamically
vector<EmyBullet*> enemyBullets;//Enemy bullet
vector<Enemy*> Enemys;//Enemy
vector<Obstacles*> Obstacle;//obstacle
vector<Obstacle_small*>ObstacleSmall;//smaller obstacle

void updatePanning()
{
	Mix_SetPanning(0, 0, 255); // Full right.
	Mix_SetPanning(1, 64, 128); // Partly right.
	Mix_SetPanning(2, 128, 64); // Partly left.
	Mix_SetPanning(3, 255, 0); // Full left.
}


bool init(const char* title, int xpos, int ypos, int width, int height, int flags)
{
	cout << "Initializing game." << endl;

	// Attempt to initialize SDL.
	if (SDL_Init(SDL_INIT_EVERYTHING) == 0)
	{
		// Create the window.
		g_pWindow = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
		if (g_pWindow != nullptr) // Window init success.
		{
			g_pRenderer = SDL_CreateRenderer(g_pWindow, -1, 0);
			if (g_pRenderer != nullptr) // Renderer init success.
			{
				if (IMG_Init(IMG_INIT_PNG))
				{
					g_pSurface = IMG_Load("backgroundNew.png");
					g_pBackground = SDL_CreateTextureFromSurface(g_pRenderer, g_pSurface);
					g_pTexture = IMG_LoadTexture(g_pRenderer, "army.png");
					g_pTextureEnemy = IMG_LoadTexture(g_pRenderer, "enemy.png");
					g_pTextureObstacle = IMG_LoadTexture(g_pRenderer, "obstacle.png");
					g_pTextureObstacleSmall = IMG_LoadTexture(g_pRenderer, "obstacle.png");
					g_pTextureShadowBall = IMG_LoadTexture(g_pRenderer, "shadowBall.png");
					g_explosion = IMG_LoadTexture(g_pRenderer, "explosion.png");
				}
				else return false; // Init init fail.

				//Now mixer subsystem.
				if (Mix_Init(MIX_INIT_MP3) != 0) // Mixer init success.
				{
					Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 8192); // Good for most games.
					Mix_AllocateChannels(16); // We don't need this many channels for this though.
					g_pMusic = Mix_LoadMUS("DiGiorno.mp3");
					g_playerShooting = Mix_LoadWAV("laser.wav");
					g_collsion = Mix_LoadWAV("death.wav");
					g_collsion2 = Mix_LoadWAV("death_emy.wav");
					g_stone = Mix_LoadWAV("stone.wav");
					g_bump = Mix_LoadWAV("bump.wav");
					
					Mix_Volume(-1, MIX_MAX_VOLUME); // Volume for all channels.
					Mix_VolumeMusic(MIX_MAX_VOLUME / 2); // Volume for channel 0 to 64.
					updatePanning();
				}
				else return false;
			}
			else return false; // Renderer init fail.
		}
		else return false; // Window init fail.
	}
	else return false; // SDL init fail.


	g_fps = (Uint32)round((1 / (double)FPS) * 1000); // Sets FPS in milliseconds and rounds.
	g_iKeystates = SDL_GetKeyboardState(nullptr);
	bgArray[0] = { {0,0,1024,768}, {0, 0, 1024, 768} }; // Src and dst rectangle objects.
	bgArray[1] = { {0,0,1024,768}, {1024, 0, 1024, 768} };

	g_src = { 0, 0, 149, 198 };
	g_dst = { width / 4 - g_src.w / 4, height / 2 - g_src.h / 4, g_src.w / 2, g_src.h / 2 };

	explosion_src = { 0, 0, 91, 136 };
	Mix_PlayMusic(g_pMusic, -1);

	g_bRunning = true; // Everything is okay, start the engine.
	cout << "Success!" << endl;
	return true;
}


void wake()
{
	g_start = SDL_GetTicks();
}

void sleep()
{
	g_end = SDL_GetTicks();
	g_delta = g_end - g_start;
	if (g_delta < g_fps) // Engine has to sleep.
		SDL_Delay(g_fps - g_delta);
}

void handleEvents()
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_QUIT: // User pressed window's 'x' button.
			g_bRunning = false;
			break;
		case SDL_KEYDOWN: // Try SDL_KEYUP instead.
			if (event.key.keysym.sym == SDLK_ESCAPE)
				g_bRunning = false;
			else if (event.key.keysym.sym == SDLK_SPACE) {
				bulletVec.push_back(new Bullet(g_dst.x + 75, g_dst.y + g_dst.h / 4));
				Mix_PlayChannel(5, g_playerShooting, 0);
			}
			break;
		}
	}
}

// Keyboard utility function.
bool keyDown(SDL_Scancode c)
{
	if (g_iKeystates != nullptr)
	{
		if (g_iKeystates[c] == 1)
			return true;
		else
			return false;
	}
	return false;
}

void update()
{
	if (!player_active) {
		explosion_src.x = explosion_src.w * static_cast<int>((SDL_GetTicks() / FPS) % 8);
	}

	updatePanning();
	//enemy and their bullets
	for (int i = 0; i < (int)Enemys.size(); i++) {
		Enemys[i]->update();
		if (Enemys[i]->m_active == false) {
			delete Enemys[i];
			Enemys[i] = nullptr;
			break;
		}
	}
	if (!Enemys.empty())
	{
		Enemys.erase(remove(Enemys.begin(), Enemys.end(), nullptr), Enemys.end());
		Enemys.shrink_to_fit();
	}
	//obtancle
	for (int i = 0; i < (int)Obstacle.size(); i++) {
		Obstacle[i]->update();
		if (Obstacle[i]->m_active == false) {
			delete Obstacle[i];
			Obstacle[i] = nullptr;
			break;
		}
	}
	if (!Obstacle.empty())
	{
		Obstacle.erase(remove(Obstacle.begin(), Obstacle.end(), nullptr), Obstacle.end());
		Obstacle.shrink_to_fit();
	}

	//small obtancle 
	for (int i = 0; i < (int)ObstacleSmall.size(); i++) {
		ObstacleSmall[i]->update();
		if (ObstacleSmall[i]->m_active == false) {
			delete ObstacleSmall[i];
			ObstacleSmall[i] = nullptr;
			break;
		}
	}
	if (!ObstacleSmall.empty())
	{
		ObstacleSmall.erase(remove(ObstacleSmall.begin(), ObstacleSmall.end(), nullptr), ObstacleSmall.end());
		ObstacleSmall.shrink_to_fit();
	}

	//if player got killed
	if (player_active == false) {
		g_dst = { 0,0,0,0 };
		delayCounter++;
		//this counter make game run 100 more frames after the player got hit
		if (delayCounter == 100) {
			cout << "Player Killed. Game Over." << endl;
			g_bRunning = false;
		}
	}
	//random location for enemys
	srand((unsigned)time(NULL));
	int location = (rand() * 66) % 586 + 25; //* 66 to make a better make a better random
	//the size is less than 1 to make sure there wont be too many enemys
	if ((int)Enemys.size() < 1) {
		Enemys.push_back(new Enemy({ WIDTH, location, 100, 100 }, &enemyBullets, Mix_LoadWAV("laser-old1.wav")) );
	}

	//random location for obstacles
	srand((unsigned)time(NULL));
	location = (rand() * 1984) % 581 + 25; //* 1984 to make a better random

	if ((int)Obstacle.size() < 1) {
		Obstacle.push_back(new Obstacles({ WIDTH, location, 100, 100 }));
	}
	//random location for small obstacles
	srand((unsigned)time(NULL));
	location = (rand() * 66) % 631 + 25; //make a better random

	if ((int)ObstacleSmall.size() < 1) {
		ObstacleSmall.push_back(new Obstacle_small({ WIDTH, location, 50, 50 }));
	}


	for (int i = 0; i < (int)enemyBullets.size(); i++)
	{ // Same code as the player bullet example.
		enemyBullets[i]->update();
		if (enemyBullets[i]->m_active == false)
		{
			delete enemyBullets[i];
			enemyBullets[i] = nullptr;
		}
	}
	if (!enemyBullets.empty())
	{
		enemyBullets.erase(remove(enemyBullets.begin(), enemyBullets.end(), nullptr), enemyBullets.end());
		enemyBullets.shrink_to_fit();
	}

	//background dont move if move == false
	bool move = false;
	// This is the main game stuff.
	if (keyDown(SDL_SCANCODE_W))
		g_dst.y -= g_iSpeed;
	if (keyDown(SDL_SCANCODE_S))
		g_dst.y += g_iSpeed;
	if (keyDown(SDL_SCANCODE_A)) {
		g_dst.x -= g_iSpeed;
		//if the player moved to the edge, he cannot go across
		if (g_dst.x < 0) {
			g_dst.x = 0;
			//move = true; //the player reached the edge, move the background
		}
		//move the background
		//I think player should not move backward since the assignment did not ask for this
		/*if (move) {
			for (int i = 0; i < BGS; i++) {
				bgArray[i].m_dst.x += g_iSpeed;
			}
		}*/
	}
	if (keyDown(SDL_SCANCODE_D)) {
		g_dst.x += g_iSpeed;
		//player cannot cross the center line
		if (g_dst.x > WIDTH / 2 - 50) {
			g_dst.x = WIDTH / 2 - 50;
			move = true;
		}
		//scroll the background
		if (move) {
			for (int i = 0; i < BGS; i++) {
				bgArray[i].m_dst.x -= g_iSpeed;
			}
		}
	}

	//keep player on the screen
	if (g_dst.y > HEIGHT - 100) {
		g_dst.y = HEIGHT - 100;
	}
	else if (g_dst.y < 0) {
		g_dst.y = 0;
	}

	// Check if they need to snap back. I chose the 2nd one.
	if (bgArray[1].m_dst.x <= 0) // Or bgArray[0].m_dst.x <= -1024
	{
		bgArray[0].m_dst.x = 0;
		bgArray[1].m_dst.x = 1024;
	}
	//bullet handling
	for (int i = 0; i < (int)bulletVec.size(); i++) {
		bulletVec[i]->update(); //combines dereference with member access
		if (bulletVec[i]->m_active == false) {
			delete bulletVec[i];
			bulletVec[i] = nullptr;
		}
	}
	if (!bulletVec.empty()) {
		bulletVec.erase(remove(bulletVec.begin(), bulletVec.end(), nullptr), bulletVec.end());
	}

	//check for collision between player and enemy, if there is a collision, player dies, game is over
	for (int i = 0; i < (int)Enemys.size(); i++) {
		g_isCol = SDL_HasIntersection(&g_dst, &Enemys[i]->m_dst);
		if (g_isCol) {
			player_active = false;
			Mix_PlayChannel(1, g_collsion, 0);
			temp_dst = g_dst;
		}
	}
	//check for collision between player and Obstacle, if there is a collision, player dies, game is over
	for (int i = 0; i < (int)Obstacle.size(); i++) {
		g_isCol = SDL_HasIntersection(&g_dst, &Obstacle[i]->m_dst);
		if (g_isCol) {
			player_active = false;
			Mix_PlayChannel(1, g_collsion, 0);
			temp_dst = g_dst;
		}
	}
	//check for collision between player and small Obstacle, if there is a collision, player dies, game is over
	for (int i = 0; i < (int)ObstacleSmall.size(); i++) {
		g_isCol = SDL_HasIntersection(&g_dst, &ObstacleSmall[i]->m_dst);
		if (g_isCol) {
			player_active = false;
			Mix_PlayChannel(1, g_collsion, 0);
			temp_dst = g_dst;
		}
	}
	//check collsion between bullet and obstacles, if there's a collsion, bullet get destroyed
	for (int i = 0; i < (int)bulletVec.size(); i++) {
		for (int j = 0; j < (int)Obstacle.size(); j++) {
			g_isCol = SDL_HasIntersection(&bulletVec[i]->m_dst, &Obstacle[j]->m_dst);
			if (g_isCol) {
				bulletVec[i]->m_active = false;
				Mix_PlayChannel(2, g_stone, 0);
			}
		}
		for (int j = 0; j < (int)ObstacleSmall.size(); j++) {
			g_isCol = SDL_HasIntersection(&bulletVec[i]->m_dst, &ObstacleSmall[j]->m_dst);
			if (g_isCol) {
				bulletVec[i]->m_active = false;
				Mix_PlayChannel(2, g_stone, 0);
			}
		}
	}
	//check for collision between player and enemy's bullet, if there is a collision, player dies, game is over
	for (int i = 0; i < (int)enemyBullets.size(); i++) {
		g_isCol = SDL_HasIntersection(&g_dst, &enemyBullets[i]->m_dst);
		if (g_isCol) {
			player_active = false;
			enemyBullets[i]->m_active = false;
			Mix_PlayChannel(3, g_collsion, 0);
			temp_dst = g_dst;
		}
	}

	//check for collision between player's bullet and enemy, if there is a collision, both object will be destroyed
	for (int i = 0; i < (int)bulletVec.size(); i++) {
		for (int j = 0; j < (int)Enemys.size(); j++) {
			g_isCol = SDL_HasIntersection(&bulletVec[i]->m_dst, &Enemys[j]->m_dst);
			if (g_isCol) {
				bulletVec[i]->m_active = false;
				Enemys[j]->m_active = false;
				Mix_PlayChannel(4, g_collsion2, 0);
			}
		}
	}

	//check for collision between player's bullet and enemy's bullet, if there is a collision, both object will be destroyed
	for (int i = 0; i < (int)bulletVec.size(); i++) {
		for (int j = 0; j < (int)enemyBullets.size(); j++) {
			g_isCol = SDL_HasIntersection(&bulletVec[i]->m_dst, &enemyBullets[j]->m_dst);
			if (g_isCol) {
				bulletVec[i]->m_active = false;
				enemyBullets[j]->m_active = false;
				Mix_PlayChannel(6, g_bump, 0);
			}
		}
	}
}

void render()
{
	SDL_SetRenderDrawColor(g_pRenderer, 255, 51, 0, 12);
	SDL_RenderClear(g_pRenderer); // Clear the screen with the draw color.

	for (int i = 0; i < BGS; i++) {
		SDL_RenderCopy(g_pRenderer, g_pBackground, &bgArray[i].m_src, &bgArray[i].m_dst);
	}
	//enemy bullet
	for (int i = 0; i < (int)enemyBullets.size(); i++) {
		SDL_RenderCopy(g_pRenderer, g_pTextureShadowBall, &g_src, &enemyBullets[i]->m_dst);
	}
	// Player
	SDL_RenderCopyEx(g_pRenderer, g_pTexture, &g_src, &g_dst, 0, nullptr, SDL_FLIP_NONE);

	if (!player_active) {
		SDL_RenderCopy(g_pRenderer, g_explosion, &explosion_src, &temp_dst);
	}
	//enemy
	for (int i = 0; i < (int)Enemys.size(); i++) {
		SDL_RenderCopy(g_pRenderer, g_pTextureEnemy, &g_src, &Enemys[i]->m_dst);
	}
	//obstacle
	for (int i = 0; i < (int)Obstacle.size(); i++) {
		SDL_RenderCopy(g_pRenderer, g_pTextureObstacle, &g_src, &Obstacle[i]->m_dst);
	}
	//obstacle small
	for (int i = 0; i < (int)ObstacleSmall.size(); i++) {
		SDL_RenderCopy(g_pRenderer, g_pTextureObstacleSmall, &g_src, &ObstacleSmall[i]->m_dst);
	}
	//change color
	SDL_SetRenderDrawColor(g_pRenderer, 0, 0, 0, 0);

	//player bullet
	for (int i = 0; i < (int)bulletVec.size(); i++) {
		SDL_RenderFillRect(g_pRenderer, &bulletVec[i]->m_dst);
	}
	// Draw anew.
	SDL_RenderPresent(g_pRenderer);
}

void clean()
{
	cout << "Cleaning game." << endl;
	SDL_DestroyTexture(g_pTexture);
	SDL_DestroyTexture(g_pTextureEnemy);
	SDL_DestroyTexture(g_pTextureShadowBall);
	SDL_DestroyTexture(g_pTextureObstacleSmall);
	SDL_DestroyTexture(g_pTextureObstacle);
	SDL_DestroyTexture(g_pBackground);
	SDL_DestroyTexture(g_explosion);
	SDL_FreeSurface(g_pSurface);
	Mix_CloseAudio();
	Mix_FreeMusic(g_pMusic);
	Mix_FreeChunk(g_playerShooting);
	Mix_FreeChunk(g_collsion);
	Mix_FreeChunk(g_collsion2);
	Mix_FreeChunk(g_stone);
	Mix_FreeChunk(g_bump);
	SDL_DestroyRenderer(g_pRenderer);
	SDL_DestroyWindow(g_pWindow);
	SDL_Quit();
}

// Main function.
int main(int argc, char* args[]) // Main MUST have these parameters for SDL.
{
	if (init("GAME1007_SDL_Setup", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0) == false)
		return 1;
	while (g_bRunning)
	{
		wake();
		handleEvents();
		update();
		render();
		if (g_bRunning)
			sleep();
	}
	clean();
	return 0;
}