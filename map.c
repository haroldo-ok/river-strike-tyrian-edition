#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/SMSlib.h"
#include "lib/PSGlib.h"
#include "actor.h"
#include "map.h"

#define MAP_W (16)
#define STREAM_MIN_W (2)
#define STREAM_MAX_W (5)
#define TILE_WATER (4)
#define TILE_LAND (17)

typedef struct river_stream {
	char x, w;
} river_stream;

struct map_data {
	char *level_data;
	char *next_row;
	char background_y;
	char lines_before_next;
	char scroll_y;

	river_stream stream1, stream2;
	char circular_buffer[SCROLL_CHAR_H][SCREEN_CHAR_W];
} map_data;

void init_map(void *level_data) {
	map_data.level_data = level_data;
	map_data.next_row = level_data;
	map_data.background_y = SCROLL_CHAR_H - 2;
	map_data.lines_before_next = 0;
	map_data.scroll_y = 0;
	
	map_data.stream1.x = 7;
	map_data.stream1.w = STREAM_MIN_W;
	map_data.stream2.x = 7;
	map_data.stream2.w = STREAM_MIN_W;
}

void update_river_stream(char *buffer, river_stream *stream) {
	static char *d;
	static char remaining;

	// Fill the stream area with water tiles
	d = buffer + stream->x;
	for (remaining = stream->w; remaining; remaining--) {
		*d = TILE_WATER;
		d++;
	}

	// Update width
	if (!(rand() & 0x03)) {
		if (rand() & 0x80) {
			stream->w--;
		} else {
			stream->w++;
		}
		
		if (stream->w < STREAM_MIN_W) {
			stream->w = STREAM_MIN_W;
		} else if (stream->w > STREAM_MAX_W) {
			stream->w = STREAM_MAX_W;
		}
	}

	// Update X coord
	if (stream->w > STREAM_MIN_W && !(rand() & 0x03)) {
		if (rand() & 0x80) {
			stream->x--;
		} else {
			stream->x++;
		}		
	}

	if (stream->x < 1) {
		stream->x = 1;
	} else if (stream->x + stream->w > MAP_W - 1) {
		stream->x = MAP_W - stream->w - 1;
	}
}

void generate_map_row(char *buffer) {
	static char *o, *d, *prev, *next;
	static char remaining, ch, repeat, pos;
	
	// Fill the row with land tiles
	d = buffer;
	for (remaining = MAP_W; remaining; remaining--) {
		*d = TILE_LAND;
		d++;
	}
	
	update_river_stream(buffer, &map_data.stream1);
	update_river_stream(buffer, &map_data.stream2);
	
	prev = buffer;
	d = buffer + 1;
	next = buffer + 2;
	for (remaining = MAP_W - 2; remaining; remaining--) {
		// Handle vertical edges
		if (*prev == TILE_LAND && *d == TILE_WATER) *prev = 18;
		if (*d == TILE_WATER && *next == TILE_LAND) *next = 16;

		prev++;
		d++;
		next++;
	}
}

void draw_map_row() {
	static char i, j;
	static char y;
	static char *map_char;
	static unsigned int base_tile, tile;
	static char *buffer;
	
	buffer = map_data.circular_buffer[map_data.background_y];

	generate_map_row(buffer);

	for (i = 2, y = map_data.background_y, base_tile = 256; i; i--, y++, base_tile++) {
		SMS_setNextTileatXY(0, y);
		for (j = 16, map_char = buffer; j; j--, map_char++) {
			tile = base_tile + (*map_char << 2);
			SMS_setTile(tile);
			SMS_setTile(tile + 2);
		}
	}
	
	//map_data.next_row += 16;
	if (*map_data.next_row == 0xFF) {
		// Reached the end of the map; reset
		map_data.next_row = map_data.level_data;
	}
	
	if (map_data.background_y) {
		map_data.background_y -= 2;
	} else {
		map_data.background_y = SCROLL_CHAR_H - 2;
	}
	map_data.lines_before_next = 15;
}

void draw_map_screen() {
	map_data.background_y = SCREEN_CHAR_H - 2;
	
	while (map_data.background_y < SCREEN_CHAR_H) {
		draw_map_row();
	}
	draw_map_row();
}

void draw_map() {
	if (map_data.lines_before_next) {
		map_data.lines_before_next--;
	} else {
		draw_map_row();
	}

	SMS_setBGScrollY(map_data.scroll_y);
	if (map_data.scroll_y) {
		map_data.scroll_y--;
	} else {
		map_data.scroll_y = SCROLL_H - 1;
	}
}
