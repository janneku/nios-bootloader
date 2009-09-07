#include "my_boot.h"
#include "jtag.h"
#include "fonts.h"
#include "graphic.h"
#include "string.h"
#include "timer.h"
#include "utils.h"
#include "fat32.h"
#include "sdcard.h"
#include "bug.h"
#include "io.h"
#include "def.h"
#include "menu.h"

#include "tausta.h"

/*
 * PIO rekisterit
 * L‰hde: www.altera.com/literature/hb/nios2/n2cpu_nii51007.pdf
 */
#define PIO_DATA		0
#define PIO_INTERRUPTMASK	2
#define PIO_EDGECAPTURE		3

/*
 * PS/2 rekisterit
 * L‰hde: ftp.altera.com/up/pub/University_Program_IP_Cores/PS2.pdf
 */
#define PS2_DATA		0
#define PS2_CONTROL		1

/*
 * L‰hde: http://www.computer-engineering.org/ps2keyboard/scancodes2.html
 */
#define KEY_EXTENDED		0xE0
#define KEY_RELEASE		0xF0
#define KEY_ENTER		0x5A
#define KEY_UPARROW		0x75
#define KEY_DOWNARROW		0x72
#define KEY_ESCAPE		0x76

/* Lataajan pino */
uint32_t stack[1024];

#define MEMTEST_ADDR		(RAM + 4)
#define MEMTEST_SIZE		((BSS_ADDR - MEMTEST_ADDR)/4 / 2)

/* muuttujia ei voi alustaa t‰ss‰ */
static uint8_t tausta_pixels[SCR_WIDTH * SCR_HEIGHT];
static struct PAL_IMAGE tausta;
static uint16_t palette[256];
static int key_release;

static volatile struct {
	int enter;
	int arrow;
	int esc;
} key_status;

void bug()
{
	disable_irqs();

	put_string("\nBUG TRAP!\n");

	/* blue screen! */
	fill_rect(&screen, 0, 0, SCR_WIDTH, SCR_HEIGHT, RGB(0, 0, 128));

	draw_string(&screen, 0, 0, "BUG TRAP", WHITE);

	/* pys‰ytet‰‰n suoritus */
	while (1) ;
}

static void messagebox(const char *s)
{
	/* Tyhj‰ tausta tekstille */
	copy_pal_image(&screen, 0, SCR_HEIGHT / 2, SCR_WIDTH,
		       get_text_height()*2 + 1, &tausta, 0, SCR_HEIGHT / 2);

	/* piiret‰‰n teksti */
	draw_string_shadow(&screen,
			   (SCR_WIDTH - get_text_width(s)) / 2,
			   SCR_HEIGHT / 2, s);
	draw_string_shadow(&screen,
			   (SCR_WIDTH - get_text_width("Press enter")) / 2,
			   SCR_HEIGHT / 2 + get_text_height(), "Press enter");

	key_status.enter = 0;
	while (!key_status.enter) ;

	/* poistetaan teksti */
	copy_pal_image(&screen, 0, SCR_HEIGHT / 2, SCR_WIDTH,
		       get_text_height()*2 + 1, &tausta, 0, SCR_HEIGHT / 2);
}

static void load_program(struct FAT32_FILE *bin)
{
	int i;
	void (*target_jmp) () = (void *)PROGRAM_ADDR;

	/* ei paluuta en‰‰n! */
	disable_irqs();

	/* tyhjennet‰‰n muisti */
	memset((void *)RAM, 0, BSS_ADDR - RAM);

	/* luetaan ohjelma */
	// TODO: latauspalkki!
	fat32_read((void *)PROGRAM_ADDR, BSS_ADDR - RAM, bin);

	/*
	 * varmistetaan ett‰ k‰skyt on kirjoitettu muistiin, josta prosessori
	 * saa noudettua ja suoritettua ne
	 */
	for (i = 0; i < NIOS2_DCACHE_SIZE; i += NIOS2_DCACHE_LINE_SIZE) {
		flush_dcache(i);
	}

	/*
	 * exception hyppykoodi on samassa paikassa kuin ladattu
	 * ohjelma, varmistetaan ett‰ prosessori hakee uudet k‰skyt
	 */
	flush_icache(EXCP_VECTOR);

	/* Fade out */
	tausta.palette = palette;
	for (i = 0; i < 32; i++) {
		shade_palette(palette, tausta_png_palette, 32 - i);
		draw_pal_image(&screen, 0, 0, &tausta);
	}
	tausta.palette = tausta_png_palette;
	fill_rect(&screen, 0, 0, SCR_WIDTH, SCR_HEIGHT, BLACK);

	put_string("load_program(): target jmp\n");

	target_jmp();
}

static void button_irq()
{
	uint8_t val = IORD(KEYS, PIO_DATA);

	if (val & 1) {
		key_status.enter = 1;
	}
	if (val & 2) {
		key_status.arrow = -1;
	}
	if (val & 4) {
		key_status.arrow = 1;
	}

	/* nollataan keskeytys */
	IOWR(KEYS, PIO_EDGECAPTURE, 0);
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

	if (!key_release) {
		switch (val) {
		case KEY_ENTER: key_status.enter = 1; break;
		case KEY_ESCAPE: key_status.esc = 1; break;
		case KEY_UPARROW: key_status.arrow = -1; break;
		case KEY_DOWNARROW: key_status.arrow = 1; break;
		}
	}

	key_release = 0;
}

void exception_handler(uint32_t irqs_pending)
{
	if (irqs_pending & 1u << KEYS_IRQ) {
		button_irq();
	} else if (irqs_pending & 1u << TIMER_IRQ) {
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

	/*
	 * varmistetaan ett‰ k‰sky on kirjoitettu muistiin, josta prosessori
	 * saa noudettua ja suoritettua sen
	 */
	flush_dcache(EXCP_VECTOR);

	write_irq_mask(0);
	enable_irqs();
}

#if 0

static void memtest()
{
	char buf[10];
	unsigned int i = 0, val = 0xdeadbeefu, count = 0;
	uint32_t *buffer1 = (void *)MEMTEST_ADDR;
	uint32_t *buffer2 = buffer1 + MEMTEST_SIZE;

	put_string("memtest()\n");

	/* t‰ytet‰‰n memtest alue */
	for (i = 0; i < MEMTEST_SIZE; ++i) {
		buffer1[i] = buffer2[i] = val;
		val ^= ((val << 4) + i) ^ 0x1337u;
	}

	key_status.enter = 0;

	i = 0;
	while (!key_status.enter) {
		/* tarkistetaan... */
		if (buffer1[i] != buffer2[i]) {
			put_string("memtest(): fail\n");
			number_str(buf, buffer1[i], 16);
			put_string(buf);
			put_string(" <> ");
			number_str(buf, buffer2[i], 16);
			put_string(buf);
			bug();
		}

		/* ylikirjoitus */
		buffer1[i] = buffer2[i] = val;
		val ^= ((val << 4) + i) ^ 0x1337u;

		++i;
		if (i >= MEMTEST_SIZE) {
			i = 0;
			count++;
			if (count*8 < SCR_WIDTH-8) {
				put_string("!");
				draw_glyph(&screen, count*8, 100, '!', WHITE);
			}
		}
	}
}

#endif

static int load_fs(struct FAT32_DIR *dir)
{
	draw_string_shadow(&screen,
			   (SCR_WIDTH - get_text_width("Loading...")) / 2,
			   SCR_HEIGHT / 2, "Loading...");

	if (SD_card_init()) {
		messagebox("ERROR: Unable to initialize SD card");
		return -1;
	}

	if (fat32_init()) {
		messagebox("ERROR: Unable to find FAT32");
		return -1;
	}

	if (fat32_opendir("/", dir))
		bug();

	return 0;
}

int main()
{
	int i;
	struct FAT32_FILE bin;
	struct FAT32_DIR dir;

	put_string("\nBOOTLOADER " __DATE__ " " __TIME__ "\n\n");

	init_graphics(1);
	init_exception();
	init_timer();

	/* puretaan taustakuva */
	tausta.width = tausta_png_width;
	tausta.height = tausta_png_height;
	tausta.palette = tausta_png_palette;
	tausta.pixels = tausta_pixels;
	extract_image(&tausta, tausta_png_data);

	key_release = 0;

	/* sallitaan nappien keskeytykset */
	IOWR(KEYS, PIO_INTERRUPTMASK, 1 | 2 | 4);

	/* PS/2 keskeytys p‰‰lle */
	IOWR(PS2, PS2_CONTROL, 1);

	write_irq_mask(1u << KEYS_IRQ | 1u << TIMER_IRQ | 1u << PS2_IRQ);

	put_string("main(): initialization ok\n");

	/* viive jotta n‰yttˆ ehtii mukaan :) */
	msleep(200);

	/* Fade in */
	tausta.palette = palette;
	for (i = 0; i < 32; i++) {
		shade_palette(palette, tausta_png_palette, i);
		draw_pal_image(&screen, 0, 0, &tausta);
	}
	tausta.palette = tausta_png_palette;
	draw_pal_image(&screen, 0, 0, &tausta);

#if 0
	draw_string_shadow(&screen, 0, SCR_HEIGHT - get_text_height() - 1,
			   "Compiled at " __DATE__ " " __TIME__);
#endif

	while (load_fs(&dir))
		messagebox("Please insert SD-card");

	init_menu(&dir, SCR_WIDTH/3, SCR_HEIGHT/3, SCR_WIDTH/3,
			get_text_height()*6, WHITE, BLACK, 0, "bin");

	key_status.enter = key_status.esc = key_status.arrow = 0;

	while (1) {
		if (key_status.arrow) {
			move_menu(key_status.arrow);

			key_status.arrow = 0;
		}
		else if (key_status.enter) {
			if (is_dir()) {
				fat32_chdir(&dir, give_current());
				load_menu(&dir);
			} else {
				fat32_fopen(&dir, give_current(), &bin);
				load_program(&bin);
			}

			key_status.enter = 0;
		}
		else if (key_status.esc) {
			while (load_fs(&dir))
				messagebox("Please insert SD-card");
			load_menu(&dir);

			key_status.esc = key_status.enter = 0;
		}
	}

	return 0;
}
