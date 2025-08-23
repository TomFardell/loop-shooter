#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"

// Makefile can set DEBUG level
#ifndef DEBUG
#define DEBUG 0
#endif

typedef enum GameScreen { GAME_SCREEN_START, GAME_SCREEN_GAME, GAME_SCREEN_SHOP, GAME_SCREEN_END } GameScreen;
typedef enum ButtonState { BUTTON_STATE_DEFAULT, BUTTON_STATE_HOVER, BUTTON_STATE_PRESSED } ButtonState;
typedef enum AnchorPosition {
  ANCHOR_TOP_LEFT,
  ANCHOR_TOP_CENTRE,
  ANCHOR_TOP_RIGHT,
  ANCHOR_CENTRE_LEFT,
  ANCHOR_CENTRE,
  ANCHOR_CENTRE_RIGHT,
  ANCHOR_BOTTOM_LEFT,
  ANCHOR_BOTTOM_CENTRE,
  ANCHOR_BOTTOM_RIGHT
} AnchorType;

/*---------*/
/* Structs */
/*---------------------------------------------------------------------------------------------------------------*/
typedef struct GameColours {
  Color red_1;
  Color red_2;
  Color red_3;

  Color blue_1;
  Color blue_2;
  Color blue_3;
  Color blue_4;

  Color green_1;
  Color green_2;
  Color green_3;

  Color yellow_1;
  Color yellow_2;
  Color yellow_3;

  Color pink_1;
  Color pink_2;

  Color brown_1;
  Color brown_2;

  Color white;
  Color grey_1;
  Color grey_2;
  Color grey_3;
  Color grey_4;
  Color grey_5;
  Color grey_6;
  Color black;
} GameColours;

typedef struct Constants {
  GameColours *game_colours;          // Pointer to location of the game's colour palette
  Vector2 initial_window_resolution;  // Initial game window dimensions in pixels
  float aspect_ratio;                 // Aspect ratio to keep the game at (we draw black bars to maintain this)
  Vector2 screen_dimensions;          // Dimensions of the displayed portion of the play space in units
  Vector2 game_area_dimensions;       // Dimensions of the game area (in units)

  int target_fps;  // Target frames per second of the game

  Vector2 player_start_pos;  // Starting position of the player
  float player_base_speed;   // Initial speed of the player
  float player_base_size;    // Initial radius of the player circle
  Color player_colour;       // Colour of the player circle

  float player_base_firerate;          // Initial firerate of the player's shots (shots per second)
  float player_base_projectile_speed;  // Initial speed at which the player's projectiles travel
  float player_base_projectile_size;   // Initial size of the player's projectiles
  Color player_projectile_colour;      // Colour of the player's projectiles

  float upgrade_cost_multiplier;  // Multiplier applied to the cost of successive upgrades in the shop

  int initial_max_enemies;           // Maximum number of enemies. Enemies stop spawning when this is reached
  int num_enemy_types;               // How many different types of enemies are in existence
  float enemy_spawn_interval_min;    // Minimum time between enemy spawns
  float enemy_spawn_interval_max;    // Maximum time between enemy spawns
  float enemy_first_spawn_interval;  // Time to spawn the first wave of enemies
  int enemy_spawn_min_wave_size;     // Minimum number of enemies to spawn at once
  float enemy_spawn_additional_enemy_chance;  // Chance to add additional enemies to the wave
  float initial_enemy_credits;                // Number of starting credits (so the fist wave doesn't take ages)
  float enemy_credit_multiplier;              // Multiplicative coefficient in enemy credit calculation
  float enemy_credit_exponent;                // Exponent in enemy credit calculation

  float enemy_update_interval;  // Time interval between attempts at updating the enemy's desired position
  float enemy_update_chance;    // Chance (each update) that the enemy updates its desired position

  int initial_max_projectiles;  // Maximum number of projectiles. This number should not be reached

  Font game_font;                           // Font used for in-game text
  float font_spacing;                       // Spacing of the in-game font
  float background_square_size;             // Side length (in units) of the squares in the background of the game
  Color background_square_colour;           // Colour of the squares in the background of the game
  Color background_colour;                  // Colour of the background of the game
  Color boss_health_bar_colour;             // Colour of the bar in the boss health bar
  Color boss_health_bar_background_colour;  // Colour of the background in the boss health bar
  unsigned char boss_health_bar_opacity;    // Opacity of the boss health bar (out of 256)
} Constants;

typedef struct Player {
  Vector2 pos;  // Current position of the player

  float speed;         // Speed of the player's movement
  float size;          // Radius of the player circle
  Color colour;        // Colour of the player
  int score;           // Score of the player in this game loop
  int boss_points;     // Boss points aquired by the player this game loop
  bool is_defeated;    // Whether the player is defeated and the game should end
  bool is_invincible;  // Whether the player is invincible and cannot be defeated

  float firerate;                 // Firerate of the player's shots (shots per second)
  float projectile_speed;         // Speed at which the player's projectiles travel
  float projectile_size;          // Radius of the player's projectile circles
  Color projectile_colour;        // Colour of the player's projectiles
  float time_of_last_projectile;  // Time at which the most recent projectile was fired
} Player;

// TODO: Add boss points upgrades
typedef struct Upgrade {
  float cost;            // Cost of the next upgrade purchase
  float stat_increment;  // Increment of the stat being upgraded (as fraction of the base value)
  float base_stat;       // Base value of the stat being upgraded
  float *stat;           // Pointer to the stat to adjust
} Upgrade;

#define NUM_UPGRADES 3
typedef struct Shop {
  int money;                       // Amount of money the player has
  int boss_points;                 // Amount of boss points the player has
  Upgrade upgrades[NUM_UPGRADES];  // Array of upgrades available in the shop
} Shop;

typedef struct EnemyType {
  float credit_cost;                   // Number of enemy manager credits this enemy type costs
  float min_speed;                     // Minimum speed of this type of enemy
  float max_speed;                     // Maximum speed of this type of enemy
  float min_size;                      // Minimum size of this type of enemy
  float max_size;                      // Maximum size of this type of enemy
  Color colour;                        // Colour of this type of enemy
  const struct EnemyType *turns_into;  // Pointer to the type of enemy that this enemy turns into upon death
} EnemyType;

typedef struct Enemy {
  Vector2 pos;          // Current position of the enemy
  Vector2 desired_pos;  // Position that the enemy will try to move towards
  bool is_active;       // Whether the enemy is processed and drawn

  float speed;  // Speed at which the enemy moves (towards its desired position)
  float size;   // Radius of the enemy circle

  const EnemyType *type;  // Pointer to the type of the enemy
} Enemy;

typedef struct BossType {
  int initial_score_to_spawn;  // Spawn the boss when this score is reached
  float max_health;            // Maximum health of the boss
  float speed;                 // Speed of the boss
  float size;                  // Size (radius) of the boss
  Color colour;                // Colour of the boss

  float firerate;           // Firerate of the boss's shots (shots per second)
  int shots_per_burst;      // Number of shots in each burst the boss fires
  float projectile_speed;   // Speed at which the boss's projectiles travel
  float projectile_size;    // Radius of the boss's projectile circles
  Color projectile_colour;  // Colour of the boss's projectiles

  float moving_duration;      // Duration of the moving part of the boss's movement cycle (in seconds)
  float stationary_duration;  // Duration of the stationary part of the boss's movement cycle (in seconds)

  int num_enemies_spawned_on_defeat;  // Number of enemies spawned when the boss is defeated
  int boss_points_on_defeat;          // Number of boss points awarded to the player when the boss is defeated
  int score_on_defeat;                // Number of points awarded to the player when the boss is defeated
} BossType;

typedef enum BossState { MOVING, STATIONARY } BossState;
typedef struct Boss {
  Vector2 pos;               // Current position of the boss
  Vector2 desired_pos;       // Position that the boss will move towards
  BossState state;           // Current state of the boss
  bool is_active;            // Whether the boss is currently active in the game
  bool is_defeated;          // Whether the boss has been defeated and death actions need to take place
  int score_for_next_spawn;  // Player score required to next spawn the boss

  float health;                     // Current health of the boss
  int shots_left_in_burst;          // Remaining shots in the current burst of shots fired by the boss
  float time_of_last_projectile;    // Time at which the most recent projectile was fired
  float time_of_last_state_switch;  // Time at which the boss last switched between moving and being stationary

  const BossType *boss_type;  // Pointer to the boss type of the boss
} Boss;

typedef struct EnemyManager {
  Enemy *enemies;   // Pointer to array of enemies
  int enemy_count;  // Number of active enemies in the array
  int capacity;     // Capacity of the enemy array

  float enemy_spawn_interval;    // Number of seconds between spawns of enemies
  float time_of_last_spawn;      // Time of the last enemy spawn (in seconds since the start of the game)
  float credits_spent;           // Number of credits spent (used in credit calculation)
  float time_of_initialisation;  // Time of the enemy manager's initialisation (used in credit calculation)

  float time_of_last_update;  // Time of the last update of enemy positions
} EnemyManager;

typedef enum ProjectileAllegiance { PLAYER, ENEMIES } ProjectileAllegiance;
typedef struct Projectile {
  Vector2 pos;                      // Current position of the projectile
  Vector2 dir;                      // Movement direction of the projectile. Should always be normalised
  bool is_active;                   // Whether the projectile is processed and drawn
  ProjectileAllegiance allegiance;  // Allegiance of the projectile (so it doesn't damage allies)

  float speed;   // Speed at which the projectile moves (in its movement direction)
  float size;    // Radius of the projectile circle
  Color colour;  // Colour of the projectile circle
} Projectile;

typedef struct ProjectileManager {
  Projectile *projectiles;  // Pointer to array of projectiles
  int projectile_count;     // Number of active projectiles in the array
  int capacity;             // Capacity of the projectile array
} ProjectileManager;

typedef struct Button {
  Rectangle bounds;        // Rectangle containing the bounds of the button (for pressing and drawing)
  AnchorType anchor_type;  // Type of anchor for displaying the button
  const char *text;        // Text to display inside the button
  ButtonState state;       // Pressed/hovered state of the button
  bool was_pressed;        // Whether the button was pressed and the press action should take place

  Color body_colour_default;  // Colour of the button when not hovered over or pressed
  Color body_colour_hover;    // Colour of the button when hovered over
  Color body_colour_pressed;  // Colour of the button when pressed
  Color text_colour;          // Colour of the text inside the button
  float font_size;            // Font size of the text inside the button
} Button;
/*---------------------------------------------------------------------------------------------------------------*/

/*-----------*/
/* Utilities */
/*---------------------------------------------------------------------------------------------------------------*/

// Generate a random float in the given range (inclusive)
float get_random_float(float min, float max) {
  float mult = GetRandomValue(0, INT_MAX) / (float)INT_MAX;
  return min + mult * (max - min);
}

// Get whether a given circle with centre `pos` and radius `rad` would be showing on the screen
bool circle_is_on_screen(Vector2 pos, float rad, Vector2 camera_pos, const Constants *constants) {
  return (-rad <= pos.x - camera_pos.x && pos.x - camera_pos.x <= constants->screen_dimensions.x + rad) &&
         (-rad <= pos.y - camera_pos.y && pos.y - camera_pos.y <= constants->screen_dimensions.y + rad);
};

bool circle_is_in_game_area(Vector2 pos, float rad, const Constants *constants) {
  return (-constants->game_area_dimensions.x / 2 <= pos.x + rad &&
          pos.x - rad <= constants->game_area_dimensions.x / 2) &&
         (-constants->game_area_dimensions.y / 2 <= pos.y + rad &&
          pos.y - rad <= constants->game_area_dimensions.y / 2);
}

// Get the centre of a given rectangle given in vectors form
Vector2 get_rectangle_centre_v(Vector2 pos, Vector2 dimensions) {
  return (Vector2){pos.x + 0.5 * dimensions.x, pos.y + 0.5 * dimensions.y};
}

// Get the centre of a given rectangle given in rectangle form
Vector2 get_rectangle_centre_rec(Rectangle rec) {
  return get_rectangle_centre_v((Vector2){rec.x, rec.y}, (Vector2){rec.width, rec.height});
}

// Given a position relative to an anchor and rectangle dimensions, get the actual position of the top left corner
Vector2 get_pos_from_anchored_vectors(Vector2 anchored_pos, Vector2 dimensions, AnchorType anchor_type,
                                      const Constants *constants) {
  // Due to the ordering of the enum, can use division with remainder to convert the anchor type to a grid position
  int grid_x = anchor_type % 3;
  int grid_y = anchor_type / 3;

  // With multiplicities depending on anchor, shift forwards by screen dimensions, and back by rectangle dimensions
  return (Vector2){anchored_pos.x + 0.5 * grid_x * (constants->screen_dimensions.x - dimensions.x),
                   anchored_pos.y + 0.5 * grid_y * (constants->screen_dimensions.y - dimensions.y)};
}

// Given a rectangle whose position is relative to an anchor, get the actual position of the top left corner
Vector2 get_pos_from_anchored_rect(Rectangle anchored_rect, AnchorType anchor_type, const Constants *constants) {
  return get_pos_from_anchored_vectors((Vector2){anchored_rect.x, anchored_rect.y},
                                       (Vector2){anchored_rect.width, anchored_rect.height}, anchor_type,
                                       constants);
}

// Get the scale of one of the two black bars required to maintain the desired aspect ratio. A positive return
// value indicates the width of the required vertical bars. A negative return value indicates the (negative) height
// of the required horizontal bars
float get_black_bar_size_in_pixels(const Constants *constants) {
  float screen_width = GetScreenWidth();
  float screen_height = GetScreenHeight();
  float aspect_ratio = screen_width / screen_height;

  // No black bars if the aspect ratios are (approximately) equal
  if (FloatEquals(aspect_ratio, constants->aspect_ratio)) return 0;

  if (aspect_ratio > constants->aspect_ratio) {  // If the window is too wide
    return 0.5 * (screen_width - constants->aspect_ratio * screen_height);
  } else {  // If the window is too tall
    return -0.5 * (screen_height - (1 / constants->aspect_ratio) * screen_width);
  }
}

// Get the scale factor required to convert a measurement in units to one in pixels
float get_units_to_pixels_scale_factor(const Constants *constants) {
  float black_bar_size = get_black_bar_size_in_pixels(constants);

  // Calculate screen size in pixels (i.e. the viewable portion of the window) divided by screen size in units
  if (black_bar_size >= 0) {  // If there are vertical (or no) black bars, use height
    return GetScreenHeight() / constants->screen_dimensions.y;
  } else {  // If there are horizontal black bars, use width
    return GetScreenWidth() / constants->screen_dimensions.x;
  }
}
// Convert a position in units to a position in pixels (for drawing), accounting for black bars
Vector2 get_draw_position_from_unit_position(Vector2 unit_position, const Constants *constants) {
  float black_bar_size = get_black_bar_size_in_pixels(constants);
  float scale_factor = get_units_to_pixels_scale_factor(constants);

  if (black_bar_size >= 0) {  // If there are vertical (or no) black bars
    return (Vector2){unit_position.x * scale_factor + black_bar_size, unit_position.y * scale_factor};
  } else {                             // If there are horizontal black bars
    black_bar_size = -black_bar_size;  // Get the actual height of the black bars

    return (Vector2){unit_position.x * scale_factor, unit_position.y * scale_factor + black_bar_size};
  }
}
// Convert a position in pixels (such as from GetMousePosition) to a position in units
Vector2 get_unit_position_from_draw_position(Vector2 draw_position, const Constants *constants) {
  float black_bar_size = get_black_bar_size_in_pixels(constants);
  float scale_factor = 1 / get_units_to_pixels_scale_factor(constants);

  if (black_bar_size >= 0) {  // If there are vertical (or no) black bars
    return (Vector2){(draw_position.x - black_bar_size) * scale_factor, draw_position.y * scale_factor};
  } else {                             // If there are horizontal black bars
    black_bar_size = -black_bar_size;  // Get the actual height of the black bars

    return (Vector2){draw_position.x * scale_factor, (draw_position.y - black_bar_size) * scale_factor};
  }
}

// Given dimensions in units, convert to dimensions in pixels
Vector2 get_draw_dimensions_from_unit_dimensions(Vector2 unit_dimensions, const Constants *constants) {
  return Vector2Scale(unit_dimensions, get_units_to_pixels_scale_factor(constants));
}

// Given dimensions in pixels (such as from MeasureTextEx), convert to dimensions in units
Vector2 get_unit_dimensions_from_draw_dimensions(Vector2 draw_dimensions, const Constants *constants) {
  return Vector2Scale(draw_dimensions, 1 / get_units_to_pixels_scale_factor(constants));
}

// Convert a length in units to the length in pixels when drawn to the screen
float get_draw_length_from_unit_length(float length, const Constants *constants) {
  return length * get_units_to_pixels_scale_factor(constants);
}

// GetMousePosition from raylib, but with the result in units (and accounting for the camera)
Vector2 get_mouse_position_in_units_game(Vector2 camera_position, const Constants *constants) {
  return Vector2Add(get_unit_position_from_draw_position(GetMousePosition(), constants), camera_position);
}

// GetMousePosition from raylib, but with the result in units, ignoring the camera position
Vector2 get_mouse_position_in_units_ui(const Constants *constants) {
  return get_unit_position_from_draw_position(GetMousePosition(), constants);
}

// MeasureTextEx from raylib, but with the result in units
Vector2 measure_text_ex_in_units(Font font, const char *text, float size, float spacing,
                                 const Constants *constants) {
  return get_unit_dimensions_from_draw_dimensions(
      MeasureTextEx(font, text, get_draw_length_from_unit_length(size, constants), spacing), constants);
}

// Get the normalised vector for the direction the player should move according to keyboard input
Vector2 get_movement_input_direction() {
  Vector2 res = {0};
  if (IsKeyDown(KEY_S)) res.y++;
  if (IsKeyDown(KEY_W)) res.y--;
  if (IsKeyDown(KEY_D)) res.x++;
  if (IsKeyDown(KEY_A)) res.x--;

  return Vector2Normalize(res);  // Normalise to prevent diagonal movement being quicker
}
/*---------------------------------------------------------------------------------------------------------------*/

/*------------*/
/* Game setup */
/*---------------------------------------------------------------------------------------------------------------*/

// Set up initial game objects. Should be called once at the start of the program and passed zeroed game objects
void initialise_game(Player *player, EnemyManager *enemy_manager, ProjectileManager *projectile_manager,
                     const Constants *constants) {
  player->speed = constants->player_base_speed;
  player->size = constants->player_base_size;
  player->colour = constants->player_colour;
  player->firerate = constants->player_base_firerate;
  player->projectile_speed = constants->player_base_projectile_speed;
  player->projectile_size = constants->player_base_projectile_size;
  player->projectile_colour = constants->player_projectile_colour;

  enemy_manager->enemies = calloc(constants->initial_max_enemies, sizeof *(enemy_manager->enemies));
  if (!enemy_manager->enemies) {
    fprintf(stderr, "Unable to allocate enemy storage.\n");
    exit(EXIT_FAILURE);
  }
  enemy_manager->capacity = constants->initial_max_enemies;

  projectile_manager->projectiles =
      calloc(constants->initial_max_projectiles, sizeof *(projectile_manager->projectiles));
  if (!projectile_manager->projectiles) {
    fprintf(stderr, "Unable to allocate projectile storage.\n");
    exit(EXIT_FAILURE);
  }
  projectile_manager->capacity = constants->initial_max_projectiles;
}

// Perform initialisation steps for game start
void start_game(Player *player, EnemyManager *enemy_manager, ProjectileManager *projectile_manager, Boss *boss,
                const Constants *constants) {
  float start_time = GetTime();
  player->pos = constants->player_start_pos;
  player->score = 0;
  player->boss_points = 0;
  player->is_defeated = false;
  player->time_of_last_projectile = start_time;

  memset(enemy_manager->enemies, 0, enemy_manager->capacity * sizeof *(enemy_manager->enemies));
  enemy_manager->enemy_count = 0;
  enemy_manager->enemy_spawn_interval = constants->enemy_first_spawn_interval;
  enemy_manager->time_of_last_spawn = start_time;
  enemy_manager->credits_spent = 0;
  enemy_manager->time_of_initialisation = start_time;
  enemy_manager->time_of_last_update = start_time;

  memset(projectile_manager->projectiles, 0,
         projectile_manager->capacity * sizeof *(projectile_manager->projectiles));
  projectile_manager->projectile_count = 0;

  // Most stats are set when boss is spawned
  boss->is_active = false;
  boss->is_defeated = false;
  boss->score_for_next_spawn = boss->boss_type->initial_score_to_spawn;
}

// Perform actions when this instance of the game ends
void end_game(Player *player, EnemyManager *enemy_manager, ProjectileManager *projectile_manager, Shop *shop) {
  shop->money += player->score;
  shop->boss_points += player->boss_points;
}

// Clean up game objects when the program ends
void cleanup_game(EnemyManager *enemy_manager, ProjectileManager *projectile_manager) {
  free(enemy_manager->enemies);
  enemy_manager->enemies = NULL;

  free(projectile_manager->projectiles);
  projectile_manager->projectiles = NULL;
}
/*---------------------------------------------------------------------------------------------------------------*/

/*-----------------------*/
/* Projectile management */
/*---------------------------------------------------------------------------------------------------------------*/

// Add a projectile to the projectile manager's storage, doubling its size if it is full
void projectile_manager_add_projectile(ProjectileManager *projectile_manager, Projectile projectile) {
  // If the projectile manager would become full, double its size
  while (projectile_manager->projectile_count + 1 > projectile_manager->capacity) {
    projectile_manager->capacity *= 2;
    projectile_manager->projectiles = realloc(
        projectile_manager->projectiles, projectile_manager->capacity * sizeof *(projectile_manager->projectiles));
  }

  for (int i = 0; i < projectile_manager->capacity; i++) {
    if (!projectile_manager->projectiles[i].is_active) {
      projectile_manager->projectiles[i] = projectile;
      projectile_manager->projectile_count++;
      return;
    }

    // Check that the loop doesn't terminate without finding an inactive projectile (this is the final i)
    assert((i != projectile_manager->capacity - 1) && "No inactive projectile found");
  }
}

// Update projectile positions according to their trajectories
void projectile_manager_update_projectile_positions(ProjectileManager *projectile_manager, Vector2 camera_position,
                                                    const Constants *constants) {
  for (int i = 0, projectiles_counted = 0;
       i < projectile_manager->capacity && projectiles_counted < projectile_manager->projectile_count; i++) {
    Projectile *this_projectile = projectile_manager->projectiles + i;
    if (!this_projectile->is_active) continue;

    projectiles_counted++;

    // Move the projectile along its trajectory according to its speed
    this_projectile->pos = Vector2Add(this_projectile->pos,
                                      Vector2Scale(this_projectile->dir, this_projectile->speed * GetFrameTime()));

    // If the projectile has moved outside the game boundaries, make it inactive
    if (!circle_is_in_game_area(this_projectile->pos, this_projectile->size, constants)) {
      this_projectile->is_active = false;
      projectile_manager->projectile_count--;
    }
  }
}

// Check for collisions between projectiles and objects of opposing allegiance
void projectile_manager_check_for_collisions(ProjectileManager *projectile_manager, EnemyManager *enemy_manager,
                                             Player *player, Boss *boss) {
  for (int i = 0, projectiles_counted = 0;
       i < projectile_manager->capacity && projectiles_counted < projectile_manager->projectile_count; i++) {
    Projectile *this_projectile = projectile_manager->projectiles + i;
    if (!this_projectile->is_active) continue;

    projectiles_counted++;

    switch (this_projectile->allegiance) {
      case PLAYER:
        // Check for collisions with enemies
        for (int j = 0, enemies_counted = 0;
             j < enemy_manager->capacity && enemies_counted < enemy_manager->enemy_count; j++) {
          Enemy *this_enemy = enemy_manager->enemies + j;
          if (!this_enemy->is_active) continue;

          enemies_counted++;

          if (!CheckCollisionCircles(this_projectile->pos, this_projectile->size, this_enemy->pos,
                                     this_enemy->size))
            continue;

          this_projectile->is_active = false;
          projectile_manager->projectile_count--;

          // If we are not at the base enemy type, decay into the next type in the chain
          if (this_enemy->type->turns_into) {
            this_enemy->type = this_enemy->type->turns_into;
            this_enemy->speed = get_random_float(this_enemy->type->min_speed, this_enemy->type->max_speed);
          } else {  // Otherwise destroy the enemy
            this_enemy->is_active = false;
            enemy_manager->enemy_count--;
          }

          player->score++;
          break;  // Exit the enemy loop so the projectile doesn't destroy a second enemy
        }

        // Check for a collision with the boss
        if (!boss->is_active) break;
        if (!CheckCollisionCircles(this_projectile->pos, this_projectile->size, boss->pos, boss->boss_type->size))
          break;

        this_projectile->is_active = false;
        projectile_manager->projectile_count--;

        boss->health--;
        if (boss->health <= 0) {
          boss->is_defeated = true;
        }

        break;
      case ENEMIES:
        if (!CheckCollisionCircles(this_projectile->pos, this_projectile->size, player->pos, player->size)) break;

        player->is_defeated = true;

        this_projectile->is_active = false;
        projectile_manager->projectile_count--;
    }
  }
}
/*---------------------------------------------------------------------------------------------------------------*/

/*----------------*/
/* Player actions */
/*---------------------------------------------------------------------------------------------------------------*/

// Update the player's position according to keyboard input
void player_update_position(Player *player, const Constants *constants) {
  player->pos =
      Vector2Add(player->pos, Vector2Scale(get_movement_input_direction(), player->speed * GetFrameTime()));

  // Clamp the player inside the screen boundaries
  Vector2 min_player_pos = Vector2Subtract(Vector2Scale(Vector2One(), player->size),
                                           Vector2Scale(constants->game_area_dimensions, 0.5));
  Vector2 max_player_pos = Vector2Negate(min_player_pos);  // Game area dimensions are symmetrical
  player->pos = Vector2Clamp(player->pos, min_player_pos, max_player_pos);
}

// Generate a new projectile that moves towards the mouse
Projectile projectile_generate_from_player(const Player *player, Vector2 camera_position,
                                           const Constants *constants) {
  Projectile projectile = {.pos = player->pos,
                           .is_active = true,
                           .allegiance = PLAYER,
                           .speed = player->projectile_speed,
                           .size = player->projectile_size,
                           .colour = player->projectile_colour};

  Vector2 mouse_pos = get_mouse_position_in_units_game(camera_position, constants);

  // If the mouse is on the player, just fire in an arbitrary direction, otherwise fire towards the mouse
  if (Vector2Equals(mouse_pos, player->pos))
    projectile.dir = (Vector2){1, 0};  // Arbitrarily choose to shoot to the right
  else
    projectile.dir = Vector2Subtract(mouse_pos, player->pos);

  projectile.dir = Vector2Normalize(projectile.dir);

  return projectile;
}

// Spawn a new projectile when it is time to do so and if the correct button is down
void player_try_to_fire_projectile(Player *player, ProjectileManager *projectile_manager, Vector2 camera_position,
                                   const Constants *constants) {
  // If the mouse button isn't held, do nothing
  if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) return;

  // If it has not been long enough since the last shot, do nothing
  float time_since_last_projectile = GetTime() - player->time_of_last_projectile;
  if (time_since_last_projectile < 1 / player->firerate) return;

  Projectile projectile = projectile_generate_from_player(player, camera_position, constants);
  projectile_manager_add_projectile(projectile_manager, projectile);

  player->time_of_last_projectile = GetTime();
}

void player_check_for_defeat(Player *player, GameScreen game_screen) {}
/*---------------------------------------------------------------------------------------------------------------*/

/*------------------*/
/* Enemy management */
/*---------------------------------------------------------------------------------------------------------------*/

// Add an enemy to the enemy manager's storage, doubling its size if necessary
void enemy_manager_add_enemy(EnemyManager *enemy_manager, Enemy enemy) {
  // If the enemy manager would become full, double its capacity
  while (enemy_manager->enemy_count + 1 > enemy_manager->capacity) {
    enemy_manager->capacity *= 2;
    enemy_manager->enemies =
        realloc(enemy_manager->enemies, enemy_manager->capacity * sizeof *(enemy_manager->enemies));
  }

  // Loop through the enemy slots until an inactive enemy is found and replace with an enemy of the desired type
  for (int j = 0; j < enemy_manager->capacity; j++) {
    if (!enemy_manager->enemies[j].is_active) {
      enemy_manager->enemies[j] = enemy;
      enemy_manager->enemy_count++;
      break;
    }

    // Check that the loop doesn't terminate without finding an inactive enemy (this is the final j)
    assert((j != enemy_manager->capacity - 1) && "No inactive enemy found");
  }
}

// Enemy manager credits, at time t and before spending, are given by: credits = mult * t ^ exp,
// where mult and exp are constants defined at game initialisation
float enemy_manager_calculate_credits(const EnemyManager *enemy_manager, const Constants *constants) {
  float t = GetTime() - enemy_manager->time_of_initialisation;
  return constants->enemy_credit_multiplier * powf(t, constants->enemy_credit_exponent) -
         enemy_manager->credits_spent + constants->initial_enemy_credits;
}

// Randomly generate a starting position of an enemy. Enemies spawn in the game area but off the screen
Vector2 get_random_enemy_start_position(float enemy_size, Vector2 camera_position, const Constants *constants) {
  // Using a for loop here is fine as it is unlikely to run more than a couple of times
  for (;;) {
    Vector2 position = {
        get_random_float(-constants->game_area_dimensions.x / 2, constants->game_area_dimensions.x / 2),
        get_random_float(-constants->game_area_dimensions.y / 2, constants->game_area_dimensions.y / 2)};

    if (!circle_is_on_screen(position, enemy_size, camera_position, constants)) return position;
  }
}

// Randomly generate a new enemy
Enemy enemy_generate(const EnemyType *enemy_type, const Player *player, Vector2 camera_position,
                     const Constants *constants) {
  Enemy enemy = {.desired_pos = player->pos,
                 .is_active = true,
                 .speed = get_random_float(enemy_type->min_speed, enemy_type->max_speed),
                 .size = get_random_float(enemy_type->min_size, enemy_type->max_size),
                 .type = enemy_type};

  enemy.pos = get_random_enemy_start_position(enemy.size, camera_position, constants);

  return enemy;
}

// Try to create and spawn a new wave of enemies (if it is time to do so)
void enemy_manager_try_to_spawn_enemies(EnemyManager *enemy_manager, const EnemyType *enemy_types,
                                        const Player *player, Vector2 camera_position,
                                        const Constants *constants) {
  // If it has not been long enough since the last enemy, do nothing
  float time_since_last_enemy = GetTime() - enemy_manager->time_of_last_spawn;
  if (time_since_last_enemy < enemy_manager->enemy_spawn_interval) return;

  // If we cannot afford the minimum wave, do nothing (i.e wait a bit longer)
  float available_credits = enemy_manager_calculate_credits(enemy_manager, constants);
  int wave_size = constants->enemy_spawn_min_wave_size;
  int wave_cost = wave_size * enemy_types[0].credit_cost;
  if (wave_cost > available_credits) return;

  // Keep trying to increase the wave size until either we fail the probability check or we cannot afford the
  // wave
  while (wave_cost + enemy_types[0].credit_cost <= available_credits &&
         get_random_float(0, 1) <= constants->enemy_spawn_additional_enemy_chance) {
    wave_size++;
    wave_cost += enemy_types[0].credit_cost;
  };

  // Store wave as an array of enemy types, initialised to all be of the weakest type
  int *wave_enemy_types = calloc(wave_size, sizeof *wave_enemy_types);
  if (!wave_enemy_types) {
    fprintf(stderr, "Error allocating wave enemy types memory.\n");
    exit(EXIT_FAILURE);
  }

  bool upgrade_instead_of_add = true;                       // First iteration should upgrade the enemies
  float cheapest_action_cost = enemy_types[0].credit_cost;  // Assume the cheapest action is adding a type 0 enemy

  // While it is possible to increase the strength of the wave, continue to do so
  while (wave_cost < available_credits - cheapest_action_cost) {
    // Upgrade existing enemies
    if (upgrade_instead_of_add) {
      // Try to upgrade each enemy in the wave
      for (int i = 0; i < wave_size; i++) {
        int this_enemy_type = wave_enemy_types[i];

        // If this enemy is already of the strongest type, don't try to upgrade it
        if (this_enemy_type == constants->num_enemy_types - 1) continue;

        int this_enemy_new_type = GetRandomValue(this_enemy_type + 1, constants->num_enemy_types - 1);

        // If upgrading this enemy to this type would be too expensive, don't upgrade it
        float cost_increase =
            enemy_types[this_enemy_new_type].credit_cost - enemy_types[this_enemy_type].credit_cost;
        if (wave_cost + cost_increase > available_credits) continue;

        // Otherwise, upgrade the enemy
        wave_enemy_types[i] = this_enemy_new_type;
        wave_cost += cost_increase;
      }
    } else {  // Add more enemies
      int prev_wave_size = wave_size;

      // Add enemies in the same way as before, but now guarentee one additional enemy
      do {
        wave_size++;
        wave_cost += enemy_types[0].credit_cost;
      } while (wave_cost + enemy_types[0].credit_cost <= available_credits &&
               get_random_float(0, 1) <= constants->enemy_spawn_additional_enemy_chance);

      // Ensure that adding a type 0 enemy was in fact the cheapest action
      assert((wave_cost <= available_credits) && "Enemies added to wave exceeded credits");

      // Increase wave array size and ensure the new enemies are of type 0
      wave_enemy_types = realloc(wave_enemy_types, wave_size * sizeof *wave_enemy_types);
      if (!wave_enemy_types) {
        fprintf(stderr, "Error reallocating wave enemy types memory.\n");
        exit(EXIT_FAILURE);
      }
      memset(wave_enemy_types + prev_wave_size, 0, (wave_size - prev_wave_size) * sizeof *wave_enemy_types);
    }
    // Further iterations randomly choose to either upgrade the current enemies or add more
    upgrade_instead_of_add = GetRandomValue(0, 1);
  }

  // Add the enemies from the wave to the enemy manager
  for (int i = 0; i < wave_size; i++) {
    Enemy this_enemy = enemy_generate(enemy_types + wave_enemy_types[i], player, camera_position, constants);
    enemy_manager_add_enemy(enemy_manager, this_enemy);
  }

  enemy_manager->credits_spent += wave_cost;
  free(wave_enemy_types);
  wave_enemy_types = NULL;

  // Reset the enemy timer and generate a new interval length
  enemy_manager->time_of_last_spawn = GetTime();
  enemy_manager->enemy_spawn_interval =
      get_random_float(constants->enemy_spawn_interval_min, constants->enemy_spawn_interval_max);
}

// Update the enemies so that they move towards the player (when it is time to do so and with probability)
void enemy_manager_update_desired_positions(EnemyManager *enemy_manager, const Player *player,
                                            const Constants *constants) {
  // If it is not time to update the enemies, do nothing
  float time_since_last_update = GetTime() - enemy_manager->time_of_last_update;
  if (time_since_last_update < constants->enemy_update_interval) return;

  enemy_manager->time_of_last_update = GetTime();

  // Otherwise, iterate through the enemies and (sometimes) update their desired positions
  for (int i = 0; i < enemy_manager->capacity; i++) {
    Enemy *this_enemy = enemy_manager->enemies + i;
    if (!this_enemy->is_active) continue;

    float r_num = get_random_float(0, 1);
    if (r_num <= constants->enemy_update_chance) {
      this_enemy->desired_pos = player->pos;  // Enemy will now move towards the current position of the player
    }
  }
}

// Update the positions of active enemies and check for collisions with the player
void enemy_manager_update_enemy_positions(EnemyManager *enemy_manager, Player *player) {
  for (int i = 0, enemies_counted = 0; i < enemy_manager->capacity && enemies_counted < enemy_manager->enemy_count;
       i++) {
    Enemy *this_enemy = enemy_manager->enemies + i;
    if (!this_enemy->is_active) continue;

    enemies_counted++;  // Keep track of enemies processed so we can exit the loop early

    // Move the enemy towards its desired position according to its speed
    Vector2 normalised_move_direction =
        Vector2Normalize(Vector2Subtract(this_enemy->desired_pos, this_enemy->pos));
    this_enemy->pos =
        Vector2Add(this_enemy->pos, Vector2Scale(normalised_move_direction, this_enemy->speed * GetFrameTime()));

    // Check for the enemy colliding with the player
    if (CheckCollisionCircles(this_enemy->pos, this_enemy->size, player->pos, player->size)) {
      // Delete the enemy (not strictly necessary at the moment)
      this_enemy->is_active = false;
      enemy_manager->enemy_count--;

      player->is_defeated = true;
    }
  }
}

// If it is time to do so, spawn the boss
void boss_try_to_spawn(Boss *boss, const Player *player, Vector2 camera_position, const Constants *constants) {
  if (boss->is_active) return;
  if (player->score < boss->score_for_next_spawn) return;

  boss->pos = get_random_enemy_start_position(boss->boss_type->size, camera_position, constants);
  boss->desired_pos = player->pos;
  boss->is_active = true;
  boss->health = boss->boss_type->max_health;
  boss->time_of_last_projectile = GetTime();
}

// If it is time to do so, change the boss between moving and being stationary
void boss_try_to_switch_states(Boss *boss, const Player *player) {
  if (!boss->is_active) return;

  if (boss->state == MOVING && GetTime() - boss->time_of_last_state_switch >= boss->boss_type->moving_duration) {
    boss->state = STATIONARY;
    boss->time_of_last_state_switch = GetTime();
    boss->shots_left_in_burst = boss->boss_type->shots_per_burst;
    boss->time_of_last_projectile = GetTime();
  }

  if (boss->state == STATIONARY &&
      GetTime() - boss->time_of_last_state_switch >= boss->boss_type->stationary_duration) {
    boss->state = MOVING;
    boss->time_of_last_state_switch = GetTime();
    boss->desired_pos = player->pos;
  }
}

// If the boss is moving, update its position
void boss_update_position(Boss *boss, Player *player) {
  if (!boss->is_active) return;

  // Check for the boss colliding with the player (even if the boss is stationary)
  if (CheckCollisionCircles(boss->pos, boss->boss_type->size, player->pos, player->size)) {
    boss->is_active = false;  // Deactivate the boss (not strictly necessary at the moment)
    player->is_defeated = true;
  }

  if (boss->state != MOVING) return;

  Vector2 normalised_move_direction = Vector2Normalize(Vector2Subtract(boss->desired_pos, boss->pos));
  boss->pos =
      Vector2Add(boss->pos, Vector2Scale(normalised_move_direction, boss->boss_type->speed * GetFrameTime()));
}

// Generate a new boss projectile that moves towards the player
Projectile projectile_generate_from_boss(const Boss *boss, const Player *player) {
  // TODO: Make projectiles spawn at the surface of the boss rather than the centre
  return (Projectile){.pos = boss->pos,
                      .dir = Vector2Normalize(Vector2Subtract(player->pos, boss->pos)),
                      .is_active = true,
                      .allegiance = ENEMIES,
                      .speed = boss->boss_type->projectile_speed,
                      .size = boss->boss_type->projectile_size,
                      .colour = boss->boss_type->projectile_colour};
}

// If it is time to do so, fire projectiles at the player
void boss_try_to_fire_projectile(Boss *boss, ProjectileManager *projectile_manager, const Player *player) {
  // Note we can still fire while moving
  if (!boss->is_active) return;
  if (boss->shots_left_in_burst <= 0) return;
  if (GetTime() - boss->time_of_last_projectile <= 1 / boss->boss_type->firerate) return;

  projectile_manager_add_projectile(projectile_manager, projectile_generate_from_boss(boss, player));
  boss->time_of_last_projectile = GetTime();
  boss->shots_left_in_burst--;
}

void boss_check_for_defeat(Boss *boss, Player *player) {
  if (!boss->is_defeated) return;
  boss->is_defeated = false;
  boss->is_active = false;
  player->score += boss->boss_type->score_on_defeat;
  player->boss_points += boss->boss_type->boss_points_on_defeat;
  // TODO: Think more about this formula
  boss->score_for_next_spawn = 2 * boss->boss_type->initial_score_to_spawn + player->score;

  // TODO: Spawn boss death enemies
}

/*---------------------------------------------------------------------------------------------------------------*/

/*---------------------*/
/* Camera calculations */
/*---------------------------------------------------------------------------------------------------------------*/

// Update the position of the camera (following the player without showing out of bounds area)
void camera_update_position(Vector2 *camera_position, const Player *player, const Constants *constants) {
  Vector2 minimum_position =
      Vector2Scale(Vector2Subtract(constants->screen_dimensions, constants->game_area_dimensions), 0.5);
  Vector2 maximum_position = Vector2Negate(minimum_position);
  Vector2 offset_amount = Vector2Scale(constants->screen_dimensions, 0.5);
  *camera_position = Vector2Subtract(Vector2Clamp(player->pos, minimum_position, maximum_position), offset_amount);
}

/*---------------*/
/* UI processing */
/*---------------------------------------------------------------------------------------------------------------*/

// Update the state and clicked status of the button from user input
void button_check_user_interaction(Button *button, const Constants *constants) {
  Vector2 unanchored_pos = get_pos_from_anchored_rect(button->bounds, button->anchor_type, constants);
  Rectangle unanchored_bounds = {unanchored_pos.x, unanchored_pos.y, button->bounds.width, button->bounds.height};

  if (CheckCollisionPointRec(get_mouse_position_in_units_ui(constants), unanchored_bounds)) {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
      button->state = BUTTON_STATE_PRESSED;
    else
      button->state = BUTTON_STATE_HOVER;

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) button->was_pressed = true;
  } else {
    button->state = BUTTON_STATE_DEFAULT;
  }
}

// Check if the player can afford a given upgrade, purchasing it if they can
void shop_try_to_purchase_upgrade(Shop *shop, Upgrade *upgrade, const Constants *constants) {
  int rounded_cost = lroundf(upgrade->cost);
  if (shop->money < rounded_cost) return;

  *(upgrade->stat) += upgrade->stat_increment * upgrade->base_stat;
  upgrade->cost *= constants->upgrade_cost_multiplier;
  shop->money -= rounded_cost;
}
/*---------------------------------------------------------------------------------------------------------------*/

/*---------------------*/
/* Game object drawing */
/*---------------------------------------------------------------------------------------------------------------*/

// Draw the player to the canvas
void draw_player(const Player *player, Vector2 camera_position, const Constants *constants) {
  Vector2 offset_position = Vector2Subtract(player->pos, camera_position);
  DrawCircleV(get_draw_position_from_unit_position(offset_position, constants),
              get_draw_length_from_unit_length(player->size, constants), player->colour);
}

// Draw the active enemies to the canvas
void draw_enemies(const EnemyManager *enemy_manager, Vector2 camera_position, const Constants *constants) {
  for (int i = 0, enemies_counted = 0; i < enemy_manager->capacity && enemies_counted < enemy_manager->capacity;
       i++) {
    Enemy this_enemy = enemy_manager->enemies[i];
    if (!this_enemy.is_active) continue;

    enemies_counted++;
    if (!circle_is_on_screen(this_enemy.pos, this_enemy.size, camera_position, constants)) continue;

    Vector2 offset_position = Vector2Subtract(this_enemy.pos, camera_position);
    DrawCircleV(get_draw_position_from_unit_position(offset_position, constants),
                get_draw_length_from_unit_length(this_enemy.size, constants), this_enemy.type->colour);
  }
}

void draw_boss(const Boss *boss, Vector2 camera_position, const Constants *constants) {
  if (!boss->is_active) return;
  if (!circle_is_on_screen(boss->pos, boss->boss_type->size, camera_position, constants)) return;

  Vector2 offset_position = Vector2Subtract(boss->pos, camera_position);
  DrawCircleV(get_draw_position_from_unit_position(offset_position, constants),
              get_draw_length_from_unit_length(boss->boss_type->size, constants), boss->boss_type->colour);
}

// Draw the active projectiles to the canvas
void draw_projectiles(const ProjectileManager *projectile_manager, Vector2 camera_position,
                      const Constants *constants) {
  for (int i = 0, projectiles_counted = 0;
       i < projectile_manager->capacity && projectiles_counted < projectile_manager->projectile_count; i++) {
    Projectile this_projectile = projectile_manager->projectiles[i];
    if (!this_projectile.is_active) continue;

    projectiles_counted++;
    if (!circle_is_on_screen(this_projectile.pos, this_projectile.size, camera_position, constants)) continue;

    Vector2 offset_position = Vector2Subtract(this_projectile.pos, camera_position);
    DrawCircleV(get_draw_position_from_unit_position(offset_position, constants),
                get_draw_length_from_unit_length(this_projectile.size, constants), this_projectile.colour);
  }
}

// Draw the squares in the background of the game (to give impression of movement while camera is stationary)
void draw_background_squares(Vector2 camera_position, const Constants *constants) {
  // Quotient and remainder are taken with respect to division by a square length
  Vector2 camera_quotient = {floor(camera_position.x / constants->background_square_size),
                             floor(camera_position.y / constants->background_square_size)};
  Vector2 camera_remainder =
      Vector2Subtract(camera_position, Vector2Scale(camera_quotient, constants->background_square_size));

  int screen_width_in_squares = constants->screen_dimensions.x / constants->background_square_size + 2;
  int screen_height_in_squares = constants->screen_dimensions.y / constants->background_square_size + 2;

  for (int x = 0; x < screen_width_in_squares; x++) {
    for (int y = 0; y < screen_height_in_squares; y++) {
      // Alternate in a grid pattern whether to draw the background squares
      if ((x + y + (int)camera_quotient.x + (int)camera_quotient.y) % 2 == 0) continue;

      Vector2 square_position =
          Vector2Subtract(Vector2Scale((Vector2){x, y}, constants->background_square_size), camera_remainder);
      Vector2 square_dimensions = Vector2Scale(Vector2One(), constants->background_square_size);

      DrawRectangleV(get_draw_position_from_unit_position(square_position, constants),
                     get_draw_dimensions_from_unit_dimensions(square_dimensions, constants),
                     constants->background_square_colour);
    }
  }
}
/*---------------------------------------------------------------------------------------------------------------*/

/*------------*/
/* UI drawing */
/*---------------------------------------------------------------------------------------------------------------*/

// Draw black bars on the screen to maintain the desired aspect ratio
void draw_black_bars(const Constants *constants) {
  // Note I haven't used get_black_bar_size since we still need the screen width and height to draw the bars
  float screen_width = GetScreenWidth();
  float screen_height = GetScreenHeight();
  float aspect_ratio = screen_width / screen_height;

  // Don't draw black bars if the aspect ratios are (approximately) equal
  if (FloatEquals(aspect_ratio, constants->aspect_ratio)) return;

  if (aspect_ratio > constants->aspect_ratio) {  // If the window is too wide
    float black_bar_width = 0.5 * (screen_width - constants->aspect_ratio * screen_height);
    DrawRectangle(0, 0, black_bar_width, screen_height, constants->game_colours->black);
    DrawRectangle(screen_width - black_bar_width, 0, black_bar_width, screen_height,
                  constants->game_colours->black);
  } else {  // If the window is too tall
    float black_bar_height = 0.5 * (screen_height - (1 / constants->aspect_ratio) * screen_width);
    DrawRectangle(0, 0, screen_width, black_bar_height, constants->game_colours->black);
    DrawRectangle(0, screen_height - black_bar_height, screen_width, black_bar_height,
                  constants->game_colours->black);
  }
}
// Same as DrawTextEx but with the text anchored
void draw_text_anchored(Font font, const char *text, Vector2 anchored_pos, float size, float spacing, Color colour,
                        AnchorType anchor_type, const Constants *constants) {
  Vector2 text_dimensions = measure_text_ex_in_units(font, text, size, spacing, constants);
  Vector2 adjusted_pos = get_pos_from_anchored_vectors(anchored_pos, text_dimensions, anchor_type, constants);

  DrawTextEx(font, text, get_draw_position_from_unit_position(adjusted_pos, constants),
             get_draw_length_from_unit_length(size, constants), spacing, colour);
}

// Same as DrawTextEx, but with the text centred
void draw_text_centred(Font font, const char *text, Vector2 pos, float size, float spacing, Color colour,
                       const Constants *constants) {
  Vector2 text_dimensions = measure_text_ex_in_units(font, text, size, spacing, constants);
  Vector2 adjusted_pos = {pos.x - 0.5 * text_dimensions.x, pos.y - 0.5 * text_dimensions.y};

  DrawTextEx(font, text, get_draw_position_from_unit_position(adjusted_pos, constants),
             get_draw_length_from_unit_length(size, constants), spacing, colour);
}

// Same as DrawRectangleV, but with the rectangle anchored
void draw_anchored_rectangle_v(Vector2 anchored_pos, Vector2 dimensions, Color colour, AnchorType anchor_type,
                               const Constants *constants) {
  Vector2 adjusted_pos = get_pos_from_anchored_vectors(anchored_pos, dimensions, anchor_type, constants);

  DrawRectangleV(get_draw_position_from_unit_position(adjusted_pos, constants),
                 get_draw_dimensions_from_unit_dimensions(dimensions, constants), colour);
}

// Same as DrawRectangleRec, but with the rectangle anchored
void draw_anchored_rectangle_rec(Rectangle anchored_rect, Color colour, AnchorType anchor_type,
                                 const Constants *constants) {
  draw_anchored_rectangle_v((Vector2){anchored_rect.x, anchored_rect.y},
                            (Vector2){anchored_rect.width, anchored_rect.height}, colour, anchor_type, constants);
}

// Draw an anchored button to the screen with the colour chaning depending on hover/pressed status
void draw_button(const Button *button, const Constants *constants) {
  Color body_colour;
  switch (button->state) {
    case BUTTON_STATE_DEFAULT:
      body_colour = button->body_colour_default;
      break;
    case BUTTON_STATE_HOVER:
      body_colour = button->body_colour_hover;
      break;
    case BUTTON_STATE_PRESSED:
      body_colour = button->body_colour_pressed;
      break;
  }

  draw_anchored_rectangle_rec(button->bounds, body_colour, button->anchor_type, constants);

  // We still want the text centred relative to the button, so these calculations are necessary
  Vector2 unanchored_pos = get_pos_from_anchored_rect(button->bounds, button->anchor_type, constants);
  Vector2 dimensions = {button->bounds.width, button->bounds.height};
  draw_text_centred(constants->game_font, button->text, get_rectangle_centre_v(unanchored_pos, dimensions),
                    button->font_size, constants->font_spacing, button->text_colour, constants);
}

// Draw score (and other stats if debug text button was pressed)
void draw_game_info(const Player *player, const EnemyManager *enemy_manager,
                    const ProjectileManager *projectile_manager, const Boss *boss, const Constants *constants,
                    bool show_debug_text) {
  draw_text_anchored(constants->game_font, TextFormat("Score: %d", player->score), (Vector2){0.25, 0.25}, 0.4,
                     constants->font_spacing, constants->game_colours->black, ANCHOR_TOP_LEFT, constants);

  if (!show_debug_text) return;

  float y_pos = 0.75;            // Vertical position of debug text (to allow for easier insertion of new text)
  float y_pos_increment = 0.25;  // Space between lines of debug text

  draw_text_anchored(constants->game_font, TextFormat("Player invincible: %d", player->is_invincible),
                     (Vector2){0.25, y_pos}, 0.25, constants->font_spacing, constants->game_colours->grey_5,
                     ANCHOR_TOP_LEFT, constants);
  y_pos += y_pos_increment;

  draw_text_anchored(constants->game_font,
                     TextFormat("Enemy count: %2d/%d", enemy_manager->enemy_count, enemy_manager->capacity),
                     (Vector2){0.25, y_pos}, 0.25, constants->font_spacing, constants->game_colours->grey_5,
                     ANCHOR_TOP_LEFT, constants);
  y_pos += y_pos_increment;

  draw_text_anchored(
      constants->game_font,
      TextFormat("Projectile count: %2d/%d", projectile_manager->projectile_count, projectile_manager->capacity),
      (Vector2){0.25, y_pos}, 0.25, constants->font_spacing, constants->game_colours->grey_5, ANCHOR_TOP_LEFT,
      constants);
  y_pos += y_pos_increment;

  draw_text_anchored(constants->game_font,
                     TextFormat("Enemy credits: %5.2f", enemy_manager_calculate_credits(enemy_manager, constants)),
                     (Vector2){0.25, y_pos}, 0.25, constants->font_spacing, constants->game_colours->grey_5,
                     ANCHOR_TOP_LEFT, constants);
  y_pos += y_pos_increment;

  draw_text_anchored(constants->game_font, TextFormat("Score for next boss: %d", boss->score_for_next_spawn),
                     (Vector2){0.25, y_pos}, 0.25, constants->font_spacing, constants->game_colours->grey_5,
                     ANCHOR_TOP_LEFT, constants);
  y_pos += y_pos_increment;

  draw_text_anchored(constants->game_font, TextFormat("Boss active: %d", boss->is_active), (Vector2){0.25, y_pos},
                     0.25, constants->font_spacing, constants->game_colours->grey_5, ANCHOR_TOP_LEFT, constants);
  y_pos += y_pos_increment;

  draw_text_anchored(constants->game_font, TextFormat("Boss stationary: %d", boss->state), (Vector2){0.25, y_pos},
                     0.25, constants->font_spacing, constants->game_colours->grey_5, ANCHOR_TOP_LEFT, constants);
  y_pos += y_pos_increment;

  draw_text_anchored(constants->game_font, TextFormat("Boss health: %.1f", boss->health), (Vector2){0.25, y_pos},
                     0.25, constants->font_spacing, constants->game_colours->grey_5, ANCHOR_TOP_LEFT, constants);
  y_pos += y_pos_increment;

  draw_text_anchored(constants->game_font, TextFormat("Boss shots left in burst: %d", boss->shots_left_in_burst),
                     (Vector2){0.25, y_pos}, 0.25, constants->font_spacing, constants->game_colours->grey_5,
                     ANCHOR_TOP_LEFT, constants);
  y_pos += y_pos_increment;

  draw_text_anchored(constants->game_font, TextFormat("%d", GetFPS()), (Vector2){-0.25, 0.25}, 0.35,
                     constants->font_spacing, constants->game_colours->green_2, ANCHOR_TOP_RIGHT, constants);
}

void draw_boss_health_bar(const Boss *boss, const Constants *constants) {
  if (!boss->is_active) return;

  float health_fraction = boss->health / boss->boss_type->max_health;

  Color health_colour = constants->boss_health_bar_colour;
  Color background_colour = constants->boss_health_bar_background_colour;
  health_colour.a = constants->boss_health_bar_opacity;
  background_colour.a = constants->boss_health_bar_opacity;

  float y_pos = -0.5;
  float width = 14;
  float height = 0.25;
  // Health portion of the bar
  draw_anchored_rectangle_v((Vector2){-0.5 * (1 - health_fraction) * width, y_pos},
                            (Vector2){health_fraction * width, height}, health_colour, ANCHOR_BOTTOM_CENTRE,
                            constants);
  // Background portion of the bar
  draw_anchored_rectangle_v((Vector2){0.5 * health_fraction * width, y_pos},
                            (Vector2){(1 - health_fraction) * width, height}, background_colour,
                            ANCHOR_BOTTOM_CENTRE, constants);
}

// Draw the text for the shop page
void draw_shop_text(const Shop *shop, const Player *player, const Constants *constants) {
  draw_text_anchored(constants->game_font, TextFormat("$%d", shop->money), (Vector2){-0.5, 0.5}, 0.4,
                     constants->font_spacing, constants->game_colours->yellow_3, ANCHOR_TOP_RIGHT, constants);

  Upgrade upgrade_firerate = shop->upgrades[0];
  draw_text_anchored(constants->game_font, "Firerate", (Vector2){1.25, 0.5}, 0.4, constants->font_spacing,
                     constants->game_colours->grey_6, ANCHOR_TOP_LEFT, constants);
  draw_text_anchored(
      constants->game_font,
      TextFormat("%.1f -> %.1f", player->firerate,
                 player->firerate + upgrade_firerate.stat_increment * constants->player_base_firerate),
      (Vector2){1.25, 0.9}, 0.3, constants->font_spacing, constants->game_colours->grey_4, ANCHOR_TOP_LEFT,
      constants);

  Upgrade upgrade_projectile_speed = shop->upgrades[1];
  draw_text_anchored(constants->game_font, "Projectile speed", (Vector2){1.25, 1.5}, 0.4, constants->font_spacing,
                     constants->game_colours->grey_6, ANCHOR_TOP_LEFT, constants);
  draw_text_anchored(constants->game_font,
                     TextFormat("%.1f -> %.1f", player->projectile_speed,
                                player->projectile_speed + upgrade_projectile_speed.stat_increment *
                                                               constants->player_base_projectile_speed),
                     (Vector2){1.25, 1.9}, 0.3, constants->font_spacing, constants->game_colours->grey_4,
                     ANCHOR_TOP_LEFT, constants);

  Upgrade upgrade_projectile_size = shop->upgrades[2];
  draw_text_anchored(constants->game_font, "Projectile size", (Vector2){1.25, 2.5}, 0.4, constants->font_spacing,
                     constants->game_colours->grey_6, ANCHOR_TOP_LEFT, constants);
  draw_text_anchored(constants->game_font,
                     TextFormat("%.2f -> %.2f", player->projectile_size,
                                player->projectile_size + upgrade_projectile_size.stat_increment *
                                                              constants->player_base_projectile_size),
                     (Vector2){1.25, 2.9}, 0.3, constants->font_spacing, constants->game_colours->grey_4,
                     ANCHOR_TOP_LEFT, constants);
}

// Update text for and draw purchase buttons in the shop screen
void draw_shop_purchase_buttons(Button *buttons_shop_purchase, const Shop *shop, const Constants *constants) {
  Button *button_purchase_firerate = buttons_shop_purchase;
  Button *button_purchase_projectile_speed = buttons_shop_purchase + 1;
  Button *button_purchase_projectile_size = buttons_shop_purchase + 2;

  // TextFormat results decay after TextFormat is called 4 times, so need to update this text here
  Upgrade upgrade_firerate = shop->upgrades[0];
  button_purchase_firerate->text = TextFormat("$%d", lroundf(upgrade_firerate.cost));
  draw_button(button_purchase_firerate, constants);

  Upgrade upgrade_projectile_speed = shop->upgrades[1];
  button_purchase_projectile_speed->text = TextFormat("$%d", lroundf(upgrade_projectile_speed.cost));
  draw_button(button_purchase_projectile_speed, constants);

  Upgrade upgrade_projectile_size = shop->upgrades[2];
  button_purchase_projectile_size->text = TextFormat("$%d", lroundf(upgrade_projectile_size.cost));
  draw_button(button_purchase_projectile_size, constants);
}

// Draw the text in the game over screen
void draw_game_over_text(const Player *player, const Constants *constants) {
  draw_text_anchored(constants->game_font, "GAME OVER", (Vector2){0, -3}, 0.8, constants->font_spacing,
                     constants->game_colours->red_2, ANCHOR_CENTRE, constants);
  draw_text_anchored(constants->game_font, TextFormat("Score: %d", player->score), (Vector2){0, -2}, 0.5,
                     constants->font_spacing, constants->game_colours->black, ANCHOR_CENTRE, constants);
}
/*---------------------------------------------------------------------------------------------------------------*/

int main() {
  /*--------------------------*/
  /* Constants initialisation */
  /*-------------------------------------------------------------------------------------------------------------*/
  GameColours game_colours = {
      .red_1 = GetColor(0xEF3939FF),
      .red_2 = GetColor(0xCB1A1AFF),
      .red_3 = GetColor(0x841616FF),

      .blue_1 = GetColor(0x7BE0F7FF),
      .blue_2 = GetColor(0x42A2E3FF),
      .blue_3 = GetColor(0x344CC6FF),
      .blue_4 = GetColor(0x2C257FFF),

      .green_1 = GetColor(0xC9D844FF),
      .green_2 = GetColor(0x89B431FF),
      .green_3 = GetColor(0x38801DFF),
      .yellow_1 = GetColor(0xFFD92FFF),
      .yellow_2 = GetColor(0xDFB51CFF),
      .yellow_3 = GetColor(0xC48C13FF),

      .pink_1 = GetColor(0xF89EA9FF),
      .pink_2 = GetColor(0xF26273FF),

      .brown_1 = GetColor(0x7F4511FF),
      .brown_2 = GetColor(0x5C3208FF),

      .white = GetColor(0xF6F9FFFF),
      .grey_1 = GetColor(0xDDE1E9FF),
      .grey_2 = GetColor(0xBAC1CEFF),
      .grey_3 = GetColor(0x90959DFF),
      .grey_4 = GetColor(0x66696EFF),
      .grey_5 = GetColor(0x45474AFF),
      .grey_6 = GetColor(0x313133FF),
      .black = GetColor(0x1A1B1BFF),
  };

  Constants constants = {.game_colours = &game_colours,
                         .initial_window_resolution = {1280, 720},
                         .aspect_ratio = 16.0 / 9.0,
                         .screen_dimensions = {16, 9},
                         .game_area_dimensions = {64, 64},
                         .target_fps = 240,

                         .player_start_pos = {0},
                         .player_base_speed = 7,
                         .player_base_size = 0.33,
                         .player_colour = game_colours.brown_1,

                         .player_base_firerate = 2,
                         .player_base_projectile_speed = 8,
                         .player_base_projectile_size = 0.12,
                         .player_projectile_colour = game_colours.grey_5,

                         .upgrade_cost_multiplier = 1.5,

                         .initial_max_enemies = 100,
                         .num_enemy_types = 5,
                         .enemy_spawn_interval_min = 3.5,
                         .enemy_spawn_interval_max = 4.5,
                         .enemy_first_spawn_interval = 1.0,
                         .enemy_spawn_min_wave_size = 3,
                         .enemy_spawn_additional_enemy_chance = 0.3,
                         .initial_enemy_credits = 2.8,
                         .enemy_credit_multiplier = 0.3,
                         .enemy_credit_exponent = 1.7,

                         .enemy_update_interval = 0.1,
                         .enemy_update_chance = 0.4,

                         .initial_max_projectiles = 40,

                         .font_spacing = 2,
                         .background_square_size = 2,
                         .background_colour = game_colours.white,
                         .background_square_colour = game_colours.grey_1,
                         .boss_health_bar_colour = game_colours.red_3,
                         .boss_health_bar_background_colour = game_colours.black,
                         .boss_health_bar_opacity = 180};

  const EnemyType enemy_types[] = {{.credit_cost = 1,
                                    .min_speed = 2.5,
                                    .max_speed = 3,
                                    .min_size = 0.27,
                                    .max_size = 0.29,
                                    .colour = game_colours.red_1,
                                    .turns_into = NULL},
                                   {.credit_cost = 3,
                                    .min_speed = 3,
                                    .max_speed = 3.5,
                                    .min_size = 0.28,
                                    .max_size = 0.31,
                                    .colour = game_colours.blue_2,
                                    .turns_into = enemy_types + 0},
                                   {.credit_cost = 7,
                                    .min_speed = 3.5,
                                    .max_speed = 4,
                                    .min_size = 0.30,
                                    .max_size = 0.34,
                                    .colour = game_colours.green_2,
                                    .turns_into = enemy_types + 1},
                                   {.credit_cost = 12,
                                    .min_speed = 4,
                                    .max_speed = 4.5,
                                    .min_size = 0.35,
                                    .max_size = 0.4,
                                    .colour = game_colours.yellow_2,
                                    .turns_into = enemy_types + 2},
                                   {.credit_cost = 19,
                                    .min_speed = 4.5,
                                    .max_speed = 5.5,
                                    .min_size = 0.37,
                                    .max_size = 0.44,
                                    .colour = game_colours.pink_2,
                                    .turns_into = enemy_types + 3}};

  const BossType red_boss = {.initial_score_to_spawn = 50,
                             .max_health = 20,
                             .speed = 5,
                             .size = 2.5,
                             .colour = game_colours.red_2,
                             .firerate = 4,
                             .shots_per_burst = 7,
                             .projectile_speed = 6,
                             .projectile_size = 0.2,
                             .projectile_colour = game_colours.red_3,
                             .moving_duration = 2,
                             .stationary_duration = 2,
                             .num_enemies_spawned_on_defeat = 4,
                             .boss_points_on_defeat = 3,
                             .score_on_defeat = 20};
  /*-------------------------------------------------------------------------------------------------------------*/

  /*-------------------*/
  /* UI initialisation */
  /*-------------------------------------------------------------------------------------------------------------*/
  Button button_start_screen_start = {.bounds = {0, -1, 3, 1.2},
                                      .anchor_type = ANCHOR_CENTRE,
                                      .text = "START",
                                      .body_colour_default = game_colours.green_1,
                                      .body_colour_hover = game_colours.green_2,
                                      .body_colour_pressed = game_colours.green_3,
                                      .text_colour = game_colours.black,
                                      .font_size = 0.6};

  // It is convenient for me to create some buttons by simply copying a previous button and changing some values
  Button button_start_screen_shop = button_start_screen_start;
  button_start_screen_shop.bounds.y += 2.5;
  button_start_screen_shop.text = "SHOP";
  button_start_screen_shop.body_colour_default = game_colours.yellow_1;
  button_start_screen_shop.body_colour_hover = game_colours.yellow_2;
  button_start_screen_shop.body_colour_pressed = game_colours.yellow_3;

  Button button_shop_screen_back = {.bounds = {0.25, -0.25, 1.2, 0.5},
                                    .anchor_type = ANCHOR_BOTTOM_LEFT,
                                    .text = "BACK",
                                    .body_colour_default = game_colours.red_1,
                                    .body_colour_hover = game_colours.red_2,
                                    .body_colour_pressed = game_colours.red_3,
                                    .text_colour = game_colours.black,
                                    .font_size = 0.28};

  // The first purchase button is created manually, and the others are identical but shifted down
  Button buttons_shop_purchase[NUM_UPGRADES] = {{.bounds = {0.18, 0.58, 0.94, 0.5},
                                                 .anchor_type = ANCHOR_TOP_LEFT,
                                                 .text = "price",
                                                 .body_colour_default = game_colours.green_1,
                                                 .body_colour_hover = game_colours.green_2,
                                                 .body_colour_pressed = game_colours.green_3,
                                                 .text_colour = game_colours.black,
                                                 .font_size = 0.25}};
  buttons_shop_purchase[1] = buttons_shop_purchase[0];
  buttons_shop_purchase[1].bounds.y += 1;
  buttons_shop_purchase[2] = buttons_shop_purchase[1];
  buttons_shop_purchase[2].bounds.y += 1;

  Button button_end_screen_back = button_start_screen_start;
  button_end_screen_back.bounds.y += 2.5;
  button_end_screen_back.text = "GO BACK";
  /*-------------------------------------------------------------------------------------------------------------*/

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);  // Allow window resizing on Windows
  InitWindow(constants.initial_window_resolution.x, constants.initial_window_resolution.y, "Loop Shooter");
  SetTargetFPS(constants.target_fps);

  if (DEBUG == 1) {
    printf("\n----- Game started with DEBUG = %d -----\n", DEBUG);
  }

  constants.game_font = GetFontDefault();  // Needs to come after window initialisation

  /*----------------------------*/
  /* Game object initialisation */
  /*-------------------------------------------------------------------------------------------------------------*/
  Player player = {0};
  EnemyManager enemy_manager = {0};
  ProjectileManager projectile_manager = {0};
  Boss boss = {.boss_type = &red_boss};

  Shop shop = {.money = 0,
               .boss_points = 0,
               .upgrades = {{100, 0.25, constants.player_base_firerate, &(player.firerate)},
                            {50, 0.3, constants.player_base_projectile_speed, &(player.projectile_speed)},
                            {30, 0.2, constants.player_base_projectile_size, &(player.projectile_size)}}};

  initialise_game(&player, &enemy_manager, &projectile_manager, &constants);

  Vector2 camera_position;

  bool show_debug_text = false;
  GameScreen game_screen = GAME_SCREEN_START;
  /*-------------------------------------------------------------------------------------------------------------*/

  while (!WindowShouldClose()) {
    /*--------*/
    /* Update */
    /*-----------------------------------------------------------------------------------------------------------*/
    switch (game_screen) {
      /*---------------------*/
      /* Start screen update */
      /*---------------------------------------------------------------------------------------------------------*/
      case GAME_SCREEN_START:
        button_check_user_interaction(&button_start_screen_start, &constants);
        if (button_start_screen_start.was_pressed) {
          button_start_screen_start.was_pressed = false;

          game_screen = GAME_SCREEN_GAME;
          start_game(&player, &enemy_manager, &projectile_manager, &boss, &constants);
        }

        button_check_user_interaction(&button_start_screen_shop, &constants);
        if (button_start_screen_shop.was_pressed) {
          button_start_screen_shop.was_pressed = false;

          game_screen = GAME_SCREEN_SHOP;
        }
        break;
      /*---------------------------------------------------------------------------------------------------------*/

      /*--------------------*/
      /* Game screen update */
      /*---------------------------------------------------------------------------------------------------------*/
      case GAME_SCREEN_GAME:
        player_update_position(&player, &constants);
        camera_update_position(&camera_position, &player, &constants);
        player_try_to_fire_projectile(&player, &projectile_manager, camera_position, &constants);

        enemy_manager_try_to_spawn_enemies(&enemy_manager, enemy_types, &player, camera_position, &constants);
        enemy_manager_update_desired_positions(&enemy_manager, &player, &constants);
        enemy_manager_update_enemy_positions(&enemy_manager, &player);

        boss_try_to_spawn(&boss, &player, camera_position, &constants);
        boss_try_to_switch_states(&boss, &player);
        boss_update_position(&boss, &player);
        boss_try_to_fire_projectile(&boss, &projectile_manager, &player);

        projectile_manager_check_for_collisions(&projectile_manager, &enemy_manager, &player, &boss);
        projectile_manager_update_projectile_positions(&projectile_manager, camera_position, &constants);

        boss_check_for_defeat(&boss, &player);
        player_check_for_defeat(&player, game_screen);

        if (player.is_defeated) {
          if (player.is_invincible) {
            player.is_defeated = false;
          } else {
            end_game(&player, &enemy_manager, &projectile_manager, &shop);
            game_screen = GAME_SCREEN_END;
          }
        }

        if (IsKeyPressed(KEY_B) && DEBUG >= 1) show_debug_text = !show_debug_text;
        if (IsKeyPressed(KEY_I) && DEBUG >= 1) player.is_invincible = !player.is_invincible;
        break;
      /*---------------------------------------------------------------------------------------------------------*/

      /*--------------------*/
      /* Shop screen update */
      /*---------------------------------------------------------------------------------------------------------*/
      case GAME_SCREEN_SHOP:
        button_check_user_interaction(&button_shop_screen_back, &constants);
        if (button_shop_screen_back.was_pressed) {
          button_shop_screen_back.was_pressed = false;

          game_screen = GAME_SCREEN_START;
        }

        for (int i = 0; i < NUM_UPGRADES; i++) {
          Button *this_purchase_button = buttons_shop_purchase + i;
          button_check_user_interaction(this_purchase_button, &constants);
          if (this_purchase_button->was_pressed) {
            this_purchase_button->was_pressed = false;

            shop_try_to_purchase_upgrade(&shop, shop.upgrades + i, &constants);
          }
        }

        if (IsKeyPressed(KEY_M) && DEBUG >= 1) shop.money += 1000;
        break;
      /*---------------------------------------------------------------------------------------------------------*/

      /*-------------------*/
      /* End screen update */
      /*---------------------------------------------------------------------------------------------------------*/
      case GAME_SCREEN_END:
        button_check_user_interaction(&button_end_screen_back, &constants);
        if (button_end_screen_back.was_pressed) {
          button_end_screen_back.was_pressed = false;

          game_screen = GAME_SCREEN_START;
        }
        break;
        /*-------------------------------------------------------------------------------------------------------*/
    }
    /*-----------------------------------------------------------------------------------------------------------*/

    /*---------*/
    /* Drawing */
    /*-----------------------------------------------------------------------------------------------------------*/
    BeginDrawing();
    {
      ClearBackground(constants.background_colour);

      switch (game_screen) {
        /*-----------------------*/
        /* Start screen drawing */
        /*-------------------------------------------------------------------------------------------------------*/
        case GAME_SCREEN_START:
          draw_button(&button_start_screen_start, &constants);
          draw_button(&button_start_screen_shop, &constants);
          break;
        /*-------------------------------------------------------------------------------------------------------*/

        /*---------------------*/
        /* Game screen drawing */
        /*-------------------------------------------------------------------------------------------------------*/
        case GAME_SCREEN_GAME:
          draw_background_squares(camera_position, &constants);
          draw_projectiles(&projectile_manager, camera_position, &constants);
          draw_enemies(&enemy_manager, camera_position, &constants);
          draw_boss(&boss, camera_position, &constants);
          draw_player(&player, camera_position, &constants);

          draw_game_info(&player, &enemy_manager, &projectile_manager, &boss, &constants, show_debug_text);
          draw_boss_health_bar(&boss, &constants);
          break;
        /*-------------------------------------------------------------------------------------------------------*/

        /*---------------------*/
        /* Shop screen drawing */
        /*-------------------------------------------------------------------------------------------------------*/
        case GAME_SCREEN_SHOP:
          draw_shop_text(&shop, &player, &constants);
          draw_shop_purchase_buttons(buttons_shop_purchase, &shop, &constants);
          draw_button(&button_shop_screen_back, &constants);
          break;
        /*-------------------------------------------------------------------------------------------------------*/

        /*--------------------*/
        /* End screen drawing */
        /*-------------------------------------------------------------------------------------------------------*/
        case GAME_SCREEN_END:
          draw_game_over_text(&player, &constants);
          draw_button(&button_end_screen_back, &constants);
          break;
          /*-----------------------------------------------------------------------------------------------------*/
      }

      draw_black_bars(&constants);
    }
    EndDrawing();
    /*-----------------------------------------------------------------------------------------------------------*/
  }

  /*---------*/
  /* Cleanup */
  /*-------------------------------------------------------------------------------------------------------------*/
  CloseWindow();

  cleanup_game(&enemy_manager, &projectile_manager);

  return EXIT_SUCCESS;
  /*-------------------------------------------------------------------------------------------------------------*/
}
