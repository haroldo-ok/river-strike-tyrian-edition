#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/SMSlib.h"
#include "lib/PSGlib.h"
#include "actor.h"
#include "data.h"

#define PLAYER_TOP (0)
#define PLAYER_LEFT (0)
#define PLAYER_RIGHT (256 - 16)
#define PLAYER_BOTTOM (SCREEN_H - 16)
#define PLAYER_SPEED (2)

#define PLAYER_SHOT_SPEED (4)
#define PLAYER_SHOT_MAX (16)
#define FOR_EACH_PLAYER_SHOT(sht) sht = player_shots; for (int i = PLAYER_SHOT_MAX; i; i--, sht++)
	
const path_step lightining_path[] = {
	{0, -4},
	{-128, -128}
};

const path_step fire_path[] = {
	{0, -2},
	{4, -2},
	{4, -2},
	{3, -2},
	{2, -2},
	{0, -2},
	{-2, -2},
	{-3, -2},
	{-4, -2},
	{-4, -2},
	{-4, -2},
	{-4, -2},
	{-3, -2},
	{-2, -2},
	{0, -2},
	{2, -2},
	{3, -2},
	{4, -2},
	{4, -2},
	{0, -2},
	{-128, -128}
};

const path_step wind_path0[] = {
	{-2, -3},
	{-128, -128}
};

const path_step wind_path1[] = {
	{0, -4},
	{-128, -128}
};

const path_step wind_path2[] = {
	{2, -3},
	{-128, -128}
};

const path_step *wind_paths[] = {
	wind_path0, wind_path1, wind_path2
};

actor player;
actor player_shots[PLAYER_SHOT_MAX];

struct ply_ctl {
	char shot_delay;
	char shot_type;
	char pressed_shot_selection;
} ply_ctl;

void load_standard_palettes() {
	SMS_loadBGPalette(sprites_palette_bin);
	SMS_loadSpritePalette(sprites_palette_bin);
	SMS_setSpritePaletteColor(0, 0);
}

char fire_player_shot();

void handle_player_input() {
	static unsigned char joy;	
	joy = SMS_getKeysStatus();

	if (joy & PORT_A_KEY_LEFT) {
		if (player.x > PLAYER_LEFT) player.x -= PLAYER_SPEED;
	} else if (joy & PORT_A_KEY_RIGHT) {
		if (player.x < PLAYER_RIGHT) player.x += PLAYER_SPEED;
	}

	if (joy & PORT_A_KEY_UP) {
		if (player.y > PLAYER_TOP) player.y -= PLAYER_SPEED;
	} else if (joy & PORT_A_KEY_DOWN) {
		if (player.y < PLAYER_BOTTOM) player.y += PLAYER_SPEED;
	}
	
	if (joy & PORT_A_KEY_2) {
		if (!ply_ctl.shot_delay) {
			if (fire_player_shot()) {
				switch (ply_ctl.shot_type) {
				case 0:
					ply_ctl.shot_delay = 4;
					break;

				case 1:
					ply_ctl.shot_delay = 8;
					break;

				case 2:
					ply_ctl.shot_delay = 12;
					break;
				}
			}
		}
	}
	
	if (joy & PORT_A_KEY_1) {
		if (!ply_ctl.pressed_shot_selection) {
			ply_ctl.shot_type++;
			if (ply_ctl.shot_type > 2) ply_ctl.shot_type = 0;
			ply_ctl.pressed_shot_selection = 1;
		}
	} else {
		ply_ctl.pressed_shot_selection = 0;
	}
	
	if (ply_ctl.shot_delay) ply_ctl.shot_delay--;
}

void init_player_shots() {
	static actor *sht;
	
	FOR_EACH_PLAYER_SHOT(sht) {
		sht->active = 0;
	}
}

void handle_player_shots() {
	static actor *sht;
	
	FOR_EACH_PLAYER_SHOT(sht) {
		if (sht->active) {
			move_actor(sht);
			if (sht->y < 0) sht->active = 0;
			if (sht->state == 1 && !sht->state_timer) sht->active = 0;
		}
	}
}

void draw_player_shots() {
	static actor *sht;
	
	FOR_EACH_PLAYER_SHOT(sht) {
		draw_actor(sht);
	}
}

char fire_player_shot() {
	static actor *sht;
	static char shots_to_fire, fired;
	shots_to_fire = ply_ctl.shot_type == 2 ? 3 : 1;
	fired = 0;
	
	FOR_EACH_PLAYER_SHOT(sht) {
		if (!sht->active) {
			switch (ply_ctl.shot_type) {
			case 0:
				init_actor(sht, player.x + 8, player.y - 8, 1, 1, 26, 3);
				sht->path = lightining_path;
				sht->state = 1;
				sht->state_timer = 45;
				break;

			case 1:
				init_actor(sht, player.x + 8, player.y - 8, 1, 1, 32, 4);
				sht->path = fire_path;
				sht->state = 1;
				sht->state_timer = 45;
				break;

			case 2:
				init_actor(sht, player.x + 8, player.y - 8, 1, 1, 40, 2);
				sht->path = wind_paths[shots_to_fire - 1];
				sht->state = 1;
				sht->state_timer = 45;
				break;
			}
						
			// Fired something
			fired = 1;
			shots_to_fire--;
			if (!shots_to_fire)	return 1;
		}
	}

	// Didn't fire anything
	return fired;
}

void main() {
	SMS_useFirstHalfTilesforSprites(1);
	SMS_setSpriteMode(SPRITEMODE_TALL);
	SMS_VDPturnOnFeature(VDPFEATURE_HIDEFIRSTCOL);

	SMS_displayOff();
	SMS_loadPSGaidencompressedTiles(sprites_tiles_psgcompr, 0);
	load_standard_palettes();

	SMS_displayOn();
	
	init_actor(&player, 116, PLAYER_BOTTOM - 16, 3, 1, 2, 3);
	player.animation_delay = 20;
	ply_ctl.shot_delay = 0;
	ply_ctl.shot_type = 0;
	
	init_player_shots();
	
	while (1) {	
		handle_player_input();
		handle_player_shots();
	
		SMS_initSprites();

		draw_actor(&player);
		draw_player_shots();
		
		SMS_finalizeSprites();
		SMS_waitForVBlank();
		SMS_copySpritestoSAT();
	}
}

SMS_EMBED_SEGA_ROM_HEADER(9999,0); // code 9999 hopefully free, here this means 'homebrew'
SMS_EMBED_SDSC_HEADER(0,1, 2021,10,10, "Haroldo-OK\\2021", "Dragon Blaster",
  "A dragon-themed shoot-em-up.\n"
  "Made for the SHMUP JAM 1 - Dragons - https://itch.io/jam/shmup-jam-1-dragons\n"
  "Built using devkitSMS & SMSlib - https://github.com/sverx/devkitSMS");
