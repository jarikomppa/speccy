/*
Graphics chunkifier for zx spectrum

Converts modern formats into zx spectrum printable color characters


Copyright (c) 2022 Jari Komppa (http://iki.fi/sol)

This software is provided 'as-is', without any express or implied warranty. In no event will 
the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial 
applications, and to alter it and redistribute it freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not claim that you 
	    wrote the original software. If you use this software in a product, an acknowledgment 
	    in the product documentation would be appreciated but is not required.

	2. Altered source versions must be plainly marked as such, and must not be misrepresented 
	   as being the original software.

	3. This notice may not be removed or altered from any source distribution.
*/

#define  _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "tapper.h"

#include <iostream>
#include "optionparser.h"

int pal[16] =
{
	0x000000,
	0xcd0000,
	0x0000cd,
	0xcd00cd,
	0x00cd00,
	0xcdcd00,
	0x00cdcd,
	0xcdcdcd,
	0x000000,
	0xff0000,
	0x0000ff,
	0xff00ff,
	0x00ff00,
	0xffff00,
	0x00ffff,
	0xffffff
};

int mask[4 * 16] =
{
	0,0,0,0,
	0,1,0,0,
	1,0,0,0,
	1,1,0,0,
	0,0,0,1,
	0,1,0,1,
	1,0,0,1,
	1,1,0,1,

	1,1,1,1,
	1,0,1,1,
	0,1,1,1,
	0,0,1,1,
	1,1,1,0,
	1,0,1,0,
	0,1,1,0,
	0,0,1,0
};

int *dst;
int *map;
int x, y;

int* charmap;
int* palmap;

void reduce()
{
	for (int i = 0; i < x / 8; i++)
	{
		for (int j = 0; j < y / 8; j++)
		{
			int rd[4];
			rd[0] = map[i * 2 + 0 + (j * 2 + 0) * (x / 4)];
			rd[1] = map[i * 2 + 1 + (j * 2 + 0) * (x / 4)];
			rd[2] = map[i * 2 + 0 + (j * 2 + 1) * (x / 4)];
			rd[3] = map[i * 2 + 1 + (j * 2 + 1) * (x / 4)];
			int pattern = 0;
			int ink = 0;
			int paper = 0;
			int bright = 0;
			int besterror = 100000000;
			for (int p = 0; p < 16; p++)
			{
				for (int b = 0; b < 2; b++)
				{
					for (int fg = 0; fg < 8; fg++)
					{
						for (int bg = 0; bg < 8; bg++)
						{
							int td[4];
							for (int k = 0; k < 4; k++)
								td[k] = mask[p * 4 + k] ? pal[fg + b * 8] : pal[bg + b * 8];
							int error = 0;
							for (int k = 0; k < 4; k++)
							{
								error += (((td[k] >> 0 ) & 0xff) - ((rd[k] >> 0 ) & 0xff)) * (((td[k] >> 0 ) & 0xff) - ((rd[k] >> 0 ) & 0xff));
								error += (((td[k] >> 8 ) & 0xff) - ((rd[k] >> 8 ) & 0xff)) * (((td[k] >> 8 ) & 0xff) - ((rd[k] >> 8 ) & 0xff));
								error += (((td[k] >> 16) & 0xff) - ((rd[k] >> 16) & 0xff)) * (((td[k] >> 16) & 0xff) - ((rd[k] >> 16) & 0xff));
							}
							if (error < besterror)
							{
								besterror = error;
								pattern = p;
								bright = b;
								paper = bg;
								ink = fg;
							}
						}
					}
				}
			}
			map[i * 2 + 0 + (j * 2 + 0) * (x / 4)] = mask[pattern * 4 + 0] ? pal[ink + bright * 8] : pal[paper + bright * 8];
			map[i * 2 + 1 + (j * 2 + 0) * (x / 4)] = mask[pattern * 4 + 1] ? pal[ink + bright * 8] : pal[paper + bright * 8];
			map[i * 2 + 0 + (j * 2 + 1) * (x / 4)] = mask[pattern * 4 + 2] ? pal[ink + bright * 8] : pal[paper + bright * 8];
			map[i * 2 + 1 + (j * 2 + 1) * (x / 4)] = mask[pattern * 4 + 3] ? pal[ink + bright * 8] : pal[paper + bright * 8];

			charmap[i + j * (x / 8)] = pattern + 128;
			palmap[i + j * (x / 8)] = ink | (paper << 3) | (bright << 6);
		}
	}
}

void reduce_bw()
{
	for (int i = 0; i < x / 8; i++)
	{
		for (int j = 0; j < y / 8; j++)
		{
			int rd[4];
			rd[0] = map[i * 2 + 0 + (j * 2 + 0) * (x / 4)];
			rd[1] = map[i * 2 + 1 + (j * 2 + 0) * (x / 4)];
			rd[2] = map[i * 2 + 0 + (j * 2 + 1) * (x / 4)];
			rd[3] = map[i * 2 + 1 + (j * 2 + 1) * (x / 4)];
			int pattern = 0;
			int ink = 0;
			int paper = 0;
			int bright = 0;
			int besterror = 100000000;
			for (int p = 0; p < 16; p++)
			{
				int td[4];
				for (int k = 0; k < 4; k++)
					td[k] = mask[p * 4 + k] ? 0xffffff : 0x000000;
				int error = 0;
				for (int k = 0; k < 4; k++)
				{
					error += (((td[k] >> 0) & 0xff) - ((rd[k] >> 0) & 0xff)) * (((td[k] >> 0) & 0xff) - ((rd[k] >> 0) & 0xff));
					error += (((td[k] >> 8) & 0xff) - ((rd[k] >> 8) & 0xff)) * (((td[k] >> 8) & 0xff) - ((rd[k] >> 8) & 0xff));
					error += (((td[k] >> 16) & 0xff) - ((rd[k] >> 16) & 0xff)) * (((td[k] >> 16) & 0xff) - ((rd[k] >> 16) & 0xff));
				}
				if (error < besterror)
				{
					besterror = error;
					pattern = p;
				}
			}
			map[i * 2 + 0 + (j * 2 + 0) * 64] = mask[pattern * 4 + 0] ? 0xffffff : 0;
			map[i * 2 + 1 + (j * 2 + 0) * 64] = mask[pattern * 4 + 1] ? 0xffffff : 0;
			map[i * 2 + 0 + (j * 2 + 1) * 64] = mask[pattern * 4 + 2] ? 0xffffff : 0;
			map[i * 2 + 1 + (j * 2 + 1) * 64] = mask[pattern * 4 + 3] ? 0xffffff : 0;
		}
	}
}

void append_print(Tapper& p, int line, const char*s, int bytes, int newline)
{
	p.putdata(0x00);
	p.putdata(line); // line number 10
	p.putdata(bytes + 5 - 1 + !newline); // bytes in line
	p.putdata(0); // 0?
//	p.putdata(' ');
	p.putdata(0xf5); // PRINT
	p.putdata('\"'); // "
	for (int i = 0; i < bytes; i++)
		p.putdata(s[i]);
	//p.putdatastr(s);
	p.putdata('\"');
	if (!newline)
		p.putdata(';');
	p.putdata('\r');
}



void output_tap(const char*fn)
{
	Tapper* header = new Tapper;
	Tapper* payload = new Tapper;
	FILE* f = fopen(fn, "wb");

	payload->putdata(0xff); // data

/*
	append_print(payload, 10, "hello", 5);
	append_print(payload, 20, "world", 5);
	// \x10 = INK
	// \x11 = PAPER
	// \x12 = FLASH
	// \x13 = BRIGHT
	append_print(payload, 30, "\x10\x03" "r" "\x10\x01" "a" "\x10\x02" "i" "\x11\x01\x10\x03" "n" "\x10\x04" "b" "\x13\x01\x10\x03" "o" "\x10\x05" "w" "\x10\x04" "?" "\x10\x03" "?", 27);
*/

	char *t = new char[40 * (1 + 2 + 2 + 2)];

	for (int j = 0; j < y / 8; j++)
	{
		int lastink = -1;
		int lastpaper = -1;
		int lastbright = -1;
		int v = 0;
		for (int i = 0; i < x / 8; i++)
		{
			int ink = (palmap[j * (x / 8) + i] >> 0) & 7;
			int paper = (palmap[j * (x / 8) + i] >> 3) & 7;
			int bright = (palmap[j * (x / 8) + i] >> 6) & 1;
			if (ink != lastink)
			{
				t[v++] = 0x10;
				t[v++] = ink;
				lastink = ink;
			}
			if (paper != lastpaper)
			{
				t[v++] = 0x11;
				t[v++] = paper;
				lastpaper = paper;
			}
			if (bright != lastbright)
			{
				t[v++] = 0x13;
				t[v++] = bright;
				lastbright = bright;
			}
			t[v++] = charmap[j * (x / 8) + i];
		}
		append_print(*payload, 10 * (j + 1), t, v, (x / 8) < 40);
	}
	delete[] t;

	header->putdata(0); // header
	header->putdata((unsigned char)0); // 0 = program
	header->putdatastr("Chunkified"); // 10 chars exact
	header->putdataint(payload->ofs - 1);
	header->putdataint(10); // autorun row
	header->putdataint(payload->ofs - 1);

	header->write(f);
	payload->write(f);

	delete payload;
	delete header;

	fclose(f);
}

void output_bas(const char* fn, int interleave, int separate, int header)
{
	FILE* f = fopen(fn, "w");
	if (!f)
	{
		printf("Unable to open %s for writing", fn);
		exit(0);
	}

	int line = 10;

	if (header)
	{
		fprintf(f, "%d DATA %d, %d\n", line, x / 8, y / 8);
		line += 10;
	}

	if (!interleave)
	{
		for (int j = 0; j < y / 8; j++)
		{
			fprintf(f, "%d DATA ", line);
			line += 10;
			for (int i = 0; i < x / 8; i++)
			{
				fprintf(f, "%s%d", i ? ", " : "", charmap[j * (x / 8) + i]);
			}
			fprintf(f, "\n");
		}
		for (int j = 0; j < y / 8; j++)
		{
			fprintf(f, "%d DATA ", line);
			line += 10;
			for (int i = 0; i < x / 8; i++)
			{
				if (!separate)
				{
					fprintf(f, "%s%d", i ? ", " : "", palmap[j * (x / 8) + i]);
				}
				else
				{
					int ink = (palmap[j * (x / 8) + i] >> 0) & 7;
					int paper = (palmap[j * (x / 8) + i] >> 3) & 7;
					int bright = (palmap[j * (x / 8) + i] >> 6) & 1;
					fprintf(f, "%s%d, %d, %d", i ? ", " : "", ink, paper, bright);
				}
			}
			fprintf(f, "\n");
		}
	}
	else // interleave
	{
		for (int j = 0; j < y / 8; j++)
		{
			fprintf(f, "%d DATA ", line);
			line += 10;
			for (int i = 0; i < x / 8; i++)
			{
				if (!separate)
				{
					fprintf(f, "%s%d, %d", i ? ", " : "", palmap[j * (x / 8) + i], charmap[j * (x / 8) + i]);
				}
				else
				{
					int ink = (palmap[j * (x / 8) + i] >> 0) & 7;
					int paper = (palmap[j * (x / 8) + i] >> 3) & 7;
					int bright = (palmap[j * (x / 8) + i] >> 6) & 1;
					fprintf(f, "%s%d, %d, %d, %d", i ? ", " : "", ink, paper, bright, charmap[j * (x / 8) + i]);
				}
			}
			fprintf(f, "\n");
		}
	}

	fclose(f);
}

void output_asm(const char* fn, int interleave, int separate, int header)
{
	FILE* f = fopen(fn, "w");
	if (!f)
	{
		printf("Unable to open %s for writing", fn);
		exit(0);
	}

	if (header)
	{
		fprintf(f, "\tdb %d, %d\n", x / 8, y / 8);
	}

	if (!interleave)
	{
		for (int j = 0; j < y / 8; j++)
		{
			fprintf(f, "\tdb ");
			for (int i = 0; i < x / 8; i++)
			{
				fprintf(f, "%s%d", i ? ", " : "", charmap[j * (x / 8) + i]);
			}
			fprintf(f, "\n");
		}
		for (int j = 0; j < y / 8; j++)
		{
			fprintf(f, "\tdb ");
			for (int i = 0; i < x / 8; i++)
			{
				if (!separate)
				{
					fprintf(f, "%s%d", i ? ", " : "", palmap[j * (x / 8) + i]);
				}
				else
				{
					int ink = (palmap[j * (x / 8) + i] >> 0) & 7;
					int paper = (palmap[j * (x / 8) + i] >> 3) & 7;
					int bright = (palmap[j * (x / 8) + i] >> 6) & 1;
					fprintf(f, "%s%d, %d, %d", i ? ", " : "", ink, paper, bright);
				}
			}
			fprintf(f, "\n");
		}
	}
	else // interleave
	{
		for (int j = 0; j < y / 8; j++)
		{
			fprintf(f, "\tdb ");
			for (int i = 0; i < x / 8; i++)
			{
				if (!separate)
				{
					fprintf(f, "%s%d, %d", i ? ", " : "", palmap[j * (x / 8) + i], charmap[j * (x / 8) + i]);
				}
				else
				{
					int ink = (palmap[j * (x / 8) + i] >> 0) & 7;
					int paper = (palmap[j * (x / 8) + i] >> 3) & 7;
					int bright = (palmap[j * (x / 8) + i] >> 6) & 1;
					fprintf(f, "%s%d, %d, %d, %d", i ? ", " : "", ink, paper, bright, charmap[j * (x / 8) + i]);
				}
			}
			fprintf(f, "\n");
		}
	}

	fclose(f);
}

void output_bin(const char* fn, int interleave, int separate, int header)
{
	FILE* f = fopen(fn, "wb");
	if (!f)
	{
		printf("Unable to open %s for writing", fn);
		exit(0);
	}

	if (header)
	{
		fputc(x / 8, f);
		fputc(y / 8, f);
	}

	if (!interleave)
	{
		for (int j = 0; j < y / 8; j++)
		{
			for (int i = 0; i < x / 8; i++)
			{
				fputc(charmap[j * (x / 8) + i], f);
			}
		}
		for (int j = 0; j < y / 8; j++)
		{
			for (int i = 0; i < x / 8; i++)
			{
				if (!separate)
				{
					fputc(palmap[j * (x / 8) + i], f);
				}
				else
				{
					int ink = (palmap[j * (x / 8) + i] >> 0) & 7;
					int paper = (palmap[j * (x / 8) + i] >> 3) & 7;
					int bright = (palmap[j * (x / 8) + i] >> 6) & 1;
					fputc(ink, f);
					fputc(paper, f);
					fputc(bright, f);
				}
			}
		}
	}
	else // interleave
	{
		for (int j = 0; j < y / 8; j++)
		{
			for (int i = 0; i < x / 8; i++)
			{
				if (!separate)
				{
					fputc(palmap[j * (x / 8) + i], f);
				}
				else
				{
					int ink = (palmap[j * (x / 8) + i] >> 0) & 7;
					int paper = (palmap[j * (x / 8) + i] >> 3) & 7;
					int bright = (palmap[j * (x / 8) + i] >> 6) & 1;
					fputc(ink, f);
					fputc(paper, f);
					fputc(bright, f);
				}
				fputc(charmap[j * (x / 8) + i], f);
			}
		}
	}

	fclose(f);
}

enum optionIndex { UNKNOWN, TAP, ASM, BAS, BIN, PNG, INTERLEAVE, SEPARATE, HEADER };
const option::Descriptor usage[] =
{
	{ UNKNOWN,		0, "", "",	option::Arg::None,				  "USAGE: chunkify inputfile outputfile [options]\nInput file can be jpg, png, bmp, psd, tga, gif, hdr, pic, ppm or pgm.\n\nOptions:"},
	{ TAP,			0, "t", "tap", option::Arg::None,			  " -t --tap\t Output a .tap file"},
	{ ASM,          0, "a", "asm", option::Arg::None,			  " -a --asm\t Output a .asm file"},
	{ BAS,          0, "b", "bas", option::Arg::None,			  " -b --bas\t Output a .bas file"},
	{ BIN,          0, "x", "bin", option::Arg::None,			  " -x --bin\t Output a .bin file"},
	{ PNG,          0, "p", "png", option::Arg::None,             " -p --png\t Output a .png file"},
	{ INTERLEAVE,   0, "i", "interleave", option::Arg::None,      " -i --interleave\t Interleave color & glyphs (default: don't)"},
	{ SEPARATE,     0, "s", "separate", option::Arg::None,        " -s --separate\t Separate ink, paper & bright (default: all in one byte)"},
	{ HEADER,       0, "h", "header", option::Arg::None,          " -h --header\t Include header (width/height bytes, default: don't)"},
	{ UNKNOWN,      0, "", "", option::Arg::None,				  "Example:\n  chunkify test.png test.bas --bas --separate"},
	{ 0,0,0,0,0,0 }
};

int main(int parc, char** pars)
{
	int format = -1;
	int interleave = 0;
	int separate = 0;
	int header = 0;
	option::Stats stats(usage, parc - 1, pars + 1);
	assert(stats.buffer_max < 32 && stats.options_max < 32);
	option::Option options[32], buffer[32];
	option::Parser parse(true, usage, parc - 1, pars + 1, options, buffer);

	if (options[UNKNOWN])
	{
		for (option::Option* opt = options[UNKNOWN]; opt; opt = opt->next())
			printf("Unknown option: %s\n", opt->name);
		printf("Run without parameters for help.\n");
		return 0;
	}

	if (parse.error() || parc < 3 || parse.nonOptionsCount() != 2)
	{
		option::printUsage(std::cout, usage);
		return 0;
	}

	if (options[TAP]) format = TAP;
	if (options[BAS]) format = BAS;
	if (options[ASM]) format = ASM;
	if (options[BIN]) format = BIN;
	if (options[PNG]) format = PNG;
	if (options[INTERLEAVE]) interleave = 1;
	if (options[SEPARATE]) separate = 1;
	if (options[HEADER]) header = 1;
	if ((format == TAP || format == PNG) && (interleave || separate || header))
		printf("Note: header, interleave and separate options ignored due to output format\n");

	if (format == -1)
	{
		printf("Please select an output format (tap, bas, asm, bin, png)\n");
		return 0;
	}

	int n;

	unsigned int* data = (unsigned int*)stbi_load(parse.nonOption(0), &x, &y, &n, 4);
	if (!data)
	{
		printf("Unable to load %s\n", parse.nonOption(0));
		return 0;
	}

	printf("Loaded %s (%d x %d pixels, output %d x %d glyphs)\n", parse.nonOption(0), x, y, x / 8, y / 8);

	dst = new int[x * y];
	map = new int[(x / 4) * (y / 4)];
	charmap = new int[(x / 8) * (y / 8)];
	palmap = new int[(x / 8) * (y / 8)];

	for (int i = 0; i < x * y; i++)
		dst[i] = 0;

	for (int i = 0; i < x / 4; i++)
		for (int j = 0; j < y / 4; j++)
		{
			int r = (((data[i * 4 + 0 + (j * 4 + 0) * x] >> 0) & 0xff) + ((data[i * 4 + 1 + (j * 4 + 0) * x] >> 0) & 0xff) + ((data[i * 4 + 0 + (j * 4 + 1) * x] >> 0) & 0xff) + ((data[i * 4 + 1 + (j * 4 + 1) * x] >> 0) & 0xff)) / 4;
			int g = (((data[i * 4 + 0 + (j * 4 + 0) * x] >> 8) & 0xff) + ((data[i * 4 + 1 + (j * 4 + 0) * x] >> 8) & 0xff) + ((data[i * 4 + 0 + (j * 4 + 1) * x] >> 8) & 0xff) + ((data[i * 4 + 1 + (j * 4 + 1) * x] >> 8) & 0xff)) / 4;
			int b = (((data[i * 4 + 0 + (j * 4 + 0) * x] >> 16) & 0xff) + ((data[i * 4 + 1 + (j * 4 + 0) * x] >> 16) & 0xff) + ((data[i * 4 + 0 + (j * 4 + 1) * x] >> 16) & 0xff) + ((data[i * 4 + 1 + (j * 4 + 1) * x] >> 16) & 0xff)) / 4;

			map[j * (x / 4) + i] = (r << 0) | (g << 8) | (b << 16);

		}

	stbi_image_free(data);

	reduce();

	for (int i = 0; i < x; i++)
		for (int j = 0; j < y; j++)
			dst[j * x + i] = map[(i / 4) + (j / 4) * (x / 4)] | 0xff000000;

	if (!((format == TAP) || (format == PNG)))
	{
		printf("Output options:\n");

		if (header)
			printf("- Starting data with byte pair for width and height.\n");
		else
			printf("- Not including any size informationas a header.\n");

		if (interleave) 
			printf("- Interleaving color data with glyphs, color first, then glyph.\n");
		else
			printf("- Outputting all glyphs first, followed by colors.\n");			

		if (separate)
			printf("- Outputting color data as three bytes for ink, color and bright.\n");
		else
			printf("- Outputting color data as single byte containing ink, color and bright.\n");
		printf("\n");
	}

	switch (format)
	{
	case PNG:
		stbi_write_png(parse.nonOption(1), x, y, 4, dst, x * 4);
		break;
	case TAP:
		output_tap(parse.nonOption(1));
		break;
	case BAS:
		output_bas(parse.nonOption(1), interleave, separate, header);
		break;
	case ASM:
		output_asm(parse.nonOption(1), interleave, separate, header);
		break;
	case BIN:
		output_bin(parse.nonOption(1), interleave, separate, header);
		break;
	default:
		printf("You are in error. No-one is screaming. Have a nice day.\n");
		exit(0);
	}
	printf("Output %s\n", parse.nonOption(1));	

	return 0;
}