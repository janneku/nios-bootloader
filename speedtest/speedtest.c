#include "graphic.h"
#include "string.h"
#include "io.h"
#include "timer.h"
#include "fonts.h"
#include "string.h"
#include "utils.h"
#include "system.h"

#include "pallo.h"

#define EXCP_VECTOR	RAM

#define PALLOJA		100
#define SPEED		(1<<16)

uint32_t stack[4*1024];

struct Pallo {
	int x, y;
	int xvel, yvel;		/* nopeus */
};

static struct Pallo pallot[PALLOJA];
static uint8_t pallo_pixels[pallo_png_width * pallo_png_height];
static uint16_t doublebuffer_pixels[SCR_WIDTH * SCR_HEIGHT];
static struct IMAGE doublebuffer;
static struct PAL_IMAGE pallo;

void bug()
{
	draw_string_shadow(&screen, 0, 0, "BUG TRAP");

	while (1) ;
}

void exception_handler(uint32_t irqs_pending)
{
	if (irqs_pending & 1u << TIMER_IRQ) {
		timer_irq();
	} else {
		bug();
	}
}

static void init_exception()
{
	uint32_t *jump = (void *)EXCP_VECTOR;
	
	/* kirjoitetaan hyppykäsky */
	*jump = (uint32_t)&excp_wrapper << 4 | 0x1;
	
	/* varmistetaan että prosessori näkee uuden käskyn */
	flush_dcache(EXCP_VECTOR);

	/* muokataan jo ajettua koodia! */
	flush_icache(EXCP_VECTOR);
	
	write_irq_mask(0);
	enable_irqs();
}

int main()
{
	unsigned int i, frames = 0, sekuntti, fps = 0;
	uint16_t random = 0xbeefu;
	char buf[10];
	
	init_graphics(1);
	init_exception();
	init_timer();
	
	doublebuffer.width = SCR_WIDTH;
	doublebuffer.height = SCR_HEIGHT;
	doublebuffer.pixels = doublebuffer_pixels;

	/* puretaan kuva */
	pallo.width = pallo_png_width;
	pallo.height = pallo_png_height;
	pallo.palette = pallo_png_palette;
	pallo.pixels = pallo_pixels;
	extract_image(&pallo, pallo_png_data);
	
	for (i = 0; i < PALLOJA; ++i) {
		pallot[i].x = (50 + (i >> 3) * 20) << 16;
		pallot[i].y = (20 + (i & 7) * 20) << 16;
		pallot[i].xvel = pallot[i].yvel = 0;
		pallot[i].xvel = ((int)(random >> 12) - 8) * SPEED / 8;
		pallot[i].yvel = ((int)((random >> 8) & 0xF) - 8) * SPEED / 8;
		random = random * 16807 + 1234;
	}
	
	fill_rect(&doublebuffer, 0, 0, SCR_WIDTH, SCR_HEIGHT, BLACK);
	
	sekuntti = get_ticks() + HZ;
	
	while (1) {
		for (i = 0; i < PALLOJA; ++i) {
			fill_rect(&doublebuffer, pallot[i].x >> 16,
				pallot[i].y >> 16,
				pallo_png_width, pallo_png_height, BLACK);
		}
		for (i = 0; i < PALLOJA; ++i) {
			pallot[i].x += pallot[i].xvel;
			pallot[i].y += pallot[i].yvel;
			
			pallot[i].xvel += ((SCR_WIDTH << 15) - pallot[i].x) >> 10;
			pallot[i].yvel += ((SCR_HEIGHT << 15) - pallot[i].y) >> 10;
			
			draw_pal_image(&doublebuffer, pallot[i].x >> 16,
				pallot[i].y >> 16, &pallo);
		}
		draw_image(&screen, 0, 0, &doublebuffer);
		
		number_str(buf, fps, 10);
		draw_string(&screen, 0, 0, buf, WHITE);
		frames++;
		
		if (get_ticks() >= sekuntti) {
			fps = frames;
			frames = 0;
			sekuntti += HZ;
		}
	}
	
	return 0;
}
