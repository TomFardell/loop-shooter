#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
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

  int max_enemies;
  float enemy_speed_min;
  float enemy_speed_max;
  float enemy_size_min;
  float enemy_size_max;
  float enemy_interval_min;
  float enemy_interval_max;
  Color enemy_colour;
} Constants;

typedef struct Player {
  Vector2 pos;
  float speed;
  float size;
  Color colour;
} Player;

typedef struct Enemy {
  Vector2 pos;
  Vector2 desired_pos;
  float speed;
  float size;
  Color colour;
} Enemy;

typedef struct EnemyManager {
  Enemy *enemies;
  int count;
  int capacity;
  float enemy_interval;
  float time_of_last_enemy;
} EnemyManager;

// Generate a random float in the given range (inclusive)
float get_random_float(float min, float max) {
  float mult = GetRandomValue(0, INT_MAX) / (float)INT_MAX;
  return min + mult * (max - min);
}

// Get the normalised vector for the direction the player should move according to keyboard input
Vector2 player_get_input_direction() {
  Vector2 res = {0};
  if (IsKeyDown(KEY_S)) res.y++;
  if (IsKeyDown(KEY_W)) res.y--;
  if (IsKeyDown(KEY_D)) res.x++;
  if (IsKeyDown(KEY_A)) res.x--;

  return Vector2Normalize(res);  // Normalise to prevent diagonal movement being quicker
}

// Update the player's position according to keyboard input
void player_update_position(Player *player, Constants constants) {
  player->pos =
      Vector2Add(player->pos, Vector2Scale(player_get_input_direction(), player->speed * GetFrameTime()));

  // Clamp the player inside the screen boundaries
  Vector2 min_player_pos = {player->size, player->size};
  Vector2 max_player_pos = {constants.screen_width - player->size, constants.screen_height - player->size};
  player->pos = Vector2Clamp(player->pos, min_player_pos, max_player_pos);
}

Vector2 enemy_get_random_start_pos(float enemy_size, Constants constants) {
  int vert_or_horiz = GetRandomValue(0, 1);  // Choose to spawn on either the two vertical or two horizontal sides
  int side_chosen = GetRandomValue(0, 1);    // Choose which of the two sides chosen above to spawn on
  int x_val = get_random_float(0, constants.screen_width);
  int y_val = get_random_float(0, constants.screen_height);

  // Use the numbers generated above to pick a random position on the edges of the play area (plus one enemy width)
  return (Vector2){vert_or_horiz * x_val +
                       !vert_or_horiz * (side_chosen * (constants.screen_width + 2 * enemy_size) - enemy_size),
                   !vert_or_horiz * y_val +
                       vert_or_horiz * (side_chosen * (constants.screen_height + 2 * enemy_size) - enemy_size)};
}

Enemy enemy_generate(Player player, Constants constants) {
  Enemy enemy = {.desired_pos = player.pos, .colour = constants.enemy_colour};

  enemy.speed = get_random_float(constants.enemy_speed_min, constants.enemy_speed_max);
  enemy.size = get_random_float(constants.enemy_size_min, constants.enemy_size_max);
  enemy.pos = enemy_get_random_start_pos(enemy.size, constants);

  return enemy;
}

void enemy_manager_try_to_spawn_enemy(EnemyManager *enemy_manager, Player player, Constants constants) {
  // If we already have the max number of enemies spawned, do nothing
  if (enemy_manager->count >= enemy_manager->capacity) return;

  // If it has not been long enough since the last enemy, do nothing
  float time_since_last_enemy = GetTime() - enemy_manager->time_of_last_enemy;
  if (time_since_last_enemy < enemy_manager->enemy_interval) return;

  // Otherwise spawn an enemy
  enemy_manager->enemies[enemy_manager->count++] = enemy_generate(player, constants);

  // Reset the enemy timer and generate a new interval length
  enemy_manager->time_of_last_enemy = GetTime();
  enemy_manager->enemy_interval = get_random_float(constants.enemy_interval_min, constants.enemy_interval_max);
}

int main() {
  /*----------------*/
  /* Initialisation */
  /*-------------------------------------------------------------------------------------------------------------*/
  const Constants constants = {.screen_width = 800,
                               .screen_height = 600,
                               .target_fps = 240,

                               .player_start = {400, 300},
                               .player_base_speed = 500,
                               .player_base_size = 20,
                               .player_colour = VIOLET,

                               .max_enemies = 10,
                               .enemy_speed_min = 150,
                               .enemy_speed_max = 250,
                               .enemy_size_min = 15,
                               .enemy_size_max = 40,
                               .enemy_interval_min = 1.0,
                               .enemy_interval_max = 1.5,
                               .enemy_colour = RED};

  InitWindow(constants.screen_width, constants.screen_height, "Test game");
  SetTargetFPS(constants.target_fps);

  Player player = {.pos = constants.player_start,
                   .size = constants.player_base_size,
                   .speed = constants.player_base_speed,
                   .colour = constants.player_colour};

  EnemyManager enemy_manager = {
      .enemies = calloc(constants.max_enemies, sizeof *(enemy_manager.enemies)),
      .count = 0,
      .capacity = constants.max_enemies,
      .enemy_interval = get_random_float(constants.enemy_interval_min, constants.enemy_interval_max),
      .time_of_last_enemy = GetTime()};
  /*-------------------------------------------------------------------------------------------------------------*/

  while (!WindowShouldClose()) {
    /*--------*/
    /* Update */
    /*-----------------------------------------------------------------------------------------------------------*/
    player_update_position(&player, constants);
    enemy_manager_try_to_spawn_enemy(&enemy_manager, player, constants);

    for (int i = 0; i < enemy_manager.count; i++) {
      Enemy this_enemy = enemy_manager.enemies[i];
      printf("(enemy %d: pos=[%f %f] spd=%f, sze=%f)\n", i, this_enemy.pos.x, this_enemy.pos.y, this_enemy.speed,
             this_enemy.size);
    }
    /*-----------------------------------------------------------------------------------------------------------*/

    /*---------*/
    /* Drawing */
    /*-----------------------------------------------------------------------------------------------------------*/
    BeginDrawing();
    {
      ClearBackground(RAYWHITE);

      DrawCircleV(player.pos, player.size, player.colour);
    }
    EndDrawing();
    /*-----------------------------------------------------------------------------------------------------------*/
  }

  /*-------------------*/
  /* De-initialisation */
  /*-------------------------------------------------------------------------------------------------------------*/
  CloseWindow();

  free(enemy_manager.enemies);
  /*-------------------------------------------------------------------------------------------------------------*/

  return EXIT_SUCCESS;
}
