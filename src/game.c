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
typedef struct Constants {
  int initial_screen_width_in_pixels;   // Initial game window width in pixels
  int initial_screen_height_in_pixels;  // Initial game window height in pixels
  float aspect_ratio;                   // Aspect ratio to keep the game at (we draw black bars to maintain this)
  float screen_width_in_units;          // Number of units in one screen's width
  float screen_height_in_units;         // Number of units in one screen's height

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

  int max_enemies;                   // Maximum number of enemies. Enemies stop spawning when this is reached
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

  int max_projectiles;  // Maximum number of projectiles. This number should not be reached

  Font game_font;      // Font used for in-game text
  float font_spacing;  // Spacing of the in-game font
} Constants;

typedef struct Player {
  Vector2 pos;  // Current position of the player

  float speed;   // Speed of the player's movement
  float size;    // Radius of the player circle
  Color colour;  // Colour of the player
  int score;     // Score of the player in this game loop

  float firerate;                 // Firerate of the player's shots (shots per second)
  float projectile_speed;         // Speed at which the player's projectiles travel
  float projectile_size;          // Radius of the player's projectile circles
  Color projectile_colour;        // Colour of the player's projectiles
  float time_of_last_projectile;  // Number of seconds since the last projectile was fired
} Player;

typedef struct Upgrade {
  float cost;            // Cost of the next upgrade purchase
  float stat_increment;  // Increment of the stat being upgraded (as fraction of the base value)
  float base_stat;       // Base value of the stat being upgraded
  float *stat;           // Pointer to the stat to adjust
} Upgrade;

#define NUM_UPGRADES 3
typedef struct Shop {
  int money;                       // Amount of money the player has
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

  const EnemyType *type;  // Enemy type of this enemy
} Enemy;

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

typedef struct Projectile {
  Vector2 pos;     // Current position of the projectile
  Vector2 dir;     // Movement direction of the projectile. Should always be normalised
  bool is_active;  // Whether the projectile is processed and drawn

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
bool circle_is_on_screen(Vector2 pos, float rad, const Constants *constants) {
  return (-rad <= pos.x && pos.x <= constants->screen_width_in_units + rad) &&
         (-rad <= pos.y && pos.y <= constants->screen_height_in_units + rad);
};

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
  return (Vector2){anchored_pos.x + 0.5 * grid_x * (constants->screen_width_in_units - dimensions.x),
                   anchored_pos.y + 0.5 * grid_y * (constants->screen_height_in_units - dimensions.y)};
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
    return GetScreenHeight() / constants->screen_height_in_units;
  } else {  // If there are horizontal black bars, use width
    return GetScreenWidth() / constants->screen_width_in_units;
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

// GetMousePosition from raylib, but with the result in units
Vector2 get_mouse_position_in_units(const Constants *constants) {
  return get_unit_position_from_draw_position(GetMousePosition(), constants);
}

// MeasureTextEx from raylib, but with the result in units
Vector2 measure_text_ex_in_units(Font font, const char *text, float size, float spacing,
                                 const Constants *constants) {
  return get_unit_dimensions_from_draw_dimensions(
      MeasureTextEx(font, text, get_draw_length_from_unit_length(size, constants), spacing), constants);
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

  enemy_manager->enemies = calloc(constants->max_enemies, sizeof *(enemy_manager->enemies));
  if (!enemy_manager->enemies) {
    fprintf(stderr, "Unable to allocate enemy storage.\n");
    exit(EXIT_FAILURE);
  }
  enemy_manager->capacity = constants->max_enemies;

  projectile_manager->projectiles = calloc(constants->max_projectiles, sizeof *(projectile_manager->projectiles));
  if (!projectile_manager->projectiles) {
    fprintf(stderr, "Unable to allocate projectile storage.\n");
    exit(EXIT_FAILURE);
  }
  projectile_manager->capacity = constants->max_projectiles;
}

// Perform initialisation steps for game start
void start_game(Player *player, EnemyManager *enemy_manager, ProjectileManager *projectile_manager,
                const Constants *constants) {
  float start_time = GetTime();
  player->pos = constants->player_start_pos;
  player->score = 0;
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
}

// Perform actions when this instance of the game ends
void end_game(Player *player, EnemyManager *enemy_manager, ProjectileManager *projectile_manager, Shop *shop) {
  shop->money += player->score;
}

// Clean up game objects when the program ends
void cleanup_game(EnemyManager *enemy_manager, ProjectileManager *projectile_manager) {
  free(enemy_manager->enemies);
  enemy_manager->enemies = NULL;

  free(projectile_manager->projectiles);
  projectile_manager->projectiles = NULL;
}
/*---------------------------------------------------------------------------------------------------------------*/

/*----------------*/
/* Player actions */
/*---------------------------------------------------------------------------------------------------------------*/

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
void player_update_position(Player *player, const Constants *constants) {
  player->pos =
      Vector2Add(player->pos, Vector2Scale(player_get_input_direction(), player->speed * GetFrameTime()));

  // Clamp the player inside the screen boundaries
  Vector2 min_player_pos = {player->size, player->size};
  Vector2 max_player_pos = {constants->screen_width_in_units - player->size,
                            constants->screen_height_in_units - player->size};
  player->pos = Vector2Clamp(player->pos, min_player_pos, max_player_pos);
}

// Generate a new projectile that moves towards the mouse
Projectile projectile_generate(const Player *player, const Constants *constants) {
  Projectile projectile = {.pos = player->pos,
                           .is_active = true,
                           .speed = player->projectile_speed,
                           .size = player->projectile_size,
                           .colour = player->projectile_colour};

  Vector2 mouse_pos = get_mouse_position_in_units(constants);

  // If the mouse is on the player, just fire in an arbitrary direction, otherwise fire towards the mouse
  if (Vector2Equals(mouse_pos, player->pos))
    projectile.dir = (Vector2){1, 0};  // Arbitrarily choose to shoot to the right (normalised manually)
  else
    projectile.dir = Vector2Normalize(Vector2Subtract(mouse_pos, player->pos));

  return projectile;
}

// Spawn a new projectile when it is time to do so and if the correct button is down
void player_try_to_spawn_projectile(Player *player, ProjectileManager *projectile_manager,
                                    const Constants *constants) {
  // The projectile manager should have capacity large enough that it never becomes full
  assert((projectile_manager->projectile_count < projectile_manager->capacity) &&
         "Trying to add projectile to full projectile manager");

  // If the mouse button isn't held, do nothing
  if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) return;

  // If it has not been long enough since the last shot, do nothing
  float time_since_last_projectile = GetTime() - player->time_of_last_projectile;
  if (time_since_last_projectile < 1 / player->firerate) return;

  for (int i = 0; i < projectile_manager->capacity; i++) {
    if (!projectile_manager->projectiles[i].is_active) {
      projectile_manager->projectiles[i] = projectile_generate(player, constants);
      projectile_manager->projectile_count++;
      break;
    }

    // Check that the loop doesn't terminate without finding an inactive projectile (this is the final i)
    assert((i != projectile_manager->capacity - 1) && "No inactive projectile found");
  }

  player->time_of_last_projectile = GetTime();
}
/*---------------------------------------------------------------------------------------------------------------*/

/*------------------*/
/* Enemy management */
/*---------------------------------------------------------------------------------------------------------------*/

// Enemy manager credits, at time t and before spending, are given by: credits = mult * t ^ exp,
// where mult and exp are constants defined at game initialisation
float enemy_manager_calculate_credits(const EnemyManager *enemy_manager, const Constants *constants) {
  float t = GetTime() - enemy_manager->time_of_initialisation;
  return constants->enemy_credit_multiplier * powf(t, constants->enemy_credit_exponent) -
         enemy_manager->credits_spent + constants->initial_enemy_credits;
}

// Randomly generate a starting position of an enemy. Enemies spawn touching the outside faces of the play area
Vector2 enemy_get_random_start_pos(float enemy_size, const Constants *constants) {
  int vert_or_horiz = GetRandomValue(0, 1);  // Choose to spawn on either the two vertical or two horizontal sides
  int side_chosen = GetRandomValue(1, 1);    // Choose which of the two sides chosen above to spawn on
  int x_val = get_random_float(0, constants->screen_width_in_units);
  int y_val = get_random_float(0, constants->screen_height_in_units);

  // Use the numbers generated above to pick a random position on the edges of the play area (plus one enemy width)
  return (Vector2){
      vert_or_horiz * x_val +
          !vert_or_horiz * (side_chosen * (constants->screen_width_in_units + 2 * enemy_size) - enemy_size),
      !vert_or_horiz * y_val +
          vert_or_horiz * (side_chosen * (constants->screen_height_in_units + 2 * enemy_size) - enemy_size)};
}

// Randomly generate a new enemy
Enemy enemy_generate(const EnemyType *enemy_type, const Player *player, const Constants *constants) {
  Enemy enemy = {.desired_pos = player->pos,
                 .is_active = true,
                 .speed = get_random_float(enemy_type->min_speed, enemy_type->max_speed),
                 .size = get_random_float(enemy_type->min_size, enemy_type->max_size),
                 .type = enemy_type};

  enemy.pos = enemy_get_random_start_pos(enemy.size, constants);

  return enemy;
}

// Try to create and spawn a new wave of enemies (if it is time to do so)
void enemy_manager_try_to_spawn_enemies(EnemyManager *enemy_manager, const EnemyType *enemy_types,
                                        const Player *player, const Constants *constants) {
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

      // Check that adding a type 0 enemy was in fact the cheapest action
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

  // Actually generate the chosen enemies
  for (int i = 0; i < wave_size; i++) {
    // Loop through the enemy slots until an inactive enemy is found and replace with an enemy of the desired type
    for (int j = 0; j < enemy_manager->capacity; j++) {
      if (!enemy_manager->enemies[j].is_active) {
        enemy_manager->enemies[j] = enemy_generate(enemy_types + wave_enemy_types[i], player, constants);
        enemy_manager->enemy_count++;
        break;
      }

      // Check that the loop doesn't terminate without finding an inactive enemy (this is the final j)
      assert((j != enemy_manager->capacity - 1) && "No inactive enemy found");
    }
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

// Get whether an enemy is currently colliding with the player
bool enemy_manager_enemy_is_colliding_with_player(EnemyManager *enemy_manager, const Player *player) {
  for (int i = 0; i < enemy_manager->capacity; i++) {
    Enemy *this_enemy = enemy_manager->enemies + i;
    if (!this_enemy->is_active) continue;

    if (!CheckCollisionCircles(this_enemy->pos, this_enemy->size, player->pos, player->size)) continue;

    this_enemy->is_active = false;
    enemy_manager->enemy_count--;
    return true;
  }

  return false;
}
/*---------------------------------------------------------------------------------------------------------------*/

/*-----------------------*/
/* Projectile management */
/*---------------------------------------------------------------------------------------------------------------*/

// Update projectile positions according to their trajectories
void projectile_manager_update_projectile_positions(ProjectileManager *projectile_manager,
                                                    const Constants *constants) {
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

// Check whether any projectile is collided with an enemy, destroying both if this is the case
void projectile_manager_check_for_collisions_with_enemies(ProjectileManager *projectile_manager,
                                                          EnemyManager *enemy_manager, Player *player) {
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
  }
}
/*---------------------------------------------------------------------------------------------------------------*/

/*---------------*/
/* UI processing */
/*---------------------------------------------------------------------------------------------------------------*/

// Update the state and clicked status of the button from user input
void button_check_user_interaction(Button *button, const Constants *constants) {
  Vector2 unanchored_pos = get_pos_from_anchored_rect(button->bounds, button->anchor_type, constants);
  Rectangle unanchored_bounds = {unanchored_pos.x, unanchored_pos.y, button->bounds.width, button->bounds.height};

  if (CheckCollisionPointRec(get_mouse_position_in_units(constants), unanchored_bounds)) {
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
void draw_player(const Player *player, const Constants *constants) {
  DrawCircleV(get_draw_position_from_unit_position(player->pos, constants),
              get_draw_length_from_unit_length(player->size, constants), player->colour);
}

// Draw the active enemies to the canvas
void draw_enemies(const EnemyManager *enemy_manager, const Constants *constants) {
  for (int i = 0; i < enemy_manager->capacity; i++) {
    Enemy this_enemy = enemy_manager->enemies[i];
    if (!this_enemy.is_active) continue;

    DrawCircleV(get_draw_position_from_unit_position(this_enemy.pos, constants),
                get_draw_length_from_unit_length(this_enemy.size, constants), this_enemy.type->colour);
  }
}

// Draw the active projectiles to the canvas
void draw_projectiles(const ProjectileManager *projectile_manager, const Constants *constants) {
  for (int i = 0; i < projectile_manager->capacity; i++) {
    Projectile this_projectile = projectile_manager->projectiles[i];
    if (!this_projectile.is_active) continue;

    DrawCircleV(get_draw_position_from_unit_position(this_projectile.pos, constants),
                get_draw_length_from_unit_length(this_projectile.size, constants), this_projectile.colour);
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
    DrawRectangle(0, 0, black_bar_width, screen_height, BLACK);
    DrawRectangle(screen_width - black_bar_width, 0, black_bar_width, screen_height, BLACK);
  } else {  // If the window is too tall
    float black_bar_height = 0.5 * (screen_height - (1 / constants->aspect_ratio) * screen_width);
    DrawRectangle(0, 0, screen_width, black_bar_height, BLACK);
    DrawRectangle(0, screen_height - black_bar_height, screen_width, black_bar_height, BLACK);
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
void draw_score_and_game_info(const Player *player, const EnemyManager *enemy_manager,
                              const ProjectileManager *projectile_manager, const Constants *constants,
                              bool show_debug_text) {
  draw_text_anchored(constants->game_font, TextFormat("Score: %d", player->score), (Vector2){0.25, 0.25}, 0.4,
                     constants->font_spacing, BLACK, ANCHOR_TOP_LEFT, constants);

  if (show_debug_text) {
    draw_text_anchored(constants->game_font,
                       TextFormat("Enemy count: %2d/%d", enemy_manager->enemy_count, enemy_manager->capacity),
                       (Vector2){0.25, 0.75}, 0.25, constants->font_spacing, DARKGRAY, ANCHOR_TOP_LEFT, constants);

    draw_text_anchored(
        constants->game_font,
        TextFormat("Projectile count: %2d/%d", projectile_manager->projectile_count, projectile_manager->capacity),
        (Vector2){0.25, 1}, 0.25, constants->font_spacing, DARKGRAY, ANCHOR_TOP_LEFT, constants);

    float num_credits = enemy_manager_calculate_credits(enemy_manager, constants);
    draw_text_anchored(constants->game_font, TextFormat("Credits: %5.2f", num_credits), (Vector2){0.25, 1.25},
                       0.25, constants->font_spacing, DARKGRAY, ANCHOR_TOP_LEFT, constants);

    draw_text_anchored(constants->game_font, TextFormat("%d", GetFPS()), (Vector2){-0.25, 0.25}, 0.35,
                       constants->font_spacing, GREEN, ANCHOR_TOP_RIGHT, constants);
  }
}

// Draw the text for the shop page
void draw_shop_text(const Shop *shop, const Player *player, const Constants *constants) {
  draw_text_anchored(constants->game_font, TextFormat("$%d", shop->money), (Vector2){-0.5, 0.5}, 0.4,
                     constants->font_spacing, GOLD, ANCHOR_TOP_RIGHT, constants);

  Upgrade upgrade_firerate = shop->upgrades[0];
  draw_text_anchored(constants->game_font, "Firerate", (Vector2){1.25, 0.5}, 0.4, constants->font_spacing, BLACK,
                     ANCHOR_TOP_LEFT, constants);
  draw_text_anchored(
      constants->game_font,
      TextFormat("%.1f -> %.1f", player->firerate,
                 player->firerate + upgrade_firerate.stat_increment * constants->player_base_firerate),
      (Vector2){1.25, 0.9}, 0.3, constants->font_spacing, DARKBROWN, ANCHOR_TOP_LEFT, constants);

  Upgrade upgrade_projectile_speed = shop->upgrades[1];
  draw_text_anchored(constants->game_font, "Projectile speed", (Vector2){1.25, 1.5}, 0.4, constants->font_spacing,
                     BLACK, ANCHOR_TOP_LEFT, constants);
  draw_text_anchored(constants->game_font,
                     TextFormat("%.1f -> %.1f", player->projectile_speed,
                                player->projectile_speed + upgrade_projectile_speed.stat_increment *
                                                               constants->player_base_projectile_speed),
                     (Vector2){1.25, 1.9}, 0.3, constants->font_spacing, DARKBROWN, ANCHOR_TOP_LEFT, constants);

  Upgrade upgrade_projectile_size = shop->upgrades[2];
  draw_text_anchored(constants->game_font, "Projectile size", (Vector2){1.25, 2.5}, 0.4, constants->font_spacing,
                     BLACK, ANCHOR_TOP_LEFT, constants);
  draw_text_anchored(constants->game_font,
                     TextFormat("%.2f -> %.2f", player->projectile_size,
                                player->projectile_size + upgrade_projectile_size.stat_increment *
                                                              constants->player_base_projectile_size),
                     (Vector2){1.25, 2.9}, 0.3, constants->font_spacing, DARKBROWN, ANCHOR_TOP_LEFT, constants);
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
  draw_text_anchored(constants->game_font, "GAME OVER", (Vector2){0, -3}, 0.8, constants->font_spacing, MAROON,
                     ANCHOR_CENTRE, constants);
  draw_text_anchored(constants->game_font, TextFormat("Score: %d", player->score), (Vector2){0, -2}, 0.5,
                     constants->font_spacing, BLACK, ANCHOR_CENTRE, constants);
}
/*---------------------------------------------------------------------------------------------------------------*/

int main() {
  /*--------------------------*/
  /* Constants initialisation */
  /*-------------------------------------------------------------------------------------------------------------*/
  Constants constants = {.initial_screen_width_in_pixels = 1280,
                         .initial_screen_height_in_pixels = 720,
                         .aspect_ratio = 16.0 / 9.0,
                         .screen_width_in_units = 16,
                         .screen_height_in_units = 9,
                         .target_fps = 240,

                         .player_start_pos = {8, 4.5},
                         .player_base_speed = 7,
                         .player_base_size = 0.33,
                         .player_colour = VIOLET,

                         .player_base_firerate = 2,
                         .player_base_projectile_speed = 8,
                         .player_base_projectile_size = 0.12,
                         .player_projectile_colour = DARKGRAY,

                         .upgrade_cost_multiplier = 1.5,

                         .max_enemies = 100,
                         .num_enemy_types = 3,
                         .enemy_spawn_interval_min = 3.5,
                         .enemy_spawn_interval_max = 4.5,
                         .enemy_first_spawn_interval = 2.0,
                         .enemy_spawn_min_wave_size = 3,
                         .enemy_spawn_additional_enemy_chance = 0.3,
                         .initial_enemy_credits = 2.8,
                         .enemy_credit_multiplier = 0.2,
                         .enemy_credit_exponent = 1.5,

                         .enemy_update_interval = 0.2,
                         .enemy_update_chance = 0.02,

                         .max_projectiles = 40,

                         .font_spacing = 2};

  const EnemyType enemy_types[] = {{.credit_cost = 1,
                                    .min_speed = 2.5,
                                    .max_speed = 3,
                                    .min_size = 0.27,
                                    .max_size = 0.29,
                                    .colour = RED,
                                    .turns_into = NULL},
                                   {.credit_cost = 3,
                                    .min_speed = 3,
                                    .max_speed = 3.5,
                                    .min_size = 0.28,
                                    .max_size = 0.31,
                                    .colour = SKYBLUE,
                                    .turns_into = enemy_types + 0},
                                   {.credit_cost = 7,
                                    .min_speed = 3.5,
                                    .max_speed = 4,
                                    .min_size = 0.30,
                                    .max_size = 0.34,
                                    .colour = LIME,
                                    .turns_into = enemy_types + 1}};
  /*-------------------------------------------------------------------------------------------------------------*/

  InitWindow(constants.initial_screen_width_in_pixels, constants.initial_screen_height_in_pixels, "Shooter Game");
  SetTargetFPS(constants.target_fps);

  if (DEBUG == 1) {
    printf("\n----- Game started with DEBUG = %d -----\n", DEBUG);
  }

  constants.game_font = GetFontDefault();  // Needs to come after window initialisation

  /*-------------------*/
  /* UI initialisation */
  /*-------------------------------------------------------------------------------------------------------------*/
  Button button_start_screen_start = {.bounds = {0, -1, 3, 1.2},
                                      .anchor_type = ANCHOR_CENTRE,
                                      .text = "START",
                                      .body_colour_default = YELLOW,
                                      .body_colour_hover = GOLD,
                                      .body_colour_pressed = ORANGE,
                                      .text_colour = BLACK,
                                      .font_size = 0.6};

  // It is convenient for me to create some buttons by simply copying a previous button and changing some values
  Button button_start_screen_shop = button_start_screen_start;
  button_start_screen_shop.bounds.y += 2.5;
  button_start_screen_shop.text = "SHOP";

  Button button_shop_screen_back = {.bounds = {0.25, -0.25, 1.2, 0.5},
                                    .anchor_type = ANCHOR_BOTTOM_LEFT,
                                    .text = "BACK",
                                    .body_colour_default = YELLOW,
                                    .body_colour_hover = GOLD,
                                    .body_colour_pressed = ORANGE,
                                    .text_colour = BLACK,
                                    .font_size = 0.28};

  // The first purchase button is created manually, and the others are identical but shifted down
  Button buttons_shop_purchase[NUM_UPGRADES] = {{.bounds = {0.18, 0.58, 0.94, 0.5},
                                                 .anchor_type = ANCHOR_TOP_LEFT,
                                                 .text = "price",
                                                 .body_colour_default = GREEN,
                                                 .body_colour_hover = LIME,
                                                 .body_colour_pressed = DARKGREEN,
                                                 .text_colour = BLACK,
                                                 .font_size = 0.25}};
  buttons_shop_purchase[1] = buttons_shop_purchase[0];
  buttons_shop_purchase[1].bounds.y += 1;
  buttons_shop_purchase[2] = buttons_shop_purchase[1];
  buttons_shop_purchase[2].bounds.y += 1;

  Button button_end_screen_back = button_start_screen_start;
  button_end_screen_back.bounds.y += 2.5;
  button_end_screen_back.text = "GO BACK";
  /*-------------------------------------------------------------------------------------------------------------*/

  /*----------------------------*/
  /* Game object initialisation */
  /*-------------------------------------------------------------------------------------------------------------*/
  Player player = {0};
  EnemyManager enemy_manager = {0};
  ProjectileManager projectile_manager = {0};

  Shop shop = {.money = 0,
               .upgrades = {{100, 0.25, constants.player_base_firerate, &(player.firerate)},
                            {50, 0.3, constants.player_base_projectile_speed, &(player.projectile_speed)},
                            {30, 0.2, constants.player_base_projectile_size, &(player.projectile_size)}}};

  initialise_game(&player, &enemy_manager, &projectile_manager, &constants);

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
          start_game(&player, &enemy_manager, &projectile_manager, &constants);
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

        player_try_to_spawn_projectile(&player, &projectile_manager, &constants);
        projectile_manager_check_for_collisions_with_enemies(&projectile_manager, &enemy_manager, &player);
        projectile_manager_update_projectile_positions(&projectile_manager, &constants);

        enemy_manager_try_to_spawn_enemies(&enemy_manager, enemy_types, &player, &constants);
        enemy_manager_update_desired_positions(&enemy_manager, &player, &constants);
        enemy_manager_update_enemy_positions(&enemy_manager);

        if (enemy_manager_enemy_is_colliding_with_player(&enemy_manager, &player)) {
          end_game(&player, &enemy_manager, &projectile_manager, &shop);
          game_screen = GAME_SCREEN_END;
        }

        if (IsKeyPressed(KEY_B) && DEBUG >= 1) {
          show_debug_text = !show_debug_text;
        }
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
      ClearBackground(RAYWHITE);

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
          draw_projectiles(&projectile_manager, &constants);
          draw_enemies(&enemy_manager, &constants);
          draw_player(&player, &constants);

          draw_score_and_game_info(&player, &enemy_manager, &projectile_manager, &constants, show_debug_text);
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
