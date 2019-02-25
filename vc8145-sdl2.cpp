/*
 * VICI VC8145 
 *
 * V0.1 - December 22, 2018
 * 
 *
 * Written by Paul L Daniels (pldaniels@gmail.com)
 *
 */

#include <SDL.h>
#include <SDL_ttf.h>

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define FL __FILE__,__LINE__

/*
 * Should be defined in the Makefile to pass to the compiler from
 * the github build revision
 *
 */
#ifndef BUILD_VER 
#define BUILD_VER 000
#endif

#ifndef BUILD_DATE
#define BUILD_DATE " "
#endif

#define SSIZE 1024

#define INTERFRAME_SLEEP	200000 // 0.2 seconds

#define DATA_FRAME_SIZE 12
#define ee ""
#define uu "\u00B5"
#define kk "k"
#define MM "M"
#define mm "m"
#define nn "n"
#define pp "p"
#define dd "\u00B0"
#define oo "\u03A9"

#define MMFLAG_AUTORANGE	0b01000000

char SEPARATOR_DP[] = ".";

struct serial_params_s {
	char *device;
	int fd, n;
	int cnt, size, s_cnt;
	struct termios oldtp, newtp;
};

struct meter_param {
	char mode[20];
	char units[20];
	int dividers[8];
	char prefix[8][2];
};

struct glb {
	uint8_t debug;
	uint8_t quiet;
	uint8_t show_mode;
	uint16_t flags;
	uint8_t range_control;
	uint8_t units_separator;
	char *com_address;
	char *output_file;

	struct serial_params_s serial_params;

	int font_size;
	int window_width, window_height;
	int wx_forced, wy_forced;
	SDL_Color font_color, background_color;
};

/*
 * A whole bunch of globals, because I need
 * them accessible in the Windows handler
 *
 * So many of these I'd like to try get away from being
 * a global.
 *
 */
struct glb *glbs;

/*
 * Test to see if a file exists
 *
 * Does not test anything else such as read/write status
 *
 */
bool fileExists(const char *filename) {
	struct stat buf;
	return (stat(filename, &buf) == 0);
}


char digit( unsigned char dg ) {

	int d;
	char g;

	switch (dg) {
		case 0x30: g = '0'; d = 0; break;
		case 0x31: g = '1'; d = 1; break;
		case 0x32: g = '2'; d = 2; break;
		case 0x33: g = '3'; d = 3; break;
		case 0x34: g = '4'; d = 4; break;
		case 0x35: g = '5'; d = 5; break;
		case 0x36: g = '6'; d = 6; break;
		case 0x37: g = '7'; d = 7; break;
		case 0x38: g = '8'; d = 8; break;
		case 0x39: g = '9'; d = 9; break;
		case 0x3E: g = 'L'; d = 0; break;
		case 0x3F: g = ' '; d = 0; break;
		default: g = ' ';
	}

	return g;
}

/*-----------------------------------------------------------------\
  Date Code:	: 20180127-220248
  Function Name	: init
  Returns Type	: int
  ----Parameter List
  1. struct glb *g ,
  ------------------
  Exit Codes	:
  Side Effects	:
  --------------------------------------------------------------------
Comments:

--------------------------------------------------------------------
Changes:

\------------------------------------------------------------------*/
int init(struct glb *g) {
	g->debug = 0;
	g->quiet = 0;
	g->flags = 0;
	g->range_control = 0;
	g->units_separator = 0;
	g->com_address = NULL;
	g->output_file = NULL;

	g->font_size = 60;
	g->window_width = 400;
	g->window_height = 100;
	g->wx_forced = 0;
	g->wy_forced = 0;

	g->show_mode = 0;
	g->font_color =  { 10, 200, 10 };
	g->background_color = { 0, 0, 0 };

	return 0;
}

void show_help(void) {
	fprintf(stdout,"VC8145 Multimeter decoder helper for FlexBV\r\n"
			"By Paul L Daniels / pldaniels@gmail.com\r\n"
			"Build %d / %s\r\n"
			"\r\n"
			" [-p <comport#>] [-s <serial port config>] [-d] [-q]\r\n"
			"\r\n"
			"\t-h: This help\r\n"
			"\t-p <comport>: Set the com port for the meter, eg: -p /dev/ttyUSB0\r\n"
			"\t-o <output file> ( used by FlexBV to read the data )\r\n"
			"\t-m: show mode on screen\r\n"
			"\t-u: use Units as the separator ( 8.09K becomes 8R09 )\r\n"
			"\t-d: debug enabled\r\n"
			"\t-q: quiet output\r\n"
			"\t-v: show version\r\n"
			"\t-z <font size in pt>\r\n"
			"\t-fc <foreground colour, f0f0ff>\r\n"
			"\t-bc <background colour, 101010>\r\n"
			"\r\n"
			"\t-r: Range control (VDC, VC8145 only)\r\n"
			"\r\n"
			"\texample: vc8145-sdl -m -p /dev/ttyUSB0\r\n"
			, BUILD_VER
			, BUILD_DATE 
			);
} 


/*-----------------------------------------------------------------\
  Date Code:	: 20180127-220258
  Function Name	: parse_parameters
  Returns Type	: int
  ----Parameter List
  1. struct glb *g,
  2.  int argc,
  3.  char **argv ,
  ------------------
  Exit Codes	:
  Side Effects	:
  --------------------------------------------------------------------
Comments:

--------------------------------------------------------------------
Changes:

\------------------------------------------------------------------*/
int parse_parameters(struct glb *g, int argc, char **argv ) {
	int i;

	if (argc == 1) {
		show_help();
		exit(1);
	}

	for (i = 0; i < argc; i++) {
		if (argv[i][0] == '-') {
			/* parameter */
			switch (argv[i][1]) {

				case 'h':
					show_help();
					exit(1);
					break;

				case 'z':
					i++;
					if (i < argc) {
						g->font_size = atoi(argv[i]);
					} else {
						fprintf(stdout,"Insufficient parameters; -z <font size pts>\n");
						exit(1);
					}
					break;

				case 'p':
					/*
					 * com port can be multiple things in linux
					 * such as /dev/ttySx or /dev/ttyUSBxx
					 */
					i++;
					if (i < argc) {
						g->serial_params.device = argv[i];
					} else {
						fprintf(stdout,"Insufficient parameters; -p <com port>\n");
						exit(1);
					}
					break;

				case 'o':
					/* 
					 * output file where this program will put the text
					 * line containing the information FlexBV will want 
					 *
					 */
					i++;
					if (i < argc) {
						g->output_file = argv[i];
					} else {
						fprintf(stdout,"Insufficient parameters; -o <output file>\n");
						exit(1);
					}
					break;

				case 'd': g->debug = 1; break;

				case 'q': g->quiet = 1; break;

				case 'v':
							 fprintf(stdout,"Build %d\r\n", BUILD_VER);
							 exit(0);
							 break;

				case 'f':
							 if (argv[i][2] == 'c') {
								 i++;
								 sscanf(argv[i], "%02x%02x%02x"
										 , &g->font_color.r
										 , &g->font_color.g
										 , &g->font_color.b
										 );

							 }
							 break;

				case 'b':
							 if (argv[i][2] == 'c') {
								 i++;
								 sscanf(argv[i], "%02x%02x%02x"
										 , &g->background_color.r
										 , &g->background_color.g
										 , &g->background_color.b
										 );
							 }
							 break;

				case 'w':
							 if (argv[i][2] == 'x') {
								 i++;
								 g->wx_forced = atoi(argv[i]);
							 } else if (argv[i][2] == 'y') {
								 i++;
								 g->wy_forced = atoi(argv[i]);
							 }
							 break;

				case 'm':
							 g->show_mode = 1;
							 break;

				case 'r':
							 g->range_control = 1;
							 break;

				case 'u':
							g->units_separator = 1;
							break;

				case 's':
							 // Not needed, we hard code at 9600-8n1 because
							 // that's what these meters should be doing.  
							 //
							 // If not, then feel free to adjust the parameter or
							 // populate this section.
							 break;

				default: break;
			} // switch
		}
	}

	return 0;
}



/*
 *
 * Open serial port for communitcations
 *
 */
void open_port(struct serial_params_s *s) {
	int r; 

	s->fd = open( s->device, O_RDWR | O_NOCTTY |O_NDELAY );
	if (s->fd <0) {
		perror( s->device );
	}

	fcntl(s->fd,F_SETFL,0);
//	fcntl(s->fd,F_SETFL,FNDELAY);

	tcgetattr(s->fd,&(s->oldtp)); // save current serial port settings 
	tcgetattr(s->fd,&(s->newtp)); // save current serial port settings in to what will be our new settings
	cfmakeraw(&(s->newtp));
	s->newtp.c_cflag = B9600 | CS8 | CLOCAL | CREAD; // Adjust the settings to suit our meter
	s->newtp.c_cflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
	s->newtp.c_cflag &= ~(PARENB | PARODD); // shut off parity
	s->newtp.c_cflag &= ~CSTOPB; 
	s->newtp.c_cflag &= ~CRTSCTS;

	r = tcsetattr(s->fd, TCSANOW, &(s->newtp));
	if (r) {
		fprintf(stderr,"%s:%d: Error setting terminal (%s)\n", FL, strerror(errno));
		exit(1);
	}
}

/*
 * Convert ASCII to hex (uint)
 *
 */
uint8_t a2h( uint8_t a ) {
	a -= 0x30;
	if (a < 10) return a;
	a -= 7;
	return a;
}

/*
 * Send single byte command to the meter
 *
 */
size_t cmd_send( struct glb *g, uint8_t cmd ) {
	size_t bytes_written = 0;

	bytes_written = write(g->serial_params.fd, &cmd, 1);

	return bytes_written;
}



/*
 * Read single byte from serial port.
 *
 * Could be generalised to bytes_read() but for
 * now this serves the purpose it's used for.
 *
 */
size_t byte_read( struct glb *g ) {
	uint8_t b = 0;
	size_t bytes_read;

	bytes_read = read(g->serial_params.fd, &b, 1);

	return b;
}


/*-----------------------------------------------------------------\
  Date Code:	: 20180127-220307
  Function Name	: main
  Returns Type	: int
  ----Parameter List
  1. int argc,
  2.  char **argv ,
  ------------------
  Exit Codes	:
  Side Effects	:
  --------------------------------------------------------------------
Comments:

--------------------------------------------------------------------
Changes:

\------------------------------------------------------------------*/
int main ( int argc, char **argv ) {

	SDL_Event event;
	SDL_Surface *surface, *surface_2;
	SDL_Texture *texture, *texture_2;

	char linetmp[SSIZE]; // temporary string for building main line of text
	char prefix[SSIZE]; // Units prefix u, m, k, M etc
	char units[SSIZE];  // Measurement units F, V, A, R
	char mmmode[SSIZE]; // Multimeter mode, Resistance/diode/cap etc

	uint8_t d[SSIZE];
	uint8_t dt[SSIZE];      // Serial data packet
	int dt_loaded = 0;	// set when we have our first valid data
	uint8_t dps = 0;     // Number of decimal places
	struct glb g;        // Global structure for passing variables around
	int i = 0;           // Generic counter
	char temp_char;        // Temporary character
	char tfn[4096];
	bool quit = false;

	glbs = &g;

	/*
	 * Initialise the global structure
	 */
	init(&g);

	/*
	 * Parse our command line parameters
	 */
	parse_parameters(&g, argc, argv);

	/* 
	 * check paramters
	 *
	 */
	if (g.font_size < 10) g.font_size = 10;
	if (g.font_size > 200) g.font_size = 200;

	if (g.output_file) snprintf(tfn,sizeof(tfn),"%s.tmp",g.output_file);

	/*
	 * Handle the COM Port
	 */
	open_port(&g.serial_params);

	/*
	 * Setup SDL2 and fonts
	 *
	 */

	SDL_Init(SDL_INIT_VIDEO);
	TTF_Init();
	TTF_Font *font = TTF_OpenFont("RobotoMono-Regular.ttf", g.font_size);
	TTF_Font *font_small = TTF_OpenFont("RobotoMono-Regular.ttf", g.font_size/4);

	/*
	 * Get the required window size.
	 *
	 * Parameters passed can override the font self-detect sizing
	 *
	 */
	TTF_SizeText(font, "-12.34mV  ", &g.window_width, &g.window_height);
	if (g.wx_forced) g.window_width = g.wx_forced;
	if (g.wy_forced) g.window_height = g.wy_forced;

	SDL_Window *window = SDL_CreateWindow("VC8145", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, g.window_width, g.window_height, 0);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
	if (!font) {
		fprintf(stderr,"Error trying to open font :( \r\n");
		exit(1);
	}

	/* Select the color for drawing. It is set to red here. */
	SDL_SetRenderDrawColor(renderer, g.background_color.r, g.background_color.g, g.background_color.b, 255 );

	/* Clear the entire screen to our selected color. */
	SDL_RenderClear(renderer);

	/*
	 *
	 * Parent will terminate us... else we'll become a zombie
	 * and hope that the almighty PID 1 will reap us
	 *
	 */
	while (!quit) {
		char line1[1024];
		char line2[1024];
		char *p, *q;
		double v = 0.0;
		int end_of_frame_received = 0;
		uint8_t range;
		uint8_t dpp = 0;
		ssize_t bytes_read = 0;
		bool units_override = false;

		while (SDL_PollEvent(&event)) {
			switch (event.type)
			{
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_q) quit = true;
					break;
				case SDL_QUIT:
					quit = true;
					break;
			}
		}

		linetmp[0] = '\0';

		/*
		 * Time to start receiving the serial block data
		 *
		 * We initially "stage" here waiting for there to
		 * be something happening on the com port.  Soon as
		 * something happens, then we move forward to trying
		 * to read the data.
		 *
		 * Send code 0x89 which requests  the main display data
		 * and includes the device state in bytes [1:3]
		 * byte 4 contains sign/range/hold
		 *
		 */
		{
			size_t bytes_written = 0;
			bytes_written = cmd_send(&g, 0x89);
			if (bytes_written == 0) continue;
		}

		if (g.debug) { fprintf(stderr,"DATA START: "); }
		end_of_frame_received = 0;
		i = 0;
		do {
			char temp_char;
			if (g.debug) fprintf(stderr,".");
			bytes_read = read(g.serial_params.fd, &temp_char, 1);
			if (bytes_read) {
				d[i] = temp_char;
				if (g.debug) { fprintf(stderr,"%02x ", d[i]); fflush(stdout); }

				i++;

				if (temp_char == 0x0A) {
					end_of_frame_received = 1;
					break;
				}
			}
		} while ((bytes_read > 0) && (i < sizeof(d)) && (!end_of_frame_received));

		if (g.debug) { fprintf(stderr,":END [%d bytes]\r\n", i); fflush(stderr); }

		/*
		 * Validate the received data
		 *
		 */
		usleep(INTERFRAME_SLEEP);

		/*
		 * We should have received our command back as the first byte
		 *
		 */
		if (d[0] != 0x89) continue;

		/*
		 * Validate our frame size
		 *
		 */
		if (i != DATA_FRAME_SIZE) {
			if (dt_loaded) memcpy(d, dt, sizeof(d));
		} else {
			memcpy(dt, d, sizeof(d)); // make a copy.
			dt_loaded = 1;
		}


		//		continue;

		/*
		 * Initialise the strings used for units, prefix and mode
		 * so we don't end up with uncleared prefixes etc
		 * ( see https://www.youtube.com/watch?v=5HUyEykicEQ )
		 *
		 * Prefix string initialised to single space, prevents 
		 * annoying string width jump (on monospace, can't stop
		 * it with variable width strings unless we draw the 
		 * prefix+units separately in a fixed location
		 *
		 */
		snprintf(prefix, sizeof(prefix), " ");
		units[0] = '\0';
		mmmode[0] = '\0';

		/*
		 * Decode our data.
		 *
		 * While the data sheet gives a very nice matrix for the RANGE and FUNCTION values
		 * it's probably more human-readable to break it down in to longer code on a per
		 * function selection.
		 *
		 * linetmp : contains the value we want show
		 * mmode   : contains the meter mode (resistance, volts, amps etc)
		 *  L"\u00B0C" = 'C
		 *  L"\u00B0F" = 'F
		 *  L"\u2126"  = ohms char
		 *  L"\u00B5"  = mu char (micro)
		 *
		 */

		range = d[2] & 0x38; // bits 3:5 of byte 2 are the range, meaning varies on mode
		switch (range) {
			case 0x00: dpp = 1; break;
			case 0x08: dpp = 2; break;
			case 0x10: dpp = 3; break;
			case 0x18: dpp = 4; break;
			case 0x20: dpp = 5; break;
			case 0x28: dpp = 6; break;
		}

		if (g.debug) fprintf(stderr,"Range %02x => DPP=%d\r\n", range, dpp);

		snprintf(mmmode, sizeof(mmmode),"---");
		switch (d[1] & 0b11111000) {
			case 0xA0: snprintf(mmmode,sizeof(mmmode),"Generator"); break;
			case 0xD0: snprintf(mmmode,sizeof(mmmode),"Frequency");
						  snprintf(units,sizeof(units),"Hz");
						  break;

			case 0xC8: snprintf(mmmode,sizeof(mmmode),"Capacitance"); 
						  snprintf(units, sizeof(units), "F"); 
						  if (dpp < 4)  {
							  snprintf(prefix, sizeof(prefix), "n");
							  dpp -= 1;
						  } else {
							  snprintf(prefix, sizeof(prefix), "%s",uu);
							  dpp -= 4;
						  }
						  break;

			case 0xC0: snprintf(mmmode,sizeof(mmmode),"Temperature");
						  snprintf(units, sizeof(units),"%sC", dd);
						  break;

			case 0xD8: snprintf(mmmode,sizeof(mmmode),"Diode"); 
						  snprintf(units, sizeof(units), "V");
						  dpp -= 1;
						  if (g.units_separator) {
							 units_override = true;
							snprintf(units, sizeof(units), "v");
						  }

						  break;

			case 0xE0: snprintf(mmmode,sizeof(mmmode),"Resistance");  
						  snprintf(units, sizeof(units), "%s", oo);
								  if (g.units_separator) {
									  units_override = true;
									  snprintf(units, sizeof(units), "");
								  }

						  switch (dpp) {
							  case 1:
								  snprintf(prefix, sizeof(prefix)," ");
								  if (g.units_separator) { snprintf(prefix, sizeof(prefix),"%s",oo); }
								  dpp+=1;
								  break;

							  case 5: 
							  case 6:
								  snprintf(prefix, sizeof(prefix),"M");
								  dpp -= 5;
								  break;

							  default:
								  snprintf(prefix, sizeof(prefix),"k");
								  dpp -= 2;
						  }

						  break;

			case 0xA8: snprintf(mmmode,sizeof(mmmode),"Current"); 
						  snprintf(units, sizeof(units), "A");
						  dpp -= 1;
						  break;

			case 0xB0: snprintf(mmmode,sizeof(mmmode),"Current");
						  snprintf(units, sizeof(units), "mA");
						  break;

			case 0xE8: snprintf(mmmode,sizeof(mmmode),"Volts (mV)");
						  snprintf(units, sizeof(units), "mV");
						  break;

			case 0xF8: snprintf(mmmode,sizeof(mmmode),"VAC");
						  snprintf(units, sizeof(units), "V");
						  if (g.units_separator) {
							  units_override = true;
							  snprintf(units, sizeof(units), "v");
						  }
						  dpp -= 1;
						  break;

			case 0xF0: snprintf(mmmode,sizeof(mmmode),"VDC"); 
						  if (g.range_control) {
							  if (d[2] & MMFLAG_AUTORANGE) {
								  uint8_t b;
								  cmd_send(&g, 0xA1);
								  b = byte_read(&g);
								  cmd_send(&g, 0xA1);
								  b = byte_read(&g);
							  }
						  }
						  snprintf(units, sizeof(units), "V");
						  dpp -= 1;
						  if (g.units_separator) {
							  units_override = true;
							  snprintf(units, sizeof(units), "v");
						  }
						  break;

			default:
						  snprintf(mmmode, sizeof(mmmode), "***");
						  snprintf(units, sizeof(units),"*");
						  break;
		}


		{
			char sign_char = ' ';
			char value[128];
			char local_separator[10];
			char *separator = SEPARATOR_DP;
			int dp = 0;

			sign_char='*';
			switch (d[4] & 0b01110000) {
				case 0:
				case 0x40: sign_char = ' '; break;
				case 0x50: sign_char = '-'; break;
			}

			if (g.units_separator) {
				if (prefix[0]==' ') prefix[0] = '\0';
				snprintf(local_separator, sizeof(local_separator), "%s%s", prefix, units);
				separator = local_separator;
			}

			snprintf(linetmp, sizeof(linetmp), "%c%c%s%c%s%c%s%c%s%c%s%s"
					,sign_char
					,digit(d[5])
					,dpp==0?separator:""
					,digit(d[6])
					,dpp==1?separator:""
					,digit(d[7])
					,dpp==2?separator:""
					,digit(d[8])
					,dpp==3?separator:""
					,digit(d[9])
					,g.units_separator?"":prefix
					,g.units_separator?"":units
					);

			if (g.units_separator) {
				char *p = linetmp+1; // skip the sign char
				while (*p == '0') { *p = ' '; p++; }
				if (!isdigit(*p)) *(p-1) = '0';
			}
		}

		/*
		 *
		 * END OF DECODIN9
		 */


		snprintf(line1, sizeof(line1), "%-40s", linetmp);
		snprintf(line2, sizeof(line2), "%-40s", mmmode);
		//		snprintf(line3, sizeof(line3), "V.%03d", BUILD_VER);

		if (!g.quiet) fprintf(stdout,"%s\r",line1); fflush(stdout);

		{
			int texW = 0;
			int texH = 0;
			int texW2 = 0;
			int texH2 = 0;
			SDL_RenderClear(renderer);
			surface = TTF_RenderUTF8_Solid(font, line1, g.font_color);
			texture = SDL_CreateTextureFromSurface(renderer, surface);
			SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);
			SDL_Rect dstrect = { 0, 0, texW, texH };
			SDL_RenderCopy(renderer, texture, NULL, &dstrect);

			if (g.show_mode) {
				surface_2 = TTF_RenderUTF8_Solid(font_small, line2, g.font_color);
				texture_2 = SDL_CreateTextureFromSurface(renderer, surface_2);
				SDL_QueryTexture(texture_2, NULL, NULL, &texW2, &texH2);
				dstrect = { 0, 0, texW2, texH2 };
				SDL_RenderCopy(renderer, texture_2, NULL, &dstrect);
			}

			SDL_RenderPresent(renderer);

			SDL_DestroyTexture(texture);
			SDL_FreeSurface(surface);
			if (g.show_mode) {
				SDL_DestroyTexture(texture_2);
				SDL_FreeSurface(surface_2);
			}



		}


		if (g.output_file) {
			/*
			 * Only write the file out if it doesn't
			 * exist. 
			 *
			 */
			if (!fileExists(g.output_file)) {
				FILE *f;
				fprintf(stderr,"%s:%d: output filename = %s\r\n", FL, g.output_file);
				f = fopen(tfn,"w");
				if (f) {
					fprintf(f,"%s", linetmp);
					fprintf(stderr,"%s:%d: %s => %s\r\n", FL, linetmp, tfn);
					fclose(f);
					rename(tfn, g.output_file);
				}
			}
		}

	} // while(1)

	if (g.serial_params.fd) close(g.serial_params.fd);

	TTF_CloseFont(font);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	TTF_Quit();
	SDL_Quit();

	return 0;

}
