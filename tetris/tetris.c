#include "fonts.h"
#include "graphic.h"
#include "string.h"
#include "timer.h"
#include "utils.h"
#include "bug.h"
#include "io.h"
#include "system.h"

#include "tausta.h"

#define EXCP_VECTOR	RAM

/*
 * PS/2 rekisterit
 * L‰hde: ftp.altera.com/up/pub/University_Program_IP_Cores/PS2.pdf
 */
#define PS2_DATA		0
#define PS2_CONTROL		1

/*
 * L‰hde: http://www.computer-engineering.org/ps2keyboard/scancodes2.html
 */
#define KEY_ENTER		0x5A
#define KEY_LEFT		0x6B
#define KEY_RIGHT		0x74
#define KEY_EXTENDED		0xE0
#define KEY_RELEASE		0xF0

#define WIDTH		10
#define HEIGHT		20

#define EMPTY		0	/* musta */

uint32_t stack[4*1024];

static uint16_t game[HEIGHT][WIDTH];
static volatile int blk_x, blk_y;

static int palikka, score;
static uint16_t vari;
static uint16_t random = 0xbeefu;
static volatile int got_enter, in_game = 0, key_release = 0;

static uint8_t tausta_pixels[SCR_WIDTH * SCR_HEIGHT];
static struct PAL_IMAGE tausta;

static unsigned char palikat[4 * 3][3] = {
	{0, 1, 0},
	{0, 1, 0},
	{0, 1, 0},

	{1, 1, 0},
	{1, 1, 0},
	{0, 0, 0},

	{0, 1, 0},
	{1, 1, 1},
	{0, 1, 0},

	{0, 1, 0},
	{0, 1, 0},
	{0, 1, 1}
};

void bug()
{
	draw_string_shadow(&screen, 0, 0, "BUG TRAP");

	while (1) ;
}

/* 
 * Piirt‰‰ pelikent‰n
 */
static void draw_game()
{
	unsigned int x, y;

	for (y = 0; y < HEIGHT; ++y) {
		for (x = 0; x < WIDTH; ++x) {
			fill_rect(&screen,
				  SCR_WIDTH / 2 - WIDTH * 5 + x * 10,
				  SCR_HEIGHT / 2 - HEIGHT * 5 + y * 10,
				  10, 10, game[y][x]);
		}
	}
}

static int check_xy(int x, int y)
{
	if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT)
		return 0;
	if (game[y][x] != EMPTY)
		return 0;
	return 1;
}

/*
 * Tarkastetaan onko palikan sijainti sallittu
 */
static int check_block()
{
	unsigned int x, y;
	for (y = 0; y < 3; ++y) {
		for (x = 0; x < 3; ++x) {
			if (palikat[palikka * 3 + y][x]) {
				if (!check_xy(blk_x + x, blk_y + y))
					return 0;
			}
		}
	}
	return 1;
}

/*
 * Asetetaan palikan ruudut kent‰ll‰
 */
static void set_block(uint16_t val)
{
	unsigned int x, y;
	for (y = 0; y < 3; ++y) {
		for (x = 0; x < 3; ++x) {
			if (palikat[palikka * 3 + y][x]) {
				game[blk_y + y][blk_x + x] = val;
			}
		}
	}
}

static void init_block()
{
	blk_x = 3;
	blk_y = 0;
	palikka = (random >> 10) & 3;

	/* arvotaan palikan v‰ri */
	do {
		vari = RGB((random & 0x8000) ? 0 : 255,
			   (random & 0x4000) ? 0 : 255,
			   (random & 0x2000) ? 0 : 255);
		random = random * 16807 + 1234;
	} while (!vari);
}

static void draw_score()
{
	char buf[10];

	copy_pal_image(&screen, 100, SCR_HEIGHT / 2 - HEIGHT * 5 + 15, 8 * 4,
		       15, &tausta, 100, SCR_HEIGHT / 2 - HEIGHT * 5 + 15);
	number_str(buf, score, 10);
	draw_string_shadow(&screen, 100, SCR_HEIGHT / 2 - HEIGHT * 5 + 15,
			   buf);
}

static void play()
{
	unsigned int x, y, i;

	draw_string_shadow(&screen, 100, SCR_HEIGHT / 2 - HEIGHT * 5,
			   "Score:");

	/* alustus */
	for (y = 0; y < HEIGHT; ++y) {
		for (x = 0; x < WIDTH; ++x) {
			game[y][x] = EMPTY;
		}
	}
	score = 0;
	init_block();
	set_block(vari);

	in_game = 1;
	draw_game();
	draw_score();

	while (1) {
		msleep(200);

		/* liikutetaan alasp‰in */
		disable_irqs();
		set_block(EMPTY);
		blk_y++;
		if (!check_block()) {
			/* ei onnistu! */
			blk_y--;
			set_block(vari);

			/* tarkistetaan t‰ydet rivit */
			y = HEIGHT;
			while (y--) {
				for (x = 0; x < WIDTH; ++x) {
					if (game[y][x] == EMPTY)
						goto skip;
				}

				/* siirret‰‰n rivej‰ alas */
				i = y;
				while (i--) {
					for (x = 0; x < WIDTH; ++x) {
						game[i + 1][x] = game[i][x];
					}
				}
				for (x = 0; x < WIDTH; ++x) {
					game[0][x] = EMPTY;
				}
				score++;
				y++;

			      skip:;
			}
			draw_score();

			init_block();
			if (!check_block()) {
				enable_irqs();
				break;
			}
		}
		set_block(vari);
		draw_game();
		enable_irqs();
	}

	in_game = 0;

	draw_string_shadow(&screen, 10, SCR_HEIGHT - get_text_height() - 1,
			   "Game over, press enter");

	got_enter = 0;
	while (!got_enter) ;

	draw_pal_image(&screen, 0, 0, &tausta);
}

static void keyboard_irq()
{
	uint8_t val = IORD(PS2, PS2_DATA);

	if (val == KEY_RELEASE) {
		/* seuraava merkki on napin vapautus */
		key_release = 1;
		return;
	}
	else if (val == KEY_EXTENDED) {
		/* seuraava merkki on laajennettu, meit‰ ei kiinnosta */
		return;
	}

	if (val == KEY_ENTER && !key_release) {
		got_enter = 1;
	} else if (val == KEY_LEFT && !key_release && in_game) {
		set_block(EMPTY);
		blk_x--;
		if (!check_block())
			blk_x++;
		set_block(vari);
		draw_game();
	} else if (val == KEY_RIGHT && !key_release && in_game) {
		set_block(EMPTY);
		blk_x++;
		if (!check_block())
			blk_x--;
		set_block(vari);
		draw_game();
	}

	key_release = 0;
}

void exception_handler(uint32_t irqs_pending)
{
	if (irqs_pending & 1u << TIMER_IRQ) {
		timer_irq();
	} else if (irqs_pending & 1u << PS2_IRQ) {
		keyboard_irq();
	} else {
		bug();
	}
}

static void init_exception()
{
	uint32_t *jump = (void *)EXCP_VECTOR;

	/* kirjoitetaan hyppyk‰sky */
	*jump = (uint32_t) & excp_wrapper << 4 | 0x1;

	/* varmistetaan ett‰ prosessori n‰kee uuden k‰skyn */
	flush_dcache(EXCP_VECTOR);

	/* muokataan jo ajettua koodia! */
	flush_icache(EXCP_VECTOR);

	write_irq_mask(0);
	enable_irqs();
}

int main()
{
	init_graphics();
	init_exception();
	init_timer();

	write_irq_mask(1u << TIMER_IRQ | 1u << PS2_IRQ);

	/* puretaan taustakuva */
	tausta.width = tausta_png_width;
	tausta.height = tausta_png_height;
	tausta.palette = tausta_png_palette;
	tausta.pixels = tausta_pixels;
	extract_image(&tausta, tausta_png_data);

	draw_pal_image(&screen, 0, 0, &tausta);

	while (1) {
		draw_string_shadow(&screen, 10,
				   SCR_HEIGHT - get_text_height() - 1,
				   "Press enter to play!");

		got_enter = 0;
		while (!got_enter) ;

		draw_pal_image(&screen, 0, 0, &tausta);

		play();
	}

	return 0;
}
