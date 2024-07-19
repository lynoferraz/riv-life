#include <riv.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

bool setingup;
bool setup;
bool starting;
bool started;
bool ended;

enum {
  SCREEN_SIZE = 256,
  MAP_SIZE = 64,
  TILE_SIZE = 4,
  DIR_KEY_FRAMES = 4,
  KEY_SET_FRAMES = 10,
  START_SET_FRAMES_START = 240,
  N_LEVELS_SYSTEM = 12,
  LEVEL_UP_ANIMATION_FRAMES = 6,
  MIN_FRAME_SOUND_LEVEL = 6,
  SPEED1 = 2,
  SPEED2 = 8,
  STARTING_FRAMES = 30,
  SETINGUP_FRAMES = 30,
  N_POLIMINOS = 14,
  N_MIRRORS = 4,
  POLIMINOS_POINTS = 4,
};

static uint32_t level_colors[] = {19,18,17,16,15,27,28,29,23,24,25,26};

static int previous_map[MAP_SIZE][MAP_SIZE];
static int map[MAP_SIZE][MAP_SIZE];
static int next_map[MAP_SIZE][MAP_SIZE];
static riv_vec2i cursor_pos = {(SCREEN_SIZE-TILE_SIZE)/2, (SCREEN_SIZE-TILE_SIZE)/2};
static int64_t cursor_speed = SPEED1;
riv_vec2i cursor_map_pos;

// up to tetromino
static int poliminos[N_POLIMINOS][POLIMINOS_POINTS][2] = {
    {{0,0},{0,1},{-1,-1},{-1,-1}}, // domino
    {{0,0},{1,0},{-1,-1},{-1,-1}}, // domino b
    {{0,0},{0,1},{0,2},{-1,-1}}, // tromino 1
    {{0,0},{1,0},{2,0},{-1,-1}}, // tromino 1b
    {{0,0},{0,1},{1,0},{-1,-1}}, // tromino 2
    {{0,0},{0,1},{0,2},{0,3}}, // tetromino 1
    {{0,0},{1,0},{2,0},{3,0}}, // tetromino 1b
    {{0,0},{0,1},{1,0},{1,1}}, // tetromino 2
    {{0,0},{0,1},{0,2},{1,2}}, // tetromino 3
    {{0,0},{0,1},{1,0},{2,0}}, // tetromino 3b
    {{0,0},{1,0},{1,1},{2,1}}, // tetromino 4
    {{0,0},{0,1},{1,1},{1,2}}, // tetromino 4b
    {{0,0},{0,1},{0,2},{1,1}}, // tetromino 5
    {{0,0},{1,0},{2,0},{1,1}}, // tetromino 5b
};
static int mirrors[4][2] = {{1,1},{1,-1},{-1,-1},{-1,1}};

int u_press = 0;
int d_press = 0;
int l_press = 0;
int r_press = 0;
int a1_press = 0;
int start_press = 0;
uint64_t start_press_frame;
int level_up_animation = 0;
int polimino_selected = -1;
int last_polimino_selected = 0;
int polimino_mirror_index = 0;
bool polimino_unset = false;

uint64_t mark_frame;
uint64_t last_sound_frame;
int level = 1;
int deaths = 0;
int births = 0;
int current_alive = 0;
int starting_alive = 0;
int score = 0;
int generations = 0;
int lifes_lvl[N_LEVELS_SYSTEM] = {0,0,0,0,0,0,0,0,0,0,0,0};

int life_points = 20000;
float level_increase = 500;
float level_increase_factor = 2;
int birth_bonus = 1;
int starting_alive_bonus = 0;
int death_penalty = 0;
int stale_penalty = 3;
int moving_bonus = 4;
int boring_penalty = 100;
int boring_threshold = 20;
int starting_cells = 30;
int setup_time = 60;
bool show_stats = 1;
float efficiency = 1.0;
int updates_sec = 1;


int64_t clamp(int64_t v, int64_t min, int64_t max) { v = v < min ? min : v; v = v > max ? max : v; return v; }

////
// Sound

// Sound effects
riv_waveform_desc start_sfx = {
    .type = RIV_WAVEFORM_PULSE,
    .attack = 0.01f, .decay = 0.01f, .sustain = 0.25f, .release = 0.25f,
    .start_frequency = RIV_NOTE_A3, .end_frequency = RIV_NOTE_A4,
    .amplitude = 0.25f, .sustain_level = 0.5f,
};
riv_waveform_desc end_sfx = {
    .type = RIV_WAVEFORM_PULSE,
    .attack = 0.01f, .decay = 0.01f, .sustain = 0.1f, .release = 0.01f,
    .start_frequency = RIV_NOTE_A3, .end_frequency = RIV_NOTE_A2,
    .amplitude = 0.5f, .sustain_level = 0.5f,
};
riv_waveform_desc set_sfx = {
    .type = RIV_WAVEFORM_PULSE,
    .attack = 0.01f, .decay = 0.01f, .sustain = 0.1f, .release = 0.01f,
    .start_frequency = RIV_NOTE_A4, .end_frequency = RIV_NOTE_C4,
    .amplitude = 0.25f, .sustain_level = 0.5f,
};
riv_waveform_desc unset_sfx = {
    .type = RIV_WAVEFORM_PULSE,
    .attack = 0.01f, .decay = 0.01f, .sustain = 0.1f, .release = 0.01f,
    .start_frequency = RIV_NOTE_C4, .end_frequency = RIV_NOTE_A4,
    .amplitude = 0.25f, .sustain_level = 0.5f,
};

riv_waveform_desc multiset_sfx = {
    .type = RIV_WAVEFORM_PULSE,
    .attack = 0.01f, .decay = 0.03f, .sustain = 0.3f, .release = 0.01f,
    .start_frequency = RIV_NOTE_A4, .end_frequency = RIV_NOTE_C4,
    .amplitude = 0.25f, .sustain_level = 0.5f,
};
riv_waveform_desc multiunset_sfx = {
    .type = RIV_WAVEFORM_PULSE,
    .attack = 0.01f, .decay = 0.03f, .sustain = 0.3f, .release = 0.01f,
    .start_frequency = RIV_NOTE_C4, .end_frequency = RIV_NOTE_A4,
    .amplitude = 0.25f, .sustain_level = 0.5f,
};
riv_waveform_desc ready_sfx = {
    .type = RIV_WAVEFORM_PULSE,
    .attack = 0.00f, .decay = 0.25f, .sustain = 0.0f, .release = 0.25f,
    .start_frequency = RIV_NOTE_F4, .end_frequency = RIV_NOTE_F4,
    .amplitude = 0.25f, .sustain_level = 0.25f,
};

riv_waveform_desc level_sfx = {
    .type = RIV_WAVEFORM_SAWTOOTH,
    .attack = 0.00f, .decay = 0.25f, .sustain = 0.25f, .release = 0.25f,
    .start_frequency = RIV_NOTE_F4, .end_frequency = RIV_NOTE_G5,
    .amplitude = 0.25f, .sustain_level = 0.25f,
};

riv_waveform_desc update_sfx = {
    .type = RIV_WAVEFORM_SAWTOOTH,
    .attack = 0.075f, .decay = 0.075f, .sustain = 0.1f, .release = 0.025f,
    .start_frequency = RIV_NOTE_G2, .end_frequency = RIV_NOTE_A2,
    .amplitude = 0.125f, .sustain_level = 0.1f,
};


static float rel_note_level[] = {
    (261.63/261.63),
    (293.66/261.63),
    (329.63/261.63),
    (349.23/261.63),
    (392.00/261.63),
    (440.00/261.63),
    (493.88/261.63),
    (523.25/261.63),
    (587.33/261.63),
    (659.25/261.63),
    (698.46/261.63),
    (783.99/261.63),
    (880.00/261.63),
};

////
// Main life logic

int adjacencies (int i, int j) {
	int	k, l, count;

	count = 0;

	/* go around the cell */
	for (k=-1; k<=1; k++) for (l=-1; l<=1; l++){
		/* only count if at least one of k,l isn't zero */
        int c = i-k;
        int r = j-l;
		if (c >= 0 && r >= 0 && c < MAP_SIZE && r < MAP_SIZE)
			if (k || l)
                if (map[c][r]) count++;
    }
	return count;
}


////
// Update functions

void start_setup() {
    cursor_map_pos = (riv_vec2i){(cursor_pos.x + TILE_SIZE/2) / TILE_SIZE, (cursor_pos.y + TILE_SIZE/2) / TILE_SIZE};
    setingup = true;
    last_polimino_selected = 0;
    polimino_selected = -1;
    polimino_mirror_index = 0;
    polimino_unset = false;
    riv_waveform(&start_sfx);

    int count = 0;
    while (count < starting_cells) {
        riv_vec2i new_cell = (riv_vec2i){riv_rand_uint(MAP_SIZE-1), riv_rand_uint(MAP_SIZE-1)};
        if (map[new_cell.x][new_cell.y] == 0) {
            map[new_cell.x][new_cell.y] = 2;
            count++;
        }
    }
    mark_frame = riv->frame;
}

void game_setingup() {
    if (riv->frame - mark_frame > SETINGUP_FRAMES) {
        mark_frame = riv->frame;
        setup = true;
    }
}

void start_game() {
    current_alive = 0;
    starting_alive = 0;
	for (int i=0; i<MAP_SIZE; i++) for (int j=0; j<MAP_SIZE; j++) {
        if (map[i][j]) current_alive++;
        if (map[i][j] == 2) starting_alive++;
    }
    life_points -= current_alive;
    riv_waveform_desc start_waveform = *(&ready_sfx);
    start_waveform.sustain = 0.25f;
    riv_waveform(&start_waveform);
    starting = true;
    mark_frame = riv->frame;
}

void game_starting() {
    if (riv->frame - mark_frame > STARTING_FRAMES) {
        mark_frame = riv->frame;
        started = true;
    }
}

// Called when game ends
void end_game() {
    // Play end sound
    // riv_waveform(&end_sfx);
    ended = true;
    riv_waveform(&end_sfx);
    // Quit in 3 seconds
    riv->quit_frame = riv->frame + 2*riv->target_fps;
}

void update_setup() {

    if (!(r_press || l_press || d_press || u_press)){
        cursor_speed = SPEED1;
    }
    
    if (riv->keys[RIV_GAMEPAD_RIGHT].down) r_press++; else r_press = 0;
    if (r_press%DIR_KEY_FRAMES == 1) {
        cursor_pos.x += cursor_speed;
        polimino_unset = false;
    } else if (r_press == 4*DIR_KEY_FRAMES) {
        cursor_speed = SPEED2;
    }

    if (riv->keys[RIV_GAMEPAD_LEFT].down) l_press++; else l_press = 0;
    if (l_press%DIR_KEY_FRAMES == 1) {
        cursor_pos.x -= cursor_speed;
        polimino_unset = false;
    } else if (l_press == 4*DIR_KEY_FRAMES) {
        cursor_speed = SPEED2;
    }

    if (riv->keys[RIV_GAMEPAD_DOWN].down) d_press++; else d_press = 0;
    if (d_press%DIR_KEY_FRAMES == 1) {
        cursor_pos.y += cursor_speed;
        polimino_unset = false;
    } else if (d_press == 4*DIR_KEY_FRAMES) {
        cursor_speed = SPEED2;
    }

    if (riv->keys[RIV_GAMEPAD_UP].down) u_press++; else u_press = 0;
    if (u_press%DIR_KEY_FRAMES == 1) {
        cursor_pos.y -= cursor_speed;
        polimino_unset = false;
    } else if (u_press == 4*DIR_KEY_FRAMES) {
        cursor_speed = SPEED2;
    }

    // activate poliminos
    if (riv->keys[RIV_GAMEPAD_A3].press) {
        polimino_selected = polimino_selected > -1 ? -1 : last_polimino_selected;
    };
    // cicle poliminos
    if (riv->keys[RIV_GAMEPAD_A2].press) {
        polimino_selected = polimino_selected < N_POLIMINOS - 1 ? polimino_selected + 1 : 0;
        last_polimino_selected = polimino_selected;
        riv_printf("polimino selected %i\n",polimino_selected);
    };

    // mirror polimino
    if (riv->keys[RIV_GAMEPAD_L1].press) {
        polimino_mirror_index = polimino_mirror_index > 0 ? polimino_mirror_index - 1 : N_MIRRORS - 1;
    };
    if (riv->keys[RIV_GAMEPAD_R1].press) {
        polimino_mirror_index = polimino_mirror_index < N_MIRRORS - 1 ? polimino_mirror_index + 1 : 0;
    };

    cursor_pos.x = clamp(cursor_pos.x, 0, SCREEN_SIZE-TILE_SIZE);
    cursor_pos.y = clamp(cursor_pos.y, 0, SCREEN_SIZE-TILE_SIZE);

    cursor_map_pos = (riv_vec2i){(cursor_pos.x + TILE_SIZE/2) / TILE_SIZE, (cursor_pos.y + TILE_SIZE/2) / TILE_SIZE};

    if (riv->keys[RIV_GAMEPAD_A1].down) a1_press++; else a1_press = 0;
    if (riv->keys[RIV_GAMEPAD_START].down || (riv->frame - mark_frame)/riv->target_fps > setup_time ) start_press++; else start_press = 0;

    if (a1_press == 1) {
        if (polimino_selected == -1) {
            if (map[cursor_map_pos.x][cursor_map_pos.y] == 2) { // can't change starting
                riv_waveform(&end_sfx);
            } else {
                if (map[cursor_map_pos.x][cursor_map_pos.y]) riv_waveform(&set_sfx);
                else riv_waveform(&unset_sfx);
                map[cursor_map_pos.x][cursor_map_pos.y] = !map[cursor_map_pos.x][cursor_map_pos.y];
            }
        } else {
            for (int i=0; i<POLIMINOS_POINTS; i++) {
                if (poliminos[polimino_selected][i][0] == -1 || poliminos[polimino_selected][i][1] == -1) continue;
                int x = clamp(cursor_map_pos.x + mirrors[polimino_mirror_index][0] * poliminos[polimino_selected][i][0], 0, SCREEN_SIZE-TILE_SIZE);
                int y = clamp(cursor_map_pos.y + mirrors[polimino_mirror_index][1] * poliminos[polimino_selected][i][1], 0, SCREEN_SIZE-TILE_SIZE);
                if (map[x][y] != 2) {
                    if (polimino_unset) {
                        riv_waveform(&multiset_sfx);
                        map[x][y] = 0;
                    } else {
                        riv_waveform(&multiunset_sfx);
                        map[x][y] = 1;
                    }
                }
            }
            polimino_unset = !polimino_unset;
        }
    } else if (d_press == KEY_SET_FRAMES) a1_press = 0;
    if (start_press == 1) {
        start_press_frame = riv->frame;
    } else if (start_press == riv->target_fps || start_press == 2*riv->target_fps || start_press == 3*riv->target_fps) {
        riv_waveform(&ready_sfx);
    } else if (start_press >= START_SET_FRAMES_START) {
        start_game();
    }
}

void update_game() {
    int a;

    uint64_t rel_frame = riv->frame - mark_frame;

    bool update_state = ((updates_sec*(rel_frame % riv->target_fps)) % clamp(N_LEVELS_SYSTEM - level,1,N_LEVELS_SYSTEM) == 0);

    if (update_state) {
        generations++;
        for (int i=0; i<MAP_SIZE; i++) for (int j=0; j<MAP_SIZE; j++) {
            a = adjacencies (i, j);
            if (a == 2) next_map[i][j] = map[i][j];
            if (a == 3) next_map[i][j] = map[i][j] ? map[i][j] : 1;
            if (a < 2) next_map[i][j] = 0;
            if (a > 3) next_map[i][j] = 0;

            if (map[i][j] && next_map[i][j]) {
                life_points -= stale_penalty;
            }

            if (map[i][j] && !next_map[i][j]) {
                deaths++;
                life_points -= death_penalty;
            }
            if (!map[i][j] && next_map[i][j]) {
                births++;
                life_points += birth_bonus;
            }
            if (!previous_map[i][j] && !map[i][j] && next_map[i][j]) {
                life_points += moving_bonus;
            }
        }
        current_alive = 0;
        starting_alive = 0;
        for (int i=0; i<MAP_SIZE; i++) for (int j=0; j<MAP_SIZE; j++) {
            previous_map[i][j] = map[i][j];
            map[i][j] = next_map[i][j];
            if (map[i][j]) current_alive++;
            if (map[i][j] == 2) starting_alive++;
        }
        life_points -= ceil(current_alive * efficiency);

        if (boring_threshold > 0) {
            life_points -= current_alive < boring_threshold ? boring_penalty * level: 0;
        }

        lifes_lvl[level-1] += current_alive;

        riv_waveform_desc waveform = *(&update_sfx);
        int note = clamp(level-1,0,N_LEVELS_SYSTEM-1);
        waveform.start_frequency = waveform.start_frequency * rel_note_level[note];
        waveform.end_frequency = waveform.end_frequency * rel_note_level[note];
        riv_waveform(&waveform);
    }


    score += level * current_alive + starting_alive * starting_alive_bonus;

    // riv_printf("level up %f,\n",(float)score/(float)level_increase);
    // update level
    if ((float)score/level_increase >= (float)level) {
        riv_waveform_desc level_waveform = *(&level_sfx);
        int level_note = clamp(level-1,0,N_LEVELS_SYSTEM-1);
        level_waveform.start_frequency = level_waveform.start_frequency * rel_note_level[level_note];
        level_waveform.end_frequency = level_waveform.end_frequency * rel_note_level[level_note];
        if (riv->frame - last_sound_frame > MIN_FRAME_SOUND_LEVEL) {
            riv_waveform(&level_waveform);
            last_sound_frame = riv->frame;
        }
        level_increase *= level_increase_factor;
        level++;
    }

    riv->outcard_len = riv_snprintf((char*)riv->outcard, RIV_SIZE_OUTCARD, 
        "JSON{\"score\":%d,\"current_alive\":%d,\"births\":%d,\"deaths\":%d,\"level\":%d,\"frame\":%d,\"starting_alive\":%d,\"generations\":%d,\"lifes_lvl_1\":%d,\"lifes_lvl_2\":%d,\"lifes_lvl_3\":%d,\"lifes_lvl_4\":%d,\"lifes_lvl_5\":%d,\"lifes_lvl_6\":%d,\"lifes_lvl_7\":%d,\"lifes_lvl_8\":%d,\"lifes_lvl_9\":%d,\"lifes_lvl_10\":%d,\"lifes_lvl_11\":%d,\"lifes_lvl_12\":%d}", 
        score, current_alive, births, deaths, level, rel_frame, starting_alive, generations, 
            lifes_lvl[0],lifes_lvl[1],lifes_lvl[2],lifes_lvl[3],lifes_lvl[4],lifes_lvl[5],lifes_lvl[6],lifes_lvl[7],lifes_lvl[8],lifes_lvl[9],lifes_lvl[10],lifes_lvl[11]);

    if (current_alive == 0 || life_points < 1) {
        end_game();
    }
}


////
// Draw functions

// Draw setup phase
void draw_setup() {
    // Clear screen
    riv_clear(RIV_COLOR_DARKSLATE);
    
    int total_cells = 0;
    // draw cells
    for (int y=0;y<MAP_SIZE;++y) { // for each row
        for (int x=0;x<MAP_SIZE;++x) { // for each column
            int cell = map[x][y];
            if (cell == 1) { // draw cell
                riv_draw_rect_fill(x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE, RIV_COLOR_LIGHTPEACH);
                total_cells++;
            } else if (cell == 2) { // draw fixed cell
                riv_draw_rect_fill(x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE, RIV_COLOR_LIGHTBLUE);
                total_cells++;
            } 
        }
    }

    // draw polimino
    if (polimino_selected > -1) {
        for (int i=0; i<POLIMINOS_POINTS; i++) {
            if (poliminos[polimino_selected][i][0] == -1 || poliminos[polimino_selected][i][1] == -1) continue;
            int x = clamp(cursor_map_pos.x + mirrors[polimino_mirror_index][0] * poliminos[polimino_selected][i][0], 0, SCREEN_SIZE-TILE_SIZE);
            int y = clamp(cursor_map_pos.y + mirrors[polimino_mirror_index][1] * poliminos[polimino_selected][i][1], 0, SCREEN_SIZE-TILE_SIZE);
            riv_draw_rect_line(x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE, RIV_COLOR_LIGHTGREY);
        }
    }

    // draw cursor
    uint32_t col = (riv->frame % 15 > 7) ? RIV_COLOR_LIGHTGREY : RIV_COLOR_GREY;
    riv_draw_rect_line(cursor_map_pos.x*TILE_SIZE, cursor_map_pos.y*TILE_SIZE, TILE_SIZE, TILE_SIZE, col);
    // draw text
    if (start_press < riv->target_fps/4){
        riv_draw_text("Setup Life", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, 128, 10, 1, RIV_COLOR_WHITE);

        char buf[128];
        int remaing_time = setup_time-(riv->frame-mark_frame)/riv->target_fps;
        riv_snprintf(buf, sizeof(buf), "%d", remaing_time > 0 ? remaing_time : 0);
        riv_draw_text(buf, RIV_SPRITESHEET_FONT_5X7, RIV_LEFT, 10, 240, 1, RIV_COLOR_WHITE);
    } else if (start_press < riv->target_fps) {
    } else if (start_press < 2*riv->target_fps) {
        int64_t size = (2*riv->target_fps - start_press) / 12 + 2;
        riv_draw_text("3", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, 128, 128, size, RIV_COLOR_WHITE);
    } else if (start_press < 3*riv->target_fps) {
        int64_t size = (3*riv->target_fps - start_press) / 12 + 2;
        riv_draw_text("2", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, 128, 128, size, RIV_COLOR_WHITE);
    } else if (start_press < 4*riv->target_fps) {
        int64_t size = (4*riv->target_fps - start_press) / 12 + 2;
        riv_draw_text("1", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, 128, 128, size, RIV_COLOR_WHITE);
    }

    char buf[128];
    riv_snprintf(buf, sizeof(buf), "LP: %d",life_points - total_cells);
    riv_draw_text(buf, RIV_SPRITESHEET_FONT_5X7, RIV_RIGHT, 246, 240, 1, RIV_COLOR_WHITE);
}

// Draw the game map
void draw_game() {
    // Clear screen
    riv_clear(RIV_COLOR_DARKSLATE);
    for (int y=0;y<MAP_SIZE;++y) { // for each row
        for (int x=0;x<MAP_SIZE;++x) { // for each column
            int cell = map[x][y];
            if (cell > 0) { // draw tile sprite
                uint32_t col = cell == 2 ? RIV_COLOR_LIGHTBLUE : level_colors[clamp(level-1,0,N_LEVELS_SYSTEM-1)];
                riv_draw_rect_fill(x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE, col);
            }
        }
    }

    if (show_stats) {
        char buf[128];
        riv_snprintf(buf, sizeof(buf), "Score: %d",score);
        riv_draw_text(buf, RIV_SPRITESHEET_FONT_5X7, RIV_LEFT, 10, 240, 1, RIV_COLOR_WHITE);

        riv_snprintf(buf, sizeof(buf), "LP: %d",life_points < 0 ? 0 : life_points);
        riv_draw_text(buf, RIV_SPRITESHEET_FONT_5X7, RIV_RIGHT, 246, 240, 1, RIV_COLOR_WHITE);

        riv_snprintf(buf, sizeof(buf), "Level: %d",level);
        riv_draw_text(buf, RIV_SPRITESHEET_FONT_5X7, RIV_LEFT, 10, 10, 1, RIV_COLOR_WHITE);
    }
}

void draw_start_screen() {
    // Clear screen
    riv_clear(RIV_COLOR_DARKSLATE);
    // Draw snake title
    riv_draw_text("Life is Hard", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, 128, 128, 2, RIV_COLOR_DARKPINK);

    // Make "press to start blink" by changing the color depending on the frame number
    uint32_t col = (riv->frame % 15 > 7) ? RIV_COLOR_LIGHTGREY : RIV_COLOR_GREY;
    // Draw press to start
    riv_draw_text("PRESS TO START", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, 128, 128+32, 1, col);
}

void draw_starting_screen() {
    // Clear screen
    riv_clear(riv->frame%2 ? RIV_COLOR_DARKSLATE : RIV_COLOR_LIGHTSLATE);

    int dx = riv_rand_int(-1,1);
    int dy = riv_rand_int(-1,1);

    // Draw snake title
    riv_draw_text("Life is Hard", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, 128+dx, 128+dy, 2, riv->frame%2 ? RIV_COLOR_DARKPINK : RIV_COLOR_DARKPURPLE);
}

// Draw game over screen
void draw_end_screen() {
    // Clear screen
    riv_clear(RIV_COLOR_DARKSLATE);
    // Draw last game frame
    draw_game();
    // Draw GAME OVER
    // riv_draw_text("GAME OVER", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, 64, 64, 2, RIV_COLOR_RED);
}


////
// Main dDraw and update functions

// Called every frame to update game state
void update() {
    if (!setingup) { // Game not started yet
        // Let enter setup phase whenever a key has been pressed
        if (riv->key_toggle_count > 0) {
            start_setup();
        }
    } else if (!setup) { // Game not in setup yet
        game_setingup();
    } else if (!starting) { // Game in setup
        update_setup();
    } else if (!started) { // Game is starting
        game_starting();
    } else if (!ended) { // Game is progressing
        update_game();
    }
}

// Called every frame to draw the game
void draw() {
    // Draw different screens depending on the game state
    if (!setingup) { // Game not in setup yet
        draw_start_screen();
    } else if (!setup) { // Game not in setup yet
        draw_starting_screen();
    } else if (!starting) { // Game not started yet
        draw_setup();
    } else if (!started) { // Game not started yet
        draw_game();
    } else if (!ended) { // Game is progressing
        draw_game();
    } else { // Game ended
        draw_end_screen();
    }
}

////
// Main loop

int main(int argc, char* argv[]) {

    if (argc > 1) {
        if (argc % 2 == 0) {
            riv_printf("Wrong number of arguments\n");
            return 1;
        }
        for (int i = 1; i < argc; i+=2) {
            // riv_printf("args %s=%s\n", argv[i],argv[i+1]);
            if (strcmp(argv[i], "-life-points") == 0) {
                life_points = atoi(argv[i+1]);
            } else if (strcmp(argv[i], "-level-increase") == 0) {
                char *endstr;
                level_increase = strtof(argv[i+1], &endstr);
            } else if (strcmp(argv[i], "-level-increase-factor") == 0) {
                char *endstr;
                level_increase_factor = strtof(argv[i+1], &endstr);
                // riv_printf("stopped '%s' scan at '%s'\n",argv[i+1], endstr);
            } else if (strcmp(argv[i], "-birth-bonus") == 0) {
                birth_bonus = atoi(argv[i+1]);
            } else if (strcmp(argv[i], "-moving-bonus") == 0) {
                moving_bonus = atoi(argv[i+1]);
            } else if (strcmp(argv[i], "-death-penalty") == 0) {
                death_penalty = atoi(argv[i+1]);
            } else if (strcmp(argv[i], "-boring-penalty") == 0) {
                boring_penalty = atoi(argv[i+1]);
            } else if (strcmp(argv[i], "-starting-cells") == 0) {
                starting_cells = atoi(argv[i+1]);
            } else if (strcmp(argv[i], "-setup-time") == 0) {
                setup_time = atoi(argv[i+1]);
            } else if (strcmp(argv[i], "-show-stats") == 0) {
                show_stats = atoi(argv[i+1]);
            } else if (strcmp(argv[i], "-starting-alive-bonus") == 0) {
                starting_alive_bonus = atoi(argv[i+1]);
            } else if (strcmp(argv[i], "-updates-sec") == 0) {
                updates_sec = atoi(argv[i+1]);
            } else if (strcmp(argv[i], "-efficiency") == 0) {
                char *endstr;
                efficiency = strtof(argv[i+1], &endstr);
            } else if (strcmp(argv[i], "-boring-threshold") == 0) {
                boring_threshold = atoi(argv[i+1]);
            } else if (strcmp(argv[i], "-stale-penalty") == 0) {
                stale_penalty = atoi(argv[i+1]);
            }
        }
        // riv_printf("-life-points %d\n",life_points);
        // riv_printf("-level-increase %f\n",level_increase);
        // riv_printf("-level-increase_factor %f\n",level_increase_factor);
        // riv_printf("-birth-bonus %d\n",birth_bonus);
        // riv_printf("-stale-penalty %d\n",stale_penalty);
        // riv_printf("-boring-penalty %d\n",boring_penalty);
        // riv_printf("-boring-penalty %d\n",boring_penalty);
        // riv_printf("-boring-threshold %d\n",boring_threshold);
        // riv_printf("-starting-cells %d\n",starting_cells);
        // riv_printf("-setup-time %d\n",setup_time);
        // riv_printf("-show-stats %d\n",show_stats);
        // riv_printf("-starting-alive-bonus %d\n",starting_alive_bonus);
        // riv_printf("-efficiency %f\n",efficiency);
    }

    do { // main loop
        update();
        draw();
    } while(riv_present());
    return 0;
}
