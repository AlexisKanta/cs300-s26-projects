#include "game_setup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "game.h"
#include "common.h"

// Some handy dandy macros for decompression
#define E_CAP_HEX 0x45
#define E_LOW_HEX 0x65
#define G_CAP_HEX 0x47
#define G_LOW_HEX 0x67
#define S_CAP_HEX 0x53
#define S_LOW_HEX 0x73
#define W_CAP_HEX 0x57
#define W_LOW_HEX 0x77
#define DIGIT_START 0x30
#define DIGIT_END 0x39

/** Initializes the board with walls around the edge of the board, and a ring
 * of grass just inside the wall.
 *
 * Modifies values pointed to by cells_p, width_p, and height_p and initializes
 * cells array to reflect this default board.
 *
 * Returns INIT_SUCCESS to indicate that it was successful.
 *
 * Arguments:
 *  - cells_p: a pointer to a memory location where a pointer to the first
 *             element in a newly initialized array of cells should be stored.
 *  - width_p: a pointer to a memory location where the newly initialized
 *             width should be stored.
 *  - height_p: a pointer to a memory location where the newly initialized
 *              height should be stored.
 */

enum board_init_status set_dimensions(char* dim_str, size_t* height_p, size_t* width_p) {
    if (dim_str[0] != 'B') {
        return INIT_ERR_BAD_CHAR;
    }

    char* height_start = dim_str + 1;
    char* width_start = strchr(height_start, 'x');

    if (width_start == NULL) {
        return INIT_ERR_BAD_CHAR;
    }

    *height_p = (size_t)strtol(height_start, NULL, 10);
    *width_p = (size_t)strtol(width_start + 1, NULL, 10);

    if (*height_p == 0 || *width_p == 0) {
        return INIT_ERR_BAD_CHAR;
    }

    return INIT_SUCCESS;
}

int get_cell_pos(size_t row, size_t col, size_t width) {
    return row * width + col;
}

enum board_init_status fill_cells(int* cells, int start_pos, int count, char cell_type, size_t* snake_count) {
    int flag = 0;

    switch (cell_type) {
        case 'W':
            flag = FLAG_WALL;
            break;
        case 'E':
            flag = PLAIN_CELL;
            break;
        case 'G':
            flag = FLAG_GRASS;
            break;
        case 'S':
            if (count != 1) {
                return INIT_ERR_WRONG_SNAKE_NUM;
            }
            flag = FLAG_SNAKE;
            (*snake_count)++;
            break;
        default:
            return INIT_ERR_BAD_CHAR;
    }

    for (int i=0; i < count; i++) {
        cells[start_pos + i] = flag;
    }

    return INIT_SUCCESS;
}


enum board_init_status initialize_default_board(int** cells_p, size_t* width_p,
                                                size_t* height_p) {
    *width_p = 20;
    *height_p = 10;
    int* cells = malloc(20 * 10 * sizeof(int));
    *cells_p = cells;
    for (int i = 0; i < 20 * 10; i++) {
        cells[i] = PLAIN_CELL;
    }

    // Set edge cells!
    // Top and bottom edges:
    for (int i = 0; i < 20; ++i) {
        cells[i] = FLAG_WALL;
        cells[i + (20 * (10 - 1))] = FLAG_WALL;
    }
    // Left and right edges:
    for (int i = 0; i < 10; ++i) {
        cells[i * 20] = FLAG_WALL;
        cells[i * 20 + 20 - 1] = FLAG_WALL;
    }

    // Set grass cells!
    // Top and bottom edges:
    for (int i = 1; i < 19; ++i) {
        cells[i + 20] = FLAG_GRASS;
        cells[i + (20 * (9 - 1))] = FLAG_GRASS;
    }
    // Left and right edges:
    for (int i = 1; i < 9; ++i) {
        cells[i * 20 + 1] = FLAG_GRASS;
        cells[i * 20 + 19 - 1] = FLAG_GRASS;
    }

    // Add snake
    cells[20 * 2 + 2] = FLAG_SNAKE;

    return INIT_SUCCESS;
}

/** Initialize variables relevant to the game board.
 * Arguments:
 *  - cells_p: a pointer to a memory location where a pointer to the first
 *             element in a newly initialized array of cells should be stored.
 *  - width_p: a pointer to a memory location where the newly initialized
 *             width should be stored.
 *  - height_p: a pointer to a memory location where the newly initialized
 *              height should be stored.
 *  - snake_p: a pointer to your snake struct (not used until part 3!)
 *  - board_rep: a string representing the initial board. May be NULL for
 * default board.
 */
enum board_init_status initialize_game(int** cells_p, size_t* width_p,
                                       size_t* height_p, snake_t* snake_p,
                                       char* board_rep) {
            
    enum board_init_status status;

    if (board_rep != NULL) {
        status = decompress_board_str(cells_p, width_p, height_p, snake_p, board_rep);
        if (status != INIT_SUCCESS) {
            return status;
        }
    } else {
        status = initialize_default_board(cells_p, width_p, height_p);
        if (status != INIT_SUCCESS) {
            return status;
        }
        snake_head = 2 * 20 + 2;
    }

    direction = INPUT_NONE;
    g_game_over = 0;
    g_score =0;

    place_food(*cells_p, *width_p, *height_p);

    return status;
}

/** Takes in a string `compressed` and initializes values pointed to by
 * cells_p, width_p, and height_p accordingly. Arguments:
 *      - cells_p: a pointer to the pointer representing the cells array
 *                 that we would like to initialize.
 *      - width_p: a pointer to the width variable we'd like to initialize.
 *      - height_p: a pointer to the height variable we'd like to initialize.
 *      - snake_p: a pointer to your snake struct (not used until part 3!)
 *      - compressed: a string that contains the representation of the board.
 * Note: We assume that the string will be of the following form:
 * B24x80|E5W2E73|E5W2S1E72... To read it, we scan the string row-by-row
 * (delineated by the `|` character), and read out a letter (E, S or W) a number
 * of times dictated by the number that follows the letter.
 */
enum board_init_status decompress_board_str(int** cells_p, size_t* width_p,
                                            size_t* height_p, snake_t* snake_p,
                                            char* compressed) {
    char* compressed_copy = malloc(strlen(compressed) + 1);
    strcpy(compressed_copy, compressed);

    size_t height = 0;
    size_t width = 0;

    char* token = strtok(compressed_copy, "|");
    if (token == NULL) {
        free(compressed_copy);
        return INIT_ERR_BAD_CHAR;
    }

    enum board_init_status status = set_dimensions(token, &height, &width);
    if (status != INIT_SUCCESS) {
        free(compressed_copy);
        return status;
    }

    int* cells = (int*)malloc(width * height * sizeof(int));
    if (cells == NULL) {
        free(compressed_copy);
        return INIT_ERR_BAD_CHAR;
    }

    size_t snake_count = 0;
    int cell_pos = 0;
    size_t total_cells = width * height;
    size_t row_count = 0;

    while ((token = strtok(NULL, "|")) != NULL) {
        if (row_count >= height) {
            free(cells);
            free(compressed_copy);
            return INIT_ERR_INCORRECT_DIMENSIONS;
        }

        size_t i = 0;
        int row_cells = 0;

        while (i < strlen(token)) {
            char cell_type = token[i];
            i++;

            char num_str[20] = {0};
            int num_idx = 0;
            while (i < strlen(token) && token[i] >= '0' && token[i] <= '9') {
                num_str[num_idx++] = token[i];
                i++;
            }

            if (num_idx == 0) {
                free(cells);
                free(compressed_copy);
                return INIT_ERR_BAD_CHAR;
            }

            int count = atoi(num_str);
            row_cells += count;

            if (row_cells > (int)width) {
                free(cells);
                free(compressed_copy);
                return INIT_ERR_INCORRECT_DIMENSIONS;
            }

            if (cell_pos + count > (int)total_cells) {
                free(cells);
                free(compressed_copy);
                return INIT_ERR_INCORRECT_DIMENSIONS;
            }

            status = fill_cells(cells, cell_pos, count, cell_type, &snake_count);
            if (status != INIT_SUCCESS) {
                free(cells);
                free(compressed_copy);
                return status;
            }

            cell_pos += count;
        }

        if (row_cells != (int)width) {
            free(cells);
            free(compressed_copy);
            return INIT_ERR_INCORRECT_DIMENSIONS;
        }

        row_count++;
    }

    if (row_count != height) {
        free(cells);
        free(compressed_copy);
        return INIT_ERR_INCORRECT_DIMENSIONS;
    }

    if (cell_pos != (int)total_cells) {
        free(cells);
        free(compressed_copy);
        return INIT_ERR_INCORRECT_DIMENSIONS;
    }

    if (snake_count != 1) {
        free(cells);
        free(compressed_copy);
        return INIT_ERR_WRONG_SNAKE_NUM;
    }

    for (int i = 0; i < (int)total_cells; i++) {
        if (cells[i] & FLAG_SNAKE) {
            snake_head = i;
            break;
        }
    }

    direction = INPUT_NONE;
    
    *cells_p = cells;
    *width_p = width;
    *height_p = height;

    free(compressed_copy);
    return INIT_SUCCESS;
}
