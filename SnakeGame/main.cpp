
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <SDL.h>
#include <SDL_Image.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
//Using SDL and standard IO
//The image we will load and show on the screen
SDL_Surface* gHelloWorld[1000];
unsigned char* clearDatas[1000];
std::size_t nDatas = 0;

enum SurfaceImgs {
	HEAD_D,
	HEAD_U,
	HEAD_R,
	HEAD_L,
	CIRCLE_RED,
	CIRCLE_YELLOW,
	SQUARE_RED,
	SQUARE_WHITE,
	GAME_OVER,
	SQUARE_BLACK,
	SQUARE_GOLD,
	GAME_PAUSE,
	GAME_START,
	GAME_WIN,
	VOID
};

const char* names[] = {
	"HeadD.png",
	"HeadU.png",
	"HeadR.png",
	"HeadL.png",
	"CircleRed.png",
	"CircleYellow.png",
	"SquareRed.png",
	"SquareWhite.png",
	"GameOver.png",
	"SquareBlack.png",
	"SquareGold.png",
	"GamePause.png",
	"GameStart.png",
	"GameWin.png"
};

SDL_Surface* getSurfaceFromImg(const char* filename, unsigned char* data, int& width, int& height) {

	int req_format = STBI_rgb_alpha;
	int orig_format;
	data = stbi_load(filename, &width, &height, &orig_format, req_format);
	if (data == NULL) {
		SDL_Log("Loading image failed: %s", stbi_failure_reason());
		exit(1);
	}

	// Set up the pixel format color masks for RGB(A) byte arrays.
	// Only STBI_rgb (3) and STBI_rgb_alpha (4) are supported here!
	Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	int shift = (req_format == STBI_rgb) ? 8 : 0;
	rmask = 0xff000000 >> shift;
	gmask = 0x00ff0000 >> shift;
	bmask = 0x0000ff00 >> shift;
	amask = 0x000000ff >> shift;
#else // little endian, like x86
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = (req_format == STBI_rgb) ? 0 : 0xff000000;
#endif

	int depth, pitch;
	if (req_format == STBI_rgb) {
		depth = 24;
		pitch = 3 * width; // 3 bytes per pixel * pixels per row
	}
	else { // STBI_rgb_alpha (RGBA)
		depth = 32;
		pitch = 4 * width;
	}

	SDL_Surface* surf = SDL_CreateRGBSurfaceFrom((void*)data, width, height, depth, pitch,
		rmask, gmask, bmask, amask);

	if (surf == NULL) {
		SDL_Log("Creating surface failed: %s", SDL_GetError());
		stbi_image_free(data);
		exit(1);
	}
	clearDatas[nDatas] = data;
	nDatas++;
	return surf;

}
//	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
//		return -1;
//	}
//	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF);
//	window = SDL_CreateWindow("GUU", SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED, 512, 512, SDL_WINDOW_SHOWN);
//	if (window == NULL) {
//		return -1;
//	}
//	surface = SDL_GetWindowSurface(window);

//Screen dimension constants
constexpr int boxSize = 20;
constexpr int nBoxes = 30;
constexpr int SCREEN_WIDTH =  boxSize * ( nBoxes + 2);
constexpr int SCREEN_HEIGHT = boxSize * ( nBoxes + 2);

//Rect for screen
SDL_Rect screenRect = {0,0,SCREEN_WIDTH,SCREEN_HEIGHT };

//Starts up SDL and creates window
bool init();

//Loads media
bool loadMedia(const char *imgName);

//Frees media and shuts down SDL
void close();

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//The surface contained by the window
SDL_Surface* gScreenSurface = NULL;


//Loop variables
bool quit = false;
SDL_Event e;

SDL_Rect playGroundRect = { boxSize,boxSize,SCREEN_WIDTH - 2 * boxSize,SCREEN_HEIGHT - 2 * boxSize };

enum Content {
	BOARD,
	BODY,
	HEAD,
	TAIL,
	SINGLE,
	FOOD,
	BRICK
};

enum Direction {
	LEFT = -2,
	UP,
	NONE,
	DOWN,
	RIGHT

};

int headX = 0;
int headY = 0;
int snakeLen = 1;
int nBricks = 0;
Direction snakeDir;
int foodX, foodY;

//Game Condition stuffs
bool gameOver = false;
bool updateData = true;
bool gameRun = true;


//Timer stuff
Uint64 timeTicks = 0;
Uint64 initTick = 0;
Uint64 scoreTick = 0;

Uint64 currScore = 0;
std::string playerName;
struct BoardUnit {
	int row;
	int col;
	Content mem = BOARD;
	Direction dir = NONE;

	BoardUnit() {
		row = 0;
		col = 0;
	}

	BoardUnit(int a, int b) :row(a), col(b) {

	}

	void draw() {
		SurfaceImgs i = SQUARE_WHITE;
		switch (mem) {
		case FOOD:
			i = CIRCLE_YELLOW;
			break;
		case SINGLE:
			i = CIRCLE_RED;
			break;
		case BOARD:
			i = SQUARE_WHITE;
			break;
		case BRICK:
			i = SQUARE_BLACK;
			break;
		case BODY:
			i = SQUARE_RED;
			break;
		case HEAD:
			switch (dir) {
			case LEFT:
				i = HEAD_R;
				break;
			case RIGHT:
				i = HEAD_L;
				break;
			case DOWN:
				i = HEAD_U;
				break;
			case UP:
				i = HEAD_D;
				break;
			}
			break;
			
		case TAIL:
			switch (dir) {
			case LEFT:
				i = HEAD_L;
				break;
			case RIGHT:
				i = HEAD_R;
				break;
			case DOWN:
				i = HEAD_D;
				break;
			case UP:
				i = HEAD_U;
				break;
			}
			break;

		}
		SDL_Rect tmp;
		tmp.x = (col + 1) * boxSize;
		tmp.y = (row + 1) * boxSize;
		tmp.h = boxSize;
		tmp.w = boxSize;
		SDL_BlitScaled(gHelloWorld[i], nullptr, gScreenSurface, &tmp);

	}


};

BoardUnit units[nBoxes * nBoxes + 1];

void generateFood() {
	//One added for this food
	if (snakeLen + nBricks + 1 >= nBoxes * nBoxes)
		return;
	do {
		foodX = rand() % nBoxes;
		foodY = rand() % nBoxes;
	} while (units[foodY * nBoxes + foodX].mem != BOARD);

	units[foodY * nBoxes + foodX].mem = FOOD;
	units[foodY * nBoxes + foodX].dir = NONE;

}


void resetSnake() {
	for (int x = 0; x < nBoxes; x++) {
		for (int y = 0; y < nBoxes; y++) {
			switch (units[x + y * nBoxes].mem) {
			case HEAD:
				units[x + y * nBoxes].mem = SINGLE;
				units[x + y * nBoxes].dir = NONE;
				break;
			case FOOD:
			case TAIL:
			case BODY:
				units[x + y * nBoxes].mem = BOARD;
				units[x + y * nBoxes].dir = NONE;
				break;
			default:
				break;
			}
		}
	}
	snakeLen = 1;
	currScore = 0;
	gameOver = false;
	updateData = true;
	gameRun = true;
	snakeDir = static_cast<Direction>(-snakeDir);
	generateFood();

}
void resetBoard() {

	for (int x = 0; x < nBoxes; x++) {
		for (int y = 0; y < nBoxes; y++) {
			units[x + y * nBoxes].row = y;
			units[x + y * nBoxes].col = x;
			units[x + y * nBoxes].mem = BOARD;
			units[x + y * nBoxes].dir = NONE;

		}
	}
	units[0].mem = SINGLE;
	units[0].dir = NONE;
	headX = 0;
	headY = 0;
	snakeDir = RIGHT;
	snakeLen = 1;
	nBricks = 0;
	currScore = 0;
	gameOver = false;
	updateData = true;
	gameRun = false;
	
	generateFood();

}

void drawBoard() {
	for (int i = 0; i < nBoxes * nBoxes; i++) {

		units[i].draw();

	}
}

void addBrick() {
	//One added for food and one for the new brick
	if (snakeLen + nBricks + 2 >= nBoxes * nBoxes)
		return;
	int brickX, brickY;
	
	do {
		brickX = rand() % nBoxes;
		brickY = rand() % nBoxes;
	} while (units[brickY * nBoxes + brickX].mem != BOARD);

	units[brickY * nBoxes + brickX].mem = BRICK;
	units[brickY * nBoxes + brickX].dir = NONE;
	nBricks++;
}

BoardUnit* getNext(BoardUnit* prev, Direction dir, bool wrapSnake = true) {
	int nextX = prev->col, nextY = prev->row;
	switch (dir) {
	case RIGHT:
		nextX++;
		break;
	case LEFT:
		nextX--;
		break;
	case UP:
		nextY--;
		break;
	case DOWN:
		nextY++;
		break;
	}
	
	if (nextX < 0 || nextX >= nBoxes || nextY < 0 || nextY >= nBoxes) {
		if (!wrapSnake)
			return nullptr;
		if (nextX < 0)
			nextX = nBoxes - 1;
		if (nextX >= nBoxes)
			nextX = 0;
		if (nextY < 0)
			nextY = nBoxes - 1;
		if (nextY >= nBoxes)
			nextY = 0;
	}
	return units + nextX + nBoxes * nextY;
}

void moveForward() {
	auto head = units + headX + headY * nBoxes;
	auto nextBox = getNext(head, snakeDir);
	
	switch (nextBox->mem) {
	case FOOD:
	{

		nextBox->mem = HEAD;
		nextBox->dir = static_cast<Direction>(-snakeDir);
		headX = nextBox->col;
		headY = nextBox->row;
		if (head->mem == SINGLE) {
			head->mem = TAIL;
			head->dir = nextBox->dir;
		}
		else {
			head->mem = BODY;
		}
		snakeLen++;
		currScore += nBricks + 1;
		addBrick();
		updateData = true;
		if (snakeLen + nBricks == nBoxes * nBoxes) {
			gameOver == true;
		}
		else
			generateFood();
	}
		break;

	case BOARD:
	{
		headX = nextBox->col;
		headY = nextBox->row;
		nextBox->mem = head->mem;
		nextBox->dir = static_cast<Direction>(-snakeDir);
		BoardUnit* prevBox = head;
		Direction nextToTailDir = NONE;
		while (prevBox->mem != SINGLE && prevBox->mem != TAIL) {
			nextToTailDir = nextBox->dir;
			nextBox = prevBox;
			prevBox = getNext(nextBox, nextBox->dir);

		}
		//If only head, make new head also that , only head
		if (prevBox->mem == SINGLE) {
			nextBox->mem = SINGLE;
			nextBox->dir = NONE;
		}
		//If not , then make new last piece a tail
		else {
			//Make old head a body
			head->mem = BODY;
			//Set tail's diretion according to it's previous direction for drawing purposes
			nextBox->dir = nextToTailDir;
			nextBox->mem = TAIL;

		}
		//Delete the old tail
		prevBox->mem = BOARD;
		prevBox->dir = NONE;




	}
		break;
	default:
		gameOver = true;
		updateData = true;
		break;
	}

}


bool loop() {
	static Direction newDir = NONE;
	static bool showHigh = true;
	bool justEntered = true;
	bool canPlay = false;
	timeTicks = SDL_GetTicks64();
	
	if ((timeTicks - initTick) >= 200) {
		canPlay = true;
		initTick = timeTicks;
	}
	if ((timeTicks - initTick) > 20) {
		SDL_Delay(5);
	}
	//Time base updating system
	//if (timeTicks - scoreTick > 10000) {
	// Change based updating system
	if (updateData) {
		if (system("cls")) system("clear");
		if (!gameOver)
			if (gameRun)
				std::cout << "Press space to pause\n";
			else
				std::cout << "Game Paused\nPress space to continue playing\nPress H to display high score\n";

		std::cout << "Current Player : " << playerName << "\tScore : " << currScore << std::endl;
		if (gameOver) {
			std::cout << "Game Over\n";
			if (snakeLen + nBricks == nBoxes * nBoxes)
				std::cout << "Congratulations you won !\n";
			else
				std::cout << "You lost\n";
			std::cout << "Press space to start new snake with same board or"
				<< "press enter to start full new game\n"
				<< "Press H to display high scores";
		}
		scoreTick = timeTicks;
		updateData = false;
	}

	while (SDL_PollEvent(&e) != 0) {

		if (e.type == SDL_QUIT) {
			quit = true;
			break;
		}
		if (gameRun && !gameOver) {
			if (justEntered) {
				switch (e.key.keysym.sym) {
				case SDLK_UP:
					newDir = UP;
					break;
				case SDLK_DOWN:
					newDir = DOWN;
					break;
				case SDLK_LEFT:
					newDir = LEFT;
					break;
				case SDLK_RIGHT:
					newDir = RIGHT;
					break;
				case SDLK_SPACE:
					if (e.key.type == SDL_KEYUP) {
						gameRun = false;
						updateData = true;
					}
					break;
				default:
					justEntered = true;
					break;
				}
				if ((static_cast<int>(newDir) != -static_cast<int>(snakeDir)) && newDir != NONE) {
					justEntered = false;

				}
			}
		}
		else if (!gameRun && e.key.type == SDL_KEYUP) {
			switch (e.key.keysym.sym) {
			case SDLK_SPACE:
				gameRun = true;
				updateData = true;
				break;
			case SDLK_h:
				updateData = true;
				showHigh = true;
				justEntered = false;
				break;
			}
		}
		else if (gameOver && e.key.type == SDL_KEYUP) {
			switch (e.key.keysym.sym) {
			case SDLK_SPACE:
				resetSnake();
				updateData = true;
				gameOver = false;
				break;
			case SDLK_h:
				updateData = true;
				showHigh = true;
				justEntered = false;
				break;
			case SDLK_RETURN:
				resetBoard();
				updateData = true;
				gameOver = false;
				break;
			}
		}
		
	}
	if (canPlay) {
		if ((static_cast<int>(newDir) != -static_cast<int>(snakeDir)) && newDir != NONE) {
			snakeDir = newDir;
		}
		if (!gameOver && gameRun)
			moveForward();
	}

	drawBoard();
	if(gameOver) {
		if (snakeLen + nBricks == nBoxes * nBoxes)

			SDL_BlitScaled(gHelloWorld[GAME_WIN], nullptr, gScreenSurface, &playGroundRect);
		else
			SDL_BlitScaled(gHelloWorld[GAME_OVER], nullptr, gScreenSurface, &playGroundRect);
	}
	else if(!gameRun)
		SDL_BlitScaled(gHelloWorld[GAME_PAUSE], nullptr, gScreenSurface, &playGroundRect);
	SDL_UpdateWindowSurface(gWindow);
	return quit;
}

bool init()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Create window
		gWindow = SDL_CreateWindow("SDL Snake", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Get window surface
			gScreenSurface = SDL_GetWindowSurface(gWindow);
		}
		timeTicks = SDL_GetTicks64();
		initTick = timeTicks;
		scoreTick = timeTicks;
	}

	srand(time(nullptr));
	resetBoard();
	return success;
}

bool loadMedia(const char *imgName)
{
	//Loading success flag
	bool success = true;

	//Load splash image
	//gHelloWorld = SDL_LoadBMP("hello_world.bmp");
	unsigned char* data=nullptr;
	int wid, hei;
	int k = nDatas;
	SDL_Surface *tmpSur = getSurfaceFromImg(imgName, data, wid, hei);
	gHelloWorld[k] = SDL_ConvertSurface(tmpSur, gScreenSurface->format, 0);
	SDL_FreeSurface(tmpSur);
	if (gHelloWorld[k] == NULL)
	{
		printf("Unable to load image %s! SDL Error: %s\n", imgName, SDL_GetError());
		success = false;
	}

	return success;
}

void close()
{
	//Deallocate surface

	for (int i = 0; i < nDatas; i++) {
		SDL_FreeSurface(gHelloWorld[i]);
		gHelloWorld[i] = NULL;

		stbi_image_free(clearDatas[i]);
	}

	//Destroy window
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;

	//Quit SDL subsystems
	SDL_Quit();
}

int main(int argc, char* args[])
{	
	std::cout << "Welcome to worst snake game made by me\n";
	std::cout << "Don't worry actual game is in GUI, but being in early phases of game development,"
		<< " important infos will be presented in console.\n";
	std::cout << "Just as game starts it will be paused, so feel free to initially rearrange console "
		<< "window and game window for easy view to console window.\n";
	std::cout << "Enter player name :";
	std::cin >> playerName;

	//Start up SDL and create window
	if (!init())
	{
		printf("Failed to initialize!\n");
	}
	else
	{
		//Load media
		bool successses = true;
		for (int i = 0; i < VOID; i++) {
			if (loadMedia(names[i]))
				continue;
			successses = false;
		}
		if (!successses) {
			std::cerr << "Failed to load one or more media \n";
			quit = true;
		}
	

		//Main loop
		while (!quit) {
			SDL_BlitScaled(gHelloWorld[SQUARE_GOLD], nullptr, gScreenSurface,&screenRect);
			loop();
		}

	}

	//Free resources and close SDL
	close();
	return 0;
}