#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <SDL2/SDL.h>

#define TILE_SIZE 40
#define MAZE_WIDTH 16
#define MAZE_HEIGHT 12

#define FOV_ANGLE 60
#define NUM_RAYS SCREEN_WIDTH

#define PI 3.14159265359

#define SCREEN_WIDTH (MAZE_WIDTH * TILE_SIZE)
#define SCREEN_HEIGHT (MAZE_HEIGHT * TILE_SIZE)

const float MOVE_SPEED = 0.5f;    // Player movement speed
const float SLIDE_FACTOR = 0.5f;  // Sliding factor for collision handling

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

// Sample map data
int map[MAZE_HEIGHT][MAZE_WIDTH] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1},
        {1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1},
        {1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1},
        {1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1},
        {1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1},
        {1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
};

// Player position and direction
float playerX = 3.5f * TILE_SIZE;
float playerY = 3.5f * TILE_SIZE;
float playerAngle = 0.0f;

bool drawMapEnabled = true;  // Toggle to enable/disable map drawing

bool initializeSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("2D Maze Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL)
    {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

void destroySDL()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void drawMap()
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    // Draw the walls
    for (int y = 0; y < MAZE_HEIGHT; ++y)
    {
        for (int x = 0; x < MAZE_WIDTH; ++x)
        {
            if (map[y][x] == 1)
            {
                SDL_Rect wallRect = { x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE };
                SDL_RenderFillRect(renderer, &wallRect);
            }
        }
    }

    // Draw player's line of sight
    float rayAngle;
    for (int i = 0; i < NUM_RAYS; i++)
    {
        rayAngle = playerAngle - FOV_ANGLE / 2 + (FOV_ANGLE / NUM_RAYS) * i;

        float distanceToWall = 0;
        bool hitWall = false;
        float eyeX = cos(rayAngle * PI / 180);
        float eyeY = sin(rayAngle * PI / 180);

        while (!hitWall && distanceToWall < SCREEN_WIDTH)
        {
            distanceToWall += 0.1f;

            int testX = (int)(playerX + eyeX * distanceToWall);
            int testY = (int)(playerY + eyeY * distanceToWall);

            if (testX < 0 || testX >= SCREEN_WIDTH || testY < 0 || testY >= SCREEN_HEIGHT)
            {
                hitWall = true;
                distanceToWall = SCREEN_WIDTH; // Set to max distance
            }
            else if (map[testY / TILE_SIZE][testX / TILE_SIZE] == 1)
            {
                hitWall = true;
            }
        }

        // Draw the player's line of sight
        int lineHeight = (int)(SCREEN_HEIGHT / distanceToWall);
        int drawStart = SCREEN_HEIGHT / 2 - lineHeight / 2;
        int drawEnd = drawStart + lineHeight;

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderDrawLine(renderer, i, drawStart, i, drawEnd);
    }
}

void movePlayer(float deltaX, float deltaY)
{
    float newX = playerX + deltaX;
    float newY = playerY + deltaY;

    // Collision detection and handling
    int newTileX = (int)(newX / TILE_SIZE);
    int newTileY = (int)(newY / TILE_SIZE);

    // Check if the new position is within the map boundaries
    if (newTileX >= 0 && newTileX < MAZE_WIDTH && newTileY >= 0 && newTileY < MAZE_HEIGHT)
    {
        // Check if the new position collides with a wall
        if (map[newTileY][newTileX] == 0)
        {
            playerX = newX;
            playerY = newY;
        }
        else
        {
            // Perform sliding on walls instead of stopping
            float slideX = playerX;
            float slideY = playerY;

            // Slide horizontally
            if (deltaX > 0)
            {
                slideX = (newTileX + SLIDE_FACTOR) * TILE_SIZE - 1;
            }
            else if (deltaX < 0)
            {
                slideX = (newTileX + 1 - SLIDE_FACTOR) * TILE_SIZE;
            }

            // Slide vertically
            if (deltaY > 0)
            {
                slideY = (newTileY + SLIDE_FACTOR) * TILE_SIZE - 1;
            }
            else if (deltaY < 0)
            {
                slideY = (newTileY + 1 - SLIDE_FACTOR) * TILE_SIZE;
            }

            // Check if the sliding position collides with a wall
            if (map[(int)(slideY / TILE_SIZE)][(int)(slideX / TILE_SIZE)] == 0)
            {
                playerX = slideX;
                playerY = slideY;
            }
        }
    }
}

void processInput()
{
    SDL_Event event;

    while (SDL_PollEvent(&event) != 0)
    {
        if (event.type == SDL_QUIT)
        {
            destroySDL();
            exit(0);
        }
        else if (event.type == SDL_KEYDOWN)
        {
            switch (event.key.keysym.sym)
            {
                case SDLK_ESCAPE:
                    destroySDL();
                    exit(0);
                    break;
                case SDLK_w:
                    movePlayer(cos(playerAngle * PI / 180) * MOVE_SPEED, sin(playerAngle * PI / 180) * MOVE_SPEED);
                    break;
                case SDLK_s:
                    movePlayer(-cos(playerAngle * PI / 180) * MOVE_SPEED, -sin(playerAngle * PI / 180) * MOVE_SPEED);
                    break;
                case SDLK_a:
                    movePlayer(cos((playerAngle - 90) * PI / 180) * MOVE_SPEED, sin((playerAngle - 90) * PI / 180) * MOVE_SPEED);
                    break;
                case SDLK_d:
                    movePlayer(cos((playerAngle + 90) * PI / 180) * MOVE_SPEED, sin((playerAngle + 90) * PI / 180) * MOVE_SPEED);
                    break;
                case SDLK_m:
                    drawMapEnabled = !drawMapEnabled;  // Toggle map drawing
                    break;
            }
        }
    }
}

void renderPlayer()
{
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect playerRect = { playerX - 5, playerY - 5, 10, 10 };
    SDL_RenderFillRect(renderer, &playerRect);

    SDL_RenderDrawLine(renderer, playerX, playerY, playerX + cos(playerAngle * PI / 180) * 40,
                       playerY + sin(playerAngle * PI / 180) * 40);
}

void renderLineOfSight()
{
    // Player position
    int playerTileX = (int)(playerX / TILE_SIZE);
    int playerTileY = (int)(playerY / TILE_SIZE);

    // Player line of sight
    float rayAngle = playerAngle - FOV_ANGLE / 2;
    float rayAngleIncrement = FOV_ANGLE / NUM_RAYS;

    for (int i = 0; i < NUM_RAYS; ++i)
    {
        // Cast a ray from the player's position
        float rayX = playerX;
        float rayY = playerY;
        float rayDistance = 0;

        bool hitWall = false;

        // Cast the ray until it hits a wall or goes beyond the map boundaries
        while (!hitWall && rayX >= 0 && rayX < SCREEN_WIDTH && rayY >= 0 && rayY < SCREEN_HEIGHT)
        {
            // Calculate the next point on the ray
            rayX += cos(rayAngle * PI / 180) * 10;
            rayY += sin(rayAngle * PI / 180) * 10;

            // Check if the next point hits a wall
            int tileX = (int)(rayX / TILE_SIZE);
            int tileY = (int)(rayY / TILE_SIZE);

            if (map[tileY][tileX] == 1)
            {
                hitWall = true;

                // Calculate the distance to the wall
                rayDistance = sqrt((rayX - playerX) * (rayX - playerX) + (rayY - playerY) * (rayY - playerY));
            }
        }

        // Draw the ray on the screen
        int lineHeight = (int)(SCREEN_HEIGHT / rayDistance * TILE_SIZE);
        int lineOffset = (SCREEN_HEIGHT - lineHeight) / 2;

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderDrawLine(renderer, i, lineOffset, i, lineOffset + lineHeight);

        // Increment the ray angle
        rayAngle += rayAngleIncrement;
    }
}

void render()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (drawMapEnabled)
    {
        drawMap();
    }

    renderPlayer();
    renderLineOfSight();

    SDL_RenderPresent(renderer);
}

int main(int argc, char* args[])
{
    if (!initializeSDL())
    {
        return 1;
    }

    while (1)
    {
        processInput();
        drawMap();
        renderPlayer();
        SDL_RenderPresent(renderer);
    }

    return 0;
}
