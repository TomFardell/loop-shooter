#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
#include <assert.h>
#include "raylib.h"
#include "raymath.h"

#define DEBUG true

typedef struct Constants {
  int screen_width;
  int screen_height;
  int target_fps;

  Vector2 player_start;
  float player_base_speed;
  float player_base_size;
  Color player_colour;

  float player_base_projectile_fire_interval;
  float player_base_projectile_speed;
  float player_base_projectile_size;
  Color player_projectile_colour;

  int max_enemies;
  float enemy_speed_min;
  float enemy_speed_max;
  float enemy_size_min;
  float enemy_size_max;
  float enemy_spawn_interval_min;
  float enemy_spawn_interval_max;
  float enemy_update_interval;
  float enemy_update_chance;  // Chance (each update) that the enemy updates its desired position
  Color enemy_colour;

  int max_projectiles;
} Constants;

typedef struct Player {
  Vector2 pos;

  float speed;
  float size;
  Color colour;

  float projectile_fire_interval;
  float projectile_speed;
  float projectile_size;
  Color projectile_colour;

  float time_of_last_projectile;
} Player;

typedef struct Enemy {
  Vector2 pos;
  Vector2 desired_pos;
  bool is_active;

  float speed;
  float size;
  Color colour;
} Enemy;

typedef struct EnemyManager {
  Enemy *enemies;
  int enemy_count;
  int capacity;

  float enemy_spawn_interval;
  float time_of_last_spawn;

  float time_of_last_update;
} EnemyManager;

typedef struct Projectile {
  Vector2 pos;
  Vector2 dir;  // Should be normalised when set
  bool is_active;

  float speed;
  float size;
  Color colour;
} Projectile;

typedef struct ProjectileManager {
  Projectile *projectiles;
  int projectile_count;
  int capacity;
} ProjectileManager;

// Generate a random float in the given range (inclusive)
float get_random_float(float min, float max) {
  float mult = GetRandomValue(0, INT_MAX) / (float)INT_MAX;
  return min + mult * (max - min);
}

// Get whether a given circle with centre `pos` and radius `rad` would be showing on the screen
bool circle_is_on_screen(Vector2 pos, float rad, Constants constants) {
  return (-rad <= pos.x && pos.x <= constants.screen_width + rad) &&
         (-rad <= pos.y && pos.y <= constants.screen_height + rad);
};

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

// Randomly generate a starting position of an enemy. Enemies spawn touching the outside faces of the play area
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

// Randomly generate a new enemy
Enemy enemy_generate(Player player, Constants constants) {
  Enemy enemy = {.desired_pos = player.pos,
                 .is_active = true,
                 .speed = get_random_float(constants.enemy_speed_min, constants.enemy_speed_max),
                 .size = get_random_float(constants.enemy_size_min, constants.enemy_size_max),
                 .colour = constants.enemy_colour};

  enemy.pos = enemy_get_random_start_pos(enemy.size, constants);

  return enemy;
}

// Try to create and spawn a new enemy (if it is time to do so)
void enemy_manager_try_to_spawn_enemy(EnemyManager *enemy_manager, Player player, Constants constants) {
  // If we already have the max number of enemies spawned, do nothing
  if (enemy_manager->enemy_count >= enemy_manager->capacity) return;

  // If it has not been long enough since the last enemy, do nothing
  float time_since_last_enemy = GetTime() - enemy_manager->time_of_last_spawn;
  if (time_since_last_enemy < enemy_manager->enemy_spawn_interval) return;

  // Loop through the enemy slots until an inactive enemy is found. Note that since the enemy array was initialised
  // to zero, by default all the slots contain inactive enemies
  for (int i = 0; i < enemy_manager->capacity; i++) {
    if (!enemy_manager->enemies[i].is_active) {
      enemy_manager->enemies[i] = enemy_generate(player, constants);
      enemy_manager->enemy_count++;
      break;
    }

#if DEBUG
    // Check that the loop doesn't terminate without finding an inactive enemy (this is the final i)
    assert((i != enemy_manager->capacity - 1) && "No inactive enemy found");
#endif
  }

  // Reset the enemy timer and generate a new interval length
  enemy_manager->time_of_last_spawn = GetTime();
  enemy_manager->enemy_spawn_interval =
      get_random_float(constants.enemy_spawn_interval_min, constants.enemy_spawn_interval_max);
}

// Update the enemies so that they move towards the player (when it is time to do so and with probability)
void enemy_manager_update_desired_positions(EnemyManager *enemy_manager, Player player, Constants constants) {
  // If it is not time to update the enemies, do nothing
  float time_since_last_update = GetTime() - enemy_manager->time_of_last_update;
  if (time_since_last_update < constants.enemy_update_interval) return;

  // Otherwise, iterate through the enemies and (sometimes) update their desired positions
  for (int i = 0; i < enemy_manager->capacity; i++) {
    Enemy *this_enemy = enemy_manager->enemies + i;
    if (!this_enemy->is_active) continue;

    float r_num = get_random_float(0, 1);
    if (r_num <= constants.enemy_update_chance) {
      this_enemy->desired_pos = player.pos;  // Enemy will now move towards the current position of the player
    }
  }
}

// Update the positions of all active enemies according to their movements
void enemy_manager_update_enemy_positions(EnemyManager *enemy_manager) {
  for (int i = 0; i < enemy_manager->capacity; i++) {
    Enemy *this_enemy = enemy_manager->enemies + i;
    if (!this_enemy->is_active) continue;

    // Move the enemy towards its desired position according to its speedA
    Vector2 normalised_move_direction =
        Vector2Normalize(Vector2Subtract(this_enemy->desired_pos, this_enemy->pos));
    this_enemy->pos =
        Vector2Add(this_enemy->pos, Vector2Scale(normalised_move_direction, this_enemy->speed * GetFrameTime()));
  }
}

// Generate a new projectile that moves towards the mouse
Projectile projectile_generate(Player player) {
  Projectile projectile = {.pos = player.pos,
                           .is_active = true,
                           .speed = player.projectile_speed,
                           .size = player.projectile_size,
                           .colour = player.projectile_colour};

  Vector2 mouse_pos = GetMousePosition();

  // If the mouse is on the player, just fire in an arbitrary direction, otherwise fire towards the mouse
  if (Vector2Equals(mouse_pos, player.pos))
    projectile.dir = (Vector2){1, 0};  // Arbitrarily choose to shoot to the right (normalised manually)
  else
    projectile.dir = Vector2Normalize(Vector2Subtract(mouse_pos, player.pos));

  return projectile;
}

// Spawn a new projectile when it is time to do so and if the correct button is down
void player_try_to_spawn_projectile(Player *player, ProjectileManager *projectile_manager) {
#if DEBUG
  // The projectile manager should have capacity large enough that it never becomes full
  assert((projectile_manager->projectile_count < projectile_manager->capacity) &&
         "Trying to add projectile to full projectile manager");
#else
  // When not in debug mode and the projectile manager is full, just do nothing
  if (projectile_manager->projectile_count >= projectile_manager->capacity) return;
#endif

  // If the mouse button isn't held, do nothing
  if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) return;

  // If it has not been long enough since the last shot, do nothing
  float time_since_last_projectile = GetTime() - player->time_of_last_projectile;
  if (time_since_last_projectile < player->projectile_fire_interval) return;

  for (int i = 0; i < projectile_manager->capacity; i++) {
    if (!projectile_manager->projectiles[i].is_active) {
      projectile_manager->projectiles[i] = projectile_generate(*player);
      projectile_manager->projectile_count++;
      break;
    }

#if DEBUG
    // Check that the loop doesn't terminate without finding an inactive projectile (this is the final i)
    assert((i != projectile_manager->capacity - 1) && "No inactive projectile found");
#endif
  }

  player->time_of_last_projectile = GetTime();
}

// Update projectile positions according to their trajectories
void projectile_manager_update_projectile_positions(ProjectileManager *projectile_manager, Constants constants) {
  for (int i = 0; i < projectile_manager->capacity; i++) {
    Projectile *this_projectile = projectile_manager->projectiles + i;
    if (!this_projectile->is_active) continue;

    // Move the projectile along its trajectory according to its speed
    this_projectile->pos = Vector2Add(this_projectile->pos,
                                      Vector2Scale(this_projectile->dir, this_projectile->speed * GetFrameTime()));

    // If the projectile has moved outside the game boundaries, make it inactive
    if (!circle_is_on_screen(this_projectile->pos, this_projectile->size, constants)) {
      this_projectile->is_active = false;
      projectile_manager->projectile_count--;
    }
  }
}

void projectile_manager_check_for_collisions_with_enemies(ProjectileManager *projectile_manager,
                                                          EnemyManager *enemy_manager) {
  for (int i = 0; i < projectile_manager->capacity; i++) {
    Projectile *this_projectile = projectile_manager->projectiles + i;
    if (!this_projectile->is_active) continue;

    for (int j = 0; j < enemy_manager->capacity; j++) {
      Enemy *this_enemy = enemy_manager->enemies + j;
      if (!this_enemy->is_active) continue;

      if (!CheckCollisionCircles(this_projectile->pos, this_projectile->size, this_enemy->pos, this_enemy->size))
        continue;

      this_projectile->is_active = false;
      projectile_manager->projectile_count--;

      this_enemy->is_active = false;
      enemy_manager->enemy_count--;
    }
  }
}

// Draw the player to the canvas
void draw_player(Player player) { DrawCircleV(player.pos, player.size, player.colour); }

// Draw the active enemies to the canvas
void draw_enemies(EnemyManager enemy_manager) {
  for (int i = 0; i < enemy_manager.capacity; i++) {
    Enemy this_enemy = enemy_manager.enemies[i];
    if (!this_enemy.is_active) continue;

    DrawCircleV(this_enemy.pos, this_enemy.size, this_enemy.colour);
  }
}

// Draw the active projectiles to the canvas
void draw_projectiles(ProjectileManager projectile_manager) {
  for (int i = 0; i < projectile_manager.capacity; i++) {
    Projectile this_projectile = projectile_manager.projectiles[i];
    if (!this_projectile.is_active) continue;

    DrawCircleV(this_projectile.pos, this_projectile.size, this_projectile.colour);
  }
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

                               .player_base_projectile_fire_interval = 0.2,
                               .player_base_projectile_speed = 1000,
                               .player_base_projectile_size = 7,
                               .player_projectile_colour = DARKGRAY,

                               .max_enemies = 10,
                               .enemy_speed_min = 150,
                               .enemy_speed_max = 250,
                               .enemy_size_min = 15,
                               .enemy_size_max = 40,
                               .enemy_spawn_interval_min = 1.0,
                               .enemy_spawn_interval_max = 1.5,
                               .enemy_update_interval = 0.2,
                               .enemy_update_chance = 0.02,
                               .enemy_colour = RED,

                               .max_projectiles = 40};

  InitWindow(constants.screen_width, constants.screen_height, "Test game");
  SetTargetFPS(constants.target_fps);

  float start_time = GetTime();

  Player player = {.pos = constants.player_start,
                   .speed = constants.player_base_speed,
                   .size = constants.player_base_size,
                   .colour = constants.player_colour,
                   .projectile_fire_interval = constants.player_base_projectile_fire_interval,
                   .projectile_speed = constants.player_base_projectile_speed,
                   .projectile_size = constants.player_base_projectile_size,
                   .projectile_colour = constants.player_projectile_colour,
                   .time_of_last_projectile = start_time};

  EnemyManager enemy_manager = {.enemies = calloc(constants.max_enemies, sizeof *(enemy_manager.enemies)),
                                .enemy_count = 0,
                                .capacity = constants.max_enemies,
                                .enemy_spawn_interval = get_random_float(constants.enemy_spawn_interval_min,
                                                                         constants.enemy_spawn_interval_max),
                                .time_of_last_spawn = start_time,
                                .time_of_last_update = start_time};

  ProjectileManager projectile_manager = {
      .projectiles = calloc(constants.max_projectiles, sizeof *(projectile_manager.projectiles)),
      .projectile_count = 0,
      .capacity = constants.max_projectiles};
  /*-------------------------------------------------------------------------------------------------------------*/

  while (!WindowShouldClose()) {
    /*--------*/
    /* Update */
    /*-----------------------------------------------------------------------------------------------------------*/
    player_update_position(&player, constants);

    player_try_to_spawn_projectile(&player, &projectile_manager);
    projectile_manager_check_for_collisions_with_enemies(&projectile_manager, &enemy_manager);
    projectile_manager_update_projectile_positions(&projectile_manager, constants);

    enemy_manager_try_to_spawn_enemy(&enemy_manager, player, constants);
    enemy_manager_update_desired_positions(&enemy_manager, player, constants);
    enemy_manager_update_enemy_positions(&enemy_manager);
    /*-----------------------------------------------------------------------------------------------------------*/

    /*---------*/
    /* Drawing */
    /*-----------------------------------------------------------------------------------------------------------*/
    BeginDrawing();
    {
      ClearBackground(RAYWHITE);

      draw_projectiles(projectile_manager);
      draw_enemies(enemy_manager);
      draw_player(player);
    }
    EndDrawing();
    /*-----------------------------------------------------------------------------------------------------------*/
  }

  /*-------------------*/
  /* De-initialisation */
  /*-------------------------------------------------------------------------------------------------------------*/
  CloseWindow();

  free(enemy_manager.enemies);
  free(projectile_manager.projectiles);
  /*-------------------------------------------------------------------------------------------------------------*/

  return EXIT_SUCCESS;
}
