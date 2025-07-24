#include <stdlib.h>
#include "raylib.h"
#include "raymath.h"

typedef struct Constants {
  int screen_width;
  int screen_height;
  int target_fps;

  Vector2 player_start;
  float player_base_speed;
  float player_base_size;
  Color player_colour;
} Constants;

typedef struct Player {
  Vector2 pos;
  float speed;
  float size;
  Color colour;
} Player;

Vector2 get_player_direction() {
  Vector2 res = {0};
  if (IsKeyDown(KEY_S)) res.y++;
  if (IsKeyDown(KEY_W)) res.y--;
  if (IsKeyDown(KEY_D)) res.x++;
  if (IsKeyDown(KEY_A)) res.x--;

  return Vector2Normalize(res);  // Normalise to prevent diagonal movement being quicker
}

int main() {
  /*----------------*/
  /* Initialisation */
  /*----------------------------------------------------------------------------------------------*/
  const Constants constants = {.screen_width = 800,
                               .screen_height = 600,
                               .target_fps = 240,
                               .player_start = {400, 300},
                               .player_base_speed = 500,
                               .player_base_size = 20,
                               .player_colour = VIOLET};

  InitWindow(constants.screen_width, constants.screen_height, "Test game");
  SetTargetFPS(constants.target_fps);

  Player player = {.pos = constants.player_start,
                   .size = constants.player_base_size,
                   .speed = constants.player_base_speed,
                   .colour = constants.player_colour};
  /*----------------------------------------------------------------------------------------------*/

  while (!WindowShouldClose()) {
    /*--------*/
    /* Update */
    /*--------------------------------------------------------------------------------------------*/
    player.pos = Vector2Add(player.pos, Vector2Scale(get_player_direction(), player.speed * GetFrameTime()));

    // Clamp the player inside the screen boundaries
    Vector2 min_player_pos = {player.size, player.size};
    Vector2 max_player_pos = {constants.screen_width - player.size, constants.screen_height - player.size};
    player.pos = Vector2Clamp(player.pos, min_player_pos, max_player_pos);
    /*--------------------------------------------------------------------------------------------*/

    /*---------*/
    /* Drawing */
    /*--------------------------------------------------------------------------------------------*/
    BeginDrawing();
    {
      ClearBackground(RAYWHITE);

      DrawCircleV(player.pos, player.size, player.colour);
    }
    EndDrawing();
    /*--------------------------------------------------------------------------------------------*/
  }

  /*-------------------*/
  /* De-initialisation */
  /*----------------------------------------------------------------------------------------------*/
  CloseWindow();
  /*----------------------------------------------------------------------------------------------*/

  return EXIT_SUCCESS;
}
