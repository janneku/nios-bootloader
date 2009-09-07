#include "types.h"
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
#include "malloc.h"
#include "mad.h"

#include "mplay_tausta.h"

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
#define KEY_A			0x1c

#define PLAYLIST_W 350
#define PLAYLIST_H 400

uint32_t stack[64 * 1024];

static uint8_t tausta_pixels[SCR_WIDTH * SCR_HEIGHT];
static struct PAL_IMAGE tausta;
static int key_release;

static struct FAT32_FILE mp3file;
static struct FAT32_DIR dir;

static uint8_t input_buffer[1024*2];

#define PLOTMEM_SIZE 290
static int plotmem[PLOTMEM_SIZE];

static volatile struct {
	int enter;
	int arrow;
	int esc;
} key_status;

static int autoplay;

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

static void init_exception()
{
	uint32_t *jump = (void *)EXCP_VECTOR;

	/* kirjoitetaan hyppyk‰sky */
	*jump = (uint32_t) & excp_wrapper << 4 | 0x1;

	/* varmistetaan ett‰ k‰skyt on kirjoitettu muistiin */
	flush_dcache(EXCP_VECTOR);

	/* muokataan jo ajettua koodia! */
	flush_icache(EXCP_VECTOR);

	write_irq_mask(0);
	enable_irqs();
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

	if (!key_release)
		switch (val) {
		case KEY_ENTER: key_status.enter = 1; break;
		case KEY_ESCAPE: key_status.esc = 1; break;
		case KEY_UPARROW: key_status.arrow = -1; break;
		case KEY_DOWNARROW: key_status.arrow = 1; break;
		case KEY_A: autoplay = 1; break;
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

static enum mad_flow input(void *data, struct mad_stream *stream)
{
	struct FAT32_FILE *f = data;
	unsigned int left, n;

	if (stream->next_frame) {
		left = ((unsigned long)input_buffer + sizeof(input_buffer)) -
				(unsigned long)stream->next_frame;

		memcpy(input_buffer, stream->next_frame, left);
	} else
		left = 0;

	n = fat32_read(input_buffer+left, sizeof(input_buffer)-left, f);
	if (!n)
		return MAD_FLOW_STOP;

	mad_stream_buffer(stream, input_buffer, left+n);

	return MAD_FLOW_CONTINUE;
}

static enum mad_flow output(void *data,
		     			struct mad_header const *header,
		     			struct mad_pcm *pcm)
{
	unsigned int nchannels, nsamples, i;
	mad_fixed_t const *left_ch, *right_ch;
	int samplel, sampler;

	if (key_status.arrow) {
		move_menu(key_status.arrow);
		key_status.arrow = 0;
	} else if (key_status.enter) {
		if (is_dir()) {
			key_status.enter = 0;
			fat32_chdir(&dir, give_current());
			load_menu(&dir);
		} else
			return MAD_FLOW_STOP;
	} else if (key_status.esc) {
		key_status.esc = 0;
		return MAD_FLOW_STOP;
	}

	nsamples = pcm->length;
	left_ch = pcm->samples[0];
	right_ch = pcm->samples[1];

	/* n‰ytet‰‰n aalto */
	for (i = 0; i < PLOTMEM_SIZE; i++) {
		samplel = left_ch[i] >> (MAD_F_FRACBITS - 5);
		if (samplel >= 32)
			samplel = 31;
		plot(&screen, i, plotmem[i],
			tausta.palette[tausta.pixels
					[i + tausta.width * plotmem[i]]
					]);
		plotmem[i] = samplel + SCR_HEIGHT/2;
		plot(&screen, i, plotmem[i], WHITE);
	}

	if (pcm->channels == 2) {
		while (nsamples--) {
			samplel = *left_ch++ >> (MAD_F_FRACBITS - 13);
			sampler = *right_ch++ >> (MAD_F_FRACBITS - 13);
			IOWR(AUDIO_FIFO, 0, (samplel << 16) | (sampler & 0xffff));
		}
	} else {
		while (nsamples--) {
			samplel = *left_ch++ >> (MAD_F_FRACBITS - 13);
			IOWR(AUDIO_FIFO, 0, (samplel << 16) | (samplel & 0xffff));
		}
	}

	return MAD_FLOW_CONTINUE;
}

static enum mad_flow error(void *data, struct mad_stream *stream,
					struct mad_frame *frame)
{
	put_string(mad_stream_errorstr(stream));
	put_string("\n");

	return MAD_FLOW_CONTINUE;
}

static int decode(struct FAT32_FILE *f)
{
	struct mad_decoder decoder;
	int result;

	mad_decoder_init(&decoder, f,
		    	input, 0 /* header */, 0 /* filter */, output,
		    	error, 0 /* message */);

	result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

  	mad_decoder_finish(&decoder);

  	return result;
}

static int load_fs(struct FAT32_DIR *dir)
{
	draw_string_shadow(&screen,
			   (SCR_WIDTH -
				get_text_width("Loading playlist...")) / 2,
			   SCR_HEIGHT / 2, "Loading playlist...");

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

	copy_pal_image(&screen, 0, SCR_HEIGHT / 2, SCR_WIDTH,
		       get_text_height(), &tausta, 0, SCR_HEIGHT / 2);

	return 0;
}

int main()
{
	int i;

	malloc_init();
	init_graphics(1);
	init_exception();
	init_timer();

	/* puretaan taustakuva */
	tausta.width = mplay_tausta_png_width;
	tausta.height = mplay_tausta_png_height;
	tausta.palette = mplay_tausta_png_palette;
	tausta.pixels = tausta_pixels;
	extract_image(&tausta, mplay_tausta_png_data);

	key_release = 0;

	/* sallitaan nappien keskeytykset */
	IOWR(KEYS, PIO_INTERRUPTMASK, 1 | 2 | 4);

	/* PS/2 keskeytys p‰‰lle */
	IOWR(PS2, PS2_CONTROL, 1);

	write_irq_mask(1u << KEYS_IRQ | 1u << TIMER_IRQ | 1u << PS2_IRQ);

	draw_pal_image(&screen, 0, 0, &tausta);

	while (load_fs(&dir))
		messagebox("Please insert SD-card");

	init_menu(&dir, SCR_WIDTH-PLAYLIST_W,
			SCR_HEIGHT-PLAYLIST_H,
			PLAYLIST_W,
			PLAYLIST_H,
			WHITE,
			BLACK,
			&tausta,
			"mp3");

	key_status.enter = key_status.esc = key_status.arrow = 0;

	autoplay = 0;

	for (i=0; i<PLOTMEM_SIZE; i++)
		plotmem[i] = 0;

	while (1) {
		if (key_status.arrow) {
			move_menu(key_status.arrow);

			key_status.arrow = 0;
		}
		else if (key_status.enter) {
			key_status.enter = key_status.esc = key_status.arrow = 0;

			if (is_dir()) {
				fat32_chdir(&dir, give_current());
				load_menu(&dir);
			} else {
				fat32_fopen(&dir, give_current(), &mp3file);
				put_string("decoding...\n");
				mark_current();
				decode(&mp3file);
				remove_marks();
				show_menu();

				while (!key_status.enter && autoplay
							&& move_menu(1))
					if (!is_dir()) {
						key_status.enter = 1;
						break;
					}
			}

			key_status.enter = key_status.esc = key_status.arrow = 0;
		}
		else if (key_status.esc) {
			while (load_fs(&dir))
				messagebox("Please insert SD-card");
			load_menu(&dir);

			key_status.esc = key_status.enter = 0;
		}
	}

}
